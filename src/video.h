#ifndef VIDEO_H
#define VIDEO_H

int fast_atoi(const char* str);
char* exec_ffmpg_query(char* cmd);
int get_frame_count(char* video_name);
int get_duration(char* video_name);
void get_frames(int frame_count, char* video_name);

#endif
