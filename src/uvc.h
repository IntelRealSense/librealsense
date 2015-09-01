#pragma once
#ifndef LIBREALSENSE_UVC_H
#define LIBREALSENSE_UVC_H

#include <cstdint>
#include <ctime>
#include <memory>
#include <vector>

#ifndef NO_UVC_TYPES
typedef struct uvc_device uvc_device_t;
typedef struct libusb_context libusb_context;
typedef struct libusb_device_handle libusb_device_handle;
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

        struct context_impl;
        struct device_impl;
        struct device_handle_impl;

        class context;
        class device;
        class device_handle;

        class context
        {
            std::shared_ptr<context_impl> impl;
        public:
            libusb_context * get_libusb_context();
            std::vector<device> query_devices();

            static context create();
        };

        class device
        {
            friend class context;
            std::shared_ptr<device_impl> impl;
        public:
            int get_vendor_id() const;
            int get_product_id() const;
            const char * get_product_name() const;

            device_handle claim_subdevice(int subdevice_index);
        };

        class device_handle
        {
            friend class device;
            std::shared_ptr<device_handle_impl> impl;
        public:
            void get_stream_ctrl_format_size(int width, int height, frame_format format, int fps);
            void start_streaming(std::function<void(const void * frame, int width, int height, frame_format format)> callback);
            void stop_streaming();

            int get_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len);
            int set_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len);
        };
    }
}

#endif
