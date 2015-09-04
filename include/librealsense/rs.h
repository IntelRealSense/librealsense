#ifndef LIBREALSENSE_INCLUDE_GUARD
#define LIBREALSENSE_INCLUDE_GUARD
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif
    
#define RS_API_VERSION 2

/* public type definitions */
typedef struct rs_context rs_context;
typedef struct rs_camera rs_camera;
typedef struct rs_error rs_error;
typedef struct rs_intrinsics rs_intrinsics;
typedef struct rs_extrinsics rs_extrinsics;
typedef enum rs_stream rs_stream;
typedef enum rs_format rs_format;
typedef enum rs_preset rs_preset;
typedef enum rs_distortion rs_distortion;
typedef enum rs_option rs_option;

/* public function declarations */
rs_context *    rs_create_context           (int api_version, rs_error ** error);
void            rs_delete_context           (rs_context * context, rs_error ** error);
int             rs_get_camera_count         (const rs_context * context, rs_error ** error);
rs_camera *     rs_get_camera               (const rs_context * context, int index, rs_error ** error);
const char *    rs_get_camera_name          (const rs_camera * camera, rs_error ** error);
int             rs_camera_supports_option   (const rs_camera * camera, rs_option option, rs_error ** error);

void            rs_enable_stream            (rs_camera * camera, rs_stream stream, int width, int height, rs_format format, int fps, rs_error ** error);
void            rs_enable_stream_preset     (rs_camera * camera, rs_stream stream, rs_preset preset, rs_error ** error);
int             rs_is_stream_enabled        (const rs_camera * camera, rs_stream stream, rs_error ** error);

void            rs_start_capture            (rs_camera * camera, rs_error ** error);
void            rs_stop_capture             (rs_camera * camera, rs_error ** error);
int             rs_is_capturing             (const rs_camera * camera, rs_error ** error);
float           rs_get_depth_scale          (const rs_camera * camera, rs_error ** error);
rs_format       rs_get_stream_format        (const rs_camera * camera, rs_stream stream, rs_error ** error);
void            rs_get_stream_intrinsics    (const rs_camera * camera, rs_stream stream, rs_intrinsics * intrin, rs_error ** error);
void            rs_get_stream_extrinsics    (const rs_camera * camera, rs_stream from, rs_stream to, rs_extrinsics * extrin, rs_error ** error);

void            rs_wait_all_streams         (rs_camera * camera, rs_error ** error);
int             rs_get_image_frame_number   (const rs_camera * camera, rs_stream stream, rs_error ** error);
const void *    rs_get_image_pixels         (const rs_camera * camera, rs_stream stream, rs_error ** error);

void            rs_set_camera_option        (rs_camera * camera, rs_option option, int value, rs_error ** error);
int             rs_get_camera_option        (const rs_camera * camera, rs_option option, rs_error ** error);

void            rs_free_error               (rs_error * error);
const char *    rs_get_failed_function      (const rs_error * error);
const char *    rs_get_failed_args          (const rs_error * error);
const char *    rs_get_error_message        (const rs_error * error);

const char *    rs_stream_to_string         (rs_stream stream);
const char *    rs_format_to_string         (rs_format format);
const char *    rs_preset_to_string         (rs_preset preset);
const char *    rs_distortion_to_string     (rs_distortion distortion);
const char *    rs_option_to_string         (rs_option option);

/* public enum type definitions */
enum rs_stream
{
    RS_STREAM_DEPTH                         = 0,
    RS_STREAM_COLOR                         = 1,
    RS_STREAM_INFRARED                      = 2,
    RS_STREAM_INFRARED_2                    = 3,
    RS_STREAM_COUNT                         = 4,
    RS_STREAM_MAX_ENUM                      = INT32_MAX
};

enum rs_format
{
    RS_FORMAT_ANY                           = 0,
    RS_FORMAT_Z16                           = 1,
    RS_FORMAT_YUYV                          = 2,
    RS_FORMAT_RGB8                          = 3,
    RS_FORMAT_BGR8                          = 4,
    RS_FORMAT_RGBA8                         = 5,
    RS_FORMAT_BGRA8                         = 6,
    RS_FORMAT_Y8                            = 7,
    RS_FORMAT_Y16                           = 8,
    RS_FORMAT_COUNT                         = 9,
    RS_FORMAT_MAX_ENUM                      = INT32_MAX
};

enum rs_preset
{
    RS_PRESET_BEST_QUALITY                  = 0,
    RS_PRESET_LARGEST_IMAGE                 = 1,
    RS_PRESET_HIGHEST_FRAMERATE             = 2,
    RS_PRESET_COUNT                         = 9,
    RS_PRESET_MAX_ENUM                      = INT32_MAX
};

enum rs_distortion
{
    RS_DISTORTION_NONE                      = 0, /* Rectilinear images, no distortion compensation required */
    RS_DISTORTION_MODIFIED_BROWN_CONRADY    = 1, /* Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points */
    RS_DISTORTION_INVERSE_BROWN_CONRADY     = 2, /* Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it */
    RS_DISTORTION_COUNT                     = 3,
    RS_DISTORTION_MAX_ENUM                  = INT32_MAX
};

enum rs_option
{
    RS_OPTION_F200_LASER_POWER              = 0, /* 0 - 15 */
    RS_OPTION_F200_ACCURACY                 = 1, /* 0 - 3 */
    RS_OPTION_F200_MOTION_RANGE             = 2, /* 0 - 100 */
    RS_OPTION_F200_FILTER_OPTION            = 3, /* 0 - 7 */
    RS_OPTION_F200_CONFIDENCE_THRESHOLD     = 4, /* 0 - 15 */
    RS_OPTION_F200_DYNAMIC_FPS              = 5, /* {2, 5, 15, 30, 60} */
    RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED = 6, /* {0, 1} */
    RS_OPTION_R200_LR_GAIN                  = 7, /* 100 - 1600 (Units of 0.01) */
    RS_OPTION_R200_LR_EXPOSURE              = 8, /* > 0 (Units of 0.1 ms) */
    RS_OPTION_R200_EMITTER_ENABLED          = 9, /* {0, 1} */
    RS_OPTION_R200_DEPTH_CONTROL_PRESET     = 10, /* {0, 5}, 0 is default, 1-5 is low to high outlier rejection */
    RS_OPTION_R200_DEPTH_UNITS              = 11, /* > 0 */
    RS_OPTION_R200_DEPTH_CLAMP_MIN          = 12, /* 0 - USHORT_MAX */
    RS_OPTION_R200_DEPTH_CLAMP_MAX          = 13, /* 0 - USHORT_MAX */
    RS_OPTION_R200_DISPARITY_MODE_ENABLED   = 14, /* {0, 1} */
    RS_OPTION_R200_DISPARITY_MULTIPLIER     = 15,
    RS_OPTION_R200_DISPARITY_SHIFT          = 16,
    RS_OPTION_COUNT                         = 17,
    RS_OPTION_MAX_ENUM                      = INT32_MAX
};

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

#ifdef __cplusplus
}
#endif
#endif
