#pragma once

#include <unordered_map>

/* UVC_COLOR_FORMAT_* have been replaced with UVC_FRAME_FORMAT_*. Please use
 * UVC_FRAME_FORMAT_* instead of using these. */
#define UVC_COLOR_FORMAT_UNKNOWN UVC_FRAME_FORMAT_UNKNOWN
#define UVC_COLOR_FORMAT_UNCOMPRESSED UVC_FRAME_FORMAT_UNCOMPRESSED
#define UVC_COLOR_FORMAT_COMPRESSED UVC_FRAME_FORMAT_COMPRESSED
#define UVC_COLOR_FORMAT_YUYV UVC_FRAME_FORMAT_YUYV
#define UVC_COLOR_FORMAT_UYVY UVC_FRAME_FORMAT_UYVY
#define UVC_COLOR_FORMAT_RGB UVC_FRAME_FORMAT_RGB
#define UVC_COLOR_FORMAT_BGR UVC_FRAME_FORMAT_BGR
#define UVC_COLOR_FORMAT_MJPEG UVC_FRAME_FORMAT_MJPEG
#define UVC_COLOR_FORMAT_GRAY8 UVC_FRAME_FORMAT_GRAY8

// convert to standard fourcc codes
const std::unordered_map<uint32_t, uint32_t> fourcc_map = {
        { 0x59382020, 0x47524559 },    /* 'GREY' from 'Y8  ' */
        { 0x52573130, 0x70524141 },    /* 'pRAA' from 'RW10'.*/
        { 0x32000000, 0x47524559 },    /* 'GREY' from 'L8  ' */
        { 0x50000000, 0x5a313620 },    /* 'Z16'  from 'D16 ' */
        { 0x52415738, 0x47524559 },    /* 'GREY' from 'RAW8' */
        { 0x52573136, 0x42595232 }     /* 'RW16' from 'BYR2' */
};

enum uvc_device_power_mode {
    UVC_VC_VIDEO_POWER_MODE_FULL = 0x000b,
    UVC_VC_VIDEO_POWER_MODE_DEVICE_DEPENDENT = 0x001b,
};

/** UVC error types, based on libusb errors
 * @ingroup diag
 */
typedef enum uvc_error {
    /** Success (no error) */
            UVC_SUCCESS = 0,
    /** Input/output error */
            UVC_ERROR_IO = -1,
    /** Invalid parameter */
            UVC_ERROR_INVALID_PARAM = -2,
    /** Access denied */
            UVC_ERROR_ACCESS = -3,
    /** No such device */
            UVC_ERROR_NO_DEVICE = -4,
    /** Entity not found */
            UVC_ERROR_NOT_FOUND = -5,
    /** Resource busy */
            UVC_ERROR_BUSY = -6,
    /** Operation timed out */
            UVC_ERROR_TIMEOUT = -7,
    /** Overflow */
            UVC_ERROR_OVERFLOW = -8,
    /** Pipe error */
            UVC_ERROR_PIPE = -9,
    /** System call interrupted */
            UVC_ERROR_INTERRUPTED = -10,
    /** Insufficient memory */
            UVC_ERROR_NO_MEM = -11,
    /** Operation not supported */
            UVC_ERROR_NOT_SUPPORTED = -12,
    /** Device is not UVC-compliant */
            UVC_ERROR_INVALID_DEVICE = -50,
    /** Mode not supported */
            UVC_ERROR_INVALID_MODE = -51,
    /** Resource has a callback (can't use polling and async) */
            UVC_ERROR_CALLBACK_EXISTS = -52,
    /** Undefined error */
            UVC_ERROR_OTHER = -99
} uvc_error_t;

/** Color coding of stream, transport-independent
* @ingroup streaming
*/
enum uvc_frame_format {
    UVC_FRAME_FORMAT_UNKNOWN = 0,
    /** Any supported format */
    UVC_FRAME_FORMAT_ANY = 0,
    UVC_FRAME_FORMAT_UNCOMPRESSED,
    UVC_FRAME_FORMAT_COMPRESSED,
    /** YUYV/YUV2/YUV422: YUV encoding with one luminance value per pixel and
    * one UV (chrominance) pair for every two pixels.
    */
    UVC_FRAME_FORMAT_YUYV,
    UVC_FRAME_FORMAT_UYVY,
    /** 24-bit RGB */
    UVC_FRAME_FORMAT_RGB,
    UVC_FRAME_FORMAT_BGR,
    /** Motion-JPEG (or JPEG) encoded images */
    UVC_FRAME_FORMAT_MJPEG,
    UVC_FRAME_FORMAT_GRAY8,
    UVC_FRAME_FORMAT_BY8,
    /** Number of formats understood */
    UVC_FRAME_FORMAT_COUNT,
};

/** VideoControl interface descriptor subtype (A.5) */
enum uvc_vc_desc_subtype {
    UVC_VC_DESCRIPTOR_UNDEFINED = 0x00,
    UVC_VC_HEADER = 0x01,
    UVC_VC_INPUT_TERMINAL = 0x02,
    UVC_VC_OUTPUT_TERMINAL = 0x03,
    UVC_VC_SELECTOR_UNIT = 0x04,
    UVC_VC_PROCESSING_UNIT = 0x05,
    UVC_VC_EXTENSION_UNIT = 0x06
};

/** VideoStreaming interface descriptor subtype (A.6) */
enum uvc_vs_desc_subtype {
    UVC_VS_UNDEFINED = 0x00,
    UVC_VS_INPUT_HEADER = 0x01,
    UVC_VS_OUTPUT_HEADER = 0x02,
    UVC_VS_STILL_IMAGE_FRAME = 0x03,
    UVC_VS_FORMAT_UNCOMPRESSED = 0x04,
    UVC_VS_FRAME_UNCOMPRESSED = 0x05,
    UVC_VS_FORMAT_MJPEG = 0x06,
    UVC_VS_FRAME_MJPEG = 0x07,
    UVC_VS_FORMAT_MPEG2TS = 0x0a,
    UVC_VS_FORMAT_DV = 0x0c,
    UVC_VS_COLORFORMAT = 0x0d,
    UVC_VS_FORMAT_FRAME_BASED = 0x10,
    UVC_VS_FRAME_FRAME_BASED = 0x11,
    UVC_VS_FORMAT_STREAM_BASED = 0x12
};

/** UVC request code (A.8) */
enum uvc_req_code {
    UVC_RC_UNDEFINED = 0x00,
    UVC_SET_CUR = 0x01,
    UVC_GET_CUR = 0x81,
    UVC_GET_MIN = 0x82,
    UVC_GET_MAX = 0x83,
    UVC_GET_RES = 0x84,
    UVC_GET_LEN = 0x85,
    UVC_GET_INFO = 0x86,
    UVC_GET_DEF = 0x87,
    UVC_REQ_TYPE_GET = 0xa1,
    UVC_REQ_TYPE_SET = 0x21
};

/** VideoControl interface control selector (A.9.1) */
enum uvc_vc_ctrl_selector {
    UVC_VC_CONTROL_UNDEFINED = 0x00,
    UVC_VC_VIDEO_POWER_MODE_CONTROL = 0x01,
    UVC_VC_REQUEST_ERROR_CODE_CONTROL = 0x02
};

/** Camera terminal control selector (A.9.4) */
enum uvc_ct_ctrl_selector {
    UVC_CT_CONTROL_UNDEFINED = 0x00,
    UVC_CT_SCANNING_MODE_CONTROL = 0x01,
    UVC_CT_AE_MODE_CONTROL = 0x02,
    UVC_CT_AE_PRIORITY_CONTROL = 0x03,
    UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL = 0x04,
    UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL = 0x05,
    UVC_CT_FOCUS_ABSOLUTE_CONTROL = 0x06,
    UVC_CT_FOCUS_RELATIVE_CONTROL = 0x07,
    UVC_CT_FOCUS_AUTO_CONTROL = 0x08,
    UVC_CT_IRIS_ABSOLUTE_CONTROL = 0x09,
    UVC_CT_IRIS_RELATIVE_CONTROL = 0x0a,
    UVC_CT_ZOOM_ABSOLUTE_CONTROL = 0x0b,
    UVC_CT_ZOOM_RELATIVE_CONTROL = 0x0c,
    UVC_CT_PANTILT_ABSOLUTE_CONTROL = 0x0d,
    UVC_CT_PANTILT_RELATIVE_CONTROL = 0x0e,
    UVC_CT_ROLL_ABSOLUTE_CONTROL = 0x0f,
    UVC_CT_ROLL_RELATIVE_CONTROL = 0x10,
    UVC_CT_PRIVACY_CONTROL = 0x11,
    UVC_CT_FOCUS_SIMPLE_CONTROL = 0x12,
    UVC_CT_DIGITAL_WINDOW_CONTROL = 0x13,
    UVC_CT_REGION_OF_INTEREST_CONTROL = 0x14
};

/** Processing unit control selector (A.9.5) */
enum uvc_pu_ctrl_selector {
    UVC_PU_CONTROL_UNDEFINED = 0x00,
    UVC_PU_BACKLIGHT_COMPENSATION_CONTROL = 0x01,
    UVC_PU_BRIGHTNESS_CONTROL = 0x02,
    UVC_PU_CONTRAST_CONTROL = 0x03,
    UVC_PU_GAIN_CONTROL = 0x04,
    UVC_PU_POWER_LINE_FREQUENCY_CONTROL = 0x05,
    UVC_PU_HUE_CONTROL = 0x06,
    UVC_PU_SATURATION_CONTROL = 0x07,
    UVC_PU_SHARPNESS_CONTROL = 0x08,
    UVC_PU_GAMMA_CONTROL = 0x09,
    UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL = 0x0a,
    UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL = 0x0b,
    UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL = 0x0c,
    UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL = 0x0d,
    UVC_PU_DIGITAL_MULTIPLIER_CONTROL = 0x0e,
    UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL = 0x0f,
    UVC_PU_HUE_AUTO_CONTROL = 0x10,
    UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL = 0x11,
    UVC_PU_ANALOG_LOCK_STATUS_CONTROL = 0x12,
    UVC_PU_CONTRAST_AUTO_CONTROL = 0x13
};

/** USB terminal type (B.1) */
enum uvc_term_type {
    UVC_TT_VENDOR_SPECIFIC = 0x0100,
    UVC_TT_STREAMING = 0x0101
};

/** Input terminal type (B.2) */
enum uvc_it_type {
    UVC_ITT_VENDOR_SPECIFIC = 0x0200,
    UVC_ITT_CAMERA = 0x0201,
    UVC_ITT_MEDIA_TRANSPORT_INPUT = 0x0202
};

/** Output terminal type (B.3) */
enum uvc_ot_type {
    UVC_OTT_VENDOR_SPECIFIC = 0x0300,
    UVC_OTT_DISPLAY = 0x0301,
    UVC_OTT_MEDIA_TRANSPORT_OUTPUT = 0x0302
};

/** External terminal type (B.4) */
enum uvc_et_type {
    UVC_EXTERNAL_VENDOR_SPECIFIC = 0x0400,
    UVC_COMPOSITE_CONNECTOR = 0x0401,
    UVC_SVIDEO_CONNECTOR = 0x0402,
    UVC_COMPONENT_CONNECTOR = 0x0403
};

enum uvc_status_class {
    UVC_STATUS_CLASS_CONTROL = 0x10,
    UVC_STATUS_CLASS_CONTROL_CAMERA = 0x11,
    UVC_STATUS_CLASS_CONTROL_PROCESSING = 0x12,
};

enum uvc_status_attribute {
    UVC_STATUS_ATTRIBUTE_VALUE_CHANGE = 0x00,
    UVC_STATUS_ATTRIBUTE_INFO_CHANGE = 0x01,
    UVC_STATUS_ATTRIBUTE_FAILURE_CHANGE = 0x02,
    UVC_STATUS_ATTRIBUTE_UNKNOWN = 0xff
};

enum uvc_vs_ctrl_selector {
    UVC_VS_CONTROL_UNDEFINED = 0x00,
    UVC_VS_PROBE_CONTROL = 0x01,
    UVC_VS_COMMIT_CONTROL = 0x02,
    UVC_VS_STILL_PROBE_CONTROL = 0x03,
    UVC_VS_STILL_COMMIT_CONTROL = 0x04,
    UVC_VS_STILL_IMAGE_TRIGGER_CONTROL = 0x05,
    UVC_VS_STREAM_ERROR_CODE_CONTROL = 0x06,
    UVC_VS_GENERATE_KEY_FRAME_CONTROL = 0x07,
    UVC_VS_UPDATE_FRAME_SEGMENT_CONTROL = 0x08,
    UVC_VS_SYNC_DELAY_CONTROL = 0x09
};

typedef struct uvc_device_descriptor {
    /** Vendor ID */
    uint16_t idVendor;
    /** Product ID */
    uint16_t idProduct;
    /** UVC compliance level, e.g. 0x0100 (1.0), 0x0110 */
    uint16_t bcdUVC;
    /** Serial number (null if unavailable) */
    const char *serialNumber;
    /** Device-reported manufacturer name (or null) */
    const char *manufacturer;
    /** Device-reporter product name (or null) */
    const char *product;
} uvc_device_descriptor_t;

typedef struct uvc_input_terminal {
    struct uvc_input_terminal *prev, *next;
    /** Index of the terminal within the device */
    uint8_t bTerminalID;
    /** Type of terminal (e.g., camera) */
    enum uvc_it_type wTerminalType;
    uint16_t wObjectiveFocalLengthMin;
    uint16_t wObjectiveFocalLengthMax;
    uint16_t wOcularFocalLength;
    /** Camera controls (meaning of bits given in {uvc_ct_ctrl_selector}) */
    uint64_t bmControls;
} uvc_input_terminal_t;

typedef struct uvc_output_terminal {
    struct uvc_output_terminal *prev, *next;
    /** @todo */
} uvc_output_terminal_t;

/** Represents post-capture processing functions */
typedef struct uvc_processing_unit {
    struct uvc_processing_unit *prev, *next;
    /** Index of the processing unit within the device */
    uint8_t bUnitID;
    /** Index of the terminal from which the device accepts images */
    uint8_t bSourceID;
    /** Processing controls (meaning of bits given in {uvc_pu_ctrl_selector}) */
    uint64_t bmControls;
} uvc_processing_unit_t;

/** Represents selector unit to connect other units */
typedef struct uvc_selector_unit {
    struct uvc_selector_unit *prev, *next;
    /** Index of the selector unit within the device */
    uint8_t bUnitID;
} uvc_selector_unit_t;

/** Custom processing or camera-control functions */
typedef struct uvc_extension_unit {
    struct uvc_extension_unit *prev, *next;
    /** Index of the extension unit within the device */
    uint8_t bUnitID;
    /** GUID identifying the extension unit */
    uint8_t guidExtensionCode[16];
    /** Bitmap of available controls (manufacturer-dependent) */
    uint64_t bmControls;
} uvc_extension_unit_t;

typedef struct uvc_stream_ctrl {
    uint16_t bmHint;
    uint8_t bFormatIndex;
    uint8_t bFrameIndex;
    uint32_t dwFrameInterval;
    uint16_t wKeyFrameRate;
    uint16_t wPFrameRate;
    uint16_t wCompQuality;
    uint16_t wCompWindowSize;
    uint16_t wDelay;
    uint32_t dwMaxVideoFrameSize;
    uint32_t dwMaxPayloadTransferSize;
    uint32_t dwClockFrequency;
    uint8_t bmFramingInfo;
    uint8_t bPreferredVersion;
    uint8_t bMinVersion;
    uint8_t bMaxVersion;
    uint8_t bInterfaceNumber;
    uint8_t bUsage;
    uint8_t bBitDepthLuma;
    uint8_t bmSettings;
    uint8_t bMaxNumberOfRefFramesPlus1;
    uint16_t bmRateControlModes;
    uint64_t bmLayoutPerStream;
} uvc_stream_ctrl_t;

typedef struct uvc_format {
    /** Width of image in pixels */
    uint32_t width;
    /** Height of image in pixels */
    uint32_t height;
    /** fourcc format */
    uint32_t fourcc;
    /** frame per second */
    uint32_t fps;
    /** Interface number */
    uint8_t interfaceNumber;
    /** next format */
    struct uvc_format *next;
} uvc_format_t;
