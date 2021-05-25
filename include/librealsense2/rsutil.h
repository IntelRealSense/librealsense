/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2015 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALSENSE_RSUTIL2_H
#define LIBREALSENSE_RSUTIL2_H

#ifdef __cplusplus
extern "C" {
#endif

#include "h/rs_types.h"
#include "h/rs_sensor.h"
#include "h/rs_frame.h"
#include "rs.h"
#include "assert.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <float.h>


/* Given a point in 3D space, compute the corresponding pixel coordinates in an image with no distortion or forward distortion coefficients produced by the same camera */
void rs2_project_point_to_pixel(float pixel[2], const struct rs2_intrinsics* intrin, const float point[3]);

/* Given pixel coordinates and depth in an image with no distortion or inverse distortion coefficients, compute the corresponding point in 3D space relative to the same camera */
void rs2_deproject_pixel_to_point(float point[3], const struct rs2_intrinsics* intrin, const float pixel[2], float depth);

/* Transform 3D coordinates relative to one sensor to 3D coordinates relative to another viewpoint */
void rs2_transform_point_to_point(float to_point[3], const struct rs2_extrinsics* extrin, const float from_point[3]);

/* Calculate horizontal and vertical feild of view, based on video intrinsics */
void rs2_fov(const struct rs2_intrinsics* intrin, float to_fov[2]);

void next_pixel_in_line(float curr[2], const float start[2], const float end[2]);

bool is_pixel_in_line(const float curr[2], const float start[2], const float end[2]);

void adjust_2D_point_to_boundary(float p[2], int width, int height);

/* Find projected pixel with unknown depth search along line. */
void rs2_project_color_pixel_to_depth_pixel(float to_pixel[2],
    const uint16_t* data, float depth_scale,
    float depth_min, float depth_max,
    const struct rs2_intrinsics* depth_intrin,
    const struct rs2_intrinsics* color_intrin,
    const struct rs2_extrinsics* color_to_depth,
    const struct rs2_extrinsics* depth_to_color,
    const float from_pixel[2]);

#ifdef __cplusplus
}
#endif

#endif 
