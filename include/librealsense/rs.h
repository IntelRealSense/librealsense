#ifndef LIBREALSENSE_INCLUDE_GUARD
#define LIBREALSENSE_INCLUDE_GUARD

#include "stdint.h"

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

typedef struct RScontext_ * RScontext;
typedef struct RScamera_ *	RScamera;
typedef uint32_t			RSenum;

#ifdef __cplusplus
extern "C" {
#endif

RScontext			rsCreateContext		();
void				rsDeleteContext		(RScontext context);
int					rsGetCameraCount	(RScontext context);
RScamera			rsGetCamera			(RScontext context, int index);

void				rsEnableStream		(RScamera camera, RSenum stream);
int 				rsConfigureStreams	(RScamera camera);
void				rsStartStream		(RScamera camera, RSenum stream, int width, int height, int fps, RSenum format);
void				rsStopStream		(RScamera camera, RSenum stream);
const uint16_t *	rsGetDepthImage		(RScamera camera);
const uint8_t *		rsGetColorImage		(RScamera camera);
int 				rsIsStreaming		(RScamera camera);
int					rsGetCameraIndex	(RScamera camera);
uint64_t			rsGetFrameCount		(RScamera camera);

int					rsGetStreamPropertyi(RScamera camera, RSenum stream, RSenum prop); // RS_IMAGE_SIZE_X, RS_IMAGE_SIZE_Y
float				rsGetStreamPropertyf(RScamera camera, RSenum stream, RSenum prop); // RS_FOCAL_LENGTH_X, RS_FOCAL_LENGTH_Y, RS_PRINCIPAL_POINT_X, RS_PRINCIPAL_POINT_Y

#ifdef __cplusplus
}
#endif

#endif