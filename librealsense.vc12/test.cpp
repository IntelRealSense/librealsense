#include <cstdint>
struct rs_context {};
struct rs_camera {};
struct rs_stream_config {};
//struct rs_frame {};
struct rs_error {};

struct rs_context *			rs_create_context			(int api_version, struct rs_error ** error){ return 0; };
void						rs_free_context				(struct rs_context * context, struct rs_error **){};

int							rs_get_camera_count			(struct rs_context * context, struct rs_error **){ return 0; };
struct rs_camera *			rs_get_camera				(struct rs_context * context, int index, struct rs_error **){ return 0; };

void						rs_get_stream_cfg_count		(struct rs_camera * camera, int stream_type, struct rs_error **){};
struct rs_stream_config *	rs_get_stream_cfg			(struct rs_camera * camera, int stream_type, int cfg_idx, struct rs_error **){ return 0; };
int							rs_get_stream_cfg_property_i(struct rs_stream_config * cfg, int prop, struct rs_error ** error){ return 0; };
int							rs_get_stream_cfg_property_f(struct rs_stream_config * cfg, float prop, struct rs_error ** error){ return 0; };
void						rs_set_stream_cfg_property_i(struct rs_stream_config * cfg, int prop, int value, struct rs_error ** error){  };
void						rs_set_stream_cfg_property_f(struct rs_stream_config * cfg, float prop, float value, struct rs_error ** error){  };
void						rs_free_stream_cfg			(struct rs_stream_config * error){};

struct rs_stream_config *	rs_enable_stream			(struct rs_camera * camera, int stream_type, int width, int height, int fps, int format, struct rs_error **){ return 0; };
void						rs_enable_stream_from_cfg	(struct rs_camera * camera, int stream_type, struct rs_stream_config * cfg, struct rs_error **){};

void						rs_start_streaming			(struct rs_camera * camera, struct rs_error **){};
void						rs_stop_streaming			(struct rs_camera * camera, struct rs_error **){};

void						rs_wait_one_update			(struct rs_camera * camera, int stream_type, struct rs_error ** error){ };
void						rs_wait_all_update			(struct rs_camera * camera, int stream_type, struct rs_error ** error){ };

void *						rs_get_image				(struct rs_camera * camera, int stream_type, struct rs_error ** error){ return 0; };

int 						rs_is_streaming				(struct rs_camera * camera, struct rs_error ** error){ return 0; };
int							rs_get_camera_index			(struct rs_camera * camera, struct rs_error ** error){ return 0; };
uint32_t					rs_get_frame_count			(struct rs_camera * camera, struct rs_error ** error){ return 0; };
int							rs_get_stream_property_i	(struct rs_camera * camera, int stream, int prop, struct rs_error ** error){ return 0; };
float						rs_get_stream_property_f	(struct rs_camera * camera, int stream, int prop, struct rs_error ** error){ return 0; };
void						rs_set_stream_property_i	(struct rs_camera * camera, int stream, int prop, int value, struct rs_error ** error){ };
void						rs_set_stream_property_f	(struct rs_camera * camera, int stream, int prop, float value, struct rs_error ** error){ };

const char *				rs_get_failed_function		(struct rs_error * error){ return 0; };
const char *				rs_get_error_message		(struct rs_error * error){ return 0; };
void						rs_free_error				(struct rs_error * error){};
#define RS_API_VERSION 0
#define NULL 0

//@tofix -- DS also supports disparity mode and changing the disparity to depth multiplier
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
#define RS_STREAM_L						2
#define RS_STREAM_R						4
#define RS_STREAM_COLOR					8

// Valid arguments to rsGetStreamProperty*
// Boolean-based properties
#define RS_IMAGE_ENABLED		0
#define RS_IMAGE_RECTIFIED		1
#define RS_IMAGE_CROPPED		2
// Integer-based properties
#define RS_IMAGE_SIZE_X			3
#define RS_IMAGE_SIZE_Y			4
#define RS_IMAGE_FPS			5
#define RS_IMAGE_FORMAT			6
// Floating-point properties
#define RS_FOCAL_LENGTH_X		7
#define RS_FOCAL_LENGTH_Y		8
#define RS_PRINCIPAL_POINT_X	9
#define RS_PRINCIPAL_POINT_Y	10
#define RS_LATEST_TIMESTAMP		11
int test_main(int argc, char * argv[])
{
	struct rs_context * ctx;

	struct rs_camera * cam;

	struct rs_error * error;
	ctx = rs_create_context(RS_API_VERSION, &error);
	int cam_count = rs_get_camera_count(ctx, NULL);
	if (cam_count) {
		cam = rs_get_camera(ctx, 0, &error);
		struct rs_stream_config * z_cfg = rs_enable_stream(cam, RS_STREAM_DEPTH, 628, 469, 30, RS_FRAME_FORMAT_Z16, &error); //should be a best-effort call.
		struct rs_stream_config * rgb_cfg = rs_enable_stream(cam, RS_STREAM_COLOR, 640, 480, 30, RS_FRAME_FORMAT_YUYV, &error);
		rs_start_streaming(cam, NULL);

		do {
			rs_wait_one_update(cam, RS_STREAM_DEPTH | RS_STREAM_COLOR, &error);
			auto z = (uint16_t*)rs_get_image(cam, RS_STREAM_DEPTH, &error);
			auto rgb = (uint8_t*)rs_get_image(cam, RS_STREAM_COLOR, &error);

		} while (!error);
	}
	rs_free_context(ctx, &error);
	return 0;
}
