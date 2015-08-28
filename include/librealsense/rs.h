#ifndef librealsense_include_guard
#define librealsense_include_guard
#include "stdint.h"
#ifdef __cplusplus
extern "c" {
#endif
	
#define rs_api_version 1

/* opaque type declarations */
struct rs_context;
struct rs_camera;
struct rs_error;

/* public enum type definitions */
#define rs_enum_range(prefix,first,last) \
	rs_##prefix##_begin_range = rs_##prefix##_##first, \
	rs_##prefix##_end_range   = rs_##prefix##_##last, \
	rs_##prefix##_num         = rs_##prefix##_##last - rs_##prefix##_##first + 1, \
	rs_##prefix##_max_enum    = 0x7fffffff
enum rs_stream
{
	rs_stream_depth                         = 0,
	rs_stream_color                         = 1,
	rs_stream_infrared                      = 2,
	rs_stream_infrared_2                    = 3,
	rs_enum_range(stream, depth, infrared_2)
};

enum rs_format
{
	rs_format_any                           = 0,
	rs_format_z16                           = 1,
	rs_format_y8                            = 2,
	rs_format_rgb8                          = 3,
	rs_enum_range(format, any, rgb8)
};

enum rs_preset
{
	rs_preset_best_quality                  = 0,
	rs_preset_largest_image                 = 1,
	rs_preset_highest_framerate             = 2,
	rs_enum_range(preset, best_quality, highest_framerate)
};

enum rs_distortion
{
	rs_distortion_none                      = 0, /* rectilinear images, no distortion compensation required */
	rs_distortion_gordon_brown_conrady      = 1, /* equivalent to brown-conrady distortion, except that tangential distortion is applied to radially distorted points */
	rs_distortion_inverse_brown_conrady     = 2, /* equivalent to brown-conrady distortion, except undistorts image instead of distorting it */
	rs_enum_range(distortion, none, inverse_brown_conrady)
};

enum rs_option
{
	rs_option_f200_laser_power,                 /* 0 - 15 */
	rs_option_f200_accuracy,                    /* 0 - 3 */
	rs_option_f200_motion_range,                /* 0 - 100 */
	rs_option_f200_filter_option,               /* 0 - 7 */
	rs_option_f200_confidence_threshold,        /* 0 - 15 */
	rs_option_f200_dynamic_fps,                 /* {2, 5, 15, 30, 60} */

	rs_option_r200_lr_auto_exposure_enabled,    /* {0, 1} */
	rs_option_r200_lr_gain,                     /* 100 - 1600 (units of 0.01) */
	rs_option_r200_lr_exposure,                 /* > 0 (units of 0.1 ms) */
	rs_option_r200_emitter_enabled,             /* {0, 1} */
	rs_option_r200_depth_control_preset,
	rs_option_r200_depth_units,                 /* > 0 */
	rs_option_r200_depth_clamp_min,             /* 0 - ushort_max */
	rs_option_r200_depth_clamp_max,             /* 0 - ushort_max */
	rs_option_r200_disparity_mode_enabled,      /* {0, 1} */
	rs_option_r200_disparity_multiplier,
	rs_option_r200_disparity_shift,

	rs_enum_range(option, f200_laser_power, r200_disparity_shift)
};
#undef rs_enum

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
struct rs_context * rs_create_context         (int api_version, struct rs_error ** error);
int					rs_get_camera_count       (struct rs_context * context, struct rs_error ** error);
struct rs_camera *  rs_get_camera             (struct rs_context * context, int index, struct rs_error ** error);
void                rs_delete_context         (struct rs_context * context, struct rs_error ** error);

const char *        rs_get_camera_name        (struct rs_camera * camera, struct rs_error ** error);

void                rs_enable_stream          (struct rs_camera * camera, enum rs_stream stream, int width, int height, enum rs_format format, int fps, struct rs_error ** error);
void                rs_enable_stream_preset   (struct rs_camera * camera, enum rs_stream stream, enum rs_preset preset, struct rs_error ** error);
int                 rs_is_stream_enabled      (struct rs_camera * camera, enum rs_stream stream, struct rs_error ** error);

void                rs_start_capture          (struct rs_camera * camera, struct rs_error ** error);
void                rs_stop_capture           (struct rs_camera * camera, struct rs_error ** error);
void                rs_wait_all_streams       (struct rs_camera * camera, struct rs_error ** error);

enum rs_format      rs_get_image_format       (struct rs_camera * camera, enum rs_stream stream, struct rs_error ** error);
const void *        rs_get_image_pixels       (struct rs_camera * camera, enum rs_stream stream, struct rs_error ** error);
float               rs_get_depth_scale        (struct rs_camera * camera, struct rs_error ** error);

void                rs_get_stream_intrinsics  (struct rs_camera * camera, enum rs_stream stream, struct rs_intrinsics * intrin, struct rs_error ** error);
void                rs_get_stream_extrinsics  (struct rs_camera * camera, enum rs_stream from, enum rs_stream to, struct rs_extrinsics * extrin, struct rs_error ** error);

int                 rs_camera_supports_option (struct rs_camera * camera, enum rs_option option, struct rs_error ** error);
int                 rs_get_camera_option      (struct rs_camera * camera, enum rs_option option, struct rs_error ** error);
void                rs_set_camera_option      (struct rs_camera * camera, enum rs_option option, int value, struct rs_error ** error);

const char *        rs_get_stream_name        (enum rs_stream stream, struct rs_error ** error);
const char *        rs_get_format_name        (enum rs_format format, struct rs_error ** error);
const char *        rs_get_preset_name        (enum rs_preset preset, struct rs_error ** error);
const char *        rs_get_distortion_name    (enum rs_distortion distortion, struct rs_error ** error);
const char *        rs_get_option_name        (enum rs_option option, struct rs_error ** error);

const char *        rs_get_failed_function    (struct rs_error * error);
const char *        rs_get_error_message      (struct rs_error * error);
void                rs_free_error             (struct rs_error * error);

#ifdef __cplusplus
}
#endif
#endif
