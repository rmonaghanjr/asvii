#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

#include "bitmap.h"

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

struct vframe_chunk_args {
    int8_t thread_id;
    ropts_t* ropts;
    vframe_t* vframe_buffer;
    uint32_t* assigned_frames;
    size_t assigned_frames_sz;
    size_t last_rendered_frame;
};

ropts_t* init_render_options(char* video_name);
void init_render_env(ropts_t* ropts);
void cleanup_render_env(ropts_t* ropts);
vframe_t* create_vframe_buffer(ropts_t* ropts);
void* vframe_chunk_loader(void* args);

#endif
