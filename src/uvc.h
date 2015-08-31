#pragma once
#ifndef LIBREALSENSE_UVC_H
#define LIBREALSENSE_UVC_H

#include <cstdint>
#include <ctime>
#include <memory>

#ifndef NO_UVC_TYPES

// Opaque handles
typedef struct libusb_context libusb_context;
typedef struct libusb_device_handle libusb_device_handle;
typedef struct uvc_context uvc_context_t;
typedef struct uvc_device uvc_device_t;
typedef struct uvc_device_handle uvc_device_handle_t;
typedef struct uvc_stream_handle uvc_stream_handle_t;

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
    /** DS4 Depth */
    UVC_FRAME_FORMAT_Y12I,
    UVC_FRAME_FORMAT_Y16,
    UVC_FRAME_FORMAT_Y8,
    UVC_FRAME_FORMAT_Z16,
    /** 24-bit RGB */
    UVC_FRAME_FORMAT_RGB,
    UVC_FRAME_FORMAT_BGR,
    /** Motion-JPEG (or JPEG) encoded images */
    UVC_FRAME_FORMAT_MJPEG,
    UVC_FRAME_FORMAT_GRAY8,
    UVC_FRAME_FORMAT_BY8,

    /** IVCam Depth */
    UVC_FRAME_FORMAT_INVI, //IR
    UVC_FRAME_FORMAT_RELI, //IR
    UVC_FRAME_FORMAT_INVR, //Depth
    UVC_FRAME_FORMAT_INVZ, //Depth
    UVC_FRAME_FORMAT_INRI, //Depth (24 bit)
    UVC_FRAME_FORMAT_INZI, //16-bit depth + 8-bit IR

    /** Number of formats understood */
    UVC_FRAME_FORMAT_COUNT,
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
  uvc_device_handle_t *source;
  /** Is the data buffer owned by the library?
   * If 1, the data buffer can be arbitrarily reallocated by frame conversion
   * functions.
   * If 0, the data buffer will not be reallocated or freed by the library.
   * Set this field to zero if you are supplying the buffer.
   */
  uint8_t library_owns_data;
} uvc_frame_t;

typedef void(uvc_frame_callback_t)(struct uvc_frame *frame, void *user_ptr);

#endif

namespace rsimpl
{
    namespace usb // Correspond to libusb_* calls
    {
        const char * error_name(int errcode);

        libusb_device_handle * open_device_with_vid_pid(libusb_context *ctx, uint16_t vendor_id, uint16_t product_id);
        int claim_interface(libusb_device_handle *dev, int interface_number);
        int release_interface(libusb_device_handle *dev, int interface_number);
        int bulk_transfer(libusb_device_handle *dev_handle, unsigned char endpoint, unsigned char *data, int length, int *actual_length, unsigned int timeout);
    }

    namespace uvc // Correspond to uvc_* calls
    {
        void init(uvc_context_t **pctx, struct libusb_context *usb_ctx);
        void exit(uvc_context_t *ctx);
        libusb_context * get_libusb_context(uvc_context_t *ctx);

        void get_device_list(uvc_context_t *ctx, uvc_device_t ***list);
        void free_device_list(uvc_device_t **list, uint8_t unref_devices);

        void ref_device(uvc_device_t *dev);
        void unref_device(uvc_device_t *dev);

        void get_device_descriptor(uvc_device_t *dev, uvc_device_descriptor_t **desc);
        void free_device_descriptor(uvc_device_descriptor_t *desc);

        class device_handle
        {
            struct impl_t; std::unique_ptr<impl_t> impl;
        public:
            device_handle(uvc_device_t * device, int camera_number);
            ~device_handle();

            void get_stream_ctrl_format_size(enum uvc_frame_format cf, int width, int height, int fps);
            void start_streaming(uvc_frame_callback_t *cb, void *user_ptr, uint8_t flags);
            void stop_streaming();

            int get_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len);
            int set_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len);
        };
    }
}

#endif
