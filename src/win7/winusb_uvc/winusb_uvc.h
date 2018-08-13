#ifndef WINUSB_UVC_H_
#define WINUSB_UVC_H_

#include "winusb.h"
#include "backend.h"
#include <thread>

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
    UVC_REQ_TYPE_INTERFACE_GET = 0xA1, // 10100001: Bit [4..0] = 0001: Request directed to interface, Bit [6..5] = 10: Vendor Specific request, Bit [7] = 1: Device to Host
    UVC_REQ_TYPE_INTERFACE_SET = 0x21  // 00100001: Bit [4..0] = 0001: Request directed to interface, Bit [6..5] = 10: Vendor Specific request, Bit [7] = 0: Host to Device
};

#define LIBUVC_XFER_BUF_SIZE	( 16 * 1024 * 1024 )
#define MAX_USB_INTERFACES 20

struct winusb_uvc_interface {
    USB_INTERFACE_DESCRIPTOR desc;
    PUCHAR extra;
    USB_ENDPOINT_DESCRIPTOR endpoints[10];
    int extraLength;
    int numEndpoints;
    uint8_t winusbInterfaceNumber;
};

struct winusb_uvc_interfaces {
    winusb_uvc_interface iface[MAX_USB_INTERFACES];
    int numInterfaces;
};

/** VideoControl interface control selector (A.9.1) */
enum uvc_vc_ctrl_selector {
    UVC_VC_CONTROL_UNDEFINED = 0x00,
    UVC_VC_VIDEO_POWER_MODE_CONTROL = 0x01,
    UVC_VC_REQUEST_ERROR_CODE_CONTROL = 0x02
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

typedef struct winusb_uvc_frame_desc {
    struct winusb_uvc_format_desc *parent;
    struct winusb_uvc_frame_desc *prev, *next;
    /** Type of frame, such as JPEG frame or uncompressed frme */
    enum uvc_vs_desc_subtype bDescriptorSubtype;
    /** Index of the frame within the list of specs available for this format */
    uint8_t bFrameIndex;
    uint8_t bmCapabilities;
    /** Image width */
    uint16_t wWidth;
    /** Image height */
    uint16_t wHeight;
    /** Bitrate of corresponding stream at minimal frame rate */
    uint32_t dwMinBitRate;
    /** Bitrate of corresponding stream at maximal frame rate */
    uint32_t dwMaxBitRate;
    /** Maximum number of bytes for a video frame */
    uint32_t dwMaxVideoFrameBufferSize;
    /** Default frame interval (in 100ns units) */
    uint32_t dwDefaultFrameInterval;
    /** Minimum frame interval for continuous mode (100ns units) */
    uint32_t dwMinFrameInterval;
    /** Maximum frame interval for continuous mode (100ns units) */
    uint32_t dwMaxFrameInterval;
    /** Granularity of frame interval range for continuous mode (100ns) */
    uint32_t dwFrameIntervalStep;
    /** Frame intervals */
    uint8_t bFrameIntervalType;
    /** number of bytes per line */
    uint32_t dwBytesPerLine;
    /** Available frame rates, zero-terminated (in 100ns units) */
    uint32_t *intervals;
} uvc_frame_desc_t;

/** Format descriptor
*
* A "format" determines a stream's image type (e.g., raw YUYV or JPEG)
* and includes many "frame" configurations.
*/
typedef struct winusb_uvc_format_desc {
    struct winusb_uvc_streaming_interface *parent;
    struct winusb_uvc_format_desc *prev, *next;
    /** Type of image stream, such as JPEG or uncompressed. */
    enum uvc_vs_desc_subtype bDescriptorSubtype;
    /** Identifier of this format within the VS interface's format list */
    uint8_t bFormatIndex;
    uint8_t bNumFrameDescriptors;
    /** Format specifier */
    union {
        uint8_t guidFormat[16];
        uint8_t fourccFormat[4];
    };
    /** Format-specific data */
    union {
        /** BPP for uncompressed stream */
        uint8_t bBitsPerPixel;
        /** Flags for JPEG stream */
        uint8_t bmFlags;
    };
    /** Default {winusb_uvc_frame_desc} to choose given this format */
    uint8_t bDefaultFrameIndex;
    uint8_t bAspectRatioX;
    uint8_t bAspectRatioY;
    uint8_t bmInterlaceFlags;
    uint8_t bCopyProtect;
    uint8_t bVariableSize;
    /** Available frame specifications for this format */
    struct winusb_uvc_frame_desc *frame_descs;
} uvc_format_desc_t;



enum uvc_device_power_mode {
    UVC_VC_VIDEO_POWER_MODE_FULL = 0x000b,
    UVC_VC_VIDEO_POWER_MODE_DEVICE_DEPENDENT = 0x001b,
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

/** VideoControl interface */
typedef struct winusb_uvc_control_interface {
    struct winusb_uvc_device_info *parent;
    struct uvc_input_terminal *input_term_descs;
    // struct uvc_output_terminal *output_term_descs;
    struct uvc_selector_unit *selector_unit_descs;
    struct uvc_processing_unit *processing_unit_descs;
    struct uvc_extension_unit *extension_unit_descs;
    uint16_t bcdUVC;
    uint32_t dwClockFrequency;
    uint8_t bEndpointAddress;
    /** Interface number */
    uint8_t bInterfaceNumber;
} winusb_uvc_control_interface_t;

/** VideoStream interface */
typedef struct winusb_uvc_streaming_interface {
    struct winusb_uvc_device_info *parent;
    struct winusb_uvc_streaming_interface *prev, *next;
    /** Interface number */
    uint8_t bInterfaceNumber;
    /** Video formats that this interface provides */
    struct winusb_uvc_format_desc *format_descs;
    /** USB endpoint to use when communicating with this interface */
    uint8_t bEndpointAddress;
    uint8_t bTerminalLink;
} winusb_uvc_streaming_interface_t;

typedef struct winusb_uvc_device_info {
    /** Configuration descriptor for USB device */
    USB_CONFIGURATION_DESCRIPTOR config;
    /** USB interface descriptor */
    winusb_uvc_interfaces *interfaces;
    /** VideoControl interface provided by device */
    winusb_uvc_control_interface_t ctrl_if;
    /** VideoStreaming interfaces on the device */
    winusb_uvc_streaming_interface_t *stream_ifs;
    /** Store the interface for multiple UVCs on a single VID/PID device (Intel RealSense, VF200, e.g) */
    int camera_number;
} winusb_uvc_device_info_t;

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



typedef struct winusb_uvc_stream_handle winusb_uvc_stream_handle_t;

typedef struct winusb_uvc_stream_context {
    winusb_uvc_stream_handle_t *stream;
    int endpoint;
    int maxPayloadTransferSize;
    winusb_uvc_interface *iface;
}winusb_uvc_stream_context_t;

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

typedef void(winusb_uvc_frame_callback_t)(struct librealsense::platform::frame_object *frame, void *user_ptr);

struct winusb_uvc_stream_handle {
    struct winusb_uvc_device *devh;
    struct winusb_uvc_stream_handle *prev, *next;
    struct winusb_uvc_streaming_interface *stream_if;

    /** if true, stream is running (streaming video to host) */
    uint8_t running;
    /** Current control block */
    uvc_stream_ctrl_t cur_ctrl;

    /* listeners may only access hold*, and only when holding a
    * lock on cb_mutex (probably signaled with cb_cond) */
    uint8_t fid;
    uint32_t seq, hold_seq;
    uint32_t pts, hold_pts;
    uint32_t last_scr, hold_last_scr;
    uint8_t *metadata_buf;
    size_t metadata_bytes, metadata_size;
    size_t got_bytes, hold_bytes;
    uint8_t *outbuf, *holdbuf;
    uint32_t last_polled_seq;

    std::thread cb_thread;
    winusb_uvc_frame_callback_t *user_cb;
    void *user_ptr;
    enum uvc_frame_format frame_format;
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

struct winusb_uvc_device
{
    WINUSB_INTERFACE_HANDLE winusbHandle;
    WINUSB_INTERFACE_HANDLE associateHandle;
    winusb_uvc_device_info_t deviceData;
    HANDLE deviceHandle;
    PWCHAR devPath;
    winusb_uvc_stream_handle_t *streams;
    int pid, vid;
};

/** Converts an unaligned one-byte integer into an int8 */
#define B_TO_BYTE(p) ((int8_t)(p)[0])

/** Converts an unaligned two-byte little-endian integer into an int16 */
#define SW_TO_SHORT(p) ((uint8_t)(p)[0] | \
                       ((int8_t)(p)[1] << 8))

/** Converts an unaligned four-byte little-endian integer into an int32 */
#define DW_TO_INT(p) ((uint8_t)(p)[0] | \
                     ((uint8_t)(p)[1] << 8) | \
                     ((uint8_t)(p)[2] << 16) | \
                     ((int8_t)(p)[3] << 24))

/** Converts an unaligned eight-byte little-endian integer into an int64 */
#define QW_TO_QUAD(p) (((uint64_t)(p)[0]) | \
                      (((uint64_t)(p)[1]) << 8) | \
                      (((uint64_t)(p)[2]) << 16) | \
                      (((uint64_t)(p)[3]) << 24) | \
                      (((uint64_t)(p)[4]) << 32) | \
                      (((uint64_t)(p)[5]) << 40) | \
                      (((uint64_t)(p)[6]) << 48) | \
                      (((int64_t)(p)[7]) << 56))


/** Converts an int16 into an unaligned two-byte little-endian integer */
#define SHORT_TO_SW(s, p) \
  (p)[0] = (uint8_t)(s); \
  (p)[1] = (uint8_t)((s) >> 8);
/** Converts an int32 into an unaligned four-byte little-endian integer */
#define INT_TO_DW(i, p) \
  (p)[0] = (uint8_t)(i); \
  (p)[1] = (uint8_t)((i) >> 8); \
  (p)[2] = (uint8_t)((i) >> 16); \
  (p)[3] = (uint8_t)((i) >> 24);

/** Converts an int64 into an unaligned eight-byte little-endian integer */
#define QUAD_TO_QW(i, p) \
  (p)[0] = (uint8_t)(i); \
  (p)[1] = (uint8_t)((i) >> 8); \
  (p)[2] = (uint8_t)((i) >> 16); \
  (p)[3] = (uint8_t)((i) >> 24); \
  (p)[4] = (uint8_t)((i) >> 32); \
  (p)[5] = (uint8_t)((i) >> 40); \
  (p)[6] = (uint8_t)((i) >> 48); \
  (p)[7] = (uint8_t)((i) >> 56); \

/** Selects the nth item in a doubly linked list. n=-1 selects the last item. */
#define DL_NTH(head, out, n) \
do \
{ \
    int dl_nth_i = 0; \
    LDECLTYPE(head) dl_nth_p = (head); \
    if ((n) < 0) \
    { \
        while (dl_nth_p && dl_nth_i > (n)) \
        { \
        dl_nth_p = dl_nth_p->prev; \
        dl_nth_i--; \
                                                                } \
    } \
    else \
    { \
        while (dl_nth_p && dl_nth_i < (n)) \
        { \
        dl_nth_p = dl_nth_p->next; \
        dl_nth_i++; \
                                                                                                                                                } \
                    } \
    (out) = dl_nth_p; \
          } while (0);


typedef struct uvc_frame {
    /** Image data for this frame */
    void *data;

    /** Size of image data buffer */
    size_t data_bytes;

    /** Width of image in pixels */
    uint32_t width;

    /** Height of image in pixels */
    uint32_t height;

    /** Pixel data format */
    enum uvc_frame_format frame_format;

    /** Number of bytes per horizontal line (undefined for compressed format) */
    size_t step;

    /** Frame number (may skip, but is strictly monotonically increasing) */
    uint32_t sequence;

    /** Estimate of system time when the device started capturing the image */
    struct timeval capture_time;

    /** Handle on the device that produced the image.
    * @warning You must not call any uvc_* functions during a callback. */
    winusb_uvc_device *source;

    /** Is the data buffer owned by the library?
    * If 1, the data buffer can be arbitrarily reallocated by frame conversion
    * functions.
    * If 0, the data buffer will not be reallocated or freed by the library.
    * Set this field to zero if you are supplying the buffer.
    */
    uint8_t library_owns_data;
} uvc_frame_t;

typedef void(winusb_uvc_frame_callback_t)(struct librealsense::platform::frame_object *frame, void *user_ptr);

// Return list of all connected IVCAM devices
uvc_error_t winusb_find_devices(winusb_uvc_device ***devs, int vid, int pid);
uvc_error_t winusb_find_devices(const std::string &uvc_interface, int vid, int pid, winusb_uvc_device ***devs, int& devs_count);

// Open a WinUSB device
uvc_error_t winusb_open(winusb_uvc_device *device);

// Close a WinUSB device
uvc_error_t winusb_close(winusb_uvc_device *device);

// Sending control packet using vendor-defined control transfer directed to WinUSB interface
int winusb_SendControl(WINUSB_INTERFACE_HANDLE ihandle, int requestType, int request, int value, int index, unsigned char *buffer, int buflen);

// Return linked list of uvc_format_t of all available formats inside winusb device
uvc_error_t winusb_get_available_formats(winusb_uvc_device *devh, int interface_idx, uvc_format_t **formats);

// Return linked list of uvc_format_t of all available formats inside winusb device
uvc_error_t winusb_get_available_formats_all(winusb_uvc_device *devh, uvc_format_t **formats);

// Free Formats allocated in winusb_get_available_formats/winusb_get_available_formats_all
uvc_error_t winusb_free_formats(uvc_format_t *formats);

// Get a negotiated streaming control block for some common parameters for specific interface
uvc_error_t winusb_get_stream_ctrl_format_size(winusb_uvc_device *devh, uvc_stream_ctrl_t *ctrl, uint32_t fourcc, int width, int height, int fps, int interface_idx);

// Get a negotiated streaming control block for some common parameters for all interfaces
uvc_error_t winusb_get_stream_ctrl_format_size_all(winusb_uvc_device *devh, uvc_stream_ctrl_t *ctrl, uint32_t fourcc, int width, int height, int fps);

// Start video streaming
uvc_error_t winusb_start_streaming(winusb_uvc_device *devh, uvc_stream_ctrl_t *ctrl, winusb_uvc_frame_callback_t *cb, void *user_ptr, uint8_t flags);

// Stop video streaming
void winusb_stop_streaming(winusb_uvc_device *devh);

int uvc_get_ctrl_len(winusb_uvc_device *devh, uint8_t unit, uint8_t ctrl);
int uvc_get_ctrl(winusb_uvc_device *devh, uint8_t unit, uint8_t ctrl, void *data, int len, enum uvc_req_code req_code);
int uvc_set_ctrl(winusb_uvc_device *devh, uint8_t unit, uint8_t ctrl, void *data, int len);
uvc_error_t uvc_get_power_mode(winusb_uvc_device *devh, enum uvc_device_power_mode *mode, enum uvc_req_code req_code);
uvc_error_t uvc_set_power_mode(winusb_uvc_device *devh, enum uvc_device_power_mode mode);

void poll_interrupts(WINUSB_INTERFACE_HANDLE handle, int ep, uint16_t timeout);

class safe_handle
{
public:
    safe_handle(HANDLE handle) :_handle(handle)
    {

    }

    ~safe_handle()
    {
        if (_handle != nullptr)
        {
            CloseHandle(_handle);
            _handle = nullptr;
        }
    }

    HANDLE GetHandle() const { return _handle; }
private:
    safe_handle() = delete;

    // Disallow copy:
    safe_handle(const safe_handle&) = delete;
    safe_handle& operator=(const safe_handle&) = delete;

    HANDLE _handle;
};

#endif
