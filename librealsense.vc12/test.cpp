#include <cstdint>
struct rs_context {};
struct rs_device {};
struct rs_frame {};
struct rs_error {};

struct rs_context *	rs_create_context		(int api_version, struct rs_error ** error){ return 0; };
int					rs_get_device_count		(struct rs_context * context, struct rs_error **){ return 0; };
struct rs_device *	rs_get_device			(struct rs_context * context, int index, struct rs_error **){ return 0; };
void				rs_delete_context		(struct rs_context * context, struct rs_error **){};

void				rs_enable_stream		(struct rs_device * device, int stream, struct rs_error **){};
int 				rs_configure_streams	(struct rs_device * device, struct rs_error **){ return 0; };
void				rs_start_stream			(struct rs_device * device, int stream, int width, int height, int fps, int format, struct rs_error **){};
void				rs_stop_stream			(struct rs_device * device, int stream, struct rs_error **){};
const uint16_t *	rs_get_depth_image		(struct rs_device * device, struct rs_error ** error){ return 0; };
const uint8_t *		rs_get_color_image		(struct rs_device * device, struct rs_error ** error){ return 0; };
int 				rs_is_streaming			(struct rs_device * device, struct rs_error ** error){ return 0; };
int					rs_get_device_index		(struct rs_device * device, struct rs_error ** error){ return 0; };
uint64_t			rs_get_frame_count		(struct rs_device * device, struct rs_error ** error){ return 0; };
int					rs_get_stream_property_i(struct rs_device * device, int stream, int prop, struct rs_error ** error){ return 0; };
float				rs_get_stream_property_f(struct rs_device * device, int stream, int prop, struct rs_error ** error){ return 0; };

const char *		rs_get_failed_function	(struct rs_error * error){ return 0; };
const char *		rs_get_error_message	(struct rs_error * error){ return 0; };
void				rs_free_error			(struct rs_error * error){};
#define RS_API_VERSION 0
#define NULL 0
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
int test_main(int argc, char * argv[])
{
	struct rs_context * ctx;
	struct rs_device * cam;

	rs_error * error;
	ctx = rs_create_context(RS_API_VERSION, &error);
	auto cam_count = rs_get_device_count(ctx, NULL);
	for (int i = 0; i < cam_count; ++i)
	{
		cam = rs_get_device(ctx, i, &error);
		rs_enable_stream(cam, RS_STREAM_DEPTH, &error);
		rs_enable_stream(cam, RS_STREAM_RGB, &error);
		rs_configure_streams(cam, &error);

		int zw = rs_get_stream_property_i(cam, RS_STREAM_DEPTH, RS_IMAGE_SIZE_X, NULL);
		int zh = rs_get_stream_property_i(cam, RS_STREAM_DEPTH, RS_IMAGE_SIZE_Y, NULL);

		int rw = rs_get_stream_property_i(cam, RS_STREAM_RGB, RS_IMAGE_SIZE_X, NULL);
		int rh = rs_get_stream_property_i(cam, RS_STREAM_RGB, RS_IMAGE_SIZE_Y, NULL);

		rs_start_stream(cam, RS_STREAM_DEPTH, zw, zh, 30, RS_FRAME_FORMAT_Z16, &error);
		rs_start_stream(cam, RS_STREAM_RGB, rw, rh, 30, RS_FRAME_FORMAT_YUYV, &error);
	}

	while (true) {
		auto z = rs_get_depth_image(cam, &error);
	}

	rs_delete_context(ctx, &error);
	return 0;
}
