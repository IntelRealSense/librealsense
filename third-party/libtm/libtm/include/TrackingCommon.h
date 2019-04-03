// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#ifdef tm_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#ifdef BUILD_STATIC
#define  DLL_EXPORT
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif
#endif /* tm_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif


#define IN
#define OUT

#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),  (mode)))==NULL
#endif

namespace perc {

#ifdef _WIN32
    typedef HANDLE  Handle;
#define ILLEGAL_HANDLE NULL
#else
    typedef int     Handle;
#define ILLEGAL_HANDLE (-1)

#endif

    #define MAC_ADDRESS_SIZE 6
    #define CONTROLLER_SENSOR_DATA_SIZE 6

    typedef uint8_t SensorId;
    typedef uint16_t SessionId;

    /**
    * @brief Defines all sensors types (bSensorID/bCameraID/bMotionID)
    */
    enum SensorType
    {
        Color = 0,
        Depth = 1,
        IR = 2,
        Fisheye = 3,
        Gyro = 4,
        Accelerometer = 5,
        Controller = 6,
        Rssi = 7,
        Velocimeter = 8,
        Stereo = 9,
        Pose = 10,
        ControllerProperty = 11,
        Max
    };

    enum class Status
    {
        SUCCESS = 0,                                      /**< No issues found                                                                                                                                                          */
        COMMON_ERROR = 1,                                 /**< FW unknown message error                                                                                                                                                 */
        FEATURE_UNSUPPORTED = 2,                          /**< Feature is partly implemented and currently unsupported by host                                                                                                          */
        ERROR_PARAMETER_INVALID = 3,                      /**< Invalid parameter                                                                                                                                                        */
        INIT_FAILED = 4,                                  /**< Init flow failed (Loading FW, Identifying device)                                                                                                                        */
        ALLOC_FAILED = 5,                                 /**< Allocation error                                                                                                                                                         */
        ERROR_USB_TRANSFER = 6,                           /**< USB Error while transmitting message to FW                                                                                                                               */
        ERROR_EEPROM_VERIFY_FAIL = 7,                     /**< EEPROM after write verification failed                                                                                                                                   */
        ERROR_FW_INTERNAL = 8,                            /**< Internal FW error                                                                                                                                                        */
        BUFFER_TOO_SMALL = 9,                             /**< Buffer is too small for this operation                                                                                                                                   */
        NOT_SUPPORTED_BY_FW = 10,                         /**< Feature is currently unsupported by FW                                                                                                                                   */
        DEVICE_BUSY = 11,                                 /**< Indicates that this command is not supported in the current device state, e.g.trying to change configuration after START                                                 */
        TIMEOUT = 12,                                     /**< Controller connection failed due to timeout                                                                                                                              */
        TABLE_NOT_EXIST = 13,                             /**< The requested configuration table does not exists on the EEPROM                                                                                                          */
        TABLE_LOCKED = 14,                                /**< The configuration table is locked for writing or permanently locked from unlocking                                                                                       */
        DEVICE_STOPPED = 15,                              /**< The last message over interrupt/stream endpoint after a DEV_STOP command                                                                                                 */
        TEMPERATURE_WARNING = 16,                         /**< The device temperature reached 10 % from its threshold                                                                                                                   */
        TEMPERATURE_STOP = 17,                            /**< The device temperature reached its threshold, and the device stopped tracking                                                                                            */
        CRC_ERROR = 18,                                   /**< CRC Error in firmware update                                                                                                                                             */
        INCOMPATIBLE = 19,                                /**< Controller version is incompatible with TM2 version                                                                                                                      */
        SLAM_NO_DICTIONARY = 20,                          /**< No relocalization dictionary was loaded                                                                                                                                  */
        NO_CALIBRATION_DATA = 21,                         /**< No calibration data is available                                                                                                                                         */
        SLAM_LOCALIZATION_DATA_SET_SUCCESS = 22,          /**< Reading all localization data chunks completed successfully                                                                                                              */
        SLAM_ERROR_CODE_VISION = 23,                      /**< No visual features were detected in the most recent image. This is normal in some circumstances, such as quick motion or if the device temporarily looks at a blank wall */
        SLAM_ERROR_CODE_SPEED = 24,                       /**< The device moved more rapidly than expected for typical handheld motion. This may indicate that rc_Tracker has failed and is providing invalid data                      */
        SLAM_ERROR_CODE_OTHER = 25,                       /**< A fatal SLAM internal error has occurred                                                                                                                                 */
        CONTROLLER_CALIBRATION_VALIDATION_FAILURE = 26,   /**< Controller Calibration validation error                                                                                                                                  */
        CONTROLLER_CALIBRATION_FLASH_ACCESS_FAILURE = 27, /**< Controller Flash access failure                                                                                                                                          */
        CONTROLLER_CALIBRATION_IMU_FAILURE = 28,          /**< Controller Calibration IMU failure                                                                                                                                       */
        CONTROLLER_CALIBRATION_INTERNAL_FAILURE = 29,     /**< Controller Calibration internal failure                                                                                                                                  */
        AUTH_ERROR = 30,                                  /**< Authentication error in firmware update or error in image signature                                                                                                      */
        LIST_TOO_BIG = 31,                                /**< Image size is too big                                                                                                                                                    */
        DEVICE_RESET = 32,                                /**< A device reset has occurred. The user may read the FW log for additional details                                                                                       */
    };

    enum PixelFormat
    {
        ANY = 0,         /**< Any pixel format                                                                                      */
        Z16 = 1,         /**< 16-bit per pixel - linear depth values. The depth is meters is equal to depth scale * pixel value     */
        DISPARITY16 = 2, /**< 16-bit per pixel - linear disparity values. The depth in meters is equal to depth scale / pixel value */
        XYZ32F = 3,      /**< 96-bit per pixel - 32 bit floating point 3D coordinates.                                              */
        YUYV = 4,        /**< 16-bit per pixel - Standard YUV pixel format as described in https://en.wikipedia.org/wiki/YUV        */
        RGB8 = 5,        /**< 24-bit per pixel - 8-bit Red, Green and Blue channels                                                 */
        BGR8 = 6,        /**< 24-bit per pixel - 8-bit Blue, Green and Red channels, suitable for OpenCV                            */
        RGBA8 = 7,       /**< 32-bit per pixel - 8-bit Red, Green, Blue channels + constant alpha channel equal to FF               */
        BGRA8 = 8,       /**< 32-bit per pixel - 8-bit Blue, Green, Red channels + constant alpha channel equal to FF               */
        Y8 = 9,          /**< 8-bit  per pixel - grayscale image                                                                    */
        Y16 = 10,        /**< 16-bit per-pixel - grayscale image                                                                    */
        RAW8 = 11,       /**< 8-bit  per pixel - raw image                                                                          */
        RAW10 = 12,      /**< 10-bit per pixel - Four 10-bit luminance values encoded into a 5-byte macropixel                      */
        RAW16 = 13       /**< 16-bit per pixel - raw image                                                                          */
    };

    /**
    * @brief Defines all 6dof interrupt rates
    */
    typedef enum {
        SIXDOF_INTERRUPT_RATE_NONE = 0X0000,    /* No interrupts*/
        SIXDOF_INTERRUPT_RATE_FISHEYE = 0x0001, /* Interrupts on every fisheye camera update (30 interrupts per second for TM2) */
        SIXDOF_INTERRUPT_RATE_IMU = 0x0002,     /* Interrupts on every IMU update (400-500 interrupts per second for TM2) - default value*/
        SIXDOF_INTERRUPT_RATE_MAX
    } SIXDOF_INTERRUPT_RATE;

    /**
    * @brief Defines all 6dof modes
    */
    typedef enum {
        SIXDOF_MODE_NORMAL = 0X0000,
        SIXDOF_MODE_FAST_PLAYBACK = 0x0001,
        SIXDOF_MODE_ENABLE_MAPPING = 0x0002,
        SIXDOF_MODE_ENABLE_RELOCALIZATION = 0x0004, 
        SIXDOF_MODE_MAX = ((SIXDOF_MODE_FAST_PLAYBACK | SIXDOF_MODE_ENABLE_MAPPING | SIXDOF_MODE_ENABLE_RELOCALIZATION) + 1)
    } SIXDOF_MODE;

}
