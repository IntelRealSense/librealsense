#include "libuvc/libuvc.h"

#define NO_UVC_TYPES
#include "uvc.h"

namespace rsimpl
{
    namespace usb // Correspond to libusb_* calls
    {
        const char * error_name(int errcode)
        {
            return libusb_error_name(errcode);
        }

        libusb_device_handle * open_device_with_vid_pid(libusb_context *ctx, uint16_t vendor_id, uint16_t product_id)
        {
            return libusb_open_device_with_vid_pid(ctx, vendor_id, product_id);
        }

        int claim_interface(libusb_device_handle *dev, int interface_number)
        {
            return libusb_claim_interface(dev, interface_number);
        }

        int release_interface(libusb_device_handle *dev, int interface_number)
        {
            return libusb_release_interface(dev, interface_number);
        }

        int bulk_transfer(libusb_device_handle *dev_handle, unsigned char endpoint, unsigned char *data, int length, int *actual_length, unsigned int timeout)
        {
            return libusb_bulk_transfer(dev_handle, endpoint, data, length, actual_length, timeout);
        }
    }

    namespace uvc // Correspond to uvc_* calls
    {
        const char* strerror(uvc_error_t err)
        {
            return uvc_strerror(err);
        }

        void perror(uvc_error_t err, const char *msg)
        {
            return uvc_perror(err, msg);
        }

        uvc_error_t init(uvc_context_t **pctx, struct libusb_context *usb_ctx)
        {
            return uvc_init(pctx, usb_ctx);
        }

        void exit(uvc_context_t *ctx)
        {
            return uvc_exit(ctx);
        }

        uvc_error_t get_device_list(uvc_context_t *ctx, uvc_device_t ***list)
        {
            return uvc_get_device_list(ctx, list);
        }

        void free_device_list(uvc_device_t **list, uint8_t unref_devices)
        {
            return uvc_free_device_list(list, unref_devices);
        }

        void ref_device(uvc_device_t *dev)
        {
            return uvc_ref_device(dev);
        }

        void unref_device(uvc_device_t *dev)
        {
            return uvc_unref_device(dev);
        }

        uvc_error_t get_device_descriptor(uvc_device_t *dev, uvc_device_descriptor_t **desc)
        {
            return uvc_get_device_descriptor(dev, desc);
        }

        void free_device_descriptor(uvc_device_descriptor_t *desc)
        {
            return uvc_free_device_descriptor(desc);
        }

        uvc_error_t open2(uvc_device_t *dev, uvc_device_handle_t **devh, int camera_number)
        {
            return uvc_open2(dev, devh, camera_number);
        }

        void close(uvc_device_handle_t *devh)
        {
            return uvc_close(devh);
        }

        uvc_error_t get_stream_ctrl_format_size(uvc_device_handle_t *devh, uvc_stream_ctrl_t *ctrl, enum uvc_frame_format cf, int width, int height, int fps)
        {
            return uvc_get_stream_ctrl_format_size(devh, ctrl, cf, width, height, fps);
        }

        uvc_error_t start_streaming(uvc_device_handle_t *devh, uvc_stream_ctrl_t *ctrl, uvc_frame_callback_t *cb, void *user_ptr, uint8_t flags)
        {
            return uvc_start_streaming(devh, ctrl, cb, user_ptr, flags);
        }

        void stop_streaming(uvc_device_handle_t *devh)
        {
            return uvc_stop_streaming(devh);
        }

        int get_ctrl(uvc_device_handle_t *devh, uint8_t unit, uint8_t ctrl, void *data, int len, enum uvc_req_code req_code)
        {
            return uvc_get_ctrl(devh, unit, ctrl, data, len, req_code);
        }

        int set_ctrl(uvc_device_handle_t *devh, uint8_t unit, uint8_t ctrl, void *data, int len)
        {
            return uvc_set_ctrl(devh, unit, ctrl, data, len);
        }

        libusb_context * get_libusb_context(uvc_context_t *ctx)
        {
            return uvc_get_libusb_context(ctx);
        }
    }
}
