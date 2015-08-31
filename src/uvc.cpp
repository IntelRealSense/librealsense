#include "libuvc/libuvc.h"

#define NO_UVC_TYPES
#include "uvc.h"
#include "types.h"

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
        static void check(const char * call, uvc_error_t status)
        {
            if (status < 0)
            {
                throw std::runtime_error(to_string() << call << "(...) returned " << uvc_strerror(status));
            }
        }

        const char* strerror(uvc_error_t err)
        {
            return uvc_strerror(err);
        }

        void perror(uvc_error_t err, const char *msg)
        {
            return uvc_perror(err, msg);
        }

        void init(uvc_context_t **pctx, struct libusb_context *usb_ctx)
        {
            check("uvc_init", uvc_init(pctx, usb_ctx));
        }

        void exit(uvc_context_t *ctx)
        {
            return uvc_exit(ctx);
        }

        libusb_context * get_libusb_context(uvc_context_t *ctx)
        {
            return uvc_get_libusb_context(ctx);
        }

        void get_device_list(uvc_context_t *ctx, uvc_device_t ***list)
        {
            check("uvc_get_device_list", uvc_get_device_list(ctx, list));
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

        void get_device_descriptor(uvc_device_t *dev, uvc_device_descriptor_t **desc)
        {
            return check("uvc_get_device_descriptor", uvc_get_device_descriptor(dev, desc));
        }

        void free_device_descriptor(uvc_device_descriptor_t *desc)
        {
            return uvc_free_device_descriptor(desc);
        }

        ///////////////////
        // device_handle //
        ///////////////////

        struct device_handle::impl_t
        {
            uvc_device_handle_t * handle;
            uvc_stream_ctrl_t ctrl;
        };

        device_handle::device_handle(uvc_device_t * device, int camera_number) : impl(new impl_t)
        {
            check("uvc_open2", uvc_open2(device, &impl->handle, camera_number));
        }

        device_handle::~device_handle()
        {
            uvc_close(impl->handle);
        }

        void device_handle::get_stream_ctrl_format_size(enum uvc_frame_format cf, int width, int height, int fps)
        {
            check("get_stream_ctrl_format_size", uvc_get_stream_ctrl_format_size(impl->handle, &impl->ctrl, cf, width, height, fps));
        }

        void device_handle::start_streaming(uvc_frame_callback_t *cb, void *user_ptr, uint8_t flags)
        {
            #if defined (ENABLE_DEBUG_SPAM)
            uvc_print_stream_ctrl(&impl->ctrl, stdout);
            #endif

            check("uvc_start_streaming", uvc_start_streaming(impl->handle, &impl->ctrl, cb, user_ptr, flags));
        }

        void device_handle::stop_streaming()
        {
            uvc_stop_streaming(impl->handle);
        }

        int device_handle::get_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len)
        {
            return uvc_get_ctrl(impl->handle, unit, ctrl, data, len, UVC_GET_CUR);
        }

        int device_handle::set_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len)
        {
            return uvc_set_ctrl(impl->handle, unit, ctrl, data, len);
        }
    }
}
