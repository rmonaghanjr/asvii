#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>

#include "bitmap.h"
#include "render.h"
#include "video.h"

#define CORE_COUNT 8

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

    return render_options;
}

void init_render_env(ropts_t* ropts) {
    get_frames(ropts->frame_count, ropts->video_name);
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
        vframe_chunk_args[i].assigned_frames = assigned_frames;
        vframe_chunk_args[i].assigned_frames_sz = assigned_frame_count;
        vframe_chunk_args[i].vframe_buffer = buf;
        vframe_chunk_args[i].ropts = ropts;
        printf("spawning thread %02ld, every %dth frame starting at frame %ld\n", i, CORE_COUNT, i+1);
        pthread_create(&threads[i], NULL, &vframe_chunk_builder, (void*) &vframe_chunk_args[i]);
    }

    for (size_t i = 0; i < CORE_COUNT; i++) {
        pthread_join(threads[i], NULL);
        printf("thread %02ld finished\n", i);
    }

    return buf;
}

void* vframe_chunk_builder(void* args) {
    struct vframe_chunk_args* vframe_chunk_args = (struct vframe_chunk_args*) args;
    for (size_t i = 0; i < vframe_chunk_args->assigned_frames_sz; i++) {
        char frame_name[256] = {0};
        sprintf(frame_name, "./frames/frame_%06d.bmp", vframe_chunk_args->assigned_frames[i]);
        read_frame(&vframe_chunk_args->vframe_buffer[vframe_chunk_args->assigned_frames[i]], frame_name);
    }
    free(vframe_chunk_args->assigned_frames);
    return NULL;
}

