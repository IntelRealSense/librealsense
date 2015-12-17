#ifndef LIBREALSENSE_RS_H
#define LIBREALSENSE_RS_H

#ifdef __cplusplus
extern "C" {
#endif

#define RS_API_VERSION 4

typedef enum rs_stream
{
    RS_STREAM_DEPTH                            = 0, /**< Native stream of depth data produced by RealSense device */
    RS_STREAM_COLOR                            = 1, /**< Native stream of color data captured by RealSense device */
    RS_STREAM_INFRARED                         = 2, /**< Native stream of infrared data captured by RealSense device */
    RS_STREAM_INFRARED2                        = 3, /**< Native stream of infrared data captured from a second viewpoint by RealSense device */
    RS_STREAM_RECTIFIED_COLOR                  = 4, /**< Synthetic stream containing undistorted color data with no extrinsic rotation from the depth stream */
    RS_STREAM_COLOR_ALIGNED_TO_DEPTH           = 5, /**< Synthetic stream containing color data but sharing intrinsics of depth stream */
    RS_STREAM_DEPTH_ALIGNED_TO_COLOR           = 6, /**< Synthetic stream containing depth data but sharing intrinsics of color stream */
    RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR = 7, /**< Synthetic stream containing depth data but sharing intrinsics of rectified color stream */
    RS_STREAM_WHATEVER_ALIGNED_TO_WHATEVER     = 8,
    RS_STREAM_COUNT                            = 9, 
    RS_STREAM_MAX_ENUM = 0x7FFFFFFF
} rs_stream;

typedef enum rs_format
{
    RS_FORMAT_ANY         = 0,  
    RS_FORMAT_Z16         = 1,  /**< 16 bit linear depth values. The depth is meters is equal to depth scale * pixel value */
    RS_FORMAT_DISPARITY16 = 2,  /**< 16 bit linear disparity values. The depth in meters is equal to depth scale / pixel value */
    RS_FORMAT_YUYV        = 3,  
    RS_FORMAT_RGB8        = 4,  
    RS_FORMAT_BGR8        = 5,  
    RS_FORMAT_RGBA8       = 6,  
    RS_FORMAT_BGRA8       = 7,  
    RS_FORMAT_Y8          = 8,  
    RS_FORMAT_Y16         = 9,  
    RS_FORMAT_RAW10       = 10, /**< Four 10-bit luminance values encoded into a 5-byte macropixel */
    RS_FORMAT_COUNT       = 11, 
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
    RS_DISTORTION_NONE                   = 0, /**< Rectilinear images, no distortion compensation required */
    RS_DISTORTION_MODIFIED_BROWN_CONRADY = 1, /**< Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points */
    RS_DISTORTION_INVERSE_BROWN_CONRADY  = 2, /**< Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it */
    RS_DISTORTION_COUNT                  = 3, 
    RS_DISTORTION_MAX_ENUM = 0x7FFFFFFF
} rs_distortion;

typedef enum rs_option
{
    RS_OPTION_COLOR_BACKLIGHT_COMPENSATION    = 0,  
    RS_OPTION_COLOR_BRIGHTNESS                = 1,  
    RS_OPTION_COLOR_CONTRAST                  = 2,  
    RS_OPTION_COLOR_EXPOSURE                  = 3,  /**< Controls exposure time of color camera. Setting any value will disable auto exposure. */
    RS_OPTION_COLOR_GAIN                      = 4,  
    RS_OPTION_COLOR_GAMMA                     = 5,  
    RS_OPTION_COLOR_HUE                       = 6,  
    RS_OPTION_COLOR_SATURATION                = 7,  
    RS_OPTION_COLOR_SHARPNESS                 = 8,  
    RS_OPTION_COLOR_WHITE_BALANCE             = 9,  /**< Controls white balance of color image. Setting any value will disable auto white balance. */
    RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE      = 10, /**< Set to 1 to enable automatic exposure control, or 0 to return to manual control */
    RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE = 11, /**< Set to 1 to enable automatic white balance control, or 0 to return to manual control */
    RS_OPTION_F200_LASER_POWER                = 12, /**< 0 - 15 */
    RS_OPTION_F200_ACCURACY                   = 13, /**< 0 - 3 */
    RS_OPTION_F200_MOTION_RANGE               = 14, /**< 0 - 100 */
    RS_OPTION_F200_FILTER_OPTION              = 15, /**< 0 - 7 */
    RS_OPTION_F200_CONFIDENCE_THRESHOLD       = 16, /**< 0 - 15 */
    RS_OPTION_F200_DYNAMIC_FPS                = 17, /**< {2, 5, 15, 30, 60} */
    RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED   = 18, /**< {0, 1} */
    RS_OPTION_R200_LR_GAIN                    = 19, /**< 100 - 1600 (Units of 0.01) */
    RS_OPTION_R200_LR_EXPOSURE                = 20, /**< > 0 (Units of 0.1 ms) */
    RS_OPTION_R200_EMITTER_ENABLED            = 21, /**< {0, 1} */
    RS_OPTION_R200_DEPTH_CONTROL_PRESET       = 22, /**< 0 - 5, 0 is default, 1-5 is low to high outlier rejection */
    RS_OPTION_R200_DEPTH_UNITS                = 23, /**< micrometers per increment in integer depth values, 1000 is default (mm scale) */
    RS_OPTION_R200_DEPTH_CLAMP_MIN            = 24, /**< 0 - USHORT_MAX */
    RS_OPTION_R200_DEPTH_CLAMP_MAX            = 25, /**< 0 - USHORT_MAX */
    RS_OPTION_R200_DISPARITY_MULTIPLIER       = 26, /**< 0 - 1000, the increments in integer disparity values corresponding to one pixel of disparity */
    RS_OPTION_R200_DISPARITY_SHIFT            = 27, 
    RS_OPTION_COUNT                           = 28, 
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

rs_context * rs_create_context(int api_version, rs_error ** error);
void rs_delete_context(rs_context * context, rs_error ** error);

/**
 * determine number of connected devices
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            the count of devices
 */
int rs_get_device_count(const rs_context * context, rs_error ** error);

/**
 * retrieve connected device by index
 * \param[in] index   the zero based index of device to retrieve
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            the requested device
 */
rs_device * rs_get_device(rs_context * context, int index, rs_error ** error);

/**
 * retrieve a human readable device model string
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            the model string, such as "Intel RealSense F200" or "Intel RealSense R200"
 */
const char * rs_get_device_name(const rs_device * device, rs_error ** error);

/**
 * retrieve the unique serial number of the device
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            the serial number, in a format specific to the device model
 */
const char * rs_get_device_serial(const rs_device * device, rs_error ** error);

/**
 * retrieve the version of the firmware currently installed on the device
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            firmware version string, in a format is specific to device model
 */
const char * rs_get_device_firmware_version(const rs_device * device, rs_error ** error);

/**
 * retrieve extrinsic transformation between the viewpoints of two different streams
 * \param[in] from_stream  stream whose coordinate space we will transform from
 * \param[in] to_stream    stream whose coordinate space we will transform to
 * \param[out] extrin      the transformation between the two streams
 * \param[out] error       if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs_get_device_extrinsics(const rs_device * device, rs_stream from_stream, rs_stream to_stream, rs_extrinsics * extrin, rs_error ** error);

/**
 * retrieve mapping between the units of the depth image and meters
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            depth in meters corresponding to a depth value of 1
 */
float rs_get_device_depth_scale(const rs_device * device, rs_error ** error);

/**
 * determine if the device allows a specific option to be queried and set
 * \param[in] option  the option to check for support
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            true if the option can be queried and set
 */
int rs_device_supports_option(const rs_device * device, rs_option option, rs_error ** error);

/**
 * determine the range of acceptable values for an option on this device
 * \param[in] option  the option whose range to query
 * \param[out] min    the minimum acceptable value, attempting to set a value below this will take no effect and raise an error
 * \param[out] max    the maximum acceptable value, attempting to set a value above this will take no effect and raise an error
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs_get_device_option_range(const rs_device * device, rs_option option, int * min, int * max, rs_error ** error);

/**
 * determine the number of streaming modes available for a given stream
 * \param[in] stream  the stream whose modes will be enumerated
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            the count of available modes
 */
int rs_get_stream_mode_count(const rs_device * device, rs_stream stream, rs_error ** error);

/**
 * determine the properties of a specific streaming mode
 * \param[in] stream      the stream whose mode will be queried
 * \param[in] index       the zero based index of the streaming mode
 * \param[out] width      the width of a frame image in pixels
 * \param[out] height     the height of a frame image in pixels
 * \param[out] format     the pixel format of a frame image
 * \param[out] framerate  the number of frames which will be streamed per second
 * \param[out] error      if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs_get_stream_mode(const rs_device * device, rs_stream stream, int index, int * width, int * height, rs_format * format, int * framerate, rs_error ** error);

/**
 * enable a specific stream and request specific properties
 * \param[in] stream     the stream to enable
 * \param[in] width      the desired width of a frame image in pixels, or 0 if any width is acceptable
 * \param[in] height     the desired height of a frame image in pixels, or 0 if any height is acceptable
 * \param[in] format     the pixel format of a frame image, or ANY if any format is acceptable
 * \param[in] framerate  the number of frames which will be streamed per second, or 0 if any framerate is acceptable
 * \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs_enable_stream(rs_device * device, rs_stream stream, int width, int height, rs_format format, int framerate, rs_error ** error);

/**
 * enable a specific stream and request properties using a preset
 * \param[in] stream  the stream to enable
 * \param[in] preset  the preset to use to enable the stream
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs_enable_stream_preset(rs_device * device, rs_stream stream, rs_preset preset, rs_error ** error);

/**
 * disable a specific stream
 * \param[in] stream  the stream to disable
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs_disable_stream(rs_device * device, rs_stream stream, rs_error ** error);

/**
 * determine if a specific stream is enabled
 * \param[in] stream  the stream to check
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            true if the stream is currently enabled
 */
int rs_is_stream_enabled(const rs_device * device, rs_stream stream, rs_error ** error);

/**
 * retrieve intrinsic camera parameters for a specific stream
 * \param[in] stream   the stream whose parameters to retrieve
 * \param[out] intrin  the intrinsic parameters of the stream
 * \param[out] error   if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs_get_stream_intrinsics(const rs_device * device, rs_stream stream, rs_intrinsics * intrin, rs_error ** error);

/**
 * retrieve the pixel format for a specific stream
 * \param[in] stream  the stream whose format to retrieve
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            the pixel format of the stream
 */
rs_format rs_get_stream_format(const rs_device * device, rs_stream stream, rs_error ** error);

/**
 * retrieve the framerate for a specific stream
 * \param[in] stream  the stream whose framerate to retrieve
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            the framerate of the stream, in frames per second
 */
int rs_get_stream_framerate(const rs_device * device, rs_stream stream, rs_error ** error);

/**
 * begin streaming on all enabled streams for this device
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs_start_device(rs_device * device, rs_error ** error);

/**
 * end streaming on all streams for this device
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs_stop_device(rs_device * device, rs_error ** error);

/**
 * determine if the device is currently streaming
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            true if the device is currently streaming
 */
int rs_is_device_streaming(const rs_device * device, rs_error ** error);

/**
 * set the value of a specific device option
 * \param[in] option  the option whose value to set
 * \param[in] value   the desired value to set
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs_set_device_option(rs_device * device, rs_option option, int value, rs_error ** error);

/**
 * query the current value of a specific device option
 * \param[in] option  the option whose value to retrieve
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            the current value of the option
 */
int rs_get_device_option(const rs_device * device, rs_option option, rs_error ** error);

/**
 * block until new frames are available
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs_wait_for_frames(rs_device * device, rs_error ** error);

/**
 * retrieve the time at which the latest frame on a stream was captured
 * \param[in] stream  the stream whose latest frame we are interested in
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            the timestamp of the frame, in milliseconds since the device was started
 */
int rs_get_frame_timestamp(const rs_device * device, rs_stream stream, rs_error ** error);

/**
 * retrieve the contents of the latest frame on a stream
 * \param[in] stream  the stream whose latest frame we are interested in
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            the pointer to the start of the frame data
 */
const void * rs_get_frame_data(const rs_device * device, rs_stream stream, rs_error ** error);
                                     
const char * rs_get_failed_function  (const rs_error * error);
const char * rs_get_failed_args      (const rs_error * error);
const char * rs_get_error_message    (const rs_error * error);
void         rs_free_error           (rs_error * error);
                                     
const char * rs_stream_to_string     (rs_stream stream);
const char * rs_format_to_string     (rs_format format);
const char * rs_preset_to_string     (rs_preset preset);
const char * rs_distortion_to_string (rs_distortion distortion);
const char * rs_option_to_string     (rs_option option);

#ifdef __cplusplus
}
#endif
#endif
