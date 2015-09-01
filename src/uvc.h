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
typedef struct uvc_device_descriptor uvc_device_descriptor_t;
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
        int get_vendor_id(uvc_device_descriptor_t * desc);
        int get_product_id(uvc_device_descriptor_t * desc);
        const char * get_product_name(uvc_device_descriptor_t * desc);

        class device_handle
        {
            struct impl_t; std::unique_ptr<impl_t> impl;
        public:
            device_handle(uvc_device_t * device, int camera_number);
            ~device_handle();

            void get_stream_ctrl_format_size(enum uvc_frame_format cf, int width, int height, int fps);
            void start_streaming(std::function<void(const void * frame, int width, int height, uvc_frame_format format)> callback);
            void stop_streaming();

            int get_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len);
            int set_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len);
        };
    }
}

#endif
