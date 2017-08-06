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

/** \brief Distortion model: defines how pixel coordinates should be mapped to sensor coordinates. */
typedef enum rs2_distortion
{
    RS2_DISTORTION_NONE                  , /**< Rectilinear images. No distortion compensation required. */
    RS2_DISTORTION_MODIFIED_BROWN_CONRADY, /**< Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points */
    RS2_DISTORTION_INVERSE_BROWN_CONRADY , /**< Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it */
    RS2_DISTORTION_FTHETA                , /**< F-Theta fish-eye distortion model */
    RS2_DISTORTION_BROWN_CONRADY         , /**< Unmodified Brown-Conrady distortion model */
    RS2_DISTORTION_COUNT                 , /**< Number of enumeration values. Not a valid input: intended to be used in for-loops. */
} rs2_distortion;

/** \brief For SR300 devices: provides optimized settings (presets) for specific types of usage. */
typedef enum rs2_ivcam_visual_preset
{
    RS2_IVCAM_VISUAL_PRESET_SHORT_RANGE             , /**< Preset for short range */
    RS2_IVCAM_VISUAL_PRESET_LONG_RANGE              , /**< Preset for long range */
    RS2_IVCAM_VISUAL_PRESET_BACKGROUND_SEGMENTATION , /**< Preset for background segmentation */
    RS2_IVCAM_VISUAL_PRESET_GESTURE_RECOGNITION     , /**< Preset for gesture recognition */
    RS2_IVCAM_VISUAL_PRESET_OBJECT_SCANNING         , /**< Preset for object scanning */
    RS2_IVCAM_VISUAL_PRESET_FACE_ANALYTICS          , /**< Preset for face analytics */
    RS2_IVCAM_VISUAL_PRESET_FACE_LOGIN              , /**< Preset for face login */
    RS2_IVCAM_VISUAL_PRESET_GR_CURSOR               , /**< Preset for GR cursor */
    RS2_IVCAM_VISUAL_PRESET_DEFAULT                 , /**< Preset for default */
    RS2_IVCAM_VISUAL_PRESET_MID_RANGE               , /**< Preset for mid-range */
    RS2_IVCAM_VISUAL_PRESET_IR_ONLY                 , /**< Preset for IR only */
    RS2_IVCAM_VISUAL_PRESET_COUNT                     /**< Number of enumeration values. Not a valid input: intended to be used in for-loops. */
} rs2_ivcam_visual_preset;


/** \brief Defines general configuration controls.
   These can generally be mapped to camera UVC controls, and unless stated otherwise, can be set/queried at any time. */
typedef enum rs2_option
{
    RS2_OPTION_BACKLIGHT_COMPENSATION                     , /**< Enable / disable color backlight compensation*/
    RS2_OPTION_BRIGHTNESS                                 , /**< Color image brightness*/
    RS2_OPTION_CONTRAST                                   , /**< Color image contrast*/
    RS2_OPTION_EXPOSURE                                   , /**< Controls exposure time of color camera. Setting any value will disable auto exposure*/
    RS2_OPTION_GAIN                                       , /**< Color image gain*/
    RS2_OPTION_GAMMA                                      , /**< Color image gamma setting*/
    RS2_OPTION_HUE                                        , /**< Color image hue*/
    RS2_OPTION_SATURATION                                 , /**< Color image saturation setting*/
    RS2_OPTION_SHARPNESS                                  , /**< Color image sharpness setting*/
    RS2_OPTION_WHITE_BALANCE                              , /**< Controls white balance of color image. Setting any value will disable auto white balance*/
    RS2_OPTION_ENABLE_AUTO_EXPOSURE                       , /**< Enable / disable color image auto-exposure*/
    RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE                  , /**< Enable / disable color image auto-white-balance*/
    RS2_OPTION_VISUAL_PRESET                              , /**< Provide access to several recommend sets of option presets for the depth camera */
    RS2_OPTION_LASER_POWER                                , /**< Power of the F200 / SR300 projector, with 0 meaning projector off*/
    RS2_OPTION_ACCURACY                                   , /**< Set the number of patterns projected per frame. The higher the accuracy value the more patterns projected. Increasing the number of patterns help to achieve better accuracy. Note that this control is affecting the Depth FPS */
    RS2_OPTION_MOTION_RANGE                               , /**< Motion vs. Range trade-off, with lower values allowing for better motion sensitivity and higher values allowing for better depth range*/
    RS2_OPTION_FILTER_OPTION                              , /**< Set the filter to apply to each depth frame. Each one of the filter is optimized per the application requirements*/
    RS2_OPTION_CONFIDENCE_THRESHOLD                       , /**< The confidence level threshold used by the Depth algorithm pipe to set whether a pixel will get a valid range or will be marked with invalid range*/
    RS2_OPTION_EMITTER_ENABLED                            , /**< Laser Emitter enabled */
    RS2_OPTION_FRAMES_QUEUE_SIZE                          , /**< Number of frames the user is allowed to keep per stream. Trying to hold-on to more frames will cause frame-drops.*/
    RS2_OPTION_TOTAL_FRAME_DROPS                          , /**< Total number of detected frame drops from all streams */
    RS2_OPTION_AUTO_EXPOSURE_MODE                         , /**< Auto-Exposure modes: Static, Anti-Flicker and Hybrid */
    RS2_OPTION_POWER_LINE_FREQUENCY                       , /**< Power Line Frequency control for anti-flickering Off/50Hz/60Hz/Auto */
    RS2_OPTION_ASIC_TEMPERATURE                           , /**< Current Asic Temperature */
    RS2_OPTION_ERROR_POLLING_ENABLED                      , /**< disable error handling */
    RS2_OPTION_PROJECTOR_TEMPERATURE                      , /**< Current Projector Temperature */
    RS2_OPTION_OUTPUT_TRIGGER_ENABLED                     , /**< Enable / disable trigger to be outputed from the camera to any external device on every depth frame */
    RS2_OPTION_MOTION_MODULE_TEMPERATURE                  , /**< Current Motion-Module Temperature */
    RS2_OPTION_DEPTH_UNITS                                , /**< Number of meters represented by a single depth unit */
    RS2_OPTION_ENABLE_MOTION_CORRECTION                   , /**< Enable/Disable automatic correction of the motion data */
    RS2_OPTION_ADVANCED_MODE_PRESET                       , /**< Camera Advanced-Mode preset */
    RS2_OPTION_COUNT                                      , /**< Number of enumeration values. Not a valid input: intended to be used in for-loops. */
} rs2_option;

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
    RS2_EXTENSION_ADVANCED_MODE,
    RS2_EXTENSION_RECORD,
    RS2_EXTENSION_VIDEO_PROFILE,
    RS2_EXTENSION_PLAYBACK,
    RS2_EXTENSION_COUNT
} rs2_extension;

typedef struct rs2_device_info rs2_device_info;
typedef struct rs2_device rs2_device;
typedef struct rs2_error rs2_error;
typedef struct rs2_raw_data_buffer rs2_raw_data_buffer;
typedef struct rs2_frame rs2_frame;
typedef struct rs2_frame_queue rs2_frame_queue;
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
typedef struct rs2_sensor_list rs2_sensor_list;
typedef struct rs2_sensor rs2_sensor;
typedef struct rs2_devices_changed_callback rs2_devices_changed_callback;
typedef struct rs2_notification rs2_notification;
typedef struct rs2_notifications_callback rs2_notifications_callback;
typedef void (*rs2_notification_callback_ptr)(rs2_notification*, void*);
typedef void (*rs2_devices_changed_callback_ptr)(rs2_device_list*, rs2_device_list*, void*);
typedef void (*rs2_frame_callback_ptr)(rs2_frame*, void*);
typedef void (*rs2_frame_processor_callback_ptr)(rs2_frame**, int, rs2_source*, void*);

typedef double      rs2_time_t;     /**< Timestamp format. units are milliseconds */
typedef long long   rs2_metadata_t; /**< Metadata attribute type is defined as 64 bit signed integer*/

rs2_exception_type rs2_get_librealsense_exception_type(const rs2_error* error);
const char* rs2_get_failed_function            (const rs2_error* error);
const char* rs2_get_failed_args                (const rs2_error* error);
const char* rs2_get_error_message              (const rs2_error* error);
void        rs2_free_error                     (rs2_error* error);
const char* rs2_exception_type_to_string       (rs2_exception_type type);
const char* rs2_extension_type_to_string       (rs2_extension type);
const char* rs2_visual_preset_to_string        (rs2_ivcam_visual_preset preset);
const char* rs2_visual_preset_to_string        (rs2_ivcam_visual_preset preset);
const char* rs2_distortion_to_string           (rs2_distortion distortion);
const char* rs2_log_severity_to_string         (rs2_log_severity info);
const char* rs2_option_to_string               (rs2_option option);

#ifdef __cplusplus
}
#endif
#endif
