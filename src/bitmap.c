#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

#include "bitmap.h"

int read_frame(FRAME* result, char* filename) {
    FILE* fp;
    fp = fopen(filename, "r");

    if (!fp) {
        printf("cannot open file! %s\n", filename);
        return 0;
    }

    fseek(fp, 0, SEEK_SET);

    for (int i = 0; i < 14; i++) {
        result->frame_file_header[i] = fgetc(fp);
    }

    if (result->frame_file_header[0] != 'B' || result->frame_file_header[1] != 'M') {
        printf("invalid header file!\n");
        return 0;
    }

    unsigned int *array_offset_ptr = (unsigned int *)(result->frame_file_header + 10);
    result->pixel_array_offset = *array_offset_ptr;

    if (result->frame_file_header[11] != 0 || result->frame_file_header[12] != 0 || result->frame_file_header[13] != 0) {
        printf("unknown error!\n");
        return 1;
    }

    for (int i = 0; i < 40; i++) {
        fseek(fp, result->pixel_array_offset, SEEK_CUR + sizeof(int));
        result->frame_info_header[i] = (int) fgetc(fp);
    }

    int *width_ptr = (int *)(result->frame_info_header + 4);
    int *height_ptr = (int *)(result->frame_info_header + 8);

    result->width = *width_ptr;
    result->height = *height_ptr;

    result->bits_per_pixel = result->frame_info_header[14];
    if (result->bits_per_pixel != 24) {
        printf("invalid file!\n");
    }

    int compressionMethod = result->frame_info_header[16];
    if (compressionMethod != 0) {
        printf("unhandled compression type!\n");
        return 0;
    }

    result->row_size = (int) (floor((result->bits_per_pixel * result->width + 31.) / 32)) * 4;
    result->pixel_array_size = result->row_size * abs(result->height);

    result->pixel_data = (unsigned char*) malloc(sizeof(char) * result->pixel_array_size);

    fseek(fp, result->pixel_array_offset, SEEK_SET);
    for (int i = 0; i < result->pixel_array_size; i++) {
        fseek(fp, result->pixel_array_offset, SEEK_CUR + sizeof(uint32_t));
        result->pixel_data[i] = (uint32_t) fgetc(fp);
    }

    fclose(fp);

    return 1;
}

int pixel_at(FRAME *frame, unsigned int* r, unsigned int* g, unsigned int* b, int x, int y) {
    if (x < frame->width && y < frame->height) {
        y = frame->height - 1 - y;
        
        (*r) = (unsigned int)(frame->pixel_data[frame->row_size * y + 3 * x + 2]);
        (*g) = (unsigned int)(frame->pixel_data[frame->row_size * y + 3 * x + 1]);
        (*b) = (unsigned int)(frame->pixel_data[frame->row_size * y + 3 * x + 0]);
        return 1;
    } else {
        return 0;
    }
}

int sample_region(FRAME* frame, int w, int h, int s_x, int s_y) {
    int avg_luminance = 0;
    int s_samples = 0;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            unsigned int r;
            unsigned int g;
            unsigned int b;

            int did_grab_pixel = pixel_at(frame, &r, &g, &b, s_x+x, s_y+y);

            if (did_grab_pixel) {
                avg_luminance += (r + g + b) / 3;
                s_samples++;
            }
        }
    }

    return avg_luminance / s_samples;
}
