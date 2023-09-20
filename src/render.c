#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "render.h"
#include "video.h"

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
    // TODO: get video width/height
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

