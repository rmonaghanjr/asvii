#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include "render.h"

#define DEBUG 1

int main(int argc, char *argv[]) {
    extern pthread_mutex_t buf_frame_locks[];
    extern vframe_t frame_buffer[];
    extern pthread_mutex_t fb_lock;
    extern int loaded_frames;

    if (argc < 2) {
        printf("Usage: %s <video_path>\n", argv[0]);
        return 1;
    }

    ropts_t* render_options = init_render_options(argv[1]); 
    init_render_env(render_options);
    struct vframe_chunk_args* renderer_args = init_vframe_loader(render_options);
    start_vframe_loader(renderer_args);
    for (size_t i = 0; i < render_options->frame_count; i++) {
        printf("frame_count %d\n", loaded_frames);
        pthread_mutex_lock(&fb_lock);
        loaded_frames--;
        pthread_mutex_unlock(&buf_frame_locks[i % 1024]);
        pthread_mutex_unlock(&fb_lock);

        if (loaded_frames <= 0) {
            start_vframe_loader(renderer_args);
            printf("loaded_frames: %d\n", loaded_frames);
        }
        usleep(1000);
    }
    cleanup_render_env(render_options);
    return 0;
}
