/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

#pragma once
#ifndef LIBREALSENSE_IMAGE_H
#define LIBREALSENSE_IMAGE_H

#include "types.h"

namespace rsimpl
{
    size_t           get_image_size                 (int width, int height, rs_format format);
                                                    
    void             unpack_subrect                 (byte * const dest[], const byte * source, const subdevice_mode & mode);

    void             unpack_y16_from_y8             (byte * const dest[], const byte * source, const subdevice_mode & mode);
    void             unpack_y16_from_y16_10         (byte * const dest[], const byte * source, const subdevice_mode & mode);
    void             unpack_y8_from_y16_10          (byte * const dest[], const byte * source, const subdevice_mode & mode);
                                                             
    void             unpack_rgb_from_yuy2           (byte * const dest[], const byte * source, const subdevice_mode & mode);
    void             unpack_rgba_from_yuy2          (byte * const dest[], const byte * source, const subdevice_mode & mode);
    void             unpack_bgr_from_yuy2           (byte * const dest[], const byte * source, const subdevice_mode & mode);
    void             unpack_bgra_from_yuy2          (byte * const dest[], const byte * source, const subdevice_mode & mode);
                                                             
    void             unpack_y8_y8_from_y8i          (byte * const dest[], const byte * source, const subdevice_mode & mode);
    void             unpack_y16_y16_from_y12i_10    (byte * const dest[], const byte * source, const subdevice_mode & mode);
                                                             
    void             unpack_z16_y8_from_f200_inzi   (byte * const dest[], const byte * source, const subdevice_mode & mode);
    void             unpack_z16_y16_from_f200_inzi  (byte * const dest[], const byte * source, const subdevice_mode & mode);
                                                             
    void             unpack_z16_y8_from_sr300_inzi  (byte * const dest[], const byte * source, const subdevice_mode & mode);
    void             unpack_z16_y16_from_sr300_inzi (byte * const dest[], const byte * source, const subdevice_mode & mode);

    void             align_z_to_color               (byte * z_aligned_to_color, const uint16_t * z_pixels, float z_scale, const rs_intrinsics & z_intrin, 
                                                     const rs_extrinsics & z_to_color, const rs_intrinsics & color_intrin);
    void             align_disparity_to_color       (byte * disparity_aligned_to_color, const uint16_t * disparity_pixels, float disparity_scale, const rs_intrinsics & disparity_intrin, 
                                                     const rs_extrinsics & disparity_to_color, const rs_intrinsics & color_intrin);
    void             align_color_to_z               (byte * color_aligned_to_z, const uint16_t * z_pixels, float z_scale, const rs_intrinsics & z_intrin, 
                                                     const rs_extrinsics & z_to_color, const rs_intrinsics & color_intrin, const byte * color_pixels, rs_format color_format);
    void             align_color_to_disparity       (byte * color_aligned_to_disparity, const uint16_t * disparity_pixels, float disparity_scale, const rs_intrinsics & disparity_intrin, 
                                                     const rs_extrinsics & disparity_to_color, const rs_intrinsics & color_intrin, const byte * color_pixels, rs_format color_format);

    std::vector<int> compute_rectification_table    (const rs_intrinsics & rect_intrin, const rs_extrinsics & rect_to_unrect, const rs_intrinsics & unrect_intrin);
    void             rectify_image                  (byte * rect_pixels, const std::vector<int> & rectification_table, const byte * unrect_pixels, rs_format format);

    extern const native_pixel_format pf_rw10;       // Four 10 bit luminance values in one 40 bit macropixel
    extern const native_pixel_format pf_yuy2;       // Y0 U Y1 V ordered chroma subsampled macropixel
    extern const native_pixel_format pf_y8;         // 8 bit (left) IR image
    extern const native_pixel_format pf_y8i;        // 8 bits left IR + 8 bits right IR per pixel
    extern const native_pixel_format pf_y16;        // 16 bit (left) IR image
    extern const native_pixel_format pf_y12i;       // 12 bits left IR + 12 bits right IR per pixel
    extern const native_pixel_format pf_z16;        // 16 bit Z image
    extern const native_pixel_format pf_invz;       // 16 bit Z image
    extern const native_pixel_format pf_f200_invi;  // 8-bit IR image
    extern const native_pixel_format pf_f200_inzi;  // 16-bit Z + 8 bit IR per pixel
    extern const native_pixel_format pf_sr300_invi; // 16-bit IR image
    extern const native_pixel_format pf_sr300_inzi; // Planar 16-bit IR image followed by 16-bit Z image
}

#endif
