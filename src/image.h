#ifndef LIBREALSENSE_IMAGE_H
#define LIBREALSENSE_IMAGE_H

#include <cstdint>

void convert_yuyv_to_rgb(uint8_t * dest, int width, int height, const uint8_t * source);

#endif
