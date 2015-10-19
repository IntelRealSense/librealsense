#ifndef LIBREALSENSE_RS_H
#define LIBREALSENSE_RS_H

#ifdef __cplusplus
extern "C" {
#endif

#define RS_API_VERSION 3

typedef enum rs_stream
{
    RS_STREAM_DEPTH                            = 0, /* Native stream of depth data produced by RealSense device */
    RS_STREAM_COLOR                            = 1, /* Native stream of color data captured by RealSense device */
    RS_STREAM_INFRARED                         = 2, /* Native stream of infrared data captured by RealSense device */
    RS_STREAM_INFRARED2                        = 3, /* Native stream of infrared data captured from a second viewpoint by RealSense device */
    RS_STREAM_RECTIFIED_COLOR                  = 4, /* Synthetic stream containing undistorted color data with no extrinsic rotation from the depth stream */
    RS_STREAM_COLOR_ALIGNED_TO_DEPTH           = 5, /* Synthetic stream containing color data but sharing intrinsics of depth stream */
    RS_STREAM_DEPTH_ALIGNED_TO_COLOR           = 6, /* Synthetic stream containing depth data but sharing intrinsics of color stream */
    RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR = 7, /* Synthetic stream containing depth data but sharing intrinsics of rectified color stream */
    RS_STREAM_COUNT                            = 8, 
    RS_STREAM_MAX_ENUM = 0x7FFFFFFF
} rs_stream;

typedef enum rs_format
{
    RS_FORMAT_ANY   = 0, 
    RS_FORMAT_Z16   = 1, 
    RS_FORMAT_YUYV  = 2, 
    RS_FORMAT_RGB8  = 3, 
    RS_FORMAT_BGR8  = 4, 
    RS_FORMAT_RGBA8 = 5, 
    RS_FORMAT_BGRA8 = 6, 
    RS_FORMAT_Y8    = 7, 
    RS_FORMAT_Y16   = 8, 
    RS_FORMAT_COUNT = 9, 
    RS_FORMAT_MAX_ENUM = 0x7FFFFFFF
} rs_format;

typedef enum rs_preset
{
    RS_PRESET_BEST_QUALITY      = 0, 
    RS_PRESET_LARGEST_IMAGE     = 1, 
    RS_PRESET_HIGHEST_FRAMERATE = 2, 
    RS_PRESET_COUNT             = 3, 
    RS_PRESET_MAX_ENUM = 0x7FFFFFFF
} rs_preset;

typedef enum rs_distortion
{
    RS_DISTORTION_NONE                   = 0, /* Rectilinear images, no distortion compensation required */
    RS_DISTORTION_MODIFIED_BROWN_CONRADY = 1, /* Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points */
    RS_DISTORTION_INVERSE_BROWN_CONRADY  = 2, /* Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it */
    RS_DISTORTION_COUNT                  = 3, 
    RS_DISTORTION_MAX_ENUM = 0x7FFFFFFF
} rs_distortion;

typedef enum rs_option
{
    RS_OPTION_COLOR_BACKLIGHT_COMPENSATION  = 0,  
    RS_OPTION_COLOR_BRIGHTNESS              = 1,  
    RS_OPTION_COLOR_CONTRAST                = 2,  
    RS_OPTION_COLOR_EXPOSURE                = 3,  
    RS_OPTION_COLOR_GAIN                    = 4,  
    RS_OPTION_COLOR_GAMMA                   = 5,  
    RS_OPTION_COLOR_HUE                     = 6,  
    RS_OPTION_COLOR_SATURATION              = 7,  
    RS_OPTION_COLOR_SHARPNESS               = 8,  
    RS_OPTION_COLOR_WHITE_BALANCE           = 9,  
    RS_OPTION_F200_LASER_POWER              = 10, /* 0 - 15 */
    RS_OPTION_F200_ACCURACY                 = 11, /* 0 - 3 */
    RS_OPTION_F200_MOTION_RANGE             = 12, /* 0 - 100 */
    RS_OPTION_F200_FILTER_OPTION            = 13, /* 0 - 7 */
    RS_OPTION_F200_CONFIDENCE_THRESHOLD     = 14, /* 0 - 15 */
    RS_OPTION_F200_DYNAMIC_FPS              = 15, /* {2, 5, 15, 30, 60} */
    RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED = 16, /* {0, 1} */
    RS_OPTION_R200_LR_GAIN                  = 17, /* 100 - 1600 (Units of 0.01) */
    RS_OPTION_R200_LR_EXPOSURE              = 18, /* > 0 (Units of 0.1 ms) */
    RS_OPTION_R200_EMITTER_ENABLED          = 19, /* {0, 1} */
    RS_OPTION_R200_DEPTH_CONTROL_PRESET     = 20, /* {0, 5}, 0 is default, 1-5 is low to high outlier rejection */
    RS_OPTION_R200_DEPTH_UNITS              = 21, /* > 0 */
    RS_OPTION_R200_DEPTH_CLAMP_MIN          = 22, /* 0 - USHORT_MAX */
    RS_OPTION_R200_DEPTH_CLAMP_MAX          = 23, /* 0 - USHORT_MAX */
    RS_OPTION_R200_DISPARITY_MODE_ENABLED   = 24, /* {0, 1} */
    RS_OPTION_R200_DISPARITY_MULTIPLIER     = 25, 
    RS_OPTION_R200_DISPARITY_SHIFT          = 26, 
    RS_OPTION_COUNT                         = 27, 
    RS_OPTION_MAX_ENUM = 0x7FFFFFFF
} rs_option;

typedef struct rs_intrinsics
{
    int           width;     /* width of the image in pixels */
    int           height;    /* height of the image in pixels */
    float         ppx;       /* horizontal coordinate of the principal point of the image, as a pixel offset from the left edge */
    float         ppy;       /* vertical coordinate of the principal point of the image, as a pixel offset from the top edge */
    float         fx;        /* focal length of the image plane, as a multiple of pixel width */
    float         fy;        /* focal length of the image plane, as a multiple of pixel height */
    rs_distortion model;     /* distortion model of the image */
    float         coeffs[5]; /* distortion coefficients */
} rs_intrinsics;

typedef struct rs_extrinsics
{
    float rotation[9];    /* column-major 3x3 rotation matrix */
    float translation[3]; /* 3 element translation vector, in meters */
} rs_extrinsics;

typedef struct rs_context rs_context;
typedef struct rs_device rs_device;
typedef struct rs_error rs_error;

rs_context * rs_create_context              (int api_version, rs_error ** error);
void         rs_delete_context              (rs_context * context, rs_error ** error);
int          rs_get_device_count            (const rs_context * context, rs_error ** error);
rs_device *  rs_get_device                  (rs_context * context, int index, rs_error ** error);
                                            
const char * rs_get_device_name             (const rs_device * device, rs_error ** error);
const char * rs_get_device_serial           (const rs_device * device, rs_error ** error);
const char * rs_get_device_firmware_version (const rs_device * device, rs_error ** error);
void         rs_get_device_extrinsics       (const rs_device * device, rs_stream from_stream, rs_stream to_stream, rs_extrinsics * extrin, rs_error ** error);
float        rs_get_device_depth_scale      (const rs_device * device, rs_error ** error);
int          rs_device_supports_option      (const rs_device * device, rs_option option, rs_error ** error);
int          rs_get_stream_mode_count       (const rs_device * device, rs_stream stream, rs_error ** error);
void         rs_get_stream_mode             (const rs_device * device, rs_stream stream, int index, int * width, int * height, rs_format * format, int * framerate, rs_error ** error);
                                            
void         rs_enable_stream               (rs_device * device, rs_stream stream, int width, int height, rs_format format, int framerate, rs_error ** error);
void         rs_enable_stream_preset        (rs_device * device, rs_stream stream, rs_preset preset, rs_error ** error);
void         rs_disable_stream              (rs_device * device, rs_stream stream, rs_error ** error);
int          rs_is_stream_enabled           (const rs_device * device, rs_stream stream, rs_error ** error);
void         rs_get_stream_intrinsics       (const rs_device * device, rs_stream stream, rs_intrinsics * intrin, rs_error ** error);
rs_format    rs_get_stream_format           (const rs_device * device, rs_stream stream, rs_error ** error);
int          rs_get_stream_framerate        (const rs_device * device, rs_stream stream, rs_error ** error);
                                            
void         rs_start_device                (rs_device * device, rs_error ** error);
void         rs_stop_device                 (rs_device * device, rs_error ** error);
int          rs_is_device_streaming         (const rs_device * device, rs_error ** error);
                                            
void         rs_set_device_option           (rs_device * device, rs_option option, int value, rs_error ** error);
int          rs_get_device_option           (const rs_device * device, rs_option option, rs_error ** error);
                                            
void         rs_wait_for_frames             (rs_device * device, rs_error ** error);
int          rs_get_frame_timestamp         (const rs_device * device, rs_stream stream, rs_error ** error);
const void * rs_get_frame_data              (const rs_device * device, rs_stream stream, rs_error ** error);
                                            
const char * rs_get_failed_function         (const rs_error * error);
const char * rs_get_failed_args             (const rs_error * error);
const char * rs_get_error_message           (const rs_error * error);
void         rs_free_error                  (rs_error * error);
                                            
const char * rs_stream_to_string            (rs_stream stream);
const char * rs_format_to_string            (rs_format format);
const char * rs_preset_to_string            (rs_preset preset);
const char * rs_distortion_to_string        (rs_distortion distortion);
const char * rs_option_to_string            (rs_option option);

#ifdef __cplusplus
}
#endif
#endif
