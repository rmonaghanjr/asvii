#include <stdio.h>
#include <stddef.h>
#include <unistd.h>

#include "render.h"

#define DEBUG 1

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <video_path>\n", argv[0]);
        return 1;
    }

    ropts_t* render_options = init_render_options(argv[1]); 
    init_render_env(render_options);
    vframe_t* vframe_buffer = create_vframe_buffer(render_options);
    for (size_t i = 0; i < render_options->frame_count; i++) {
        printf("at index %ld, frame_count %d\n", i, render_options->frame_count);

        #ifdef DEBUG
        usleep(10);
        #endif

    }
    free(vframe_buffer);
    cleanup_render_env(render_options);
    return 0;
}
