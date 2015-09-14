#pragma once
#ifndef LIBREALSENSE_IMAGE_H
#define LIBREALSENSE_IMAGE_H

#include "types.h"

namespace rsimpl
{
    size_t get_image_size(int width, int height, rs_format format);
    size_t get_image_size(int width, int height, uint32_t fourcc);

    void unpack_subrect             (void * dest[], const void * source, const subdevice_mode & mode);

    void unpack_rgb_from_yuy2       (void * dest[], const void * source, const subdevice_mode & mode);
    void unpack_rgba_from_yuy2      (void * dest[], const void * source, const subdevice_mode & mode);
    void unpack_bgr_from_yuy2       (void * dest[], const void * source, const subdevice_mode & mode);
    void unpack_bgra_from_yuy2      (void * dest[], const void * source, const subdevice_mode & mode);

    void unpack_y16_from_y10        (void * dest[], const void * source, const subdevice_mode & mode);
    void unpack_y8_y8_from_y8i      (void * dest[], const void * source, const subdevice_mode & mode);
    void unpack_y16_y16_from_y12i   (void * dest[], const void * source, const subdevice_mode & mode);

    void unpack_z16_y8_from_inri    (void * dest[], const void * source, const subdevice_mode & mode);
}

#endif
