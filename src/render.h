#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

typedef struct {
    // renderer logic settings
    uint8_t use_threading;
    uint8_t threads_per_frame;
    uint8_t use_fast_playback;
    uint8_t draw_fps_counter;

    // video settings
    char* video_name;
    uint8_t fps;
    uint32_t frame_count;
    uint16_t v_width;
    uint16_t v_height;
    uint16_t w_width;
    uint16_t w_height;

    // frame conversion settings
    uint8_t rows_per_thread;
    uint8_t greyscale_sz;
    char* greyscale;
} ropts_t;

ropts_t* init_render_options(char* video_name);
void init_render_env(ropts_t* ropts);

#endif
