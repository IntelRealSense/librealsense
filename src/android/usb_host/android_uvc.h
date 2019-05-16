/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
#pragma once

#include "../../usbhost/usbhost.h"
#include "../../usbhost/device-usbhost.h"
#include "../../usb/usb-messenger.h"
#include <thread>
#include <linux/usb/ch9.h>
#include "../../backend.h"
#include "../../libuvc/uvc_types.h"

#define LIBUVC_XFER_BUF_SIZE    ( 4 * 1024 * 1024 )
#define MAX_USB_INTERFACES 20

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

typedef unsigned char UCHAR;
typedef  UCHAR* PUCHAR;

struct usbhost_uvc_interface {
    usb_interface_descriptor desc;
    PUCHAR extra;
    usb_endpoint_descriptor endpoints[10];
    int extraLength;
    int numEndpoints;
    uint8_t winusbInterfaceNumber;
};

struct usbhost_uvc_interfaces {
    usbhost_uvc_interface iface[MAX_USB_INTERFACES];
    int numInterfaces;
};

typedef struct usbhost_uvc_frame_desc {
    struct usbhost_uvc_format_desc *parent;
    struct usbhost_uvc_frame_desc *prev, *next;
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
typedef struct usbhost_uvc_format_desc {
    struct usbhost_uvc_streaming_interface *parent;
    struct usbhost_uvc_format_desc *prev, *next;
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
    /** Default {usbhost_uvc_frame_desc} to choose given this format */
    uint8_t bDefaultFrameIndex;
    uint8_t bAspectRatioX;
    uint8_t bAspectRatioY;
    uint8_t bmInterlaceFlags;
    uint8_t bCopyProtect;
    uint8_t bVariableSize;
    /** Available frame specifications for this format */
    struct usbhost_uvc_frame_desc *frame_descs;
} uvc_format_desc_t;

/** VideoControl interface */
typedef struct usbhost_uvc_control_interface {
    struct usbhost_uvc_device_info *parent;
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
} usbhost_uvc_control_interface_t;

/** VideoStream interface */
typedef struct usbhost_uvc_streaming_interface {
    struct usbhost_uvc_device_info *parent;
    struct usbhost_uvc_streaming_interface *prev, *next;
    /** Interface number */
    uint8_t bInterfaceNumber;
    /** Video formats that this interface provides */
    struct usbhost_uvc_format_desc *format_descs;
    /** USB endpoint to use when communicating with this interface */
    uint8_t bEndpointAddress;
    uint8_t bTerminalLink;
    std::shared_ptr<librealsense::platform::usb_interface> interface;
} usbhost_uvc_streaming_interface_t;

typedef struct usbhost_uvc_device_info {
    /** Configuration descriptor for USB device */
    usb_config_descriptor config;
    /** USB interface descriptor */
    usbhost_uvc_interfaces *interfaces;
    /** VideoControl interface provided by device */
    usbhost_uvc_control_interface_t ctrl_if;
    /** VideoStreaming interfaces on the device */
    usbhost_uvc_streaming_interface_t *stream_ifs;
    /** Store the interface for multiple UVCs on a single VID/PID device (Intel RealSense, VF200, e.g) */
    int camera_number;
} usbhost_uvc_device_info_t;

typedef struct usbhost_uvc_stream_handle usbhost_uvc_stream_handle_t;

typedef struct usbhost_uvc_stream_context {
    usbhost_uvc_stream_handle_t *stream;
    int endpoint;
    int maxPayloadTransferSize;
    int maxVideoBufferSize;
    usbhost_uvc_interface *iface;
}usbhost_uvc_stream_context_t;

typedef void(usbhost_uvc_frame_callback_t)(struct librealsense::platform::frame_object *frame, void *user_ptr);

struct usbhost_uvc_stream_handle {
    struct usbhost_uvc_device *devh;
    struct usbhost_uvc_stream_handle *prev, *next;
    struct usbhost_uvc_streaming_interface *stream_if;

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
    usbhost_uvc_frame_callback_t *user_cb;
    void *user_ptr;
    enum uvc_frame_format frame_format;
    pthread_mutex_t cb_mutex;
    pthread_cond_t cb_cond;
};

struct usbhost_uvc_device
{
    usbhost_uvc_device_info_t deviceData;
    std::shared_ptr<librealsense::platform::usb_device_usbhost> device;
    usbhost_uvc_stream_handle_t *streams;
    int pid, vid;
};

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
    usbhost_uvc_device *source;

    /** Is the data buffer owned by the library?
    * If 1, the data buffer can be arbitrarily reallocated by frame conversion
    * functions.
    * If 0, the data buffer will not be reallocated or freed by the library.
    * Set this field to zero if you are supplying the buffer.
    */
    uint8_t library_owns_data;
} uvc_frame_t;

typedef void(usbhost_uvc_frame_callback_t)(struct librealsense::platform::frame_object *frame, void *user_ptr);

// Open a WinUSB device
uvc_error_t usbhost_open(usbhost_uvc_device *device, int InterfaceNumber);

// Close a WinUSB device
uvc_error_t usbhost_close(usbhost_uvc_device *device);

// Sending control packet using vendor-defined control transfer directed to WinUSB interface
int usbhost_SendControl(std::shared_ptr<librealsense::platform::usb_device_usbhost> ihandle,int interface_number, int requestType, int request, int value, int index, unsigned char *buffer, int buflen);

// Return linked list of uvc_format_t of all available formats inside winusb device
uvc_error_t usbhost_get_available_formats(usbhost_uvc_device *devh, int interface_idx, uvc_format_t **formats);

// Return linked list of uvc_format_t of all available formats inside winusb device
uvc_error_t usbhost_get_available_formats_all(usbhost_uvc_device *devh, uvc_format_t **formats);

// Free Formats allocated in usbhost_get_available_formats/usbhost_get_available_formats_all
uvc_error_t usbhost_free_formats(uvc_format_t *formats);

// Get a negotiated streaming control block for some common parameters for specific interface
uvc_error_t usbhost_get_stream_ctrl_format_size(usbhost_uvc_device *devh, uvc_stream_ctrl_t *ctrl, uint32_t fourcc, int width, int height, int fps, int interface_idx);

// Get a negotiated streaming control block for some common parameters for all interfaces
uvc_error_t usbhost_get_stream_ctrl_format_size_all(usbhost_uvc_device *devh, uvc_stream_ctrl_t *ctrl, uint32_t fourcc, int width, int height, int fps);

// Start video streaming
uvc_error_t usbhost_start_streaming(usbhost_uvc_device *devh, uvc_stream_ctrl_t *ctrl, usbhost_uvc_frame_callback_t *cb, void *user_ptr, uint8_t flags);

uvc_error_t update_stream_if_handle(usbhost_uvc_device *devh, int interface_idx);

// Stop video streaming
void usbhost_stop_streaming(usbhost_uvc_device *devh);

int uvc_get_ctrl_len(usbhost_uvc_device *devh, uint8_t unit, uint8_t ctrl);
int uvc_get_ctrl(usbhost_uvc_device *devh, uint8_t unit, uint8_t ctrl, void *data, int len, enum uvc_req_code req_code);
int uvc_set_ctrl(usbhost_uvc_device *devh, uint8_t unit, uint8_t ctrl, void *data, int len);
uvc_error_t uvc_get_power_mode(usbhost_uvc_device *devh, enum uvc_device_power_mode *mode, enum uvc_req_code req_code);
uvc_error_t uvc_set_power_mode(usbhost_uvc_device *devh, enum uvc_device_power_mode mode);
