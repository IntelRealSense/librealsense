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

        struct context_impl
        {
            uvc_context_t * ctx;

            context_impl() : ctx() { check("uvc_init", uvc_init(&ctx, nullptr)); }
            ~context_impl() { if(ctx) uvc_exit(ctx); }
        };

        struct device_impl
        {
            std::shared_ptr<context_impl> context;
            uvc_device_t * device;
            uvc_device_descriptor_t * desc;

            device_impl() : device(), desc() {}
            device_impl(std::shared_ptr<context_impl> context, uvc_device_t * device) : device_impl()
            {
                this->context = context;
                this->device = device;
                uvc_ref_device(device);
                check("uvc_get_device_descriptor", uvc_get_device_descriptor(device, &desc));
            }
            ~device_impl()
            {
                if(desc) uvc_free_device_descriptor(desc);
                if(device) uvc_unref_device(device);
            }
        };

        struct device_handle_impl
        {
            std::shared_ptr<device_impl> device;
            uvc_device_handle_t * handle;
            uvc_stream_ctrl_t ctrl;
            std::function<void(const void * frame, int width, int height, frame_format format)> callback;

            device_handle_impl(std::shared_ptr<device_impl> device, int subdevice_index)
            {
                this->device = device;
                check("uvc_open2", uvc_open2(device->device, &handle, subdevice_index));
            }
            ~device_handle_impl() { uvc_close(handle); }
        };

        ///////////////////
        // device_handle //
        ///////////////////

        void device_handle::get_stream_ctrl_format_size(int width, int height, frame_format cf, int fps)
        {
            check("get_stream_ctrl_format_size", uvc_get_stream_ctrl_format_size(impl->handle, &impl->ctrl, (uvc_frame_format)cf, width, height, fps));
        }

        void device_handle::start_streaming(std::function<void(const void * frame, int width, int height, frame_format format)> callback)
        {
            #if defined (ENABLE_DEBUG_SPAM)
            uvc_print_stream_ctrl(&impl->ctrl, stdout);
            #endif

            impl->callback = callback;
            check("uvc_start_streaming", uvc_start_streaming(impl->handle, &impl->ctrl, [](uvc_frame * frame, void * user)
            {
                reinterpret_cast<device_handle *>(user)->impl->callback(frame->data, frame->width, frame->height, (frame_format)frame->frame_format);
            }, this, 0));
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

        ////////////
        // device //
        ////////////

        int device::get_vendor_id() const { return impl->desc->idVendor; }
        int device::get_product_id() const { return impl->desc->idProduct; }
        const char * device::get_product_name() const { return impl->desc->product; }

        device_handle device::claim_subdevice(int subdevice_index)
        {
            device_handle handle;
            handle.impl.reset(new device_handle_impl(impl, subdevice_index));
            return handle;
        }

        /////////////
        // context //
        /////////////

        context context::create()
        {
            context c;
            c.impl.reset(new context_impl);
            return c;
        }

        libusb_context * context::get_libusb_context()
        {
            uvc_get_libusb_context(impl->ctx);
        }

        std::vector<device> context::query_devices()
        {
            uvc_device_t ** list;
            check("uvc_get_device_list", uvc_get_device_list(impl->ctx, &list));
            std::vector<device> devices;
            for(auto it = list; *it; ++it)
            {
                device d;
                d.impl.reset(new device_impl(impl, *it));
                devices.push_back(d);
            }
            uvc_free_device_list(list, 1);
            return devices;
        }
    }


}
