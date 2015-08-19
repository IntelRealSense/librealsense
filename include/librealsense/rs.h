#ifndef LIBREALSENSE_INCLUDE_GUARD
#define LIBREALSENSE_INCLUDE_GUARD

#include "stdint.h"

#define RS_API_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif
	
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

const uint16_t *	rs_get_depth_image		(struct rs_camera * camera, struct rs_error ** error);
const float *       rs_get_vertex_image		(struct rs_camera * camera, struct rs_error ** error);
const uint8_t *		rs_get_color_image		(struct rs_camera * camera, struct rs_error ** error);
int					rs_get_camera_index		(struct rs_camera * camera, struct rs_error ** error);
uint64_t			rs_get_frame_count		(struct rs_camera * camera, struct rs_error ** error);
int					rs_get_stream_property_i(struct rs_camera * camera, int stream, int prop, struct rs_error ** error); // RS_IMAGE_SIZE_X, RS_IMAGE_SIZE_Y
float				rs_get_stream_property_f(struct rs_camera * camera, int stream, int prop, struct rs_error ** error); // RS_FOCAL_LENGTH_X, RS_FOCAL_LENGTH_Y, RS_PRINCIPAL_POINT_X, RS_PRINCIPAL_POINT_Y
void                rs_get_stream_intrinsics(struct rs_camera * camera, int stream, struct rs_intrinsics * intrin, struct rs_error ** error);
void                rs_get_stream_extrinsics(struct rs_camera * camera, int stream_from, int stream_to, struct rs_extrinsics * extrin, struct rs_error ** error);
float               rs_get_depth_scale      (struct rs_camera * camera, struct rs_error ** error);

const char *		rs_get_failed_function	(struct rs_error * error);
const char *		rs_get_error_message	(struct rs_error * error);
void				rs_free_error			(struct rs_error * error);

struct rs_intrinsics
{
    int image_size[2];          /* width and height of the image in pixels */
    float focal_length[2];      /* focal length of the image plane, as a multiple of pixel width and height */
    float principal_point[2];   /* coordinates of the principal point of the image, as a pixel offset from the top left */
    float distortion_coeff[5];  /* distortion coefficients */
};

struct rs_extrinsics
{
    float rotation[9];
    float translation[3];
};

#ifdef __cplusplus
}
#endif

// TODO: Clean up this format list to the bare minimum supported format
// Valid arguments for rsStartStream
#define RS_FRAME_FORMAT_UNKNOWN			0
#define RS_FRAME_FORMAT_ANY				0 // Any supported format
#define RS_FRAME_FORMAT_UNCOMPRESSED	1
#define RS_FRAME_FORMAT_COMPRESSED		2
#define RS_FRAME_FORMAT_YUYV			3 // YUYV/YUV2/YUV422: YUV encoding with one luminance value per pixel and one UV (chrominance) pair for every two pixels.
#define RS_FRAME_FORMAT_UYVY			4
#define RS_FRAME_FORMAT_Y12I			5
#define RS_FRAME_FORMAT_Y16				6
#define RS_FRAME_FORMAT_Y8				7
#define RS_FRAME_FORMAT_Z16				8
#define RS_FRAME_FORMAT_RGB				9
#define RS_FRAME_FORMAT_BGR				10
#define RS_FRAME_FORMAT_MJPEG			11
#define RS_FRAME_FORMAT_GRAY8			12
#define RS_FRAME_FORMAT_BY8				13
#define RS_FRAME_FORMAT_INVI			14 //IR
#define RS_FRAME_FORMAT_RELI			15 //IR
#define RS_FRAME_FORMAT_INVR			16 //Depth
#define RS_FRAME_FORMAT_INVZ			17 //Depth
#define RS_FRAME_FORMAT_INRI			18 //Depth (24 bit)

// Valid arguments for rsEnableStream/rsStartStream/rsStopStream/rsGetStreamProperty*
#define RS_DEPTH                        0
#define RS_COLOR                        1

#define RS_STREAM_PRESET_BEST_QUALITY   0   /* Preset recommended for best quality and stability */

// Valid arguments to rsGetStreamProperty*
#define RS_IMAGE_SIZE_X			1
#define RS_IMAGE_SIZE_Y			2
#define RS_FOCAL_LENGTH_X		3
#define RS_FOCAL_LENGTH_Y		4
#define RS_PRINCIPAL_POINT_X	5
#define RS_PRINCIPAL_POINT_Y	6

#endif
