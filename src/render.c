#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include "bitmap.h"
#include "render.h"
#include "video.h"

#define CORE_COUNT 8
// 1024 / fps = # of seconds to buffer
#define BUFFER_SIZE 1024

static int frame_buffer_index = 0;
static int loaded_frames = 0;
static vframe_t* frame_buffer[BUFFER_SIZE];
static pthread_mutex_t buf_frame_locks[BUFFER_SIZE];
static sem_t* fb_lock; 

static sem_t* thread_join_lock;
static int8_t join_idx = 0;
static int8_t thread_join_back_queue[CORE_COUNT] = {-1, -1, -1, -1, -1, -1, -1, -1};

ropts_t* init_render_options(char* video_name) {
    uint32_t frame_count = get_frame_count(video_name);
    struct winsize window_size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size);
    uint16_t win_width = window_size.ws_col;
    uint16_t win_height = window_size.ws_row;
    uint16_t vid_width;
    uint16_t vid_height;
    get_dimensions(video_name, &vid_width, &vid_height);

    ropts_t* render_options = (ropts_t*) malloc(sizeof(ropts_t));    
    render_options->use_threading = 1;
    render_options->threads_per_frame = 4;
    render_options->use_fast_playback = 1;
    render_options->draw_fps_counter = 1;

    render_options->video_name = video_name;
    render_options->fps = frame_count / get_duration(video_name); 
    render_options->frame_count = frame_count;

    render_options->v_width = vid_width; 
    render_options->v_height = vid_height;
    render_options->w_width = win_width;
    render_options->w_height = win_height;

    // TODO: set to (v_height / tpf) but also factor in
    //       scaling down, maybe some rows will intersect? race condition!!!
    render_options->rows_per_thread = (render_options->v_height / render_options->threads_per_frame);
    render_options->greyscale_sz = 10;
    render_options->greyscale = " .:-=+*#%@";

    // print options
    printf("ropts_t: \n");
    printf("  use_threading: %d\n", render_options->use_threading);
    printf("  threads_per_frame: %d\n", render_options->threads_per_frame);
    printf("  use_fast_playback: %d\n", render_options->use_fast_playback);
    printf("  draw_fps_counter: %d\n", render_options->draw_fps_counter);
    printf("  video_name: %s\n", render_options->video_name);
    printf("  fps: %d\n", render_options->fps);
    printf("  frame_count: %d\n", render_options->frame_count);
    printf("  v_width: %d\n", render_options->v_width);
    printf("  v_height: %d\n", render_options->v_height);
    printf("  w_width: %d\n", render_options->w_width);
    printf("  w_height: %d\n", render_options->w_height);
    printf("  rows_per_thread: %d\n", render_options->rows_per_thread);
    printf("  greyscale_sz: %d\n", render_options->greyscale_sz);
    printf("  greyscale: %s\n", render_options->greyscale);

    putchar('\n');

    return render_options;
}

void init_render_env(ropts_t* ropts) {
    get_frames(ropts->frame_count, ropts->video_name);

    // clear buffer, init semaphore
    if ((fb_lock = sem_open("fb_lock", O_CREAT, 0644, 1)) == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    if ((thread_join_lock = sem_open("thread_join_lock", O_CREAT, 0644, 1)) == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        frame_buffer[i] = 0;
        pthread_mutex_init(&buf_frame_locks[i], NULL);
    }
    loaded_frames = 0;
    frame_buffer_index = 0;
}

void cleanup_render_env(ropts_t* ropts) {
    if (sem_close(fb_lock) == -1) {
        perror("sem_close");
        exit(EXIT_FAILURE);
    }

    if (sem_unlink("fb_lock") == -1) {
        perror("sem_unlink");
        exit(EXIT_FAILURE);
    }
}

vframe_t* create_vframe_buffer(ropts_t* ropts) {
    vframe_t* buf = (vframe_t*) malloc(sizeof(vframe_t) * ropts->frame_count);
    pthread_t threads[CORE_COUNT];
    struct vframe_chunk_args vframe_chunk_args[CORE_COUNT];
    for (size_t i = 0; i < CORE_COUNT; i++) {
        uint32_t* assigned_frames = (uint32_t*) malloc(sizeof(uint32_t) * (ropts->frame_count) / CORE_COUNT);
        size_t assigned_frame_count = 0;
        for (size_t j = 1; j < ropts->frame_count; j++) {
            if (j % CORE_COUNT != i) continue; 
            assigned_frames[assigned_frame_count++] = j;
        }

        vframe_chunk_args[i].thread_id = i;
        vframe_chunk_args[i].assigned_frames = assigned_frames;
        vframe_chunk_args[i].assigned_frames_sz = assigned_frame_count;
        vframe_chunk_args[i].vframe_buffer = buf;
        vframe_chunk_args[i].ropts = ropts;
        vframe_chunk_args[i].last_rendered_frame = 0;

        //print options
        printf("pthread_t(%02ld): \n", i);
        printf("  assigned_frames: %p\n", (void*) vframe_chunk_args[i].assigned_frames);
        printf("  assigned_frames_sz: %ld\n", vframe_chunk_args[i].assigned_frames_sz);
        printf("  vframe_buffer: %p\n", (void*) vframe_chunk_args[i].vframe_buffer);
        printf("  ropts: %p\n", (void*) vframe_chunk_args[i].ropts);
        printf("  last_rendered_frame: %ld\n", vframe_chunk_args[i].last_rendered_frame);

        pthread_create(&threads[i], NULL, &vframe_chunk_loader, (void*) &vframe_chunk_args[i]);
    }

    // wait for all threads to finish
    int8_t joined_threads = 0;
    int8_t thread_idx = 0;
    while (joined_threads != CORE_COUNT) {
        if (thread_join_back_queue[thread_idx] != -1) {
            printf("joining thread %d...", thread_join_back_queue[thread_idx]);
            pthread_join(threads[thread_join_back_queue[thread_idx]], NULL);
            joined_threads++;
            printf("joined thread\n");
            thread_join_back_queue[thread_idx++] = -1;
        } 
    }

    printf("all threads joined, total frames in buffer: %d\n", loaded_frames);

    return buf;
}

void* vframe_chunk_loader(void* args) {
    struct vframe_chunk_args* vframe_chunk_args = (struct vframe_chunk_args*) args;
    for (size_t i = vframe_chunk_args->last_rendered_frame; i < vframe_chunk_args->assigned_frames_sz; i++) {
        uint32_t curr_frame = vframe_chunk_args->assigned_frames[i];
        if (loaded_frames >= BUFFER_SIZE) {
            printf("buffer full, stopping vframe loader\n");
            vframe_chunk_args->last_rendered_frame = i;
            sem_wait(thread_join_lock);
            thread_join_back_queue[join_idx++] = vframe_chunk_args->thread_id;
            sem_post(thread_join_lock);
            return NULL;
        }

        char frame_name[256] = {0};
        sprintf(frame_name, "./frames/frame_%06d.bmp", curr_frame);
        read_frame(&vframe_chunk_args->vframe_buffer[curr_frame], frame_name);
        printf("loaded frame %d\n", curr_frame);

        if (pthread_mutex_trylock(&buf_frame_locks[curr_frame % BUFFER_SIZE]) != 0) {
            printf("frame %d already loaded, stopping vframe loader\n", curr_frame);
            vframe_chunk_args->last_rendered_frame = i-1;
            sem_wait(thread_join_lock);
            thread_join_back_queue[join_idx++] = vframe_chunk_args->thread_id;
            sem_post(thread_join_lock);
            return NULL;
        }

        frame_buffer[curr_frame % BUFFER_SIZE] = &vframe_chunk_args->vframe_buffer[curr_frame];
        sem_wait(fb_lock);
        loaded_frames++;
        sem_post(fb_lock);
    }
    free(vframe_chunk_args->assigned_frames);
    vframe_chunk_args->assigned_frames_sz = 0;

    sem_wait(thread_join_lock);
    thread_join_back_queue[join_idx++] = vframe_chunk_args->thread_id;
    sem_post(thread_join_lock);
    return NULL;
}

