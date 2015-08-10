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

typedef struct RSerror_ *	RSerror;
typedef struct RScontext_ * RScontext;
typedef struct RScamera_ *	RScamera;
typedef uint32_t			RSenum;

#ifdef __cplusplus
extern "C" {
#endif


const char *		rsGetFailedFunction	(RSerror error);
const char *		rsGetErrorMessage	(RSerror error);
void				rsFreeError			(RSerror error);

RScontext			rsCreateContext		(RSerror * error);
void				rsDeleteContext		(RScontext context, RSerror * error);
int					rsGetCameraCount	(RScontext context, RSerror * error);
RScamera			rsGetCamera			(RScontext context, int index, RSerror * error);

void				rsEnableStream		(RScamera camera, RSenum stream, RSerror * error);
int 				rsConfigureStreams	(RScamera camera, RSerror * error);
void				rsStartStream		(RScamera camera, RSenum stream, int width, int height, int fps, RSenum format, RSerror * error);
void				rsStopStream		(RScamera camera, RSenum stream, RSerror * error);
const uint16_t *	rsGetDepthImage		(RScamera camera, RSerror * error);
const uint8_t *		rsGetColorImage		(RScamera camera, RSerror * error);
int 				rsIsStreaming		(RScamera camera, RSerror * error);
int					rsGetCameraIndex	(RScamera camera, RSerror * error);
uint64_t			rsGetFrameCount		(RScamera camera, RSerror * error);

int					rsGetStreamPropertyi(RScamera camera, RSenum stream, RSenum prop, RSerror * error); // RS_IMAGE_SIZE_X, RS_IMAGE_SIZE_Y
float				rsGetStreamPropertyf(RScamera camera, RSenum stream, RSenum prop, RSerror * error); // RS_FOCAL_LENGTH_X, RS_FOCAL_LENGTH_Y, RS_PRINCIPAL_POINT_X, RS_PRINCIPAL_POINT_Y

#ifdef __cplusplus
}
#endif

#endif