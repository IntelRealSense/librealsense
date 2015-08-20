#ifndef LIBREALSENSE_IMAGE_H
#define LIBREALSENSE_IMAGE_H

#include <cstdint>

void copy_strided_image(void * dest_image, int dest_stride, const void * source_image, int source_stride, int rows);
void convert_yuyv_to_rgb(uint8_t * dest, int width, int height, const uint8_t * source);

#endif
