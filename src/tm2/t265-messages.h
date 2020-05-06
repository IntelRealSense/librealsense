// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

/**
* @brief This file contains protocols definitions for TM2 USB vendor specific host/device interface
*
* Bulk In/Out endpoints - for general communication, the protocol on the bulk endpoints is in a request/response convention
* Additional Bulk In endpoint - for video stream (Stream endpoint)
* Interrupt endpoint - for periodic low latency data (namely 6DoF output) and IMU outputs
* All structures below should be treated as little-endian
*/

#pragma once

#include <stdint.h>
#include <math.h>

#pragma pack(push, 1)

#ifdef _WIN32
#pragma warning (push)
#pragma warning (disable : 4200)
#else
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#define MAX_VIDEO_STREAMS 8
#define MAX_FW_LOG_BUFFER_ENTRIES (512)
#define MAX_LOG_PAYLOAD_SIZE (44)
#define MAX_MESSAGE_LEN 1024
#define MAX_EEPROM_BUFFER_SIZE (MAX_MESSAGE_LEN - sizeof(bulk_message_request_header) - 4)
#define MAX_TABLE_SIZE (MAX_MESSAGE_LEN - sizeof(bulk_message_response_header))
#define MAX_EEPROM_CONFIGURATION_SIZE 1200
#define MAX_GUID_LENGTH 128
#define MAX_FW_UPDATE_FILE_COUNT 6
#define MAX_SLAM_CALIBRATION_SIZE 10000

// Added from TrackingCommon.h
namespace t265
{
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
        Mask = 12,
        Max
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
    * @brief Defines all 6dof modes
    */
    typedef enum {
        SIXDOF_MODE_NORMAL = 0X0000,
        SIXDOF_MODE_FAST_PLAYBACK = 0x0001,
        SIXDOF_MODE_ENABLE_MAPPING = 0x0002,
        SIXDOF_MODE_ENABLE_RELOCALIZATION = 0x0004,
        SIXDOF_MODE_DISABLE_JUMPING = 0x0008,
        SIXDOF_MODE_DISABLE_DYNAMIC_CALIBRATION = 0x0010,
        SIXDOF_MODE_ENABLE_MAP_PRESERVATION = 0x0020,
        SIXDOF_MODE_MAX = ((SIXDOF_MODE_FAST_PLAYBACK | SIXDOF_MODE_ENABLE_MAPPING | SIXDOF_MODE_ENABLE_RELOCALIZATION | SIXDOF_MODE_DISABLE_JUMPING | SIXDOF_MODE_DISABLE_DYNAMIC_CALIBRATION | SIXDOF_MODE_ENABLE_MAP_PRESERVATION) + 1)
    } SIXDOF_MODE;

    inline SIXDOF_MODE &operator|=(SIXDOF_MODE &x, SIXDOF_MODE y) {
        return x = static_cast<SIXDOF_MODE>(static_cast<typename std::underlying_type<SIXDOF_MODE>::type>(x) |
                                            static_cast<typename std::underlying_type<SIXDOF_MODE>::type>(y));
    }
    inline SIXDOF_MODE &operator&=(SIXDOF_MODE &x, SIXDOF_MODE y) {
        return x = static_cast<SIXDOF_MODE>(static_cast<typename std::underlying_type<SIXDOF_MODE>::type>(x) &
                                            static_cast<typename std::underlying_type<SIXDOF_MODE>::type>(y));
    }
    inline SIXDOF_MODE operator~(SIXDOF_MODE x) {
        return static_cast<SIXDOF_MODE>(~static_cast<typename std::underlying_type<SIXDOF_MODE>::type>(x));
    }
    inline SIXDOF_MODE operator|(SIXDOF_MODE x, SIXDOF_MODE y) { return x |= y; }
    inline SIXDOF_MODE operator&(SIXDOF_MODE x, SIXDOF_MODE y) { return x &= y; }

    /**
    * @brief Defines all bulk messages ids
    */
    typedef enum {

        /* Core device messages */
        DEV_GET_DEVICE_INFO = 0x0001,
        DEV_GET_TIME = 0x0002,
        DEV_GET_AND_CLEAR_EVENT_LOG = 0x0003,
        DEV_GET_SUPPORTED_RAW_STREAMS = 0x0004,
        DEV_RAW_STREAMS_CONTROL = 0x0005,
        DEV_GET_CAMERA_INTRINSICS = 0x0006,
        DEV_GET_MOTION_INTRINSICS = 0x0007,
        DEV_GET_EXTRINSICS = 0x0008,
        DEV_SET_CAMERA_INTRINSICS = 0x0009,
        DEV_SET_MOTION_INTRINSICS = 0x000A,
        DEV_SET_EXTRINSICS = 0x000B,
        DEV_LOG_CONTROL = 0x000C,
        DEV_STREAM_CONFIG = 0x000D,
        DEV_RAW_STREAMS_PLAYBACK_CONTROL = 0x000E,
        DEV_READ_EEPROM = 0x000F,
        DEV_WRITE_EEPROM = 0x0010,
        DEV_SAMPLE = 0x0011,
        DEV_START = 0x0012,
        DEV_STOP = 0x0013,
        DEV_STATUS = 0x0014,
        DEV_GET_POSE = 0x0015,
        DEV_EXPOSURE_MODE_CONTROL = 0x0016,
        DEV_SET_EXPOSURE = 0x0017,
        DEV_GET_TEMPERATURE = 0x0018,
        DEV_SET_TEMPERATURE_THRESHOLD = 0x0019,
        DEV_FIRMWARE_UPDATE = 0x001C,
        DEV_GPIO_CONTROL = 0x001D,
        DEV_TIMEOUT_CONFIGURATION = 0x001E,
        DEV_SNAPSHOT = 0x001F,
        DEV_READ_CONFIGURATION = 0x0020,
        DEV_WRITE_CONFIGURATION = 0x0021,
        DEV_RESET_CONFIGURATION = 0x0022,
        DEV_LOCK_CONFIGURATION = 0x0023,
        DEV_LOCK_EEPROM = 0x0024,
        DEV_SET_LOW_POWER_MODE = 0x0025,

        /* SLAM messages */
        SLAM_STATUS = 0x1001,
        SLAM_GET_OCCUPANCY_MAP_TILES = 0x1002,
        SLAM_GET_LOCALIZATION_DATA = 0x1003,
        SLAM_SET_LOCALIZATION_DATA_STREAM = 0x1004,
        SLAM_SET_6DOF_INTERRUPT_RATE = 0x1005,
        SLAM_6DOF_CONTROL = 0x1006,
        SLAM_OCCUPANCY_MAP_CONTROL = 0x1007,
        SLAM_GET_LOCALIZATION_DATA_STREAM = 0x1009,
        SLAM_SET_STATIC_NODE = 0x100A,
        SLAM_GET_STATIC_NODE = 0x100B,
        SLAM_APPEND_CALIBRATION = 0x100C,
        SLAM_CALIBRATION = 0x100D,
        SLAM_RELOCALIZATION_EVENT = 0x100E,
        SLAM_REMOVE_STATIC_NODE = 0x100F,

        /* Error messages */
        DEV_ERROR = 0x8000,
        SLAM_ERROR = 0x9000,

        /* Message IDs are 16-bits */
        MAX_MESSAGE_ID = 0xFFFF,
    } BULK_MESSAGE_ID;


    /**
    * @brief Defines all bulk message return statuses
    */
    typedef enum {
        SUCCESS = 0X0000,
        UNKNOWN_MESSAGE_ID = 0x0001,
        INVALID_REQUEST_LEN = 0x0002,
        INVALID_PARAMETER = 0x0003,
        INTERNAL_ERROR = 0x0004,
        UNSUPPORTED = 0x0005,
        LIST_TOO_BIG = 0x0006,
        MORE_DATA_AVAILABLE = 0x0007,
        DEVICE_BUSY = 0x0008,         /* Indicates that this command is not supported in the current device state, e.g.trying to change configuration after START */
        TIMEOUT = 0x0009,
        TABLE_NOT_EXIST = 0x000A,     /* The requested configuration table does not exists on the EEPROM */
        TABLE_LOCKED = 0x000B,        /* The configuration table is locked for writing or permanently locked from unlocking */
        DEVICE_STOPPED = 0x000C,      /* Used with DEV_STATUS messages to mark the last message over an endpoint after a DEV_STOP command */
        TEMPERATURE_WARNING = 0x0010, /* The device temperature reached 10 % from its threshold */
        TEMPERATURE_STOP = 0x0011,    /* The device temperature reached its threshold, and the device stopped tracking */
        CRC_ERROR = 0x0012,           /* CRC Error in firmware update */
        INCOMPATIBLE = 0x0013,        /* Controller version is incompatible with TM2 version */
        AUTH_ERROR = 0x0014,          /* Authentication error in firmware update */
        DEVICE_RESET = 0x0015,        /* A device reset has occurred. The user may read the FW log for additional detail */
        SLAM_NO_DICTIONARY = 0x9001,  /* No relocalization dictionary was loaded */
    } MESSAGE_STATUS;

    /**
    * @brief Defines EEPROM lock states
    */
    typedef enum {
        EEPROM_LOCK_STATE_WRITEABLE = 0x0000,
        EEPROM_LOCK_STATE_LOCKED = 0x0001,
        EEPROM_LOCK_STATE_PERMANENT_LOCKED = 0x0003,
        EEPROM_LOCK_STATE_MAX = 0xFFFF,
    } EEPROM_LOCK_STATE;
    
    /**
    * @brief Defines SKU info types
    */
    typedef enum {
        SKU_INFO_TYPE_WITHOUT_BLUETOOTH = 0x0000,
        SKU_INFO_TYPE_WITH_BLUETOOTH = 0x0001,
        SKU_INFO_TYPE_MAX = 0xFFFF,
    } SKU_INFO_TYPE;
    
    /**
    * @brief Defines all control messages ids
    */
    typedef enum {
        CONTROL_USB_RESET = 0x0010,
        CONTROL_GET_INTERFACE_VERSION = 0x0011,

        /* Message IDs are 16-bits */
        MAX_CONTROL_ID = 0xFFFF,
    } CONTROL_MESSAGE_ID;

    /**
    * @brief Defines all FW status codes
    */
    typedef enum {
        FW_STATUS_CODE_OK = 0,
        FW_STATUS_CODE_FAIL = 1,
        FW_STATUS_CODE_NO_CALIBRATION_DATA = 1000
    } FW_STATUS_CODE;

    /**
    * @brief Defines all SLAM status codes
    */
    typedef enum {
        SLAM_STATUS_CODE_SUCCESS = 0,                       /**< No error has occurred.                                      */
        SLAM_STATUS_CODE_LOCALIZATION_DATA_SET_SUCCESS = 1, /**< Reading all localization data chunks completed successfully */
    } SLAM_STATUS_CODE;

    /**
    * @brief Defines all SLAM error codes
    */
    typedef enum {
        SLAM_ERROR_CODE_NONE = 0,   /**< No error has occurred.                                                                                                                                                    */
        SLAM_ERROR_CODE_VISION = 1, /**< No visual features were detected in the most recent image. This is normal in some circumstances, such as quick motion or if the device temporarily looks at a blank wall. */
        SLAM_ERROR_CODE_SPEED = 2,  /**< The device moved more rapidly than expected for typical handheld motion. This may indicate that rc_Tracker has failed and is providing invalid data.                      */
        SLAM_ERROR_CODE_OTHER = 3,  /**< A fatal internal error has occurred.                                                                                                                                      */
    } SLAM_ERROR_CODE;

    /**
    * @brief Bulk message request header struct
    *
    * Start of all USB message requests.
    */
    typedef struct {
        uint32_t dwLength;   /**< Message length in bytes */
        uint16_t wMessageID; /**< ID of message           */
    } bulk_message_request_header;


    /**
    * @brief Bulk message response header struct
    *
    * Start of all USB message responses.
    */
    typedef struct {
        uint32_t dwLength;   /**< Message length in bytes            */
        uint16_t wMessageID; /**< ID of message                      */
        uint16_t wStatus;    /**< Status of request (MESSAGE_STATUS) */
    } bulk_message_response_header;


    /**
    * @brief Device Info libtm Message
    *
    * Retrieve information on the TM2 device.
    */
    typedef struct {
        uint8_t bDeviceType;                    /**< Device identifier: 0x1 = T250                                                      */
        uint8_t bHwVersion;                     /**< ASIC Board version: 0x00 = ES0, 0x01 = ES1, 0x02 = ES2, 0x03 = ES3, 0xFF = Unknown */
        uint8_t bStatus;                        /**< Bits 0-3: device status: 0x0 = device functional, 0x1 = error, Bits 4-7: Reserved  */
        uint8_t bEepromDataMajor;               /**< Major part of the EEPROM data version                                              */
        uint8_t bEepromDataMinor;               /**< Minor part of the EEPROM data version                                              */
        uint8_t bFWVersionMajor;                /**< Major part of the Myriad firmware version                                          */
        uint8_t bFWVersionMinor;                /**< Minor part of the Myriad firmware version                                          */
        uint8_t bFWVersionPatch;                /**< Patch part of the Myriad firmware version                                          */
        uint32_t dwFWVersionBuild;              /**< Build part of the Myriad firmware version                                          */
        uint32_t dwRomVersion;                  /**< Myriad ROM version                                                                 */
        uint32_t dwStatusCode;                  /**< Status Code: S_OK = 0, E_FAIL = 1, E_NO_CALIBRATION_DATA = 1000                    */
        uint32_t dwExtendedStatus;              /**< Extended status information (details TBD)                                          */
        uint64_t llSerialNumber;                /**< Device serial number                                                               */
        uint8_t bCentralProtocolVersion;        /**< Central BLE Protocol Version                                                       */
        uint8_t bCentralAppVersionMajor;        /**< Major part of the Central firmware version                                         */
        uint8_t bCentralAppVersionMinor;        /**< Minor part of the Minor Central firmware version                                   */
        uint8_t bCentralAppVersionPatch;        /**< Patch part of the Patch Central firmware version                                   */
        uint8_t bCentralSoftdeviceVersion;      /**< Central BLE Softdevice Version                                                     */
        uint8_t bCentralBootloaderVersionMajor; /**< Major part of the Central firmware version                                         */
        uint8_t bCentralBootloaderVersionMinor; /**< Minor part of the Minor Central firmware version                                   */
        uint8_t bCentralBootloaderVersionPatch; /**< Patch part of the Patch Central firmware version                                   */
        uint32_t dwCentralAppVersionBuild;      /**< Build part of the Build Central firmware version                                   */
        uint8_t bEepromLocked;                  /**< 0x0 - The EEPROM is fully writeable                                                */
                                                /**< 0x1 - The upper quarter of the EEPROM memory is write-protected                    */
                                                /**< 0x3 - The upper quarter of the EEPROM memory is permanently write-protected        */
        uint8_t bSKUInfo;                       /**< 1 for T260 - with ble, 0 for T265 - w/o ble                                        */
    } device_info_libtm_message;


    /**
    * @brief Bulk Get Device Info Message
    *
    * Retrieve TM2 device information.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 6 bytes, wMessageID = DEV_GET_DEVICE_INFO */
    } bulk_message_request_get_device_info;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 49 or 8 bytes, wMessageID = DEV_GET_DEVICE_INFO */
        device_info_libtm_message message;   /**< Device info                                                                         */
    } bulk_message_response_get_device_info;


    /**
    * @brief Bulk Device stream config Message
    *
    * Sets the maximum message size the host can receive
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 10 bytes, wMessageID = DEV_STREAM_CONFIG  */
        uint16_t wReserved;                 /**< Reserved = 0                                                                 */
        uint32_t dwMaxSize;                 /**< Max stream endpoint message buffer size (Android 16K, Windows/Linux 678,400) */
    } bulk_message_request_stream_config;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 8 bytes, wMessageID = DEV_STREAM_CONFIG */
    } bulk_message_response_stream_config;

    /**
    * @brief Bulk Get Time Message
    *
    * Retrieve the TM2 time, representred in nanoseconds since device initialization.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 6 bytes, wMessageID = DEV_GET_TIME */
    } bulk_message_request_get_time;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 16 or 8 bytes, wMessageID = DEV_GET_TIME */
        uint64_t llNanoseconds;              /**< Number of nanoseconds since device initialization                            */
    } bulk_message_response_get_time;


    /**
    * @brief Bulk Get and Clear Event Log Message
    *
    * Retrieves the device event log and clears it, used for debugging purposes.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 6 bytes, wMessageID = DEV_GET_AND_CLEAR_EVENT_LOG */
    } bulk_message_request_get_and_clear_event_log;

    typedef struct {
        uint8_t source;                        /**< Log level and source - Bits 0-3: Verbosity Level, Bits 4-6: Log index, Bit 7 - log source.                               */
        uint8_t timestamp[7];                  /**< Timestamp - Lowest 56bits of MV2 timestamp in nanoseconds.                                                               */
        uint16_t eventID;                      /**< Event ID - Bits 0-14: Event ID from an event enum which the host software shall be able to map to a descriptive message. */
                                               /**<          - Bit 15: 0 - the payload type (if exists) is according to bits 7-8 from the "Payload size" field.              */
                                               /**<          -       : 1 - the payload is a null-terminated C-string (bits 7-8 can be ignored).                              */
        uint8_t payloadSize;                   /**< Actual payload size - Bits 0-6: Actual payload size in bytes, 0 - 44 bytes.                                              */
                                               /**<                                 size = 0-12, the entry is a basic entry (32 bytes long).                                 */
                                               /**<                                 size = 13-44, the entry is an extended entry(64 bytes long).                             */
                                               /**<                       Bits 7-8: Payload data type: 0x0 - 32 bits integers array                                          */
                                               /**<                                                    0x1 - 32 bits floats array                                            */
                                               /**<                                                    0x2 - 8 bits byte array(binary data)                                  */
                                               /**<                                                    0x3 - Event specific defined structure                                */
        uint8_t threadID;                      /**< RTEMS thread ID.                                                                                                         */
        uint16_t moduleID;                     /**< Module ID.                                                                                                               */
        uint16_t lineNumber;                   /**< Line number in source code.                                                                                              */
        uint32_t functionSymbol;               /**< Address of the current function.                                                                                         */
        uint8_t payload[MAX_LOG_PAYLOAD_SIZE]; /**< Event specific payload - This field is 12 bytes long for basic entries, and 44 bytes long for extended entries.          */
    } device_event_log;

    typedef struct {
        bulk_message_response_header header;                   /**< Message response header: wMessageID = DEV_GET_AND_CLEAR_EVENT_LOG */
        device_event_log bEventLog[MAX_FW_LOG_BUFFER_ENTRIES]; /**< Event log buffers                                                 */
    } bulk_message_response_get_and_clear_event_log;


    /**
    * @brief Supported Raw Stream libtm Message
    *
    * Single supported raw stream that can be streamed out of the device.
    */
    typedef struct {
        uint8_t bSensorID;         /**< Bits 0-4: Type of sensor, supported values are: Color = 0, Depth = 1, IR = 2, Fisheye = 3, Gyro = 4, Accelerometer = 5 */
                                   /**< Bits 5-7: Sensor index - Zero based index of sensor with the same type within device. For example if the device supports two fisheye cameras, */
                                   /**<                          The first will use index 0 (bSensorID = 0x03) and the second will use index 1 (bSensorID = 0x23)                     */
        uint8_t bReserved;         /**< Reserved = 0                                                                                                                                  */
        uint16_t wWidth;           /**< Supported width (in pixels) of first stream, 0 for non-camera streams                                                                         */
        uint16_t wHeight;          /**< Supported height (in pixels) or first stream, 0 for non-camera streams                                                                        */
        uint8_t bPixelFormat;      /**< Pixel format of the stream, according to enum PixelFormat                                                                                     */
        union {                    /**<                                                                                                                                               */
            uint8_t bOutputMode;   /**< 0x0 - Send sensor outputs to the internal middlewares only, 0x1 - Send this sensor outputs also to the host over the USB interface.           */
            uint8_t bReserved2;    /**< Reserved and always 0. Sent from device to host.                                                                                              */
        };                         /**<                                                                                                                                               */
        uint16_t wStride;          /**< Length in bytes of each line in the image (including padding). 0 for non-camera streams.                                                      */
        uint16_t wFramesPerSecond; /**< Supported FPS for first stream to be enabled                                                                                                  */
    } supported_raw_stream_libtm_message;

    /**
    * @brief Bulk Get Supported Raw Streams Message
    *
    * Retrieves a list of supported raw streams that can be streamed out of the device.
    * Note that multiple entries indicated multiple stream configurations may be supported on the same camera.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 6 bytes, wMessageID = DEV_GET_SUPPORTED_RAW_STREAMS */
    } bulk_message_request_get_supported_raw_streams;

    typedef struct {
        bulk_message_response_header header;         /**< Message response header: wMessageID = DEV_GET_SUPPORTED_RAW_STREAMS */
        uint16_t wNumSupportedStreams;               /**< Number of supported streams in list below                           */
        uint16_t wReserved;                          /**< Reserved = 0                                                        */
        supported_raw_stream_libtm_message stream[]; /**< Supported stream info variable sized array                          */
    } bulk_message_response_get_supported_raw_streams;


    /**
    * @brief Bulk Raw Streams Control Message
    *
    * Enables streams to be directly streamed to the host.
    * Following a successful invocation of this command, and a call to the start command, the enabled streams shall be sent at the expected frame rate to the algorithms, and every streams whose bOutputMode bit was set to 1 shall be sent over the stream endpoint.
    * Other streams will be disabled, including those that were enabled by previous invocations.
    * To disable all image streaming, this command should be called with wNumEnabledStreams = 0, in this case, all fields after wNumEnabledStreams are dropped.
    * If there is a mismatch between wNumEnabledStreams and the size of the message, the firmware will return an INVALID_REQUEST_LEN error message.
    * If the profile is not supported, the firmware shall return an INVALID_PARAMETER error message (e.g. trying to open an unsupported configuration by a sensor, not valid FPS or requesting two profiles of the same sensor).
    * If the device is already streaming live from its sensors sending a DEV_ RAW_STREAMS_PLAYBACK_CONTROL shall fail with a DEVICE_BUSY error message.
    * If no DEV_RAW_STREAMS_CONTROL command was called before the host calls to DEV_START, the first stream configuration for each sensor as reported by DEV_GET_SUPPORTED_RAW_STREAMS shall be used as default.
    */
    typedef struct {
        bulk_message_request_header header;          /**< Message request header: wMessageID = DEV_RAW_STREAMS_CONTROL */
        uint16_t wNumEnabledStreams;                 /**< Number of enabled streams in list below                      */
        supported_raw_stream_libtm_message stream[]; /**< Supported stream info variable sized array                   */
    } bulk_message_request_raw_streams_control;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 8 bytes, wMessageID = DEV_RAW_STREAMS_CONTROL */
    } bulk_message_response_raw_streams_control;


    /**
    * @brief Bulk Raw Streams Playback Control Message
    *
    * Enables receiving streams from the host to be streamed to the MWs instead from the real sensors.
    * Following a successful invocation of this command, and a call to the start command, the enabled streams shall be sent at the expected frame rate to the algorithms, and every streams whose bOutputMode bit was set to 1 shall be sent over the stream endpoint (even in the playback case, effectively loopback-ing the samples back to the host).
    * Other streams will be disabled, including those that were enabled by previous invocations.
    * To disable all image streaming, this command should be called with wNumEnabledStreams = 0, in this case, all fields after wNumEnabledStreams are dropped.
    * If there is a mismatch between wNumEnabledStreams and the size of the message, the firmware will return an INVALID_REQUEST_LEN error message.
    * If the profile is not supported, the firmware shall return an INVALID_PARAMETER error message (e.g. trying to open an unsupported configuration by a sensor, not valid FPS or requesting two profiles of the same sensor).
    * If the device is already streaming live from its sensors sending a DEV_ RAW_STREAMS_PLAYBACK_CONTROL shall fail with a DEVICE_BUSY error message.
    */
    typedef struct {
        bulk_message_request_header header;          /**< Message request header: wMessageID = DEV_RAW_STREAMS_PLAYBACK_CONTROL */
        uint16_t wNumEnabledStreams;                 /**< Number of enabled streams in list below                               */
        supported_raw_stream_libtm_message stream[]; /**< Supported stream info variable sized array                            */
    } bulk_message_request_raw_streams_playback_control;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 8 bytes, wMessageID = DEV_RAW_STREAMS_PLAYBACK_CONTROL */
    } bulk_message_response_raw_streams_playback_control;


    /**
    * @brief Bulk Get Camera Intrinsics Message
    *
    * Retrieves the intrinsic parameters of an individual camera in the device.
    */
    typedef struct {
        uint32_t dwWidth;           /**< Width of the image in pixels                                                                                                    */
        uint32_t dwHeight;          /**< Height of the image in pixels                                                                                                   */
        float_t flPpx;              /**< Horizontal coordinate of the principal point of the image, as a pixel offset from the left edge                                 */
        float_t flPpy;              /**< Vertical coordinate of the principal point of the image, as a pixel offset from the top edge                                    */
        float_t flFx;               /**< Focal length of the image plane, as a multiple of pixel width                                                                   */
        float_t flFy;               /**< Focal length of the image plane, as a multiple of pixel Height                                                                  */
        uint32_t dwDistortionModel; /**< Distortion model of the image: F-THETA = 1, NONE (UNDISTORTED) = 3, KANNALA_BRANDT4 = 4 */
        float_t flCoeffs[5];        /**< Distortion coefficients                                                                                                         */
    } camera_intrinsics;

    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 7 bytes, wMessageID = DEV_GET_CAMERA_INTRINSICS                                                            */
        uint8_t bCameraID;                  /**< Bits 0-4: Type of requested camera intrinsics, supported values are: Color = 0, Depth = 1, IR = 2, Fisheye = 3                                */
                                            /**< Bits 5-7: Camera index - Zero based index of camera with the same type within device. For example if the device supports two fisheye cameras, */
                                            /**<                          The first will use index 0 (bCameraID = 0x03) and the second will use index 1 (bCameraID = 0x23)                     */
    } bulk_message_request_get_camera_intrinsics;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 56 or 8 bytes, wMessageID = DEV_GET_CAMERA_INTRINSICS */
        camera_intrinsics intrinsics;        /**< Intrinsics parameters of an individual camera in the device                               */
    } bulk_message_response_get_camera_intrinsics;


    /**
    * @brief Bulk Get Motion Module Intrinsics Message
    *
    * Retrieves the intrinsic parameters of an individual motion module in the device.
    */
    typedef struct {
        float_t flData[3][4];        /**< Scale matrix    */
        float_t flNoiseVariances[3]; /**< Noise variances */
        float_t flBiasVariances[3];  /**< Bias variances  */
    } motion_intrinsics;

    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 7 bytes, wMessageID = DEV_GET_MOTION_INTRINSICS                                                                   */
        uint8_t bMotionID;                  /**< Bits 0-4: Type of requested motion module, supported values are: Gyro = 4, Accelerometer = 5                                                         */
                                            /**< Bits 5-7: Motion module index - Zero based index of module with the same type within device. For example if the device supports two gyroscopes,      */
                                            /**<                                 The first gyroscope will use index 0 (bMotionID = 0x04) and the second gyroscope will use index 1 (bMotionID = 0x24) */
    } bulk_message_request_get_motion_intrinsics;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 80 or 8 bytes, wMessageID = DEV_GET_MOTION_INTRINSICS */
        motion_intrinsics intrinsics;        /**< Intrinsics parameters of an individual motion module in the device                        */
    } bulk_message_response_get_motion_intrinsics;


    /**
    * @brief Bulk Get Extrinsics Message
    *
    * Retrieves the extrinsic pose of on individual sensor in the device relative to another one.
    */
    typedef struct {
        float_t flRotation[9];      /**< Column-major 3x3 rotation matrix                 */
        float_t flTranslation[3];   /**< 3 element translation vector, in meters          */
        uint8_t bReferenceSensorID; /**< Reference sensor uses for extrinsics calculation */
    } sensor_extrinsics;

    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 8 bytes, wMessageID = DEV_GET_EXTRINSICS                                                                   */
        uint8_t bSensorID;                  /**< Bits 0-4: Type of requested sensor extrinsics, supported values are: Color = 0, Depth = 1, IR = 2, Fisheye = 3, Gyro = 4, Accelerometer = 5,  */
                                            /**< Bits 5-7: Camera index - Zero based index of sensor with the same type within device. For example if the device supports two fisheye cameras, */
                                            /**<                          The first will use index 0 (bSensorID = 0x03) and the second will use index 1 (bSensorID = 0x23)                     */
    } bulk_message_request_get_extrinsics;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 56 or 8 bytes, wMessageID = DEV_GET_EXTRINSICS */
        sensor_extrinsics extrinsics;        /**< Extrinsics pose of an individual sensor in the device relative to another one      */
    } bulk_message_response_get_extrinsics;

    typedef struct {
        bulk_message_request_header header;
        uint8_t bSensorID;
        sensor_extrinsics extrinsics;
    } bulk_message_request_set_extrinsics;

    typedef struct {
        bulk_message_response_header header;
    } bulk_message_response_set_extrinsics;

    /**
    * @brief Bulk Set Camera Intrinsics Message
    *
    * Sets the intrinsic parameters of an individual camera in the device.
    * These parameters shall not be written to the EEPROM. They shall be used in the next streaming session only and shall be discarded once a "stop" command is called. 
    * This command is available only when the middlewares (6DoF / Occupancy map) are disabled.
    * If any middleware is enabled, this command will return an UNSUPPORTED error.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 56 bytes, wMessageID = DEV_SET_CAMERA_INTRINSICS                                                           */
        uint8_t bCameraID;                  /**< Bits 0-4: Type of requested camera intrinsics, supported values are: Color = 0, Depth = 1, IR = 2, Fisheye = 3                                */
                                            /**< Bits 5-7: Camera index - Zero based index of camera with the same type within device. For example if the device supports two fisheye cameras, */
                                            /**<                          The first will use index 0 (bCameraID = 0x03) and the second will use index 1 (bCameraID = 0x23)                     */
        uint8_t bReserved;                  /**< Reserved = 0                                                                                                                                  */
        camera_intrinsics intrinsics;       /**< Intrinsics parameters of an individual camera in the device                                                                                   */
    } bulk_message_request_set_camera_intrinsics;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 8 bytes, wMessageID = DEV_SET_CAMERA_INTRINSICS */
    } bulk_message_response_set_camera_intrinsics;


    /**
    * @brief Bulk Set Motion Module Intrinsics Message
    *
    * Sets the intrinsic parameters of an individual motion module in the device.
    * These parameters shall not be written to the EEPROM. They shall be used in the next streaming session only and shall be discarded once a "stop" command is called. 
    * This command is available only when the middlewares (6DoF / Occupancy map) are disabled.
    * If any middleware is enabled, this command will return an UNSUPPORTED error.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 80 bytes, wMessageID = DEV_SET_MOTION_INTRINSICS                                                                  */
        uint8_t bMotionID;                  /**< Bits 0-4: Type of requested motion module, supported values are: Gyro = 4, Accelerometer = 5                                                         */
                                            /**< Bits 5-7: Motion module index - Zero based index of module with the same type within device. For example if the device supports two gyroscopes,      */
                                            /**<                                 The first gyroscope will use index 0 (bMotionID = 0x04) and the second gyroscope will use index 1 (bMotionID = 0x24) */
        uint8_t bReserved;                  /**< Reserved = 0                                                                                                                                         */
        motion_intrinsics intrinsics;       /**< Intrinsics parameters of an individual motion module in the device                                                                                   */
    } bulk_message_request_set_motion_intrinsics;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 8 bytes, wMessageID = DEV_SET_MOTION_INTRINSICS */
    } bulk_message_response_set_motion_intrinsics;


    /**
    * @brief Bulk Log Control Message
    *
    * Controls the logging parameters, such as verbosity and log mode.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 8 bytes, wMessageID = DEV_LOG_CONTROL                                   */
        uint8_t bVerbosity;                 /**< Verbosity level                                                                                            */
        uint8_t bLogMode;                   /**< 0x00 - No rollover, logging will be paused after log is filled. Until cleared, first events will be stored */
                                            /**< 0x01 - Rollover mode                                                                                       */
    } bulk_message_request_log_control;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 8 bytes, wMessageID = DEV_LOG_CONTROL */
    } bulk_message_response_log_control;


    /**
    * @brief Bulk Reset Message
    *
    * Resets the device. The command supports a firmware reset with and without reload of the firmware.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 8 bytes, wMessageID = DEV_RESET                                                                                    */
        uint8_t bVerbosity;                 /**< Reset action: 0 - reset the VPU device and boot from the firmware already loaded in memory                                                            */
                                            /**<               1 - reset the VPU device to a "pre-load" state, i.e. firmware is loaded from ROM and awaits new firmware to be pushed from host via USB */
        uint8_t bReserved;                  /**< Reserved = 0                                                                                                                                          */
    } bulk_message_request_reset;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 8 bytes, wMessageID = DEV_RESET */
    } bulk_message_response_reset;


    /**
    * @brief Bulk Read EEPROM Message
    *
    * Read data from the EEPROM memory.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 10 bytes, wMessageID = DEV_READ_EEPROM */
        uint16_t wOffset;                   /**< EEPROM offset address to read from                                        */
        uint16_t wSize;                     /**< The requested size in bytes of the data to read                           */
    } bulk_message_request_read_eeprom;

    typedef struct {
        bulk_message_response_header header;   /**< Message response header: dwLength = 10+wSize bytes, wMessageID = DEV_READ_EEPROM */
        uint16_t wSize;                        /**< Size in bytes of the read data                                                   */
        uint8_t bData[MAX_EEPROM_BUFFER_SIZE]; /**< The requested EEPROM data array                                                  */
    } bulk_message_response_read_eeprom;


    /**
    * @brief Bulk Write EEPROM data Message
    *
    * Write data to the EEPROM memory.
    */
    typedef struct {
        bulk_message_request_header header;    /**< Message request header: dwLength = 10+wSize bytes, wMessageID = DEV_WRITE_EEPROM */
        uint16_t wOffset;                      /**< EEPROM offset address to write to                                                */
        uint16_t wSize;                        /**< The size in bytes of the data to write to                                        */
        uint8_t bData[MAX_EEPROM_BUFFER_SIZE]; /**< The requested EEPROM data array                                                  */
    } bulk_message_request_write_eeprom;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 10+wSize bytes, wMessageID = DEV_WRITE_EEPROM */
        uint16_t wSize;                      /**< The size of bytes written                                                         */
    } bulk_message_response_write_eeprom;


    /**
    * @brief Bulk Start Message
    *
    * Starts all the enabled streams and middlewares, using either default configuration or the parameters from calls to any of the configuration commands.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 6 bytes, wMessageID = DEV_START */
    } bulk_message_request_start;

    typedef struct {
        bulk_message_response_header header; /**< Message request header: dwLength = 8 bytes, wMessageID = DEV_START */
    } bulk_message_response_start;


    /**
    * @brief Bulk Stop Message
    *
    * Stops all streams and middlewares running on the device.
    * After a "stop" command the device shall suspend to a low-power state. It shall also reset all saved configuration parameters, 
    * Such as the next call to start shall return to use all the default parameters (unless new calls to any of the configuration commands are made).
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 6 bytes, wMessageID = DEV_STOP */
    } bulk_message_request_stop;

    typedef struct {
        bulk_message_response_header header; /**< Message request header: dwLength = 8 bytes, wMessageID = DEV_STOP */
    } bulk_message_response_stop;


    /**
    * @brief Bulk Get Pose Message
    *
    * Returns the latest pose of the camera relative to its initial position,
    * If relocalization data was set, this pose is relative to the relocalization database.
    */
    typedef struct {
        float_t flX;                  /**< X value of translation, in meters (relative to initial position)                              */
        float_t flY;                  /**< Y value of translation, in meters (relative to initial position)                              */
        float_t flZ;                  /**< Z value of translation, in meters (relative to initial position)                              */
        float_t flQi;                 /**< Qi component of rotation as represented in quaternion rotation (relative to initial position) */
        float_t flQj;                 /**< Qj component of rotation as represented in quaternion rotation (relative to initial position) */
        float_t flQk;                 /**< Qk component of rotation as represented in quaternion rotation (relative to initial position) */
        float_t flQr;                 /**< Qr component of rotation as represented in quaternion rotation (relative to initial position) */
        float_t flVx;                 /**< X value of velocity, in meter/sec                                                             */
        float_t flVy;                 /**< Y value of velocity, in meter/sec                                                             */
        float_t flVz;                 /**< Z value of velocity, in meter/sec                                                             */
        float_t flVAX;                /**< X value of angular velocity, in RAD/sec                                                       */
        float_t flVAY;                /**< Y value of angular velocity, in RAD/sec                                                       */
        float_t flVAZ;                /**< Z value of angular velocity, in RAD/sec                                                       */
        float_t flAx;                 /**< X value of acceleration, in meter/sec^2                                                       */
        float_t flAy;                 /**< Y value of acceleration, in meter/sec^2                                                       */
        float_t flAz;                 /**< Z value of acceleration, in meter/sec^2                                                       */
        float_t flAAX;                /**< X value of angular acceleration, in RAD/sec^2                                                 */
        float_t flAAY;                /**< Y value of angular acceleration, in RAD/sec^2                                                 */
        float_t flAAZ;                /**< Z value of angular acceleration, in RAD/sec^2                                                 */
        uint64_t llNanoseconds;       /**< Timestamp of pose, measured in nanoseconds since device system initialization                 */
        uint32_t dwTrackerConfidence; /**< pose data confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High                        */
        uint32_t dwMapperConfidence;  /**< Bits 0-1: 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High, Bits 2-31: Reserved              */
        uint32_t dwTrackerState;      /**< tracker state 0x0 - Inactive, 0x3 Active 3DOF, 0x4 Active 6DOF, 0x7 Inertial only 3DOF        */
    } pose_data;

    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 7 bytes, wMessageID = DEV_GET_POSE          */
        uint8_t bIndex;                     /**< Index of HMD - 0x0 = HMD */
    } bulk_message_request_get_pose;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 100 or 8 bytes, wMessageID = DEV_GET_POSE */
        pose_data pose;                      /**< Short low-latency data (namely 6DoF pose data)                                */
    } bulk_message_response_get_pose;


    /**
    * @brief Bulk Set Exposure Control Message
    * Enable/disable the auto-exposure management of the different video streams, and configure the powerline frequency rate.
    * Calling this message is only supported before the streams are started.
    * The default values for video streams 0 and 1 shall be auto-exposure enable with 50Hz flicker rate.
    * The firmware only supports the following:
    * 1. Disabling auto-exposure for all streams (bVideoStreamsMask==0)
    * 2. Enabling auto-exposure for both video stream 0 and 1 together (bVideoStreamsMask ==0x3).
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 8 bytes, wMessageID = DEV_EXPOSURE_MODE_CONTROL                            */
        uint8_t bVideoStreamsMask;          /**< Bitmask of the streams to apply the configuration to. bit X -> stream X: 0 - disable, 1 - enable              */
        uint8_t bAntiFlickerMode;           /**< Anti Flicker Mode: 0 - disable (e.g. for outside use), 1 - 50Hz, 2 - 60Hz, 3 - Auto (currently not supported) */
    } bulk_message_request_set_exposure_mode_control;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 8 bytes, wMessageID = DEV_EXPOSURE_MODE_CONTROL */
    } bulk_message_response_set_exposure_mode_control;


    /**
    * @brief Bulk Set Exposure Message
    *
    * Sets manual values for the video streams integration time and gain.
    * The command supports setting these values to all required streams with a single call.
    * The command shall return UNSUPPORTED if the stream is configure to use auto-exposure, and INVALID_PARAMETER if any of the values are out of range.
    */
    typedef struct {
        uint8_t bCameraID;          /**< Bits 0-4: Type of requested camera, supported values are: Color = 0, Depth = 1, IR = 2, Fisheye = 3 */
                                    /**< Bits 5-7: Camera index - Zero based index of camera with the same type within device                */
        uint8_t  bReserved[3];      /**< Reserved = 0                                                                                        */
        uint32_t dwIntegrationTime; /**< Integration time in micro-seconds */
        float_t fGain;              /**< Digital gain                                                                                        */
    } stream_exposure;

    typedef struct {
        bulk_message_request_header header;        /**< Message request header: dwLength = (8 + bNumberOfStreams*8 bytes), wMessageID = DEV_SET_EXPOSURE */
        uint8_t bNumOfVideoStreams;                /**< Number of video streams to configure: Bits 0-2: stream index, Bits 3-7: reserved                 */
        uint8_t bReserved;                         /**< Reserved = 0                                                                                     */
        stream_exposure stream[MAX_VIDEO_STREAMS]; /**< Stream exposure data variable sized array, according to wNumberOfStreams                         */
    } bulk_message_request_set_exposure;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 8 bytes, wMessageID = DEV_SET_EXPOSURE */
    } bulk_message_response_set_exposure;


    /**
    * @brief Bulk Get Temperature Message
    *
    * Returns temperature and temperature threshold from all temperature sensors (VPU, IMU, BLE)
    */
    typedef struct {
        uint32_t dwIndex;     /**< Temperature sensor index: 0x0 - VPU, 0x1 - IMU, 0x2 - BLE */
        float_t fTemperature; /**< Sensor's temperature in Celius                            */
        float_t fThreshold;   /**< The sensor's threshold temperature                        */
    } sensor_temperature;

    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 6 bytes, wMessageID = DEV_GET_TEMPERATURE */
    } bulk_message_request_get_temperature;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLendth = (12 + 12 * dwCount) bytes, wMessageID = DEV_GET_TEMPERATURE */
        uint32_t dwCount;                    /**< Number or temperature sensors                                                                   */
        sensor_temperature temperature[];    /**< temperature variable sized array                                                                */
    } bulk_message_response_get_temperature;


    /**
    * @brief Bulk Set Temperature Threshold Message
    *
    * Set temperature threshold to requested sensors (VPU, IMU, BLE)
    * The firmware shall actively monitor the temperature of the underlying sensors.
    * When a component temperature is 10% close to its defined threshold, the firmware shall send a DEV_ERROR message with a TEMPERATURE_WARNING status to the host, 
    * When the temperature reach the threshold, the firmware shall stop all running algorithms and sensors (as if DEV_STOP was called), and send a TEMPERATURE_SHUTDOWN status to the user.
    */
    typedef struct {
        uint32_t dwIndex;     /**< Temperature sensor index: 0x0 - VPU, 0x1 - IMU, 0x2 - BLE */
        float_t fThreshold;   /**< The new sensor's threshold temperature                    */
    } sensor_set_temperature;

    typedef struct {
        bulk_message_request_header header;   /**< Message request header: dwLength = (12 + 8 * dwCount) bytes, wMessageID = DEV_SET_TEMPERATURE_THRESHOLD */
        uint16_t wForceToken;                 /**< Token to force higher temperature threshold (80-100)                                                    */
        uint32_t dwCount;                     /**< Number or temperature sensors                                                                           */
        sensor_set_temperature temperature[]; /**< temperature variable sized array                                                                        */
    } bulk_message_request_set_temperature_threshold;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLendth = 8, wMessageID = DEV_SET_TEMPERATURE_THRESHOLD */
    } bulk_message_response_set_temperature_threshold;

    /**
    * @brief Bulk GPIO control Message
    *
    * Enable manufacturing tools to directly control the following GPIOs - 74, 75, 76 & 77.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 7 bytes, wMessageID = DEV_GPIO_CONTROL */
        uint8_t bGpioControl;               /**< Reserved = 0                                                              */
    } bulk_message_request_gpio_control;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLendth = 8, wMessageID = DEV_GPIO_CONTROL */
    } bulk_message_response_gpio_control;


    /**
    * @brief Bulk configuration Lock Message
    *
    * Write-protect the manufactoring configuration tables from future changes.
    * The locking is done in software by the firmware managing a lock bits in each configuration table metadata.
    * The lock can be applied permanently, meaning it cannot be latter un-locked.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 12 bytes, wMessageID = DEV_LOCK_CONFIGURATION                                             */
        uint16_t wTableType;                /**< Table ID to lock                                                                                                             */
        uint32_t dwLock;                    /**< 0x0 - Unlock, 0x1 - Lock                                                                                                     */
                                            /**< 0xDEAD10CC - the configuration data shall be permanently locked. *** WARNING *** this might be an irreversible action.       */
                                            /**< After calling this the write protection the settings cannot be modified and therefore the memory write protection is frozen. */
    } bulk_message_request_lock_configuration;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLendth = 8, wMessageID = DEV_LOCK_CONFIGURATION */
    } bulk_message_response_lock_configuration;


    /**
    * @brief Bulk eeprom Lock Message
    *
    * Write-protect the manufactoring configuration tables from future changes.
    * Tables that will be locked: 0x8 - ODM, 0x6 - OEM, 0x10 - module info, 0x11 - internal data, 0x12 - HVS
    * The locking is done in hardware by locking the upper quarter of the EEPROM memory
    * The lock can be applied permanently, meaning it cannot be latter un-locked.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 12 bytes, wMessageID = DEV_LOCK_CONFIGURATION or DEV_LOCK_EEPROM                          */
        uint16_t wReserved;                 /**< Reserved = 0                                                                                                                 */
        uint32_t dwLock;                    /**< 0x0 - Unlock, 0x1 - Lock                                                                                                     */
                                            /**< 0xDEAD10CC - the configuration data shall be permanently locked. *** WARNING *** this might be an irreversible action.       */
                                            /**< After calling this the write protection the settings cannot be modified and therefore the memory write protection is frozen. */
    } bulk_message_request_lock_eeprom;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLendth = 8, wMessageID = DEV_LOCK_CONFIGURATION */
    } bulk_message_response_lock_eeprom;


    /**
    * @brief Bulk read configuration Message
    *
    * Reads a configuration table from the device memory.
    * The device shall return UNSUPPORTED if it does not recognize the requested TableType.
    * The device shall return TABLE_NOT_EXIST if it recognize the table type but no such table exists yet in the EEPROM.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 8, wMessageID = DEV_READ_CONFIGURATION */
        uint16_t wTableId;                  /**< The ID of the requested configuration table                               */
    } bulk_message_request_read_configuration;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLendth = 8 bytes +  size of table, wMessageID = DEV_READ_CONFIGURATION    */
        uint8_t bTable[MAX_TABLE_SIZE];      /**< The requested configuration table, starting with the "standard" table header.                        */
    } bulk_message_response_read_configuration;
    

    /**
    * @brief Bulk write configuration Message
    *
    * Writes a configuration table to the device's EEPROM memory.
    * This command shall only be supported while the device is stopped, otherwise it shall return DEVICE_BUSY.
    * The device shall return UNSUPPORTED if it does not recognize the requested TableType.
    * The device shall return TABLE_LOCKED if the configuration table is write protected and cannot be overridden.
    * The new data shall be available immediately after completion without requiring a device reset, both to any firmware code or to an external client through a "read configuration" command.
    * All internal data object in the firmware memory that were already initialized from some EEPROM data or previous "write configuration" command shall be invalidated and refreshed to the new written data.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 8 + size of table bytes, wMessageID = DEV_WRITE_CONFIGURATION */
        uint16_t wTableId;                  /**< The ID of the requested configuration table                                                      */
        uint8_t bTable[MAX_TABLE_SIZE];     /**< The requested configuration table, starting with the "standard" table header.                    */
    } bulk_message_request_write_configuration;

    typedef struct {
        bulk_message_response_header header;        /**< Message response header: dwLendth = 8, wMessageID = DEV_WRITE_CONFIGURATION    */
    } bulk_message_response_write_configuration;


    /**
    * @brief Bulk reset configuration Message
    *
    * Delete a configuration table from the internal EEPROM storage.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 8, wMessageID = DEV_RESET_CONFIGURATION */
        uint16_t wTableId;                  /**< The ID of the requested configuration table                                */
    } bulk_message_request_reset_configuration;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLendth = 8, wMessageID = DEV_RESET_CONFIGURATION    */
    } bulk_message_response_reset_configuration;


    /**
    * @brief Bulk timeout configuration Message
    *
    * Host timeout - When the firmware is actively streaming or tracking, it shall monitor the host status to identify the cases where the host app crashed or stopped responding.
    * The firmware shall use the host USB reads as the host's "heartbeat", so if outgoing packets were continuously dropped for over 1 second the firmware shall stop sending packets and stop all operations, 
    * as if a DEV_STOP command was sent, but without sending the DEV_STOPPED status event packet.
    * The timeout shall be configurable or disabled through this DEV_TIMEOUT_CONFIGURATION command.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 8, wMessageID = DEV_TIMEOUT_CONFIGURATION */
        uint16_t wTimeoutInMillis;          /**< The commands TX timeout in milliseconds to use. Zero to disable timeout.     */
    } bulk_message_request_timeout_configuration;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLendth = 8, wMessageID = DEV_TIMEOUT_CONFIGURATION    */
    } bulk_message_response_timeout_configuration;

    
    /**
    * @brief Bulk enable low power Message
    *
    * enable or disable low power mode of TM2 device
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 8, wMessageID = DEV_SET_LOW_POWER_MODE    */
        uint8_t bEnabled;                   /**< 1 to enable low power mode, 0 to disable                                   */
        uint8_t bReserved;                  /**< Reserved = 0                                                               */
    } bulk_message_request_set_low_power_mode;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLendth = 8, wMessageID = DEV_SET_LOW_POWER_MODE  */
    } bulk_message_response_set_low_power_mode;

    /**
    * @brief Bulk Get Localization Data Message
    *
    * Trigger the FW to send interrupt_message_get_localization_data_stream with the localization data as created during a 6DoF session
    * The response to this message is generated and streamed by the underlying firmware algorithm in run-time, so the total size of the data cannot be known in advance. 
    * The entire data will be streams in "chunks" sent using SLAM_GET_LOCALIZATION_DATA_STREAM messages over the stream endpoint.
    * The firmware shall use a MORE_DATA_AVAILABLE status to indicate there are more data to send. The last chunk (possibly even a zero-size chunk) shall be marked with a SUCCESS status code.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 7 bytes, wMessageID = SLAM_GET_LOCALIZATION_DATA */
        uint8_t bReserved;                  /**< Reserved = 0                                                                        */
    } bulk_message_request_get_localization_data;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: wMessageID = SLAM_GET_LOCALIZATION_DATA */
    } bulk_message_response_get_localization_data;

    /**
    * @brief Bulk Set Localization Data Stream Message
    *
    * Sets the localization data to be used to localize the 6DoF reports.
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 10 bytes + bPayload size, wMessageID = according to large message type */
        uint16_t wStatus;                   /**< SUCCESS: indicate last chunk, MORE_DATA_AVAILABLE: any other chunk                                         */
        uint16_t wIndex;                    /**< A running counter starting at 0 identifying the chunk index in a single data transaction                   */
        uint8_t bPayload[];                 /**< payload data variable sized array. Data format is algorithm specific and opaque to the USB and host stack */
    } bulk_message_large_stream;


    /**
    * @brief Bulk Set Static Node Message
    *
    * Set relative position of a static node
    */
    typedef struct {
        float_t flX;  /**< X value of translation, in meters (in the coordinate system of the tracker relative to the current position)                              */
        float_t flY;  /**< Y value of translation, in meters (in the coordinate system of the tracker relative to the current position)                              */
        float_t flZ;  /**< Z value of translation, in meters (in the coordinate system of the tracker relative to the current position)                              */
        float_t flQi; /**< Qi component of rotation as represented in quaternion rotation (in the coordinate system of the tracker relative to the current position) */
        float_t flQj; /**< Qj component of rotation as represented in quaternion rotation (in the coordinate system of the tracker relative to the current position) */
        float_t flQk; /**< Qk component of rotation as represented in quaternion rotation (in the coordinate system of the tracker relative to the current position) */
        float_t flQr; /**< Qr component of rotation as represented in quaternion rotation (in the coordinate system of the tracker relative to the current position) */
    } static_node_data;

    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 164 bytes, wMessageID = SLAM_SET_STATIC_NODE                      */
        uint16_t wReserved;                 /**< Reserved = 0                                                                                         */
        uint8_t bGuid[MAX_GUID_LENGTH];     /**< Null-terminated C-string, with max length 127 bytes plus one byte for the terminating null character */
        static_node_data data;              /**< Static node data                                                                                     */
    } bulk_message_request_set_static_node;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: wMessageID = SLAM_SET_STATIC_NODE */
    } bulk_message_response_set_static_node;

    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 164 bytes, wMessageID = SLAM_REMOVE_STATIC_NODE                      */
        uint16_t wReserved;                 /**< Reserved = 0                                                                                         */
        uint8_t bGuid[MAX_GUID_LENGTH];     /**< Null-terminated C-string, with max length 127 bytes plus one byte for the terminating null character */
    } bulk_message_request_remove_static_node;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: wMessageID = SLAM_REMOVE_STATIC_NODE */
    } bulk_message_response_remove_static_node;

    /**
    * @brief Bulk Get Static Node Message
    *
    * Get relative position of a static node
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 136 bytes, wMessageID = SLAM_GET_STATIC_NODE                      */
        uint16_t wReserved;                 /**< Reserved = 0                                                                                         */
        uint8_t bGuid[MAX_GUID_LENGTH];     /**< Null-terminated C-string, with max length 127 bytes plus one byte for the terminating null character */
    } bulk_message_request_get_static_node;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 36 byte, wMessageID = SLAM_GET_STATIC_NODE */
        static_node_data data;               /**< Static node data                                                               */
    } bulk_message_response_get_static_node;

    /**
    * @brief Bulk SLAM override calibration Message
    *
    * Append current SLAM calibration
    */
    typedef struct {
        bulk_message_request_header header;                           /**< Message request header: dwLength = 10006 bytes, wMessageID = SLAM_APPEND_CALIBRATION */
        uint8_t calibration_append_string[MAX_SLAM_CALIBRATION_SIZE]; /**< Calibration string                                                                   */
    } bulk_message_request_slam_append_calibration;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 8 byte, wMessageID = SLAM_APPEND_CALIBRATION */
    } bulk_message_response_slam_append_calibration;


    /**
    * @brief Bulk SLAM calibration Message
    *
    * Override current SLAM calibration
    */
    typedef struct {
        bulk_message_request_header header;                    /**< Message request header: dwLength = 10006 bytes, wMessageID = SLAM_CALIBRATION */
        uint8_t calibration_string[MAX_SLAM_CALIBRATION_SIZE]; /**< Calibration string                                                            */
    } bulk_message_request_slam_calibration;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 8 byte, wMessageID = SLAM_CALIBRATION */
    } bulk_message_response_slam_calibration;

    /**
    * @brief Bulk Set 6DoF Interrupt Rate Message
    *
    * Sets the rate of 6DoF interrupts. This is typically related to the host rendering rate.
    */
    typedef struct {
        uint8_t bInterruptRate; /**< Rate of 6DoF interrupts. The following values are supported:                                 */
                                /**< 0x0 - no interrupts                                                                          */
                                /**< 0x1 - interrupts on every fisheye camera update (30 interrupts per second for TM2)           */
                                /**< 0x2 - interrupts on every IMU update (400-500 interrupts per second for TM2) - default value */
    } sixdof_interrupt_rate_libtm_message;

    typedef struct {
        bulk_message_request_header header;          /**< Message request header: dwLength = 7 bytes, wMessageID = SLAM_SET_6DOF_INTERRUPT_RATE */
        sixdof_interrupt_rate_libtm_message message; /**< Rate of 6DoF interrupts                                                               */
    } bulk_message_request_set_6dof_interrupt_rate;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 8 bytes, wMessageID = SLAM_SET_6DOF_INTERRUPT_RATE */
    } bulk_message_response_set_6dof_interrupt_rate;


    /**
    * @brief Bulk 6DoF Control Message
    *
    * Enables / disables 6DoF calculation.
    * If no SLAM_6DOF_CONTROL command was called before the host calls to DEV_START, the default value used shall be "Disable 6DoF".
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 9 bytes, wMessageID = SLAM_6DOF_CONTROL                       */
        uint8_t bEnable;                    /**< 0x00 - Disable 6DoF, 0x01 - Enable 6DoF                                                          */
        uint8_t bMode;                      /**< 0x00 - Normal Mode, 0x01 - Fast Playback, 0x02 - Mapping Enabled , 0x04 - Relocalization Enabled */
    } bulk_message_request_6dof_control;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 8 bytes, wMessageID = SLAM_6DOF_CONTROL */
    } bulk_message_response_6dof_control;


    /**
    * @brief Bulk Occupancy Map Control Message
    *
    * Enables/disables occupancy map calculation. Occupancy map calculation is based on 6DoF calculation,
    * So it cannot be enabled when 6DoF is disabled, and an UNSUPPORTED error will be returned.
    * If no SLAM_6DOF_CONTROL command was called before the host calls to DEV_START, the default value used shall be "Disable occupancy map".
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 7 bytes, wMessageID = SLAM_OCCUPANCY_MAP_CONTROL */
        uint8_t bEnable;                    /**< 0x00 - Disable occupancy map, 0x01 - Enable occupancy map                           */
    } bulk_message_request_occupancy_map_control;

    typedef struct {
        bulk_message_response_header header; /**< Message response header: dwLength = 8 bytes, wMessageID = SLAM_OCCUPANCY_MAP_CONTROL */
    } bulk_message_response_occupancy_map_control;


    /**
    * @brief Stream Endpoint Protocol
    *
    * The stream endpoint is a bulk IN / OUT endpoint, used to stream out image streams as raw streams, as enabled in the "Enable Raw Streams" command,
    * Or stream in image and IMU streams, as enabled in the "Enable Playback Streams" command. (IMU outputs are sent over the interrupt endpoint).
    * Streaming streams both out and in at the same time shall be supported (effectively loopback - ing the input streams).
    * The host reads from this endpoint to get new streaming data, and write new samples to playback them.
    * Every sample is made from the common fields from below, with a variable specific metadata and data part (starting at byte 24 below).
    */


    /**
    * @brief Bulk raw stream header
    */
    typedef struct {
        bulk_message_request_header header; /**< Message request header: dwLength = 28 + dwMetadataLength + dwFrameLength bytes, wMessageID = DEV_SAMPLE                                       */
        uint8_t bSensorID;                  /**< Bits 0-4: Type of sensor, supported values are: Color = 0, Depth = 1, IR = 2, Fisheye = 3, Gyro = 4, Accelerometer = 5 */
                                            /**< Bits 5-7: Sensor index - Zero based index of sensor with the same type within device. For example if the device supports two fisheye cameras, */
                                            /**<                          The first will use index 0 (bSensorID = 0x03) and the second will use index 1 (bSensorID = 0x23)                     */
        uint8_t bReserved;                  /**< Reserved = 0                                                                                                                                  */
        uint64_t llNanoseconds;             /**< Frame integration timestamp, as measured in nanoseconds since device initialization                                                           */
        uint64_t llArrivalNanoseconds;      /**< Frame arrival timestamp, as measured in nanoseconds since device initialization                                                               */
        uint32_t dwFrameId;                 /**< A running index of frames from every unique sensor. Starting from 0.                                                                          */
    } bulk_message_raw_stream_header;


    /**
    * @brief Bulk raw video stream metadata
    */
    typedef struct
    {
        uint32_t dwMetadataLength; /**< Metadata length in bytes (8 bytes)                                       */
        uint32_t dwExposuretime;   /**< Exposure time of this frame in microseconds                              */
        float_t fGain;             /**< Gain multiplier of this frame                                            */
        uint32_t dwFrameLength;    /**< Length of frame below, in bytes, shall be equal to Stride X Height X BPP */
        uint8_t bFrameData[];      /**< Frame data variable sized array                                          */
    } bulk_message_video_stream_metadata;


    /**
    * @brief Bulk raw video stream message
    *
    * Specific frame metadata and data for sensor IDs, color, depth, IR, Fisheye
    */
    typedef struct
    {
        bulk_message_raw_stream_header rawStreamHeader;
        bulk_message_video_stream_metadata metadata;
    } bulk_message_video_stream;


    /**
    * @brief Bulk raw accelerometer stream metadata
    */
    typedef struct
    {
        uint32_t dwMetadataLength; /**< Metadata length in bytes (4 bytes)      */
        float_t flTemperature;     /**< Accelerometer temperature               */
        uint32_t dwFrameLength;    /**< Length of frame below (12 bytes)        */
        float_t flAx;              /**< X value of acceleration, in meter/sec^2 */
        float_t flAy;              /**< Y value of acceleration, in meter/sec^2 */
        float_t flAz;              /**< Z value of acceleration, in meter/sec^2 */
    } bulk_message_accelerometer_stream_metadata;

    /**
    * @brief Bulk raw accelerometer stream message
    *
    * Specific frame metadata and data for sensor IDs - accelerometer
    */
    typedef struct
    {
        bulk_message_raw_stream_header rawStreamHeader;
        bulk_message_accelerometer_stream_metadata metadata;
    } bulk_message_accelerometer_stream;


    /**
    * @brief Bulk raw gyro stream metadata
    */
    typedef struct
    {
        uint32_t dwMetadataLength; /**< Metadata length in bytes (4 bytes) */
        float_t flTemperature;     /**< Gyro temperature                   */
        uint32_t dwFrameLength;    /**< Length of frame below (12 bytes)   */
        float_t flGx;              /**< X value of gyro, in radians/sec    */
        float_t flGy;              /**< Y value of gyro, in radians/sec    */
        float_t flGz;              /**< Z value of gyro, in radians/sec    */
    } bulk_message_gyro_stream_metadata;

    /**
    * @brief Bulk raw gyro stream message
    *
    * Specific frame metadata and data for sensor IDs - gyro
    */
    typedef struct
    {
        bulk_message_raw_stream_header rawStreamHeader;
        bulk_message_gyro_stream_metadata metadata;
    } bulk_message_gyro_stream;

    /**
    * @brief Bulk raw velocimeter stream metadata
    */
    typedef struct
    {
        uint32_t dwMetadataLength; /**< Metadata length in bytes (4 bytes)     */
        float_t flTemperature;     /**< velocimeter temperature                */
        uint32_t dwFrameLength;    /**< Length of frame below (12 bytes)       */
        float_t flVx;              /**< X value of velocimeter, in meters/sec */
        float_t flVy;              /**< Y value of velocimeter, in meters/sec */
        float_t flVz;              /**< Z value of velocimeter, in meters/sec */
    } bulk_message_velocimeter_stream_metadata;

    /**
    * @brief Bulk raw velocimeter stream message
    *
    * Specific frame metadata and data for sensor IDs - velocimeter
    */
    typedef struct
    {
        bulk_message_raw_stream_header rawStreamHeader;
        bulk_message_velocimeter_stream_metadata metadata;
    } bulk_message_velocimeter_stream;

    /**
    * @brief Interrupt Endpoint Protocol
    *
    * The interrupt endpoint is used for short low-latency data (namely 6DoF pose data).
    * The rate of the interrupts is determined by using the "Set 6DoF Interrupt Data" command.
    */

    /**
    * @brief Interrupt message header struct
    *
    * Start of all Interrupt messages.
    */
    typedef struct {
        uint32_t dwLength;    /**< Message length in bytes */
        uint16_t wMessageID;  /**< ID of message           */
    } interrupt_message_header;


    /**
    * @brief Interrupt error message
    *
    * A general error message
    */
    typedef struct {
        interrupt_message_header header; /**< Interrupt message header: dwLength = 8 bytes, wMessageID = DEV_ERROR */
        uint16_t wStatus;                /**< Error code                                                           */
    } interrupt_message_general_error;


    /**
    * @brief Interrupt status message
    *
    * A status message
    */
    typedef struct {
        interrupt_message_header header; /**< Interrupt message header: dwLength = 8 bytes, wMessageID = DEV_STATUS */
        uint16_t wStatus;                /**< Status of request (MESSAGE_STATUS)                                   */
    } interrupt_message_status;

    /**
    * @brief Interrupt get pose message
    *
    * returns the latest pose of the camera relative to its initial position
    * If relocalization data was set, this pose is relative to the relocalization database
    * The rate of the interrupts is determined by using the SLAM_SET_6DOF_INTERRUPT_RATE command.
    */
    typedef struct {
        interrupt_message_header header; /**< Interrupt message header: dwLength = 92 bytes, wMessageID = DEV_GET_POSE       */
        uint8_t bIndex;                  /**< Index of HMD - 0x0 = HMD */
        uint8_t wReserved;               /**< Reserved = 0                                                                   */
        pose_data pose;                  /**< Short low-latency data (namely 6DoF pose data)                                 */
    } interrupt_message_get_pose;


    /**
    * @brief Interrupt raw stream header
    */
    typedef struct {
        interrupt_message_header header; /**< Message request header: dwLength = 28 + dwMetadataLength + dwFrameLength bytes, wMessageID = DEV_SAMPLE                                       */
        uint8_t bSensorID;               /**< Bits 0-4: Type of sensor, supported values are: Color = 0, Depth = 1, IR = 2, Fisheye = 3, Gyro = 4, Accelerometer = 5       */
                                         /**< Bits 5-7: Sensor index - Zero based index of sensor with the same type within device. For example if the device supports two fisheye cameras, */
                                         /**<                          The first will use index 0 (bSensorID = 0x03) and the second will use index 1 (bSensorID = 0x23)                     */
        uint8_t bReserved;               /**< Reserved = 0                                                                                                                                  */
        uint64_t llNanoseconds;          /**< Frame integration timestamp, as measured in nanoseconds since device initialization                                                           */
        uint64_t llArrivalNanoseconds;   /**< Frame arrival timestamp, as measured in nanoseconds since device initialization                                                               */
        uint32_t dwFrameId;              /**< A running index of frames from every unique sensor. Starting from 0.                                                                          */
    } interrupt_message_raw_stream_header;


    /**
    * @brief Interrupt raw accelerometer stream metadata
    */
    typedef struct
    {
        uint32_t dwMetadataLength; /**< Metadata length in bytes (4 bytes)      */
        float_t flTemperature;     /**< Accelerometer temperature               */
        uint32_t dwFrameLength;    /**< Length of frame below (12 bytes)        */
        float_t flAx;              /**< X value of acceleration, in meter/sec^2 */
        float_t flAy;              /**< Y value of acceleration, in meter/sec^2 */
        float_t flAz;              /**< Z value of acceleration, in meter/sec^2 */
    } interrupt_message_accelerometer_stream_metadata;


    /**
    * @brief Interrupt raw accelerometer stream message
    *
    * Specific frame metadata and data for sensor IDs - accelerometer
    */
    typedef struct
    {
        interrupt_message_raw_stream_header rawStreamHeader;
        interrupt_message_accelerometer_stream_metadata metadata;
    } interrupt_message_accelerometer_stream;


    /**
    * @brief Interrupt raw gyro stream metadata
    */
    typedef struct
    {
        uint32_t dwMetadataLength; /**< Metadata length in bytes (4 bytes) */
        float_t flTemperature;     /**< Gyro temperature                   */
        uint32_t dwFrameLength;    /**< Length of frame below (12 bytes)   */
        float_t flGx;              /**< X value of gyro, in radians/sec    */
        float_t flGy;              /**< Y value of gyro, in radians/sec    */
        float_t flGz;              /**< Z value of gyro, in radians/sec    */
    } interrupt_message_gyro_stream_metadata;


    /**
    * @brief Interrupt raw gyro stream message
    *
    * Specific frame metadata and data for sensor IDs - gyro
    */
    typedef struct
    {
        interrupt_message_raw_stream_header rawStreamHeader;
        interrupt_message_gyro_stream_metadata metadata;
    } interrupt_message_gyro_stream;


    /**
    * @brief Interrupt raw velocimeter stream metadata
    */
    typedef struct
    {
        uint32_t dwMetadataLength; /**< Metadata length in bytes (4 bytes)     */
        float_t flTemperature;     /**< velocimeter temperature                */
        uint32_t dwFrameLength;    /**< Length of frame below (12 bytes)       */
        float_t flVx;              /**< X value of velocimeter, in radians/sec */
        float_t flVy;              /**< Y value of velocimeter, in radians/sec */
        float_t flVz;              /**< Z value of velocimeter, in radians/sec */
    } interrupt_message_velocimeter_stream_metadata;


    /**
    * @brief Interrupt raw velocimeter stream message
    *
    * Specific frame metadata and data for sensor IDs - velocimeter
    */
    typedef struct
    {
        interrupt_message_raw_stream_header rawStreamHeader;
        interrupt_message_velocimeter_stream_metadata metadata;
    } interrupt_message_velocimeter_stream;

    /**
    * @brief Interrupt SLAM error message
    *
    * SLAM error code message
    */
    typedef struct {
        interrupt_message_header header; /**< Interrupt message header: dwLength = 8 bytes, wMessageID = SLAM_ERROR                                                                          */
        uint16_t wStatus;                /**< Status: SLAM_ERROR_CODE_NONE = 0   - No error has occurred.                                                                                    */
                                         /**<         SLAM_ERROR_CODE_VISION = 1 - No visual features were detected in the most recent image.                                                */
                                         /**<                                 This is normal in some circumstances, such as quick motion or if the device temporarily looks at a blank wall. */
                                         /**<         SLAM_ERROR_CODE_SPEED = 2  - The device moved more rapidly than expected for typical handheld motion.                                  */
                                         /**<                                 This may indicate that rc_Tracker has failed and is providing invalid data.                                    */
                                         /**<         SLAM_ERROR_CODE_OTHER = 3  - A fatal internal error has occurred.                                                                      */
    } interrupt_message_slam_error;

    /**
    * @brief Interrupt Get Localization Data Stream message
    *
    * Triggers by bulk_message_request_get_localization_data.
    * Returns the localization data as created during a 6DoF session.
    * This message is generated and streamed by the underlying firmware algorithm in run-time, so the total size of the data cannot be known in advance.
    * The entire data will be streams in "chunks" and the firmware will use the MORE_DATA_AVAILABLE to indicate there are more data to send.
    * The last chunk (possibly even a zero-size chunk) will be marked with a SUCCESS status code.
    */
    typedef struct {
        interrupt_message_header header; /**< Interrupt message header: dwLength = 8 bytes + bLocalizationData size, wMessageID = SLAM_GET_LOCALIZATION_DATA_STREAM */
        uint16_t wStatus;                /**< SUCCESS: indicate last chunk, MORE_DATA_AVAILABLE: any other chunk                                                    */
        uint16_t wIndex;                 /**< A running counter starting at 0 identifying the chunk index in a single data transaction                              */
        uint8_t bLocalizationData[];     /**< Localization data variable sized array. Data format is algorithm specific and opaque to the USB and host stack        */
    } interrupt_message_get_localization_data_stream;


    /**
    * @brief Interrupt Set Localization Data Stream message
    *
    * Response after setting localization data using bulk_message_set_localization_data_stream
    */
    typedef struct {
        interrupt_message_header header; /**< Interrupt message header: dwLength = 8 bytes, wMessageID = SLAM_SET_LOCALIZATION_DATA_STREAM */
        uint16_t wStatus;                /**< Status                                                                                       */
        uint8_t bReserved;               /**< Reserved = 0                                                                                 */
    } interrupt_message_set_localization_data_stream;


    /**
    * @brief Interrupt SLAM Relocalization Event message
    *
    * Event information when SLAM relocalizes.
    */
    typedef struct {
        interrupt_message_header header; /**< Interrupt message header: wMessageID = SLAM_RELOCALIZATION_EVENT                              */
        uint64_t llNanoseconds;          /**< Timestamp of relocalization event, measured in nanoseconds since device system initialization */
        uint16_t wSessionId;             /**< Session id of the relocalized map. Current session if 0, previous session otherwise           */
    } interrupt_message_slam_relocalization_event;

    /**
    * @brief Control message request header struct
    *
    * Start of all USB control message requests.
    */
    typedef struct {
        uint8_t bmRequestType; /**< Bit 7: Request direction (0 = Host to device - Out, 1 = Device to host - In)  */
                               /**< Bits 5 - 6 : Request type (0 = standard, 1 = class, 2 = vendor, 3 = reserved) */
                               /**< Bits 0 - 4 : Recipient (0 = device, 1 = interface, 2 = endpoint, 3 = other)   */
        uint8_t bRequest;      /**< Actual request ID                                                             */
        uint8_t wValueL;       /**< Unused                                                                        */
        uint8_t wValueH;       /**< Unused                                                                        */
        uint8_t wIndexL;       /**< Unused                                                                        */
        uint8_t wIndexH;       /**< Unused                                                                        */
        uint8_t wLengthL;      /**< Unused                                                                        */
        uint8_t wLengthH;      /**< Unused                                                                        */
    } control_message_request_header;

    /**
    * @brief Control reset Message
    *
    * Reset the device
    */
    typedef struct {
        control_message_request_header header; /**< Message request header: bmRequestType = 0x40 (Direction - host to device, type - vendor, recipient - device), wMessageID = CONTROL_USB_RESET */
    } control_message_request_reset;

    /**
    * @brief Control get interface version Message
    *
    * Get FW interface version
    */
    typedef struct {
        uint32_t dwMajor; /**< Major part of the device supported interface API version, updated upon an incompatible API change    */
        uint32_t dwMinor; /**< Minor part of the device supported interface API version, updated upon a backwards-compatible change */
    } interface_version_libtm_message;

    typedef struct {
        control_message_request_header header; /**< Message request header: bmRequestType = 0xC0 (Direction - device to host, type - vendor, recipient - device), wMessageID = CONTROL_GET_INTERFACE_VERSION */
    } control_message_request_get_interface_version;

    typedef struct {
        interface_version_libtm_message message; /**< Interface version */
    } control_message_response_get_interface_version;


#pragma pack(pop)

}
#ifdef _WIN32
#pragma warning (pop)
#endif
