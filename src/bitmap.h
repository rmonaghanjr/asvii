#ifndef BITMAP_H
#define BITMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

typedef struct {
    unsigned char frame_file_header[14];
    unsigned int pixel_array_offset;
    unsigned char frame_info_header[40];

    int height;
    int width;
    int bits_per_pixel;

    int row_size;
    int pixel_array_size;

    unsigned char *pixel_data;
    const char *filename;
} vframe_t;

int read_frame(vframe_t* result, char* filename);
int pixel_at(vframe_t* frame, unsigned int* r, unsigned int* g, unsigned int* b, int x, int y);
int sample_region(vframe_t* frame, int w, int h, int s_x, int s_y);

#endif
