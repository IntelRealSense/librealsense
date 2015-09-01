#pragma once
#ifndef LIBREALSENSE_UVC_H
#define LIBREALSENSE_UVC_H

#include <cstdint>
#include <ctime>
#include <memory>

#ifndef NO_UVC_TYPES
typedef struct libusb_context libusb_context;
typedef struct libusb_device_handle libusb_device_handle;
typedef struct uvc_context uvc_context_t;
typedef struct uvc_device uvc_device_t;
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
        enum class frame_format
        {
            ANY     = 0,
            YUYV    = 3,
            Y12I    = 5,    // R200 - 12 bit infrared (stereo interleaved)
            Y8      = 7,    // R200 - 8 bit infrared
            Z16     = 8,    // R200 - 16 bit depth
            INVI    = 14,   // F200 - 8 bit infrared
            INVR    = 16,   // F200 - 16 bit depth
            INRI    = 18,   // F200 - 16 bit depth + 8 bit infrared
        };

        void init(uvc_context_t **pctx, struct libusb_context *usb_ctx);
        void exit(uvc_context_t *ctx);
        libusb_context * get_libusb_context(uvc_context_t *ctx);

        void get_device_list(uvc_context_t *ctx, uvc_device_t ***list);
        void free_device_list(uvc_device_t **list, uint8_t unref_devices);

        class device_handle;

        class device
        {
            friend class device_handle;
            struct impl_t; std::shared_ptr<impl_t> impl;
        public:
            device(uvc_device_t * dev);
            ~device();

            int get_vendor_id() const;
            int get_product_id() const;
            const char * get_product_name() const;
        };

        class device_handle
        {
            struct impl_t; std::unique_ptr<impl_t> impl;
        public:
            device_handle(device device, int camera_number);
            ~device_handle();

            void get_stream_ctrl_format_size(int width, int height, frame_format format, int fps);
            void start_streaming(std::function<void(const void * frame, int width, int height, frame_format format)> callback);
            void stop_streaming();

            int get_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len);
            int set_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len);
        };
    }
}

#endif
