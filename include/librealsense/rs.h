/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2015 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALSENSE_RS_H
#define LIBREALSENSE_RS_H

#ifdef __cplusplus
extern "C" {
#endif

#define RS_API_MAJOR_VERSION    2
#define RS_API_MINOR_VERSION    2
#define RS_API_PATCH_VERSION    0

#define STRINGIFY(arg) #arg
#define VAR_ARG_STRING(arg) STRINGIFY(arg)

/* Version in encoded integer format (1,9,x) -> 1090x note that each component is limited into [0-99] range by design*/
#define RS_API_VERSION  (((RS_API_MAJOR_VERSION) * 10000) + ((RS_API_MINOR_VERSION) * 100) + (RS_API_PATCH_VERSION))
/* Return version in "X.Y.Z" format */
#define RS_API_VERSION_STR (VAR_ARG_STRING(RS_API_MAJOR_VERSION.RS_API_MINOR_VERSION.RS_API_PATCH_VERSION))

typedef enum rs_frame_metadata
{
    RS_FRAME_METADATA_ACTUAL_EXPOSURE,
    RS_FRAME_METADATA_COUNT
} rs_frame_metadata;

typedef enum rs_stream
{
    RS_STREAM_DEPTH,
    RS_STREAM_COLOR,
    RS_STREAM_INFRARED,
    RS_STREAM_INFRARED2,
    RS_STREAM_FISHEYE,
    RS_STREAM_COUNT
} rs_stream;

typedef enum rs_format
{
    RS_FORMAT_ANY         ,
    RS_FORMAT_Z16         , /**< 16 bit linear depth values. The depth is meters is equal to depth scale * pixel value */
    RS_FORMAT_DISPARITY16 , /**< 16 bit linear disparity values. The depth in meters is equal to depth scale / pixel value */
    RS_FORMAT_XYZ32F      , /**< 32 bit floating point 3D coordinates. */
    RS_FORMAT_YUYV        ,
    RS_FORMAT_RGB8        ,
    RS_FORMAT_BGR8        ,
    RS_FORMAT_RGBA8       ,
    RS_FORMAT_BGRA8       ,
    RS_FORMAT_Y8          ,
    RS_FORMAT_Y16         ,
    RS_FORMAT_RAW10       , /**< Four 10-bit luminance values encoded into a 5-byte macropixel */
    RS_FORMAT_RAW16       ,
    RS_FORMAT_RAW8        ,
    RS_FORMAT_COUNT
} rs_format;

typedef enum rs_preset
{
    RS_PRESET_BEST_QUALITY,
    RS_PRESET_LARGEST_IMAGE,
    RS_PRESET_HIGHEST_FRAMERATE,
    RS_PRESET_COUNT
} rs_preset;

typedef enum rs_distortion
{
    RS_DISTORTION_NONE                  , /**< Rectilinear images, no distortion compensation required */
    RS_DISTORTION_MODIFIED_BROWN_CONRADY, /**< Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points */
    RS_DISTORTION_INVERSE_BROWN_CONRADY , /**< Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it */
    RS_DISTORTION_FTHETA                , /**< F-Theta fish-eye distortion model */
    RS_DISTORTION_BROWN_CONRADY         , /**< Unmodified Brown-Conrady distortion model */
    RS_DISTORTION_COUNT                 ,
} rs_distortion;

typedef enum rs_visual_preset
{
    RS_VISUAL_PRESET_SHORT_RANGE             ,
    RS_VISUAL_PRESET_LONG_RANGE              ,
    RS_VISUAL_PRESET_BACKGROUND_SEGMENTATION ,
    RS_VISUAL_PRESET_GESTURE_RECOGNITION     ,
    RS_VISUAL_PRESET_OBJECT_SCANNING         ,
    RS_VISUAL_PRESET_FACE_ANALYTICS          ,
    RS_VISUAL_PRESET_FACE_LOGIN              ,
    RS_VISUAL_PRESET_GR_CURSOR               ,
    RS_VISUAL_PRESET_DEFAULT                 ,
    RS_VISUAL_PRESET_MID_RANGE               ,
    RS_VISUAL_PRESET_IR_ONLY                 ,
    RS_VISUAL_PRESET_COUNT
} rs_visual_preset;

typedef enum rs_option
{
    RS_OPTION_BACKLIGHT_COMPENSATION                     , /**< Enable / disable color backlight compensation*/
    RS_OPTION_BRIGHTNESS                                 , /**< Color image brightness*/
    RS_OPTION_CONTRAST                                   , /**< Color image contrast*/
    RS_OPTION_EXPOSURE                                   , /**< Controls exposure time of color camera. Setting any value will disable auto exposure*/
    RS_OPTION_GAIN                                       , /**< Color image gain*/
    RS_OPTION_GAMMA                                      , /**< Color image gamma setting*/
    RS_OPTION_HUE                                        , /**< Color image hue*/
    RS_OPTION_SATURATION                                 , /**< Color image saturation setting*/
    RS_OPTION_SHARPNESS                                  , /**< Color image sharpness setting*/
    RS_OPTION_WHITE_BALANCE                              , /**< Controls white balance of color image. Setting any value will disable auto white balance*/
    RS_OPTION_ENABLE_AUTO_EXPOSURE                       , /**< Enable / disable color image auto-exposure*/
    RS_OPTION_ENABLE_AUTO_WHITE_BALANCE                  , /**< Enable / disable color image auto-white-balance*/
    RS_OPTION_VISUAL_PRESET                              , /**< Provide access to several recommend sets of option presets for the depth camera */
    RS_OPTION_LASER_POWER                                , /**< Power of the F200 / SR300 projector, with 0 meaning projector off*/
    RS_OPTION_ACCURACY                                   , /**< Set the number of patterns projected per frame. The higher the accuracy value the more patterns projected. Increasing the number of patterns help to achieve better accuracy. Note that this control is affecting the Depth FPS */
    RS_OPTION_MOTION_RANGE                               , /**< Motion vs. Range trade-off, with lower values allowing for better motion sensitivity and higher values allowing for better depth range*/
    RS_OPTION_FILTER_OPTION                              , /**< Set the filter to apply to each depth frame. Each one of the filter is optimized per the application requirements*/
    RS_OPTION_CONFIDENCE_THRESHOLD                       , /**< The confidence level threshold used by the Depth algorithm pipe to set whether a pixel will get a valid range or will be marked with invalid range*/
    RS_OPTION_EMITTER_ENABLED                            , /**< RS400 Emitter enabled */
    RS_OPTION_FRAMES_QUEUE_SIZE                          , /**< Number of frames the user is allowed to keep per stream. Trying to hold-on to more frames will cause frame-drops.*/
    RS_OPTION_HARDWARE_LOGGER_ENABLED                    , /**< Enable / disable fetching log data from the device */
    RS_OPTION_TOTAL_FRAME_DROPS                          , /**< Total number of detected frame drops from all streams */
    RS_OPTION_COUNT                                      ,

} rs_option;

typedef enum rs_camera_info {
    RS_CAMERA_INFO_DEVICE_NAME                   ,
    RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER          ,
    RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION       ,
    RS_CAMERA_INFO_DEVICE_LOCATION               ,
    RS_CAMERA_INFO_COUNT
} rs_camera_info;

typedef enum rs_log_severity {
    RS_LOG_SEVERITY_DEBUG, /* Detailed information about ordinary operations */
    RS_LOG_SEVERITY_INFO , /* Terse information about ordinary operations */
    RS_LOG_SEVERITY_WARN , /* Indication of possible failure */
    RS_LOG_SEVERITY_ERROR, /* Indication of definite failure */
    RS_LOG_SEVERITY_FATAL, /* Indication of unrecoverable failure */
    RS_LOG_SEVERITY_NONE , /* No logging will occur */
    RS_LOG_SEVERITY_COUNT
} rs_log_severity;

typedef enum rs_timestamp_domain
{
    RS_TIMESTAMP_DOMAIN_CAMERA,
    RS_TIMESTAMP_DOMAIN_EXTERNAL,
    RS_TIMESTAMP_DOMAIN_SYSTEM,
    RS_TIMESTAMP_DOMAIN_COUNT
}rs_timestamp_domain;

typedef enum rs_subdevice
{
    RS_SUBDEVICE_COLOR,
    RS_SUBDEVICE_DEPTH,
    RS_SUBDEVICE_FISHEYE,
    RS_SUBDEVIEC_MOTION,
    RS_SUBDEVICE_COUNT
} rs_subdevice;

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

/* represents motion device intrinsic - scale, bias and variances */
typedef struct rs_motion_device_intrinsic
{
    /* Scale X        cross axis        cross axis      Bias X */
    /* cross axis     Scale Y           cross axis      Bias Y */
    /* cross axis     cross axis        Scale Z         Bias Z */
    float data[3][4];

    float noise_variances[3];
    float bias_variances[3];
} rs_motion_device_intrinsic;

/* represents motion module intrinsics including accelerometer and gyro intrinsics */
typedef struct rs_motion_intrinsics
{
    rs_motion_device_intrinsic acc;
    rs_motion_device_intrinsic gyro;
} rs_motion_intrinsics;

typedef struct rs_extrinsics
{
    float rotation[9];    /* column-major 3x3 rotation matrix */
    float translation[3]; /* 3 element translation vector, in meters */
} rs_extrinsics;


typedef struct rs_context rs_context;
typedef struct rs_device_list rs_device_list;
typedef struct rs_device rs_device;
typedef struct rs_error rs_error;
typedef struct rs_active_stream rs_active_stream;
typedef struct rs_stream_profile_list rs_stream_profile_list;
typedef struct rs_raw_data_buffer rs_raw_data_buffer;
typedef struct rs_frame rs_frame;
typedef struct rs_frame_queue rs_frame_queue;

typedef struct rs_frame_callback rs_frame_callback;
typedef struct rs_log_callback rs_log_callback;

typedef void (*rs_frame_callback_ptr)(const rs_active_stream*, rs_frame*, void*);
typedef void (*rs_log_callback_ptr)(rs_log_severity min_severity, const char* message, void* user);

rs_context* rs_create_context(int api_version, rs_error** error);
void rs_delete_context(rs_context* context);

rs_device_list* rs_query_devices(const rs_context* context, rs_error** error);
int rs_get_device_count(const rs_device_list* info_list, rs_error** error);
void rs_delete_device_list(rs_device_list* info_list);

rs_device* rs_create_device(const rs_device_list* list, int index, rs_error** error);
void rs_delete_device(rs_device* device);

/**
* retrieve mapping between the units of the depth image and meters
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            depth in meters corresponding to a depth value of 1
*/
float rs_get_device_depth_scale(const rs_device * device, rs_error ** error);

int rs_is_subdevice_supported(const rs_device* device, rs_subdevice subdevice, rs_error** error);

rs_stream_profile_list* rs_get_supported_profiles(rs_device* device, rs_subdevice subdevice, rs_error** error);
void rs_get_profile(const rs_stream_profile_list* list, int index, rs_stream* stream, int* width, int* height, int* fps, rs_format* format, rs_error** error);
int rs_get_profile_list_size(const rs_stream_profile_list* list, rs_error** error);
void rs_delete_profiles_list(rs_stream_profile_list* list);

rs_raw_data_buffer* rs_send_and_receive_raw_data(rs_device* device, void* raw_data_to_send, unsigned size_of_raw_data_to_send, rs_error** error);
int rs_get_raw_data_size(const rs_raw_data_buffer* list, rs_error** error);
void rs_delete_raw_data(rs_raw_data_buffer* list);
const unsigned char* rs_get_raw_data(const rs_raw_data_buffer* buffer, rs_error** error);

rs_active_stream* rs_open(rs_device* device, rs_subdevice subdevice, rs_stream stream, int width, int height, int fps, rs_format format, rs_error** error);
rs_active_stream* rs_open_many(rs_device* device, rs_subdevice subdevice, 
    const rs_stream* stream, const int* width, const int* height, const int* fps, const rs_format* format, int count, rs_error** error);
void rs_close(rs_active_stream* lock);

void rs_start(rs_active_stream* lock, rs_frame_callback_ptr on_frame, void* user, rs_error** error);
void rs_start_cpp(rs_active_stream* lock, rs_frame_callback* callback, rs_error** error);
void rs_stop(rs_active_stream* lock, rs_error** error);

double rs_get_frame_metadata(const rs_frame* frame, rs_frame_metadata frame_metadata, rs_error** error);
int rs_supports_frame_metadata(const rs_frame* frame, rs_frame_metadata frame_metadata, rs_error** error);
double rs_get_frame_timestamp(const rs_frame* frame, rs_error** error);
rs_timestamp_domain rs_get_frame_timestamp_domain(const rs_frame* frameset, rs_error** error);
unsigned long long rs_get_frame_number(const rs_frame* frame, rs_error** error);
const void* rs_get_frame_data(const rs_frame* frame, rs_error** error);
int rs_get_frame_width(const rs_frame* frame, rs_error** error);
int rs_get_frame_height(const rs_frame* frame, rs_error** error);
int rs_get_frame_stride_in_bytes(const rs_frame* frame, rs_error** error);
int rs_get_frame_bits_per_pixel(const rs_frame* frame, rs_error** error);
rs_format rs_get_frame_format(const rs_frame* frame, rs_error** error);
rs_stream rs_get_frame_stream_type(const rs_frame* frameset, rs_error** error);
void rs_release_frame(const rs_active_stream* lock, rs_frame* frame);

float rs_get_subdevice_option(const rs_device* device, rs_subdevice subdevice, rs_option option, rs_error** error);
void rs_set_subdevice_option(const rs_device* device, rs_subdevice subdevice, rs_option option, float value, rs_error** error);
int rs_supports_subdevice_option(const rs_device* device, rs_subdevice subdevice, rs_option option, rs_error** error);
void rs_get_subdevice_option_range(const rs_device* device, rs_subdevice subdevice, rs_option option, float* min, float* max, float* step, float* def, rs_error** error);
const char* rs_get_subdevice_option_description(const rs_device* device, rs_subdevice subdevice, rs_option option, rs_error ** error);
const char* rs_get_subdevice_option_value_description(const rs_device* device, rs_subdevice subdevice, rs_option option, float value, rs_error ** error);

const char* rs_get_camera_info(const rs_device* device, rs_camera_info info, rs_error** error);
int rs_supports_camera_info(const rs_device* device, rs_camera_info info, rs_error** error);

rs_frame_queue* rs_create_frame_queue(int capacity, rs_error** error);
void rs_delete_frame_queue(rs_frame_queue* queue);
rs_frame* rs_wait_for_frame(rs_frame_queue* queue, const rs_active_stream** output_stream, rs_error** error);
int rs_poll_for_frame(rs_frame_queue* queue, rs_frame** output_frame, const rs_active_stream** output_stream, rs_error** error);
void rs_enqueue_frame(const rs_active_stream* sender, rs_frame* frame, void* queue);
void rs_flush_queue(rs_frame_queue* queue, rs_error** error);

rs_context* rs_create_recording_context(int api_version, rs_error** error);
void rs_save_recording_to_file(const rs_context* ctx, const char* filename, rs_error** error);
rs_context* rs_create_mock_context(int api_version, const char* filename, rs_error** error);

/**
* retrieve the API version from the source code. Evaluate that the value is conformant to the established policies
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            the version API encoded into integer value "1.9.3" -> 10903
*/
int          rs_get_api_version      (rs_error ** error);

const char * rs_get_failed_function  (const rs_error * error);
const char * rs_get_failed_args      (const rs_error * error);
const char * rs_get_error_message    (const rs_error * error);
void         rs_free_error           (rs_error * error);


const char * rs_stream_to_string     (rs_stream stream);
const char * rs_format_to_string     (rs_format format);
const char * rs_preset_to_string     (rs_preset preset);
const char * rs_distortion_to_string (rs_distortion distortion);
const char * rs_option_to_string     (rs_option option);
const char * rs_camera_info_to_string(rs_camera_info info);
const char * rs_camera_info_to_string(rs_camera_info info);
const char * rs_timestamp_domain_to_string(rs_timestamp_domain info);
const char * rs_subdevice_to_string  (rs_subdevice subdevice);
const char * rs_visual_preset_to_string  (rs_visual_preset preset);

void rs_log_to_console(rs_log_severity min_severity, rs_error ** error);
void rs_log_to_file(rs_log_severity min_severity, const char * file_path, rs_error ** error);

#ifdef __cplusplus
}
#endif
#endif
