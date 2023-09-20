#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>

int fast_atoi(const char* str);
char* exec_ffmpg_query(char* cmd);
uint32_t get_frame_count(char* video_name);
int get_duration(char* video_name);
void get_dimensions(char* video_name, uint16_t* width, uint16_t* height);
void get_frames(int frame_count, char* video_name);

#endif
