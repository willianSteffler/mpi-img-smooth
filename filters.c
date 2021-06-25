#include "filters.h"

void sort(unsigned char* array, int length){
    int i = 1;
    int j;
    int x;

    for (i=1; i<length; i++){
        x = array[i];
        for(j = i-1; j >= 0 && array[j]>x; j--){
            array[j+1] = array[j];
        }
        array[j+1] = x;
    }
}

RGB median(RGB* pixels,int length){

    RGB result;

    unsigned char* pixels_r = (char *)malloc(length);
    unsigned char* pixels_g = (char *)malloc(length);
    unsigned char* pixels_b = (char *)malloc(length);

    for (int i = 0; i < length; i++)
    {
        pixels_r[i] = pixels[i].r;
        pixels_g[i] = pixels[i].g;
        pixels_b[i] = pixels[i].b;
    }
    
    sort(pixels_r,length);
    sort(pixels_g,length);
    sort(pixels_b,length);

    result.r = pixels_r[length/2];
    result.g = pixels_r[length/2];
    result.b = pixels_r[length/2];

    return result;
}

void apply_median_filter(RGB** image,int mask,int h, int w,int i, int j){
    // raio do centro
    int r = mask - (mask / 2) - 1;
    int l;
    int k;
    int sz = 0;

    RGB* pixels = (RGB *)malloc(sizeof(RGB*) * mask * mask);

    for (l = i-r < 0 ? 0 : i-r; l < i+r && l < h; l++)
    {
        for (k = j-r < 0 ? 0 : j-r; k < j+r && k < w ; k++)
        {
            pixels[sz++] = image[l][k];
        }
    }

    image[i][j] = median(pixels,sz);
}