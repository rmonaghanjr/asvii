#include <stdio.h>

#include "render.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <video_path>\n", argv[0]);
        return 1;
    }

    ropts_t* render_options = init_render_options(argv[1]); 
    init_render_env(render_options);
    return 0;
}
