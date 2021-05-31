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

/* Helper inner function (not part of the API) */
inline void next_pixel_in_line(float curr[2], const float start[2], const float end[2])
{
    float line_slope = (end[1] - start[1]) / (end[0] - start[0]);
    if (fabs(end[0] - curr[0]) > fabs(end[1] - curr[1]))
    {
        curr[0] = end[0] > curr[0] ? curr[0] + 1 : curr[0] - 1;
        curr[1] = end[1] - line_slope * (end[0] - curr[0]);
    }
    else
    {
        curr[1] = end[1] > curr[1] ? curr[1] + 1 : curr[1] - 1;
        curr[0] = end[0] - ((end[1] + curr[1]) / line_slope);
    }
}

/* Helper inner function (not part of the API) */
inline bool is_pixel_in_line(const float curr[2], const float start[2], const float end[2])
{
    return ((end[0] >= start[0] && end[0] >= curr[0] && curr[0] >= start[0]) || (end[0] <= start[0] && end[0] <= curr[0] && curr[0] <= start[0])) &&
        ((end[1] >= start[1] && end[1] >= curr[1] && curr[1] >= start[1]) || (end[1] <= start[1] && end[1] <= curr[1] && curr[1] <= start[1]));
}

/* Helper inner function (not part of the API) */
inline void adjust_2D_point_to_boundary(float p[2], int width, int height)
{
    if (p[0] < 0) p[0] = 0;
    if (p[0] > width) p[0] = (float)width;
    if (p[1] < 0) p[1] = 0;
    if (p[1] > height) p[1] = (float)height;
}


/* Given a point in 3D space, compute the corresponding pixel coordinates in an image with no distortion or forward distortion coefficients produced by the same camera */
void rs2_project_point_to_pixel(float pixel[2], const struct rs2_intrinsics* intrin, const float point[3]);

/* Given pixel coordinates and depth in an image with no distortion or inverse distortion coefficients, compute the corresponding point in 3D space relative to the same camera */
void rs2_deproject_pixel_to_point(float point[3], const struct rs2_intrinsics* intrin, const float pixel[2], float depth);

/* Transform 3D coordinates relative to one sensor to 3D coordinates relative to another viewpoint */
void rs2_transform_point_to_point(float to_point[3], const struct rs2_extrinsics* extrin, const float from_point[3]);

/* Calculate horizontal and vertical feild of view, based on video intrinsics */
void rs2_fov(const struct rs2_intrinsics* intrin, float to_fov[2]);

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
