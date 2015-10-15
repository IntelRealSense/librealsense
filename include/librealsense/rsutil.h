#ifndef LIBREALSENSE_RSUTIL_H
#define LIBREALSENSE_RSUTIL_H

#include "rs.h"

/* Given a point in 3D space, compute the corresponding pixel coordinates in an image with no distortion or forward distortion coefficients produced by the same camera */
void rs_project_point_to_pixel(float pixel[2], const struct rs_intrinsics * intrin, const float point[3]);

/* Given pixel coordinates and depth in an image with no distortion or inverse distortion coefficients, compute the corresponding point in 3D space relative to the same camera */
void rs_deproject_pixel_to_point(float point[3], const struct rs_intrinsics * intrin, const float pixel[2], float depth);

/* Transform 3D coordinates relative to one sensor to 3D coordinates relative to another viewpoint */
void rs_transform_point_to_point(float to_point[3], const struct rs_extrinsics * extrin, const float from_point[3]);

/* Helper functions for basic linear algebra operations */
void rs_add_vec3_vec3(float r[3], const float a[3], const float b[3]);
void rs_multiply_mat3x3_vec3(float r[3], const float a[9], const float b[3]);
void rs_multiply_mat3x3_mat3x3(float r[9], const float a[9], const float b[9]);
void rs_transpose_mat3x3(float r[9], const float a[9]);
void rs_identity_mat3x3(float r[9]);

/* Helper functions for manipulating extrinsics */
void rs_invert_extrinsics(rs_extrinsics * b_to_a, const rs_extrinsics * a_to_b);
void rs_compose_extrinsics(rs_extrinsics * a_to_c, const rs_extrinsics * a_to_b, const rs_extrinsics * b_to_c);

/* Helper functions for rectifying images */
void rs_compute_rectified_parameters(rs_intrinsics * rect_intrinsics, rs_extrinsics * depth_to_rect_extrinsics, 
    const rs_intrinsics * unrect_intrinsics, const rs_extrinsics * depth_to_unrect_extrinsics);
void rs_compute_rectification_table(int * rectification_table, 
    const rs_intrinsics * rect_intrin, const rs_extrinsics * depth_to_rect_extrin,
    const rs_intrinsics * unrect_intrin, const rs_extrinsics * depth_to_unrect_extrin);
void rs_rectify_image(void * rect_pixels, const rs_intrinsics * rect_intrin, const int * rectification_table, const void * unrect_pixels, rs_format format);

#endif

#ifdef RSUTIL_IMPLEMENTATION
#include "stdint.h" /* for uint16_t */
#include "assert.h" /* for assert(...) */
#include "math.h" /* for roundf(...) */

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



void rs_add_vec3_vec3(float r[3], const float a[3], const float b[3])
{
    r[0] = a[0] + b[0];
    r[1] = a[1] + b[1];
    r[2] = a[2] + b[2];
}

void rs_multiply_mat3x3_vec3(float r[3], const float a[9], const float b[3])
{
    r[0] = a[0] * b[0] + a[3] * b[1] + a[6] * b[2];
    r[1] = a[1] * b[0] + a[4] * b[1] + a[7] * b[2];
    r[2] = a[2] * b[0] + a[5] * b[1] + a[8] * b[2];
}

void rs_multiply_mat3x3_mat3x3(float r[9], const float a[9], const float b[9])
{
    rs_multiply_mat3x3_vec3(r+0, a, b+0);
    rs_multiply_mat3x3_vec3(r+3, a, b+3);
    rs_multiply_mat3x3_vec3(r+6, a, b+6);
}

void rs_transpose_mat3x3(float r[9], const float a[9])
{
    r[0] = a[0]; 
    r[1] = a[3];
    r[2] = a[6];
    r[3] = a[1];
    r[4] = a[4];
    r[5] = a[7];
    r[6] = a[2];
    r[7] = a[5];
    r[8] = a[8];
}

void rs_identity_mat3x3(float r[9])
{
    r[0] = 1; 
    r[1] = 0;
    r[2] = 0;
    r[3] = 0;
    r[4] = 1;
    r[5] = 0;
    r[6] = 0;
    r[7] = 0;
    r[8] = 1;
}



void rs_transform_point_to_point(float to_point[3], const struct rs_extrinsics * extrin, const float from_point[3])
{
    rs_multiply_mat3x3_vec3(to_point, extrin->rotation, from_point);
    rs_add_vec3_vec3(to_point, to_point, extrin->translation);
}

void rs_compose_extrinsics(rs_extrinsics * a_to_c, const rs_extrinsics * a_to_b, const rs_extrinsics * b_to_c)
{
    rs_multiply_mat3x3_mat3x3(a_to_c->rotation, a_to_b->rotation, b_to_c->rotation);
    rs_transform_point_to_point(a_to_c->translation, a_to_b, b_to_c->translation);
}

void rs_invert_extrinsics(rs_extrinsics * b_to_a, const rs_extrinsics * a_to_b)
{
    rs_transpose_mat3x3(b_to_a->rotation, a_to_b->rotation);
    rs_multiply_mat3x3_vec3(b_to_a->translation, b_to_a->rotation, a_to_b->translation);
    b_to_a->translation[0] = -b_to_a->translation[0];
    b_to_a->translation[1] = -b_to_a->translation[1];
    b_to_a->translation[2] = -b_to_a->translation[2];
}



void rs_compute_rectified_parameters(rs_intrinsics * rect_intrinsics, rs_extrinsics * depth_to_rect_extrinsics, const rs_intrinsics * unrect_intrinsics, const rs_extrinsics * depth_to_unrect_extrinsics)
{
    float unrect_to_rect_rotation[9];

    /* Rectified intrinsics will match unrectified intrinsics, but have no distortion */
    *rect_intrinsics = *unrect_intrinsics;
    rect_intrinsics->model = RS_DISTORTION_NONE;

    /* Rectified extrinsics will be set up to have no rotation relative to the depth camera */
    rs_transpose_mat3x3(unrect_to_rect_rotation, depth_to_unrect_extrinsics->rotation);
    rs_identity_mat3x3(depth_to_rect_extrinsics->rotation); /* depth_to_unrect * unrect_to_rect = depth_to_unrect * depth_to_unrect^-1 = identity */
    rs_multiply_mat3x3_vec3(depth_to_rect_extrinsics->translation, unrect_to_rect_rotation, depth_to_unrect_extrinsics->translation);
}

void rs_compute_rectification_table(int * rectification_table, 
    const rs_intrinsics * rect_intrin, const rs_extrinsics * depth_to_rect_extrin,
    const rs_intrinsics * unrect_intrin, const rs_extrinsics * depth_to_unrect_extrin)
{   
    int x, y, ux, uy;
    rs_extrinsics rect_to_depth, rect_to_unrect;

    rs_invert_extrinsics(&rect_to_depth, depth_to_rect_extrin);
    rs_compose_extrinsics(&rect_to_unrect, &rect_to_depth, depth_to_unrect_extrin);
    /* NOTE: rect_to_unrect.translation will be approximately the zero vector, and we can ignore it going forward */

    for(y=0; y<rect_intrin->height; ++y)
    {
        for(x=0; x<rect_intrin->width; ++x)
        {
            float rect_pixel[2] = {x,y}, rect_ray[3], unrect_ray[3], unrect_pixel[2];
            rs_deproject_pixel_to_point(rect_ray, rect_intrin, rect_pixel, 1);
            rs_multiply_mat3x3_vec3(unrect_ray, rect_to_unrect.rotation, rect_ray);
            rs_project_point_to_pixel(unrect_pixel, unrect_intrin, unrect_ray);
            ux = roundf(unrect_pixel[0]);
            ux = ux < 0 ? 0 : ux;
            ux = ux >= unrect_intrin->width ? unrect_intrin->width - 1 : ux;
            uy = roundf(unrect_pixel[1]);
            uy = uy < 0 ? 0 : uy;
            uy = uy >= unrect_intrin->height ? unrect_intrin->height - 1 : uy;
            *rectification_table++ = uy * unrect_intrin->width + ux;
        }
    }
}



#pragma pack(push, 1)
typedef struct rs_byte1 { char b[1]; } rs_byte1;
typedef struct rs_byte2 { char b[2]; } rs_byte2;
typedef struct rs_byte3 { char b[3]; } rs_byte3;
typedef struct rs_byte4 { char b[4]; } rs_byte4;
#pragma pack(pop)

void rs_rectify_image_8bpp(void * rect_pixels, const rs_intrinsics * rect_intrin, const int * rectification_table, const void * unrect_pixels)
{
    rs_byte1 * rect = (rs_byte1 *)rect_pixels;
    const rs_byte1 * unrect = (const rs_byte1 *)unrect_pixels;
    int i, n = rect_intrin->width * rect_intrin->height;
    for(i=0; i<n; ++i) rect[i] = unrect[rectification_table[i]];
}

void rs_rectify_image_16bpp(void * rect_pixels, const rs_intrinsics * rect_intrin, const int * rectification_table, const void * unrect_pixels)
{
    rs_byte2 * rect = (rs_byte2 *)rect_pixels;
    const rs_byte2 * unrect = (const rs_byte2 *)unrect_pixels;
    int i, n = rect_intrin->width * rect_intrin->height;
    for(i=0; i<n; ++i) rect[i] = unrect[rectification_table[i]];
}

void rs_rectify_image_24bpp(void * rect_pixels, const rs_intrinsics * rect_intrin, const int * rectification_table, const void * unrect_pixels)
{
    rs_byte3 * rect = (rs_byte3 *)rect_pixels;
    const rs_byte3 * unrect = (const rs_byte3 *)unrect_pixels;
    int i, n = rect_intrin->width * rect_intrin->height;
    for(i=0; i<n; ++i) rect[i] = unrect[rectification_table[i]];
}

void rs_rectify_image_32bpp(void * rect_pixels, const rs_intrinsics * rect_intrin, const int * rectification_table, const void * unrect_pixels)
{
    rs_byte4 * rect = (rs_byte4 *)rect_pixels;
    const rs_byte4 * unrect = (const rs_byte4 *)unrect_pixels;
    int i, n = rect_intrin->width * rect_intrin->height;
    for(i=0; i<n; ++i) rect[i] = unrect[rectification_table[i]];
}

void rs_rectify_image(void * rect_pixels, const rs_intrinsics * rect_intrin, const int * rectification_table, const void * unrect_pixels, rs_format format)
{
    switch(format)
    {
    case RS_FORMAT_Y8:
        rs_rectify_image_8bpp(rect_pixels, rect_intrin, rectification_table, unrect_pixels);
        break;
    case RS_FORMAT_Y16: case RS_FORMAT_Z16:
        rs_rectify_image_16bpp(rect_pixels, rect_intrin, rectification_table, unrect_pixels);
        break;
    case RS_FORMAT_RGB8: case RS_FORMAT_BGR8:
        rs_rectify_image_24bpp(rect_pixels, rect_intrin, rectification_table, unrect_pixels);
        break;
    case RS_FORMAT_RGBA8: case RS_FORMAT_BGRA8:
        rs_rectify_image_32bpp(rect_pixels, rect_intrin, rectification_table, unrect_pixels);
        break;
    default:
        /* NOTE: rs_rectify_image_16bpp(...) is not appropriate for RS_FORMAT_YUYV images, no logic prevents U/V channels from being written to one another */
        assert(0);
    }
}

#endif
