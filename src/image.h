// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_IMAGE_H
#define LIBREALSENSE_IMAGE_H

#include "types.h"

namespace librealsense
{
    void unpack_yuy2_y8(byte * const d[], const byte * s, int w, int h);
    void unpack_yuy2_y16(byte * const d[], const byte * s, int w, int h);
    void unpack_yuy2_rgb8(byte * const d[], const byte * s, int w, int h);
    void unpack_yuy2_rgba8(byte * const d[], const byte * s, int w, int h);
    void unpack_yuy2_bgr8(byte * const d[], const byte * s, int w, int h);
    void unpack_yuy2_bgra8(byte * const d[], const byte * s, int w, int h);

    size_t           get_image_size                 (int width, int height, rs2_format format);
    int              get_image_bpp                  (rs2_format format);
    void             deproject_z                    (float * points, const rs2_intrinsics & z_intrin, const uint16_t * z_pixels, float z_scale);
    void             deproject_disparity            (float * points, const rs2_intrinsics & disparity_intrin, const uint16_t * disparity_pixels, float disparity_scale);

    void             align_z_to_other               (byte * z_aligned_to_other, const uint16_t * z_pixels, float z_scale, const rs2_intrinsics & z_intrin,
                                                     const rs2_extrinsics & z_to_other, const rs2_intrinsics & other_intrin);
    void             align_disparity_to_other       (byte * disparity_aligned_to_other, const uint16_t * disparity_pixels, float disparity_scale, const rs2_intrinsics & disparity_intrin,
                                                     const rs2_extrinsics & disparity_to_other, const rs2_intrinsics & other_intrin);
    void             align_other_to_z               (byte * other_aligned_to_z, const uint16_t * z_pixels, float z_scale, const rs2_intrinsics & z_intrin,
                                                     const rs2_extrinsics & z_to_other, const rs2_intrinsics & other_intrin, const byte * other_pixels, rs2_format other_format);
    void             align_other_to_disparity       (byte * other_aligned_to_disparity, const uint16_t * disparity_pixels, float disparity_scale, const rs2_intrinsics & disparity_intrin,
                                                     const rs2_extrinsics & disparity_to_other, const rs2_intrinsics & other_intrin, const byte * other_pixels, rs2_format other_format);

    std::vector<int> compute_rectification_table    (const rs2_intrinsics & rect_intrin, const rs2_extrinsics & rect_to_unrect, const rs2_intrinsics & unrect_intrin);
    void             rectify_image                  (uint8_t * rect_pixels, const std::vector<int> & rectification_table, const uint8_t * unrect_pixels, rs2_format format);

    extern const native_pixel_format pf_fe_raw8_unpatched_kernel; // W/O for unpatched kernel
    extern const native_pixel_format pf_raw8;       // 8 bit luminance
    extern const native_pixel_format pf_rw10;       // Four 10 bit luminance values in one 40 bit macropixel
    extern const native_pixel_format pf_w10;        // Four 10 bit luminance values in one 40 bit macropixel
    extern const native_pixel_format pf_rw16;       // 10 bit in 16 bit WORD with 6 bit unused
    extern const native_pixel_format pf_bayer16;    // 16-bit Bayer raw
    extern const native_pixel_format pf_yuy2;       // Y0 U Y1 V ordered chroma subsampled macropixel
    extern const native_pixel_format pf_yuyv;       // Y0 U Y1 V ordered chroma subsampled macropixel
    extern const native_pixel_format pf_y8;         // 8 bit IR/Luminosity (left) imager
    extern const native_pixel_format pf_y8i;        // 8 bits left IR + 8 bits right IR per pixel
    extern const native_pixel_format pf_y16;        // 16 bit (left) IR image
    extern const native_pixel_format pf_y12i;       // 12 bits left IR + 12 bits right IR per pixel
    extern const native_pixel_format pf_z16;        // 16 bit Z + Disparity image
    extern const native_pixel_format pf_invz;       // 16 bit Z image
    extern const native_pixel_format pf_f200_invi;  // 8-bit IR image
    extern const native_pixel_format pf_f200_inzi;  // 16-bit Z + 8 bit IR per pixel
    extern const native_pixel_format pf_sr300_invi; // 16-bit IR image
    extern const native_pixel_format pf_sr300_inzi; // Planar 16-bit IR image followed by 16-bit Z image
    extern const native_pixel_format pf_uyvyl;      // U Y0 V Y1 ordered chroma subsampled macropixel for Infrared stream
    extern const native_pixel_format pf_uyvyc;      // U Y0 V Y1 ordered chroma subsampled macropixel for Color stream
    extern const native_pixel_format pf_accel_axes;   // Parse accel HID raw data to 3 axes
    extern const native_pixel_format pf_gyro_axes;   // Parse gyro HID raw data to 3 axes
    extern const native_pixel_format pf_rgb888;
    extern const native_pixel_format pf_gpio_timestamp; // Parse GPIO timestamp
    extern const native_pixel_format pf_confidence_l500;
    extern const native_pixel_format pf_z16_l500;
    extern const native_pixel_format pf_y8_l500;
}

#endif
