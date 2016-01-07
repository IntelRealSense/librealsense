/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

#ifndef LIBREALSENSE_RSUTIL_H
#define LIBREALSENSE_RSUTIL_H

#include "rs.h"
#include "assert.h"

/* Given a point in 3D space, compute the corresponding pixel coordinates in an image with no distortion or forward distortion coefficients produced by the same camera */
static void rs_project_point_to_pixel(float pixel[2], const struct rs_intrinsics * intrin, const float point[3])
{
    assert(intrin->model != RS_DISTORTION_INVERSE_BROWN_CONRADY); // Cannot project to an inverse-distorted image

    float x = point[0] / point[2], y = point[1] / point[2];
    if(intrin->model == RS_DISTORTION_MODIFIED_BROWN_CONRADY)
    {
        float r2  = x*x + y*y;
        float f = 1 + intrin->coeffs[0]*r2 + intrin->coeffs[1]*r2*r2 + intrin->coeffs[4]*r2*r2*r2;
        x *= f;
        y *= f;
        float dx = x + 2*intrin->coeffs[2]*x*y + intrin->coeffs[3]*(r2 + 2*x*x);
        float dy = y + 2*intrin->coeffs[3]*x*y + intrin->coeffs[2]*(r2 + 2*y*y);
        x = dx;
        y = dy;
    }
    pixel[0] = x * intrin->fx + intrin->ppx;
    pixel[1] = y * intrin->fy + intrin->ppy;
}

/* Given pixel coordinates and depth in an image with no distortion or inverse distortion coefficients, compute the corresponding point in 3D space relative to the same camera */
static void rs_deproject_pixel_to_point(float point[3], const struct rs_intrinsics * intrin, const float pixel[2], float depth)
{
    assert(intrin->model != RS_DISTORTION_MODIFIED_BROWN_CONRADY); // Cannot deproject from a forward-distorted image

    float x = (pixel[0] - intrin->ppx) / intrin->fx;
    float y = (pixel[1] - intrin->ppy) / intrin->fy;
    if(intrin->model == RS_DISTORTION_INVERSE_BROWN_CONRADY)
    {
        float r2  = x*x + y*y;
        float f = 1 + intrin->coeffs[0]*r2 + intrin->coeffs[1]*r2*r2 + intrin->coeffs[4]*r2*r2*r2;
        float ux = x*f + 2*intrin->coeffs[2]*x*y + intrin->coeffs[3]*(r2 + 2*x*x);
        float uy = y*f + 2*intrin->coeffs[3]*x*y + intrin->coeffs[2]*(r2 + 2*y*y);
        x = ux;
        y = uy;
    }
    point[0] = depth * x;
    point[1] = depth * y;
    point[2] = depth;
}

/* Transform 3D coordinates relative to one sensor to 3D coordinates relative to another viewpoint */
static void rs_transform_point_to_point(float to_point[3], const struct rs_extrinsics * extrin, const float from_point[3])
{
    to_point[0] = extrin->rotation[0] * from_point[0] + extrin->rotation[3] * from_point[1] + extrin->rotation[6] * from_point[2] + extrin->translation[0];
    to_point[1] = extrin->rotation[1] * from_point[0] + extrin->rotation[4] * from_point[1] + extrin->rotation[7] * from_point[2] + extrin->translation[1];
    to_point[2] = extrin->rotation[2] * from_point[0] + extrin->rotation[5] * from_point[1] + extrin->rotation[8] * from_point[2] + extrin->translation[2];
}

/* Provide access to several recommend sets of depth control parameters */
void rs_apply_depth_control_preset(rs_device * device, int preset)
{
    static const rs_option depth_control_options[10] = {
        RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_DECREMENT,
        RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_INCREMENT,
        RS_OPTION_R200_DEPTH_CONTROL_MEDIAN_THRESHOLD,
        RS_OPTION_R200_DEPTH_CONTROL_SCORE_MINIMUM_THRESHOLD,
        RS_OPTION_R200_DEPTH_CONTROL_SCORE_MAXIMUM_THRESHOLD,
        RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_COUNT_THRESHOLD, 
        RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_DIFFERENCE_THRESHOLD,
        RS_OPTION_R200_DEPTH_CONTROL_SECOND_PEAK_THRESHOLD,
        RS_OPTION_R200_DEPTH_CONTROL_NEIGHBOR_THRESHOLD,
        RS_OPTION_R200_DEPTH_CONTROL_LR_THRESHOLD
    };
    double depth_control_presets[6][10] = {
        {5, 5, 192,  1,  512, 6, 24, 27,  7,   24}, /* (DEFAULT)   Default settings on chip. Similiar to the medium setting and best for outdoors. */
        {5, 5,   0,  0, 1023, 0,  0,  0,  0, 2047}, /* (OFF)       Disable almost all hardware-based outlier removal */
        {5, 5, 115,  1,  512, 6, 18, 25,  3,   24}, /* (LOW)       Provide a depthmap with a lower number of outliers removed, which has minimal false negatives. */
        {5, 5, 185,  5,  505, 6, 35, 45, 45,   14}, /* (MEDIUM)    Provide a depthmap with a medium number of outliers removed, which has balanced approach. */
        {5, 5, 175, 24,  430, 6, 48, 47, 24,   12}, /* (OPTIMIZED) Provide a depthmap with a medium/high number of outliers removed. Derived from an optimization function. */
        {5, 5, 235, 27,  420, 8, 80, 70, 90,   12}, /* (HIGH)      Provide a depthmap with a higher number of outliers removed, which has minimal false positives. */
    };
    rs_set_options(device, depth_control_options, 10, depth_control_presets[preset], nullptr);
}

#endif
