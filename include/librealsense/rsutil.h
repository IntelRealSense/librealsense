#ifndef LIBREALSENSE_UTIL_INCLUDE_GUARD
#define LIBREALSENSE_UTIL_INCLUDE_GUARD

#include "rs.h"
#include "assert.h"

/* Given a point in 3D space, compute the corresponding pixel coordinates in an image with no distortion or forward distortion coefficients produced by the same camera */
static void rs_project_point_to_pixel(float pixel[2], const struct rs_intrinsics * intrin, const float point[3])
{
    assert(intrin->distortion_model != RS_DISTORTION_INVERSE_BROWN_CONRADY); // Cannot project to an inverse-distorted image

    float x = point[0] / point[2], y = point[1] / point[2];
    if(intrin->distortion_model == RS_DISTORTION_GORDON_BROWN_CONRADY)
    {
        float r2  = x*x + y*y;
        float f = 1 + intrin->distortion_coeff[0]*r2 + intrin->distortion_coeff[1]*r2*r2 + intrin->distortion_coeff[4]*r2*r2*r2;
        x *= f;
        y *= f;
        float dx = x + 2*intrin->distortion_coeff[2]*x*y + intrin->distortion_coeff[3]*(r2 + 2*x*x);
        float dy = y + 2*intrin->distortion_coeff[3]*x*y + intrin->distortion_coeff[2]*(r2 + 2*y*y);
        x = dx;
        y = dy;
    }
    pixel[0] = x * intrin->focal_length[0] + intrin->principal_point[0];
    pixel[1] = y * intrin->focal_length[1] + intrin->principal_point[1];
}

/* Given pixel coordinates and depth in an image with no distortion or inverse distortion coefficients, compute the corresponding point in 3D space relative to the same camera */
static void rs_deproject_pixel_to_point(float point[3], const struct rs_intrinsics * intrin, const float pixel[2], float depth)
{
    assert(intrin->distortion_model != RS_DISTORTION_GORDON_BROWN_CONRADY); // Cannot deproject from a forward-distorted image

    float x = (pixel[0] - intrin->principal_point[0]) / intrin->focal_length[0];
    float y = (pixel[1] - intrin->principal_point[1]) / intrin->focal_length[1];
    if(intrin->distortion_model == RS_DISTORTION_INVERSE_BROWN_CONRADY)
    {
        float r2  = x*x + y*y;
        float f = 1 + intrin->distortion_coeff[0]*r2 + intrin->distortion_coeff[1]*r2*r2 + intrin->distortion_coeff[4]*r2*r2*r2;
        float ux = x*f + 2*intrin->distortion_coeff[2]*x*y + intrin->distortion_coeff[3]*(r2 + 2*x*x);
        float uy = y*f + 2*intrin->distortion_coeff[3]*x*y + intrin->distortion_coeff[2]*(r2 + 2*y*y);
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

#endif
