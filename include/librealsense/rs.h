#ifndef LIBREALSENSE_INCLUDE_GUARD
#define LIBREALSENSE_INCLUDE_GUARD
#include "stdint.h"
#ifdef __cplusplus
extern "C" {
#endif
	


struct rs_context;
struct rs_camera;
struct rs_error;

struct rs_intrinsics
{
    int image_size[2];          /* width and height of the image in pixels */
    float focal_length[2];      /* focal length of the image plane, as a multiple of pixel width and height */
    float principal_point[2];   /* coordinates of the principal point of the image, as a pixel offset from the top left */
    float distortion_coeff[5];  /* distortion coefficients */
    int distortion_model;       /* distortion model of the image */
};

struct rs_extrinsics
{
    float rotation[9];          /* column-major 3x3 rotation matrix */
    float translation[3];       /* 3 element translation vector, in meters */
};

struct rs_context *	rs_create_context		(int api_version, struct rs_error ** error);
int					rs_get_camera_count		(struct rs_context * context, struct rs_error ** error);
struct rs_camera *	rs_get_camera			(struct rs_context * context, int index, struct rs_error ** error);
void				rs_delete_context		(struct rs_context * context, struct rs_error ** error);

const char *        rs_get_camera_name      (struct rs_camera * camera, struct rs_error ** error);

void				rs_enable_stream		(struct rs_camera * camera, int stream, int width, int height, int fps, int format, struct rs_error ** error);
void				rs_enable_stream_preset	(struct rs_camera * camera, int stream, int preset, struct rs_error ** error);
void 				rs_start_streaming  	(struct rs_camera * camera, struct rs_error ** error);
void 				rs_stop_streaming       (struct rs_camera * camera, struct rs_error ** error);
void                rs_wait_all_streams     (struct rs_camera * camera, struct rs_error ** error);

const uint8_t *		rs_get_color_image		(struct rs_camera * camera, struct rs_error ** error);
const uint16_t *	rs_get_depth_image		(struct rs_camera * camera, struct rs_error ** error);
const void *        rs_get_image_pixels     (struct rs_camera * camera, int stream, struct rs_error ** error);
float               rs_get_depth_scale      (struct rs_camera * camera, struct rs_error ** error);

void                rs_get_stream_intrinsics(struct rs_camera * camera, int stream, struct rs_intrinsics * intrin, struct rs_error ** error);
void                rs_get_stream_extrinsics(struct rs_camera * camera, int stream_from, int stream_to, struct rs_extrinsics * extrin, struct rs_error ** error);

const char *		rs_get_failed_function	(struct rs_error * error);
const char *		rs_get_error_message	(struct rs_error * error);
void				rs_free_error			(struct rs_error * error);

/* Pass this constant to rs_create_context */
#define RS_API_VERSION      1

/* Valid arguments for rs_enable_stream / rs_enable_stream_preset */
#define RS_DEPTH            0
#define RS_COLOR            1
#define RS_INFRARED         2
    
/* Valid arguments for rs_enable_stream's format argument */
#define RS_Z16              1
#define RS_RGB8             2
#define RS_Y8               3

/* Valid arguments for rs_enable_stream_preset's preset argument */
#define RS_BEST_QUALITY     0                       /* Preset recommended for best quality and stability */

/* Valid values for rs_intrinsics' distortion_model field */
#define RS_NO_DISTORTION                        0   /* Rectilinear images, no distortion compensation required */
#define RS_GORDON_BROWN_CONRADY_DISTORTION      1   /* Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points */
#define RS_INVERSE_BROWN_CONRADY_DISTORTION     2   /* Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it */

#ifdef __cplusplus
}
#endif
#endif
