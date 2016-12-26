/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2015 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALSENSE_RS_H
#define LIBREALSENSE_RS_H

#ifdef __cplusplus
extern "C" {
#endif

#define RS_API_MAJOR_VERSION    2
#define RS_API_MINOR_VERSION    3
#define RS_API_PATCH_VERSION    1

#define STRINGIFY(arg) #arg
#define VAR_ARG_STRING(arg) STRINGIFY(arg)

/* Version in encoded integer format (1,9,x) -> 1090x note that each component is limited into [0-99] range by design*/
#define RS_API_VERSION  (((RS_API_MAJOR_VERSION) * 10000) + ((RS_API_MINOR_VERSION) * 100) + (RS_API_PATCH_VERSION))
/* Return version in "X.Y.Z" format */
#define RS_API_VERSION_STR (VAR_ARG_STRING(RS_API_MAJOR_VERSION.RS_API_MINOR_VERSION.RS_API_PATCH_VERSION))


typedef enum rs_stream
{
    RS_STREAM_ANY,
    RS_STREAM_DEPTH,
    RS_STREAM_COLOR,
    RS_STREAM_INFRARED,
    RS_STREAM_INFRARED2,
    RS_STREAM_FISHEYE,
    RS_STREAM_GYRO,
    RS_STREAM_ACCEL,
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
    RS_FORMAT_UYVY        ,
    RS_FORMAT_MOTION_DATA ,
    RS_FORMAT_COUNT
} rs_format;

typedef enum rs_frame_metadata
{
    RS_FRAME_METADATA_ACTUAL_EXPOSURE,
    RS_FRAME_METADATA_COUNT
} rs_frame_metadata;

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
    RS_OPTION_ENABLE_FW_LOGGER                           , /**< Enable / disable device FW logger*/
    RS_OPTION_COUNT                                      ,
} rs_option;

typedef enum rs_camera_info {
    RS_CAMERA_INFO_DEVICE_NAME                   ,
    RS_CAMERA_INFO_MODULE_NAME                   ,
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
} rs_timestamp_domain;

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
typedef struct rs_stream_profile_list rs_stream_modes_list;
typedef struct rs_raw_data_buffer rs_raw_data_buffer;
typedef struct rs_frame rs_frame;
typedef struct rs_frame_queue rs_frame_queue;

typedef struct rs_frame_callback rs_frame_callback;
typedef struct rs_log_callback rs_log_callback;

typedef void (*rs_frame_callback_ptr)(rs_frame*, void*);
typedef void (*rs_log_callback_ptr)(rs_log_severity min_severity, const char* message, void* user);

/**
* create default librealsense context
* \param[in] api_version realsense API version as provided by RS_API_VERSION macro
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            context object, should be released by rs_delete_context
*/
rs_context* rs_create_context(int api_version, rs_error** error);

/**
* delete relasense context
* \param[in] context realsense context to delete
*/
void rs_delete_context(rs_context* context);

/**
* create a static snapshot of all connected devices at the time of the call
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            the list of devices, should be released by rs_delete_device_list
*/
rs_device_list* rs_query_devices(const rs_context* context, rs_error** error);

/**
* determine number of devices in a list
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            the count of devices
*/
int rs_get_device_count(const rs_device_list* info_list, rs_error** error);

/**
* delete device list, any devices created from this list will remain unaffected
* \param[in] info_list list to delete
*/
void rs_delete_device_list(rs_device_list* info_list);

/**
* create device by index
* \param[in] index   the zero based index of device to retrieve
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            the requested device, should be released by rs_delete_device
*/
rs_device* rs_create_device(const rs_device_list* list, int index, rs_error** error);

/**
* delete relasense device
* \param[in] device realsense device to delete
*/
void rs_delete_device(rs_device* device);

/**
* get list of devices adjacent to a given device
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            the list of devices, should be released by rs_delete_device_list
*/
rs_device_list* rs_query_adjacent_devices(const rs_device* device, rs_error** error);

//TODO
void rs_get_extrinsics(const rs_device * from, const rs_device * to, rs_extrinsics * extrin, rs_error ** error);

//TODO
void rs_get_stream_intrinsics(const rs_device * device, rs_stream stream, int width, int height, int fps, rs_format format, rs_intrinsics * intrinsics, rs_error ** error);

/**
* retrieve mapping between the units of the depth image and meters
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            depth in meters corresponding to a depth value of 1
*/
float rs_get_device_depth_scale(const rs_device * device, rs_error ** error);

/**
* check if physical subdevice is supported
* \param[in] device  input RealSense device
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            list of stream profiles that given subdevice can provide, should be released by rs_delete_profiles_list
*/
rs_stream_modes_list* rs_get_stream_modes(rs_device* device, rs_error** error);

/**
* determine the properties of a specific streaming mode
* \param[in] list        the list of supported profiles returned by rs_get_supported_profiles
* \param[in] index       the zero based index of the streaming mode
* \param[out] stream     the stream type
* \param[out] width      the width of a frame image in pixels
* \param[out] height     the height of a frame image in pixels
* \param[out] fps  the number of frames which will be streamed per second
* \param[out] format     the pixel format of a frame image
* \param[out] error      if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs_get_stream_mode(const rs_stream_modes_list* list, int index, rs_stream* stream, int* width, int* height, int* fps, rs_format* format, rs_error** error);

/**
* get the number of supported stream profiles
* \param[in] list        the list of supported profiles returned by rs_get_supported_profiles
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return number of supported subdevice profiles
*/
int rs_get_modes_count(const rs_stream_modes_list* list, rs_error** error);

/**
* delete stream profiles list
* \param[in] list        the list of supported profiles returned by rs_get_supported_profiles
*/
void rs_delete_modes_list(rs_stream_modes_list* list);

/**
* open subdevice for exclusive access, by commiting to a configuration
* \param[in] device relevant RealSense device
* \param[in] stream     the stream type
* \param[in] width      the width of a frame image in pixels
* \param[in] height     the height of a frame image in pixels
* \param[in] fps  the number of frames which will be streamed per second
* \param[in] format     the pixel format of a frame image
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs_open(rs_device* device, rs_stream stream, int width, int height, int fps, rs_format format, rs_error** error);

/**
* open subdevice for exclusive access, by commiting to composite configuration, specifying one or more stream profiles
* this method should be used for interdendent streams, such as depth and infrared, that have to be configured together
* \param[in] device relevant RealSense device
* \param[in] stream     the stream type
* \param[in] width      the width of a frame image in pixels
* \param[in] height     the height of a frame image in pixels
* \param[in] fps  the number of frames which will be streamed per second
* \param[in] format     the pixel format of a frame image
* \param[in] count      number of simultenous stream profiles to configure
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs_open_multiple(rs_device* device, const rs_stream* stream, const int* width,
    const int* height, const int* fps, const rs_format* format, int count, rs_error** error);

/**
* stop any streaming from specified subdevice
* \param[in] device     RealSense device
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs_close(const rs_device* device, rs_error** error);

/**
* start streaming from specified configured device
* \param[in] device  RealSense device
* \param[in] on_frame function pointer to register as per-frame callback
* \param[in] user auxilary data the user wishes to receive together with every frame callback
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs_start(const rs_device* device, rs_frame_callback_ptr on_frame, void* user, rs_error** error);

/**
* start streaming from specified configured device
* \param[in] device  RealSense device
* \param[in] callback callback object created from c++ application. ownership over the callback object is moved into the relevant streaming lock
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs_start_cpp(const rs_device* device, rs_frame_callback* callback, rs_error** error);

/**
* stops streaming from specified configured device
* \param[in] device  RealSense device
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs_stop(const rs_device* device, rs_error** error);

/**
* retrive metadata from frame handle
* \param[in] frame      handle returned from a callback
* \param[in] frame_metadata  the rs_frame_metadata whose latest frame we are interested in
* \return            the metadata value
*/
double rs_get_frame_metadata(const rs_frame* frame, rs_frame_metadata frame_metadata, rs_error** error);

/**
* determine device metadata
* \param[in] frame      handle returned from a callback
* \param[in] metadata    the metadata to check for support
* \return                true if device has this metadata
*/
int rs_supports_frame_metadata(const rs_frame* frame, rs_frame_metadata frame_metadata, rs_error** error);

/**
* retrive timestamp from frame handle in milliseconds
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               the timestamp of the frame in milliseconds
*/
double rs_get_frame_timestamp(const rs_frame* frame, rs_error** error);

/**
* retrive timestamp domain from frame handle. timestamps can only be comparable if they are in common domain
* (for example, depth timestamp might come from system time while color timestamp might come from the device)
* this method is used to check if two timestamp values are comparable (generated from the same clock)
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               the timestamp domain of the frame (camera / microcontroller / system time)
*/
rs_timestamp_domain rs_get_frame_timestamp_domain(const rs_frame* frameset, rs_error** error);

/**
* retrive frame number from frame handle
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               the frame nubmer of the frame, in milliseconds since the device was started
*/
unsigned long long rs_get_frame_number(const rs_frame* frame, rs_error** error);

/**
* retrive data from frame handle
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               the pointer to the start of the frame data
*/
const void* rs_get_frame_data(const rs_frame* frame, rs_error** error);

/**
* retrive frame width in pixels
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               frame width in pixels
*/
int rs_get_frame_width(const rs_frame* frame, rs_error** error);

/**
* retrive frame height in pixels
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               frame height in pixels
*/
int rs_get_frame_height(const rs_frame* frame, rs_error** error);

/**
* retrive frame stride in bytes (number of bytes from start of line N to start of line N+1)
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               stride in bytes
*/
int rs_get_frame_stride_in_bytes(const rs_frame* frame, rs_error** error);

/**
* retrive bits per pixels in the frame image
* (note that bits per pixel is not nessesarily devided by 8, as in 12bpp)
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               bits per pixel
*/
int rs_get_frame_bits_per_pixel(const rs_frame* frame, rs_error** error);

/**
* retrive pixel format of the frame
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               pixel format as described in rs_format enum
*/
rs_format rs_get_frame_format(const rs_frame* frame, rs_error** error);

/**
* retrive the origin stream type that produced the frame
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               stream type of the frame
*/
rs_stream rs_get_frame_stream_type(const rs_frame* frameset, rs_error** error);

/**
* create additional reference to a frame without duplicating frame data
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               new frame reference, has to be released by rs_release_frame
*/
rs_frame* rs_clone_frame_ref(rs_frame* frame, rs_error ** error);

/**
* relases the frame handle
* \param[in] frame handle returned from a callback
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs_release_frame(rs_frame* frame);

/**
* read option value from the device
* \param[in] device   the RealSense device
* \param[in] option   option id to be queried
* \param[out] error   if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return value of the option
*/
float rs_get_option(const rs_device* device, rs_option option, rs_error** error);

/**
* write new value to device option
* \param[in] device     the RealSense device
* \param[in] option     option id to be queried
* \param[in] value      new value for the option
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs_set_option(const rs_device* device, rs_option option, float value, rs_error** error);

/**
* check if particular option is supported by a subdevice
* \param[in] device     the RealSense device
* \param[in] option     option id to be checked
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return true if option is supported
*/
int rs_supports_option(const rs_device* device, rs_option option, rs_error** error);

/**
* retrieve the available range of values of a supported option
* \param[in] device  the RealSense device
* \param[in] option  the option whose range should be queried
* \param[out] min    the minimum value which will be accepted for this option
* \param[out] max    the maximum value which will be accepted for this option
* \param[out] step   the granularity of options which accept discrete values, or zero if the option accepts continuous values
* \param[out] def    the default value of the option
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs_get_option_range(const rs_device* device, rs_option option, float* min, float* max, float* step, float* def, rs_error** error);

/**
* get option description
* \param[in] device     the RealSense device
* \param[in] option     option id to be checked
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return human-readable option description
*/
const char* rs_get_option_description(const rs_device* device, rs_option option, rs_error ** error);

/**
* get option value description (in case specific option value hold special meaning)
* \param[in] device     the RealSense device
* \param[in] option     option id to be checked
* \param[in] value      value of the option
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return human-readable description of a specific value of an option or null if no special meaning
*/
const char* rs_get_option_value_description(const rs_device* device, rs_option option, float value, rs_error ** error);

/**
* retrieve camera specific information, like versions of various internal componnents
* \param[in] device     the RealSense device
* \param[in] info       camera info type to retrieve
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               the requested camera info string, in a format specific to the device model
*/
const char* rs_get_camera_info(const rs_device* device, rs_camera_info info, rs_error** error);

/**
* check if specific camera info is supported
* \param[in] info    the parameter to check for support
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                true if the parameter both exist and well-defined for the specific device
*/
int rs_supports_camera_info(const rs_device* device, rs_camera_info info, rs_error** error);

/**
* create frame queue. frame queues are the simplest x-platform syncronization primitive provided by librealsense
* to help developers who are not using async APIs
* \param[in] device     the RealSense device
* \param[in] capacity max number of frames to allow to be stored in the queue before older frames will start to get dropped
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return handle to the frame queue, must be released using rs_delete_frame_queue
*/
rs_frame_queue* rs_create_frame_queue(int capacity, rs_error** error);

/**
* deletes frame queue and releases all frames inside it
* \param[in] frame queue to delete
*/
void rs_delete_frame_queue(rs_frame_queue* queue);

/**
* wait until new frame becomes available in the queue and dequeue it
* \param[in] queue the frame queue data structure
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return frame handle to be released using rs_release_frame
*/
rs_frame* rs_wait_for_frame(rs_frame_queue* queue, rs_error** error);

/**
* poll if a new frame is available and dequeue if it is
* \param[in] queue the frame queue data structure
* \param[out] output_frame frame handle to be released using rs_release_frame
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return true if new frame was stored to output_frame
*/
int rs_poll_for_frame(rs_frame_queue* queue, rs_frame** output_frame, rs_error** error);

/**
* enqueue new frame into a queue
* \param[in] frame frame handle to enqueue (this operation passed ownership to the queue)
* \param[in] queue the frame queue data structure
*/
void rs_enqueue_frame(rs_frame* frame, void* queue);

/**
* release all frames inside the queue
* \param[in] queue the frame queue data structure
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs_flush_queue(rs_frame_queue* queue, rs_error** error);

/**
* send raw data to device
* \param[in] device  input RealSense device
* \param[in] raw_data_to_send   raw data to be send to device
* \param[in] size_of_raw_data_to_send   size of raw_data_to_send
* \param[out] error   if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            rs_raw_data_buffer, should be released by rs_delete_raw_data
*/
rs_raw_data_buffer* rs_send_and_receive_raw_data(rs_device* device, void* raw_data_to_send, unsigned size_of_raw_data_to_send, rs_error** error);

/**
* get the size of rs_raw_data_buffer
* \param[in] buffer        pointer to rs_raw_data_buffer returned by rs_send_and_receive_raw_data
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return size of rs_raw_data_buffer
*/
int rs_get_raw_data_size(const rs_raw_data_buffer* buffer, rs_error** error);

/**
* delete rs_raw_data_buffer
* \param[in] buffer        rs_raw_data_buffer returned by rs_send_and_receive_raw_data
*/
void rs_delete_raw_data(rs_raw_data_buffer* buffer);

/**
* retrieve char array from rs_raw_data_buffer
* \param[in] buffer        rs_raw_data_buffer returned by rs_send_and_receive_raw_data
* \param[out] error   if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return raw data
*/
const unsigned char* rs_get_raw_data(const rs_raw_data_buffer* buffer, rs_error** error);

/*
librealsense recorder is intended for validation purposes. 
it supports three modes of operation:
*/
typedef enum rs_recording_mode
{
    RS_RECORDING_MODE_BLANK_FRAMES, /* frame metadata will be recorded, but pixel data will be replaced with zeros to save space */
    RS_RECORDING_MODE_COMPRESSED,   /* frames will be encoded using a proprietary lossy encoding, aiming at x5 compression at some CPU expense */
    RS_RECORDING_MODE_BEST_QUALITY, /* frames will not be compressed, but rather stored as-is. This gives best quality and low CPU overhead, but you might run out of memory */
    RS_RECORDING_MODE_COUNT
} rs_recording_mode;

/**
* create librealsense context that will try to record all operations over librealsense into a file
* \param[in] api_version realsense API version as provided by RS_API_VERSION macro
* \param[in] filename string representing the name of the file to record
* \param[in] section  string representing the name of the section within existing recording
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            context object, should be released by rs_delete_context
*/
rs_context* rs_create_recording_context(int api_version, const char* filename, const char* section, rs_recording_mode mode, rs_error** error);

/**
* create librealsense context that given a file will respond to calls exactly as the recording did
* if the user calls a method that was either not called during recording or voilates causality of the recording error will be thrown
* \param[in] api_version realsense API version as provided by RS_API_VERSION macro
* \param[in] filename string representing the name of the file to play back from
* \param[in] section  string representing the name of the section within existing recording
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            context object, should be released by rs_delete_context
*/
rs_context* rs_create_mock_context(int api_version, const char* filename, const char* section, rs_error** error);

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
const char * rs_distortion_to_string (rs_distortion distortion);
const char * rs_option_to_string     (rs_option option);
const char * rs_camera_info_to_string(rs_camera_info info);
const char * rs_camera_info_to_string(rs_camera_info info);
const char * rs_timestamp_domain_to_string(rs_timestamp_domain info);
const char * rs_visual_preset_to_string  (rs_visual_preset preset);

void rs_log_to_console(rs_log_severity min_severity, rs_error ** error);
void rs_log_to_file(rs_log_severity min_severity, const char * file_path, rs_error ** error);

#ifdef __cplusplus
}
#endif
#endif
