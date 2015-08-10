#ifndef LIBREALSENSE_INCLUDE_GUARD
#define LIBREALSENSE_INCLUDE_GUARD

#include "stdint.h"

typedef struct rs_error_ *		rs_error;
typedef struct rs_context_ *	rs_context;
typedef struct rs_camera_ *		rs_camera;

#ifdef __cplusplus
extern "C" {
#endif

const char *		rs_get_failed_function	(rs_error error);
const char *		rs_get_error_message	(rs_error error);
void				rs_free_error			(rs_error error);

rs_context			rs_create_context		(rs_error * error);
void				rs_delete_context		(rs_context context, rs_error * error);
int					rs_get_camera_count		(rs_context context, rs_error * error);
rs_camera			rs_get_camera			(rs_context context, int index, rs_error * error);

void				rs_enable_stream		(rs_camera camera, int stream, rs_error * error);
int 				rs_configure_streams	(rs_camera camera, rs_error * error);
void				rs_start_stream			(rs_camera camera, int stream, int width, int height, int fps, int format, rs_error * error);
void				rs_stop_stream			(rs_camera camera, int stream, rs_error * error);
const uint16_t *	rs_get_depth_image		(rs_camera camera, rs_error * error);
const uint8_t *		rs_get_color_image		(rs_camera camera, rs_error * error);
int 				rs_is_streaming			(rs_camera camera, rs_error * error);
int					rs_get_camera_index		(rs_camera camera, rs_error * error);
uint64_t			rs_get_frame_count		(rs_camera camera, rs_error * error);

int					rs_get_stream_property_i(rs_camera camera, int stream, int prop, rs_error * error); // RS_IMAGE_SIZE_X, RS_IMAGE_SIZE_Y
float				rs_get_stream_property_f(rs_camera camera, int stream, int prop, rs_error * error); // RS_FOCAL_LENGTH_X, RS_FOCAL_LENGTH_Y, RS_PRINCIPAL_POINT_X, RS_PRINCIPAL_POINT_Y

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

// @tofix - this has an overloaded meaning for R200
// Valid arguments for rsEnableStream/rsStartStream/rsStopStream/rsGetStreamProperty*
#define RS_STREAM_DEPTH					1
#define RS_STREAM_LR					2
#define RS_STREAM_RGB					4

// Valid arguments to rsGetStreamProperty*
#define RS_IMAGE_SIZE_X			1
#define RS_IMAGE_SIZE_Y			2
#define RS_FOCAL_LENGTH_X		3
#define RS_FOCAL_LENGTH_Y		4
#define RS_PRINCIPAL_POINT_X	5
#define RS_PRINCIPAL_POINT_Y	6

#endif