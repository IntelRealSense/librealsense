#pragma once
#ifndef LIBREALSENSE_IMAGE_H
#define LIBREALSENSE_IMAGE_H

#include "types.h"

namespace rsimpl
{
    size_t           get_image_size              (int width, int height, rs_format format);
    size_t           get_image_size              (int width, int height, uint32_t fourcc);
                                                 
    void             unpack_subrect              (void * dest[], const void * source, const subdevice_mode & mode);
                                                 
    void             unpack_y16_from_y8          (void * dest[], const void * source, const subdevice_mode & mode);
    void             unpack_y16_from_y16_10      (void * dest[], const void * source, const subdevice_mode & mode);
                                                 
    void             unpack_rgb_from_yuy2        (void * dest[], const void * source, const subdevice_mode & mode);
    void             unpack_rgba_from_yuy2       (void * dest[], const void * source, const subdevice_mode & mode);
    void             unpack_bgr_from_yuy2        (void * dest[], const void * source, const subdevice_mode & mode);
    void             unpack_bgra_from_yuy2       (void * dest[], const void * source, const subdevice_mode & mode);
                                                 
    void             unpack_y8_y8_from_y8i       (void * dest[], const void * source, const subdevice_mode & mode);
    void             unpack_y16_y16_from_y12i_10 (void * dest[], const void * source, const subdevice_mode & mode);
                                                 
    void             unpack_z16_y8_from_inri     (void * dest[], const void * source, const subdevice_mode & mode);
    void             unpack_z16_y16_from_inri    (void * dest[], const void * source, const subdevice_mode & mode);
                                                 
    void             align_depth_to_color        (void * depth_aligned_to_color, const uint16_t * depth_pixels, float depth_scale, const rs_intrinsics & depth_intrin, 
                                                  const rs_extrinsics & depth_to_color, const rs_intrinsics & color_intrin);
    void             align_color_to_depth        (void * color_aligned_to_depth, const uint16_t * depth_pixels, float depth_scale, const rs_intrinsics & depth_intrin, 
                                                  const rs_extrinsics & depth_to_color, const rs_intrinsics & color_intrin, const void * color_pixels, rs_format color_format);

    std::vector<int> compute_rectification_table (const rs_intrinsics & rect_intrin, const rs_extrinsics & rect_to_unrect, const rs_intrinsics & unrect_intrin);
    void             rectify_image               (void * rect_pixels, const std::vector<int> & rectification_table, const void * unrect_pixels, rs_format format);
}

#endif
