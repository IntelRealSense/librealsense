/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

/** \file rs2_streaming.h
* \brief
* Exposes streaming functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_STREAMING_H
#define LIBREALSENSE_RS2_STREAMING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rs2_types.h"

/** \brief Streams are different types of data provided by RealSense devices */
typedef enum rs2_stream
{
    RS2_STREAM_ANY,
    RS2_STREAM_DEPTH                            , /**< Native stream of depth data produced by RealSense device */
    RS2_STREAM_COLOR                            , /**< Native stream of color data captured by RealSense device */
    RS2_STREAM_INFRARED                         , /**< Native stream of infrared data captured by RealSense device */
    RS2_STREAM_FISHEYE                          , /**< Native stream of fish-eye (wide) data captured from the dedicate motion camera */
    RS2_STREAM_GYRO                             , /**< Native stream of gyroscope motion data produced by RealSense device */
    RS2_STREAM_ACCEL                            , /**< Native stream of accelerometer motion data produced by RealSense device */
    RS2_STREAM_GPIO                             , /**< Signals from external device connected through GPIO */
    RS2_STREAM_COUNT
} rs2_stream;

/** \brief Format identifies how binary data is encoded within a frame */
typedef enum rs2_format
{
    RS2_FORMAT_ANY             , /**< When passed to enable stream, librealsense will try to provide best suited format */
    RS2_FORMAT_Z16             , /**< 16-bit linear depth values. The depth is meters is equal to depth scale * pixel value. */
    RS2_FORMAT_DISPARITY16     , /**< 16-bit linear disparity values. The depth in meters is equal to depth scale / pixel value. */
    RS2_FORMAT_XYZ32F          , /**< 32-bit floating point 3D coordinates. */
    RS2_FORMAT_YUYV            , /**< Standard YUV pixel format as described in https://en.wikipedia.org/wiki/YUV */
    RS2_FORMAT_RGB8            , /**< 8-bit red, green and blue channels */
    RS2_FORMAT_BGR8            , /**< 8-bit blue, green, and red channels -- suitable for OpenCV */
    RS2_FORMAT_RGBA8           , /**< 8-bit red, green and blue channels + constant alpha channel equal to FF */
    RS2_FORMAT_BGRA8           , /**< 8-bit blue, green, and red channels + constant alpha channel equal to FF */
    RS2_FORMAT_Y8              , /**< 8-bit per-pixel grayscale image */
    RS2_FORMAT_Y16             , /**< 16-bit per-pixel grayscale image */
    RS2_FORMAT_RAW10           , /**< Four 10-bit luminance values encoded into a 5-byte macropixel */
    RS2_FORMAT_RAW16           , /**< 16-bit raw image */
    RS2_FORMAT_RAW8            , /**< 8-bit raw image */
    RS2_FORMAT_UYVY            , /**< Similar to the standard YUYV pixel format, but packed in a different order */
    RS2_FORMAT_MOTION_RAW      , /**< Raw data from the motion sensor */
    RS2_FORMAT_MOTION_XYZ32F   , /**< Motion data packed as 3 32-bit float values, for X, Y, and Z axis */
    RS2_FORMAT_GPIO_RAW        , /**< Raw data from the external sensors hooked to one of the GPIO's */
    RS2_FORMAT_COUNT             /**< Number of enumeration values. Not a valid input: intended to be used in for-loops. */
} rs2_format;

/** \brief Category of the librealsense notifications */
typedef enum rs2_notification_category{
    RS2_NOTIFICATION_CATEGORY_FRAMES_TIMEOUT,   /**< Frames didn't arrived within 5 seconds */
    RS2_NOTIFICATION_CATEGORY_FRAME_CORRUPTED,  /**< Received partial/incomplete frame */
    RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR,   /**< Error reported from the device */
    RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR,    /**< Received unknown error from the device */
    RS2_NOTIFICATION_CATEGORY_COUNT             /**< Number of enumeration values. Not a valid input: intended to be used in for-loops. */
} rs2_notification_category;

/** \brief Video stream intrinsics */
typedef struct rs2_intrinsics
{
    int           width;     /**< Width of the image in pixels */
    int           height;    /**< Height of the image in pixels */
    float         ppx;       /**< Horizontal coordinate of the principal point of the image, as a pixel offset from the left edge */
    float         ppy;       /**< Vertical coordinate of the principal point of the image, as a pixel offset from the top edge */
    float         fx;        /**< Focal length of the image plane, as a multiple of pixel width */
    float         fy;        /**< Focal length of the image plane, as a multiple of pixel height */
    rs2_distortion model;     /**< Distortion model of the image */
    float         coeffs[5]; /**< Distortion coefficients */
} rs2_intrinsics;

/** \brief Cross-stream extrinsics: encode the topology describing how the different devices are connected. */
typedef struct rs2_extrinsics
{
    float rotation[9];    /**< Column-major 3x3 rotation matrix */
    float translation[3]; /**< Three-element translation vector, in meters */
} rs2_extrinsics;

const char* rs2_stream_to_string(rs2_stream stream);
const char* rs2_format_to_string(rs2_format format);
const char* rs2_notification_category_to_string(rs2_notification_category category);

/**
* open subdevice for exclusive access, by committing to a configuration
* \param[in] sensor relevant RealSense device
* \param[in] profile TODO
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_open(rs2_sensor* device, const rs2_stream_profile* profile, rs2_error** error);

/**
* open subdevice for exclusive access, by committing to composite configuration, specifying one or more stream profiles
* this method should be used for interdependent  streams, such as depth and infrared, that have to be configured together
* \param[in] sensor relevant RealSense device
TODO
* \param[in] count      number of simultaneous  stream profiles to configure
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_open_multiple(rs2_sensor* device, const rs2_stream_profile** profiles, int count, rs2_error** error);

/**
* stop any streaming from specified subdevice
* \param[in] sensor     RealSense device
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_close(const rs2_sensor* sensor, rs2_error** error);

/**
* start streaming from specified configured sensor
* \param[in] sensor  RealSense device
* \param[in] on_frame function pointer to register as per-frame callback
* \param[in] user auxiliary  data the user wishes to receive together with every frame callback
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_start(const rs2_sensor* sensor, rs2_frame_callback_ptr on_frame, void* user, rs2_error** error);

/**
* start streaming from specified configured sensor
* \param[in] sensor  RealSense device
* \param[in] callback callback object created from c++ application. ownership over the callback object is moved into the relevant streaming lock
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_start_cpp(const rs2_sensor* sensor, rs2_frame_callback* callback, rs2_error** error);

/**
* stops streaming from specified configured device
* \param[in] sensor  RealSense sensor
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_stop(const rs2_sensor* sensor, rs2_error** error);

/**
* set callback to get notifications from specified sensor
* \param[in] sensor  RealSense device
* \param[in] device  RealSense device
* \param[in] callback function pointer to register as per-notifications callback
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_set_notifications_callback(const rs2_sensor* sensor, rs2_notification_callback_ptr on_notification, void* user, rs2_error** error);

/**
* set callback to get notifications from specified device
* \param[in] sensor  RealSense sensor
* \param[in] callback callback object created from c++ application. ownership over the callback object is moved into the relevant subdevice lock
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_set_notifications_callback_cpp(const rs2_sensor* sensor, rs2_notifications_callback* callback, rs2_error** error);

/**
* retrieve description from notification handle
* \param[in] notification      handle returned from a callback
* \return            the notification description
*/
const char* rs2_get_notification_description(rs2_notification* notification, rs2_error** error);

/**
* retrieve timestamp from notification handle
* \param[in] notification      handle returned from a callback
* \return            the notification timestamp
*/
rs2_time_t rs2_get_notification_timestamp(rs2_notification * notification, rs2_error** error);

/**
* retrieve severity from notification handle
* \param[in] notification      handle returned from a callback
* \return            the notification severity
*/
rs2_log_severity rs2_get_notification_severity(rs2_notification * notification, rs2_error** error);

/**
* retrieve category from notification handle
* \param[in] notification      handle returned from a callback
* \return            the notification category
*/
rs2_notification_category rs2_get_notification_category(rs2_notification * notification, rs2_error** error);

/**
* check if physical subdevice is supported
* \param[in] device  input RealSense device
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            list of stream profiles that given subdevice can provide, should be released by rs2_delete_profiles_list
*/
rs2_stream_profile_list* rs2_get_stream_profiles(rs2_sensor* device, rs2_error** error);

/**
* Get pointer to specific stream profile
* \param[in] list        the list of supported profiles returned by rs2_get_supported_profiles
* \param[in] index       the zero based index of the streaming mode
* \param[out] error      if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
const rs2_stream_profile* rs2_get_stream_profile(const rs2_stream_profile_list* list, int index, rs2_error** error);

void rs2_get_stream_profile_data(const rs2_stream_profile* mode, rs2_stream* stream, rs2_format* format, int* index, int* unique_id, int* framerate, rs2_error** error);

void rs2_set_stream_profile_data(rs2_stream_profile* mode, rs2_stream stream, int index, rs2_format format, rs2_error** error);

rs2_stream_profile* rs2_clone_stream_profile(const rs2_stream_profile* mode, rs2_error** error);

void rs2_delete_stream_profile(rs2_stream_profile* mode);

int rs2_stream_profile_is(const rs2_stream_profile* mode, rs2_extension type, rs2_error** error);

void rs2_get_video_stream_resolution(const rs2_stream_profile* from, int* width, int* height, rs2_error** error);

int rs2_get_stream_profile_size(const rs2_stream_profile* profile, rs2_error** error);

int rs2_is_stream_profile_recommended(const rs2_stream_profile* profile, rs2_error** error);

/**
* get the number of supported stream profiles
* \param[in] list        the list of supported profiles returned by rs2_get_supported_profiles
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return number of supported subdevice profiles
*/
int rs2_get_stream_profiles_count(const rs2_stream_profile_list* list, rs2_error** error);

/**
* delete stream profiles list
* \param[in] list        the list of supported profiles returned by rs2_get_supported_profiles
*/
void rs2_delete_stream_profiles_list(rs2_stream_profile_list* list);

/**
 * TODO
 * \param[out] error        if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs2_get_extrinsics(const rs2_stream_profile* from,
                        const rs2_stream_profile* to,
                        rs2_extrinsics * extrin, rs2_error ** error);

/**
 * returns the intrinsics of specific stream configuration
   TODO
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs2_get_video_stream_intrinsics(const rs2_stream_profile* from, rs2_intrinsics * intrinsics, rs2_error ** error);

#ifdef __cplusplus
}
#endif
#endif
