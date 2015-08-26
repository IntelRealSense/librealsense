#ifndef LIBREALSENSE_INCLUDE_GUARD
#define LIBREALSENSE_INCLUDE_GUARD
#include "stdint.h"
#ifdef __cplusplus
extern "C" {
#endif
	
#define RS_API_VERSION 1

/* opaque type declarations */
struct rs_context;
struct rs_camera;
struct rs_error;

/* public enum type definitions */
#define RS_ENUM_RANGE(PREFIX,FIRST,LAST) \
    RS_##PREFIX##_BEGIN_RANGE = RS_##PREFIX##_##FIRST, \
    RS_##PREFIX##_END_RANGE   = RS_##PREFIX##_##LAST, \
    RS_##PREFIX##_NUM         = RS_##PREFIX##_##LAST - RS_##PREFIX##_##FIRST + 1, \
    RS_##PREFIX##_MAX_ENUM    = 0x7FFFFFFF
enum rs_stream
{
    RS_STREAM_DEPTH                         = 0,
    RS_STREAM_COLOR                         = 1,
    RS_STREAM_INFRARED                      = 2,
    RS_STREAM_INFRARED_2                    = 3,
    RS_ENUM_RANGE(STREAM, DEPTH, INFRARED_2)
};

enum rs_format
{
    RS_FORMAT_ANY                           = 0,
    RS_FORMAT_Z16                           = 1,
    RS_FORMAT_Y8                            = 2,
    RS_FORMAT_RGB8                          = 3,
    RS_ENUM_RANGE(FORMAT, ANY, RGB8)
};

enum rs_preset
{
    RS_PRESET_BEST_QUALITY                  = 0,
    RS_PRESET_LARGEST_IMAGE                 = 1,
    RS_PRESET_HIGHEST_FRAMERATE             = 2,
    RS_ENUM_RANGE(PRESET, BEST_QUALITY, HIGHEST_FRAMERATE)
};

enum rs_distortion
{
    RS_DISTORTION_NONE                      = 0, /* Rectilinear images, no distortion compensation required */
    RS_DISTORTION_GORDON_BROWN_CONRADY      = 1, /* Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points */
    RS_DISTORTION_INVERSE_BROWN_CONRADY     = 2, /* Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it */
    RS_ENUM_RANGE(DISTORTION, NONE, INVERSE_BROWN_CONRADY)
};
#undef RS_ENUM

/* public struct type definitions */
struct rs_intrinsics
{
    int image_size[2];                      /* width and height of the image in pixels */
    float focal_length[2];                  /* focal length of the image plane, as a multiple of pixel width and height */
    float principal_point[2];               /* coordinates of the principal point of the image, as a pixel offset from the top left */
    float distortion_coeff[5];              /* distortion coefficients */
    enum rs_distortion distortion_model;    /* distortion model of the image */
};

struct rs_extrinsics
{
    float rotation[9];                      /* column-major 3x3 rotation matrix */
    float translation[3];                   /* 3 element translation vector, in meters */
};

/* public function declarations */
struct rs_context *	rs_create_context		(int api_version, struct rs_error ** error);
int					rs_get_camera_count		(struct rs_context * context, struct rs_error ** error);
struct rs_camera *	rs_get_camera			(struct rs_context * context, int index, struct rs_error ** error);
void				rs_delete_context		(struct rs_context * context, struct rs_error ** error);

const char *        rs_get_camera_name      (struct rs_camera * camera, struct rs_error ** error);

void				rs_enable_stream		(struct rs_camera * camera, enum rs_stream stream, int width, int height, enum rs_format format, int fps, struct rs_error ** error);
void				rs_enable_stream_preset	(struct rs_camera * camera, enum rs_stream stream, enum rs_preset preset, struct rs_error ** error);
int                 rs_is_stream_enabled    (struct rs_camera * camera, enum rs_stream stream, struct rs_error ** error);

void 				rs_start_capture      	(struct rs_camera * camera, struct rs_error ** error);
void 				rs_stop_capture         (struct rs_camera * camera, struct rs_error ** error);
void                rs_wait_all_streams     (struct rs_camera * camera, struct rs_error ** error);

const uint8_t *		rs_get_color_image		(struct rs_camera * camera, struct rs_error ** error);
const uint16_t *	rs_get_depth_image		(struct rs_camera * camera, struct rs_error ** error);
const void *        rs_get_image_pixels     (struct rs_camera * camera, enum rs_stream stream, struct rs_error ** error);
float               rs_get_depth_scale      (struct rs_camera * camera, struct rs_error ** error);

void                rs_get_stream_intrinsics(struct rs_camera * camera, enum rs_stream stream, struct rs_intrinsics * intrin, struct rs_error ** error);
void                rs_get_stream_extrinsics(struct rs_camera * camera, enum rs_stream from, enum rs_stream to, struct rs_extrinsics * extrin, struct rs_error ** error);

const char *        rs_get_stream_name      (enum rs_stream stream, struct rs_error ** error);
const char *        rs_get_format_name      (enum rs_format format, struct rs_error ** error);
const char *        rs_get_preset_name      (enum rs_preset preset, struct rs_error ** error);
const char *        rs_get_distortion_name  (enum rs_distortion distortion, struct rs_error ** error);

const char *		rs_get_failed_function	(struct rs_error * error);
const char *		rs_get_error_message	(struct rs_error * error);
void				rs_free_error			(struct rs_error * error);

#ifdef __cplusplus
}
#endif
#endif
