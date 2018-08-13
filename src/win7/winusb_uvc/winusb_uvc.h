#ifndef WINUSB_UVC_H_
#define WINUSB_UVC_H_

#include "winusb_internal.h"

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
