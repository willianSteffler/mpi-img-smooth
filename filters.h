#ifndef FILTERS_H
# define FILTERS_H
#include"types.h"

void sort(unsigned char* array, int length);
RGB median(RGB* pixels,int length);
void apply_median_filter(RGB** image,int mask,int h, int w,int i, int j);

#endif