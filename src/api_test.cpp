#include "stdint.h"
struct rs_context{ int x; };
struct rs_camera{ int x; };
struct rs_stream_config{ int x; };
/*struct rs_frame {}; */
struct rs_error{ int x; };

struct rs_intrinsics
{
    int image_size[2];          /* width and height of the image in pixels */
    float focal_length[2];      /* focal length of the image plane, as a multiple of pixel width and height */
    float principal_point[2];   /* coordinates of the principal point of the image, as a pixel offset from the top left */
    float distortion_coeff[5];  /* distortion coefficients */
    int distortion_model;       /* distortion model */
};

struct rs_extrinsics
{
    float rotation[9];
    float translation[3];
};

struct rs_context *         rs_create_context           (int api_version, struct rs_error ** error){ return 0; };
void                        rs_free_context             (struct rs_context * context, struct rs_error ** err){};

int                         rs_get_camera_count         (struct rs_context * context, struct rs_error **err){ return 0; };
struct rs_camera *          rs_get_camera               (struct rs_context * context, int index, struct rs_error **err){ return 0; };

int                         rs_get_stream_cfg_count     (struct rs_camera * camera, int stream_type, struct rs_error **err){return 0;};
struct rs_stream_config *   rs_get_stream_cfg_by_index  (struct rs_camera * camera, int stream_type, int index, struct rs_error **err){ return 0; };
int                         rs_get_stream_cfg_property_i(struct rs_stream_config * cfg, int prop, struct rs_error ** error){ return 0; };
int                         rs_get_stream_cfg_property_f(struct rs_stream_config * cfg, float prop, struct rs_error ** error){ return 0; };
void                        rs_set_stream_cfg_property_i(struct rs_stream_config * cfg, int prop, int value, struct rs_error ** error){  };
void                        rs_set_stream_cfg_property_f(struct rs_stream_config * cfg, float prop, float value, struct rs_error ** error){  };
void                        rs_free_stream_cfg          (struct rs_stream_config * error){};

void                        rs_enable_stream_from_cfg   (struct rs_camera * camera, struct rs_stream_config * cfg, struct rs_error **err){};
struct rs_stream_config *   rs_get_current_stream_cfg   (struct rs_camera * camera, int stream_type, struct rs_error ** err){ return 0; };

/*convenience functions. Perform best-effort selection of a valid configuration and return result of rs_get_current_stream_cfg*/
struct rs_stream_config *   rs_enable_stream            (struct rs_camera * camera, int stream_type, int width, int height, int fps, int format, struct rs_error **err){ return 0; };
struct rs_stream_config *   rs_enable_stream            (struct rs_camera * camera, int stream_type, int width, int height, int fps, int format_depth, int format_intensity, struct rs_error **err){ return 0; };

/*alternative would be to create/destroy a new opaque type (rs_stream) */
void                        rs_start_streaming          (struct rs_camera * camera, struct rs_error **err){};
void                        rs_stop_streaming           (struct rs_camera * camera, struct rs_error **err){};

/*alternative could be to create/destroy a new opaque type (rs_frame)*/
void                        rs_wait_one_update          (struct rs_camera * camera, int stream_type, int blocking_call, struct rs_error ** error){ };
void                        rs_wait_all_update          (struct rs_camera * camera, int stream_type, int blocking_call, struct rs_error ** error){ };

typedef void(rs_frame_callback_t)(struct rs_camera *camera, void *user_ptr);
typedef void(rs_error_callback_t)(struct rs_camera *camera, struct rs_error ** error, void *user_ptr);

void                        rs_register_wait_one_update (struct rs_camera * camera, int stream_type, rs_frame_callback_t *cb, void *user_ptr, struct rs_error ** error){ };
void                        rs_register_wait_all_update (struct rs_camera * camera, int stream_type, rs_frame_callback_t *cb, void *user_ptr, struct rs_error ** error){ };
void                        rs_register_streaming_error (struct rs_camera * camera, int stream_type, rs_error_callback_t *cb, void *user_ptr, struct rs_error ** error){ };

void *                      rs_get_data                 (struct rs_camera * camera, int stream_type, struct rs_error ** error){ return 0; };

/*alternatives for async operation should go here set stream callback for all/one and given*/
int                         rs_is_streaming             (struct rs_camera * camera, struct rs_error ** error){ return 0; };
int                         rs_get_camera_index         (struct rs_camera * camera, struct rs_error ** error){ return 0; };
uint32_t                    rs_get_frame_count          (struct rs_camera * camera, struct rs_error ** error){ return 0; };
int                         rs_get_stream_property_i    (struct rs_camera * camera, int stream_type, int prop, struct rs_error ** error){ return 0; };
float                       rs_get_stream_property_f    (struct rs_camera * camera, int stream_type, int prop, struct rs_error ** error){ return 0; };
int                         rs_get_s2s_property_i       (struct rs_camera * camera, int stream_type_from, int stream_type_to, int prop, struct rs_error ** error){ return 0; };
float                       rs_get_s2s_property_f       (struct rs_camera * camera, int stream_type_from, int stream_type_to, int prop, struct rs_error ** error){ return 0; };
void                        rs_set_stream_property_i    (struct rs_camera * camera, int stream_type, int prop, int value, struct rs_error ** error){ };
void                        rs_set_stream_property_f    (struct rs_camera * camera, int stream_type, int prop, float value, struct rs_error ** error){ };

/*convenience functions. Image dimensions and camera model calibration information */
void                        rs_get_stream_intrinsics    (struct rs_camera * camera, int stream_type, struct rs_intrinsics * intrin, struct rs_error ** error);
void                        rs_get_stream_extrinsics    (struct rs_camera * camera, int stream_type_from, int stream_type_to, struct rs_extrinsics * extrin, struct rs_error ** error);

const char *                rs_get_failed_function      (struct rs_error * error){ return 0; };
const char *                rs_get_error_message        (struct rs_error * error){ return 0; };
void                        rs_free_error               (struct rs_error * error){};
#define RS_API_VERSION 0
#define NULL 0

/*define vs. enum?*/
/*@tofix -- DS also supports disparity mode and changing the disparity to depth multiplier*/
#define RS_FRAME_FORMAT_UNKNOWN         0
#define RS_FRAME_FORMAT_ANY             0 /* Any supported format*/
#define RS_FRAME_FORMAT_UNCOMPRESSED    1
#define RS_FRAME_FORMAT_COMPRESSED      2
#define RS_FRAME_FORMAT_YUYV            3 /* YUYV/YUV2/YUV422: YUV encoding with one luminance value per pixel and one UV (chrominance) pair for every two pixels.*/
#define RS_FRAME_FORMAT_UYVY            4
#define RS_FRAME_FORMAT_Y12I            5
#define RS_FRAME_FORMAT_Y16             6
#define RS_FRAME_FORMAT_Y8              7
#define RS_FRAME_FORMAT_Z16             8
#define RS_FRAME_FORMAT_RGB             9
#define RS_FRAME_FORMAT_BGR             10
#define RS_FRAME_FORMAT_MJPEG           11
#define RS_FRAME_FORMAT_GRAY8           12
#define RS_FRAME_FORMAT_BY8             13
#define RS_FRAME_FORMAT_INVI            14 /*IR*/
#define RS_FRAME_FORMAT_RELI            15 /*IR*/
#define RS_FRAME_FORMAT_INVR            16 /*Depth*/
#define RS_FRAME_FORMAT_INVZ            17 /*Depth*/
#define RS_FRAME_FORMAT_INRI            18 /*Depth (24 bit)*/

/* @tofix - this has an overloaded meaning for R200*/
/* Valid arguments for rsEnableStream/rsStartStream/rsStopStream/rsGetStreamProperty**/
#define RS_STREAM_DEPTH                 1
#define RS_STREAM_L                     2
#define RS_STREAM_R                     4
#define RS_STREAM_COLOR                 8

/* Valid Distortion Models*/
#define RS_DISTORTION_MODEL_NONE                0
#define RS_DISTORTION_MODEL_FORWARD_BROWN       1
#define RS_DISTORTION_MODEL_FORWARD_FITZGIBBON  2
#define RS_DISTORTION_MODEL_INVERSE_BROWN       3


/* Valid arguments to rsGetStreamProperty**/
/* Boolean-based properties*/
#define RS_IMAGE_ENABLED        0
#define RS_IMAGE_RECTIFIED      1
#define RS_IMAGE_CROPPED        2
/* Integer-based properties*/
#define RS_IMAGE_SIZE_X         3
#define RS_IMAGE_SIZE_Y         4
#define RS_DATA_FPS             5
#define RS_DATA_FORMAT          6
/* Floating-point properties*/
#define RS_FOCAL_LENGTH_X       7
#define RS_FOCAL_LENGTH_Y       8
#define RS_PRINCIPAL_POINT_X    9
#define RS_PRINCIPAL_POINT_Y    10
#define RS_LATEST_TIMESTAMP     11
#define RS_DISTORTION_MODEL     12
#define RS_DISTORTION_K1        13
#define RS_DISTORTION_K2        14
#define RS_DISTORTION_K3        15
#define RS_DISTORTION_K4        16
#define RS_DISTORTION_K5        17

/* Integer-based properties betweens streams*/
#define RS_SYNCHRONIZED         0

/* Floating-point properties between streams*/
#define RS_TRANSLATION_X        1
#define RS_TRANSLATION_Y        2
#define RS_TRANSLATION_Z        3
#define RS_ROTATION_XX          4
#define RS_ROTATION_XY          5
#define RS_ROTATION_XZ          6
#define RS_ROTATION_YX          7
#define RS_ROTATION_YY          8
#define RS_ROTATION_YZ          9
#define RS_ROTATION_ZX          10
#define RS_ROTATION_ZY          11
#define RS_ROTATION_ZZ          12
#define RS_SYNCHRONIZED_OFFSET  13


int short_main(int argc, char * argv[])
{
    struct rs_context * ctx;

    struct rs_camera * cam;

    struct rs_error * error;
    ctx = rs_create_context(RS_API_VERSION, &error);
    int cam_count = rs_get_camera_count(ctx, NULL);
    if (cam_count) {
        cam = rs_get_camera(ctx, 0, &error);
        struct rs_stream_config * z_cfg = rs_enable_stream(cam, RS_STREAM_DEPTH, 628, 469, 30, RS_FRAME_FORMAT_Z16, &error); /*should be a best-effort call.*/
        struct rs_stream_config * rgb_cfg = rs_enable_stream(cam, RS_STREAM_COLOR, 640, 480, 30, RS_FRAME_FORMAT_YUYV, &error);
        rs_start_streaming(cam, NULL);

        do {
            rs_wait_one_update(cam, RS_STREAM_DEPTH | RS_STREAM_COLOR, 1, &error);
            uint16_t * z = (uint16_t*)rs_get_data(cam, RS_STREAM_DEPTH, &error);
            uint8_t * rgb = (uint8_t*)rs_get_data(cam, RS_STREAM_COLOR, &error);
        } while (!error);
    }
    rs_free_context(ctx, &error);
    return 0;
}

int full_main(int argc, char * argv[])
{
    struct rs_context * ctx;

    struct rs_camera * cam;

    struct rs_error * error;
    ctx = rs_create_context(RS_API_VERSION, &error);
    int cam_count = rs_get_camera_count(ctx, NULL);
    if (cam_count) {
        cam = rs_get_camera(ctx, 0, &error);
        int z_stream_count = rs_get_stream_cfg_count(cam, RS_STREAM_DEPTH, NULL);
        int rgb_stream_count = rs_get_stream_cfg_count(cam, RS_STREAM_COLOR, NULL);

        int i = 0;
        struct rs_stream_config * z_cfg=NULL;
        struct rs_stream_config * rgb_cfg=NULL;
        for (i = 0; i < z_stream_count; i++) {
            z_cfg = rs_get_stream_cfg_by_index(cam, RS_STREAM_DEPTH, i, NULL);
            int w = rs_get_stream_cfg_property_i(z_cfg, RS_IMAGE_SIZE_X, NULL);
            int h = rs_get_stream_cfg_property_i(z_cfg, RS_IMAGE_SIZE_Y, NULL);
            int t = rs_get_stream_cfg_property_i(z_cfg, RS_DATA_FPS, NULL);
            if (w == 628) {
                break;
            }  else {
                rs_free_stream_cfg(z_cfg);
            }
        }
        for (i = 0; i < rgb_stream_count; i++) {
            rgb_cfg = rs_get_stream_cfg_by_index(cam, RS_STREAM_DEPTH, i, NULL);
            int w = rs_get_stream_cfg_property_i(rgb_cfg, RS_IMAGE_SIZE_X, NULL);
            int h = rs_get_stream_cfg_property_i(rgb_cfg, RS_IMAGE_SIZE_Y, NULL);
            int t = rs_get_stream_cfg_property_i(rgb_cfg, RS_DATA_FPS, NULL);
            if (w == 640) {
                break;
            }
            else {
                rs_free_stream_cfg(rgb_cfg);
            }
        }
        rs_enable_stream_from_cfg(cam, z_cfg, &error);
        rs_enable_stream_from_cfg(cam, rgb_cfg, &error);
        rs_start_streaming(cam, NULL);

        do {
            rs_wait_one_update(cam, RS_STREAM_DEPTH | RS_STREAM_COLOR, 1, &error);
            uint16_t * z = (uint16_t*)rs_get_data(cam, RS_STREAM_DEPTH, &error);
            uint8_t * rgb = (uint8_t*)rs_get_data(cam, RS_STREAM_COLOR, &error);
        } while (!error);
    }
    rs_free_context(ctx, &error);
    return 0;
}

void frame_callback(struct rs_camera *camera, void *user_ptr)
{
    struct rs_error * error;
    uint16_t * z = (uint16_t*)rs_get_data(camera, RS_STREAM_DEPTH, &error);
    uint8_t * rgb = (uint8_t*)rs_get_data(camera, RS_STREAM_COLOR, &error);
}

int test_async(int argc, char * argv[])
{
    struct rs_context * ctx;

    struct rs_camera * cam;

    struct rs_error * error;
    ctx = rs_create_context(RS_API_VERSION, &error);
    int cam_count = rs_get_camera_count(ctx, NULL);
    if (cam_count) {
        cam = rs_get_camera(ctx, 0, &error);
        struct rs_stream_config * z_cfg = rs_enable_stream(cam, RS_STREAM_DEPTH, 628, 469, 30, RS_FRAME_FORMAT_Z16, &error); /*should be a best-effort call.*/
        struct rs_stream_config * rgb_cfg = rs_enable_stream(cam, RS_STREAM_COLOR, 640, 480, 30, RS_FRAME_FORMAT_YUYV, &error);
        rs_register_wait_all_update(cam, RS_STREAM_DEPTH | RS_STREAM_COLOR, frame_callback, NULL, &error); 
        rs_start_streaming(cam, NULL);
        
        do {
            //maybe have the error point get updated as set in the main loop?
        } while (!error);
    }
    rs_free_context(ctx, &error);
    return 0;
}
