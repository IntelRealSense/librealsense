#pragma once
#ifndef LIBREALSENSE_IMAGE_H
#define LIBREALSENSE_IMAGE_H

namespace rsimpl
{
    void copy_strided_image(void * dest_image, int dest_stride, const void * source_image, int source_stride, int rows);
    void convert_yuyv_to_rgb(void * dest_image, int width, int height, const void * source_image);
    void convert_yuyv_to_rgba(void * dest_image, int width, int height, const void * source_image);
    void convert_yuyv_to_bgr(void * dest_image, int width, int height, const void * source_image);
    void convert_yuyv_to_bgra(void * dest_image, int width, int height, const void * source_image);
    void convert_y12i_to_y8_y8(void * dest_left, void * dest_right, int width, int height, const void * source_image, int source_stride);
    void convert_y12i_to_y16_y16(void * dest_left, void * dest_right, int width, int height, const void * source_image, int source_stride);
}

#endif
