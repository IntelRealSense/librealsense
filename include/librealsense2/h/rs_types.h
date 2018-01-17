/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

/** \file rs2_types.h
* \brief
* Exposes RealSense structs
*/

#ifndef LIBREALSENSE_RS2_TYPES_H
#define LIBREALSENSE_RS2_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Category of the librealsense notifications */
typedef enum rs2_notification_category{
    RS2_NOTIFICATION_CATEGORY_FRAMES_TIMEOUT,   /**< Frames didn't arrived within 5 seconds */
    RS2_NOTIFICATION_CATEGORY_FRAME_CORRUPTED,  /**< Received partial/incomplete frame */
    RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR,   /**< Error reported from the device */
    RS2_NOTIFICATION_CATEGORY_HARDWARE_EVENT,   /**< General Hardeware notification that is not an error */
    RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR,    /**< Received unknown error from the device */
    RS2_NOTIFICATION_CATEGORY_COUNT             /**< Number of enumeration values. Not a valid input: intended to be used in for-loops. */
} rs2_notification_category;
const char* rs2_notification_category_to_string(rs2_notification_category category);

/** \brief Exception types are the different categories of errors that RealSense API might return */
typedef enum rs2_exception_type
{
    RS2_EXCEPTION_TYPE_UNKNOWN,
    RS2_EXCEPTION_TYPE_CAMERA_DISCONNECTED,      /**< Device was disconnected, this can be caused by outside intervention, by internal firmware error or due to insufficient power */
    RS2_EXCEPTION_TYPE_BACKEND,                  /**< Error was returned from the underlying OS-specific layer */
    RS2_EXCEPTION_TYPE_INVALID_VALUE,            /**< Invalid value was passed to the API */
    RS2_EXCEPTION_TYPE_WRONG_API_CALL_SEQUENCE,  /**< Function precondition was violated */
    RS2_EXCEPTION_TYPE_NOT_IMPLEMENTED,          /**< The method is not implemented at this point */
    RS2_EXCEPTION_TYPE_DEVICE_IN_RECOVERY_MODE,  /**< Device is in recovery mode and might require firmware update */
    RS2_EXCEPTION_TYPE_IO,                       /**< IO Device failure */
    RS2_EXCEPTION_TYPE_COUNT                     /**< Number of enumeration values. Not a valid input: intended to be used in for-loops. */
} rs2_exception_type;
const char* rs2_exception_type_to_string(rs2_exception_type type);

/** \brief Distortion model: defines how pixel coordinates should be mapped to sensor coordinates. */
typedef enum rs2_distortion
{
    RS2_DISTORTION_NONE                  , /**< Rectilinear images. No distortion compensation required. */
    RS2_DISTORTION_MODIFIED_BROWN_CONRADY, /**< Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points */
    RS2_DISTORTION_INVERSE_BROWN_CONRADY , /**< Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it */
    RS2_DISTORTION_FTHETA                , /**< F-Theta fish-eye distortion model */
    RS2_DISTORTION_BROWN_CONRADY         , /**< Unmodified Brown-Conrady distortion model */
    RS2_DISTORTION_COUNT                   /**< Number of enumeration values. Not a valid input: intended to be used in for-loops. */
} rs2_distortion;
const char* rs2_distortion_to_string(rs2_distortion distortion);

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

/** \brief Motion device intrinsics: scale, bias, and variances */
typedef struct rs2_motion_device_intrinsic
{
    /* \internal
    * Scale X       cross axis  cross axis  Bias X \n
    * cross axis    Scale Y     cross axis  Bias Y \n
    * cross axis    cross axis  Scale Z     Bias Z */
    float data[3][4];          /**< Interpret data array values */

    float noise_variances[3];  /**< Variance of noise for X, Y, and Z axis */
    float bias_variances[3];   /**< Variance of bias for X, Y, and Z axis */
} rs2_motion_device_intrinsic;

/** \brief Severity of the librealsense logger */
typedef enum rs2_log_severity {
    RS2_LOG_SEVERITY_DEBUG, /**< Detailed information about ordinary operations */
    RS2_LOG_SEVERITY_INFO , /**< Terse information about ordinary operations */
    RS2_LOG_SEVERITY_WARN , /**< Indication of possible failure */
    RS2_LOG_SEVERITY_ERROR, /**< Indication of definite failure */
    RS2_LOG_SEVERITY_FATAL, /**< Indication of unrecoverable failure */
    RS2_LOG_SEVERITY_NONE , /**< No logging will occur */
    RS2_LOG_SEVERITY_COUNT  /**< Number of enumeration values. Not a valid input: intended to be used in for-loops. */
} rs2_log_severity;
const char* rs2_log_severity_to_string(rs2_log_severity info);

/** \brief Specifies advanced interfaces (capabilities) objects may implement */
typedef enum rs2_extension
{
    RS2_EXTENSION_UNKNOWN,
    RS2_EXTENSION_DEBUG,
    RS2_EXTENSION_INFO,
    RS2_EXTENSION_MOTION,
    RS2_EXTENSION_OPTIONS,
    RS2_EXTENSION_VIDEO,
    RS2_EXTENSION_ROI,
    RS2_EXTENSION_DEPTH_SENSOR,
    RS2_EXTENSION_VIDEO_FRAME,
    RS2_EXTENSION_MOTION_FRAME,
    RS2_EXTENSION_COMPOSITE_FRAME,
    RS2_EXTENSION_POINTS,
    RS2_EXTENSION_DEPTH_FRAME,
    RS2_EXTENSION_ADVANCED_MODE,
    RS2_EXTENSION_RECORD,
    RS2_EXTENSION_VIDEO_PROFILE,
    RS2_EXTENSION_PLAYBACK,
    RS2_EXTENSION_DEPTH_STEREO_SENSOR,
    RS2_EXTENSION_DISPARITY_FRAME,
    RS2_EXTENSION_MOTION_PROFILE,
    RS2_EXTENSION_POSE_FRAME,
    RS2_EXTENSION_POSE_PROFILE,
    RS2_EXTENSION_TM2,
    RS2_EXTENSION_COUNT
} rs2_extension;
const char* rs2_extension_type_to_string(rs2_extension type);
const char* rs2_extension_to_string(rs2_extension type);

typedef struct rs2_device_info rs2_device_info;
typedef struct rs2_device rs2_device;
typedef struct rs2_error rs2_error;
typedef struct rs2_raw_data_buffer rs2_raw_data_buffer;
typedef struct rs2_frame rs2_frame;
typedef struct rs2_frame_queue rs2_frame_queue;
typedef struct rs2_pipeline rs2_pipeline;
typedef struct rs2_pipeline_profile rs2_pipeline_profile;
typedef struct rs2_config rs2_config;
typedef struct rs2_device_list rs2_device_list;
typedef struct rs2_stream_profile_list rs2_stream_profile_list;
typedef struct rs2_stream_profile rs2_stream_profile;
typedef struct rs2_frame_callback rs2_frame_callback;
typedef struct rs2_log_callback rs2_log_callback;
typedef struct rs2_syncer rs2_syncer;
typedef struct rs2_device_serializer rs2_device_serializer;
typedef struct rs2_source rs2_source;
typedef struct rs2_processing_block rs2_processing_block;
typedef struct rs2_frame_processor_callback rs2_frame_processor_callback;
typedef struct rs2_playback_status_changed_callback rs2_playback_status_changed_callback;
typedef struct rs2_context rs2_context;
typedef struct rs2_device_hub rs2_device_hub;
typedef struct rs2_sensor_list rs2_sensor_list;
typedef struct rs2_sensor rs2_sensor;
typedef struct rs2_options rs2_options;
typedef struct rs2_devices_changed_callback rs2_devices_changed_callback;
typedef struct rs2_notification rs2_notification;
typedef struct rs2_notifications_callback rs2_notifications_callback;
typedef void (*rs2_notification_callback_ptr)(rs2_notification*, void*);
typedef void (*rs2_devices_changed_callback_ptr)(rs2_device_list*, rs2_device_list*, void*);
typedef void (*rs2_frame_callback_ptr)(rs2_frame*, void*);
typedef void (*rs2_frame_processor_callback_ptr)(rs2_frame**, int, rs2_source*, void*);

typedef double      rs2_time_t;     /**< Timestamp format. units are milliseconds */
typedef long long   rs2_metadata_type; /**< Metadata attribute type is defined as 64 bit signed integer*/

rs2_exception_type rs2_get_librealsense_exception_type(const rs2_error* error);
const char* rs2_get_failed_function            (const rs2_error* error);
const char* rs2_get_failed_args                (const rs2_error* error);
const char* rs2_get_error_message              (const rs2_error* error);
void        rs2_free_error                     (rs2_error* error);

#ifdef __cplusplus
}
#endif
#endif
