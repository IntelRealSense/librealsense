#include "uvc.h"
#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h" // For LibUSB punchthrough

#include <iostream>

namespace rsimpl
{
    namespace usb // Correspond to libusb_* calls
    {
        const char * error_name(int errcode)
        {
            return libusb_error_name(errcode);
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
            std::vector<int> claimed_interfaces;

            device_handle_impl(std::shared_ptr<device_impl> device, int subdevice_index) : handle()
            {
                this->device = device;
                check("uvc_open2", uvc_open2(device->device, &handle, subdevice_index));
            }
            ~device_handle_impl()
            {
                for(auto interface_number : claimed_interfaces)
                {
                    int status = libusb_release_interface(handle->usb_devh, interface_number);
                    if(status < 0) std::cerr << "Warning: libusb_release_interface(...) returned " << libusb_error_name(status) << std::endl;
                }
                uvc_close(handle);
            }
        };

        ///////////////////
        // device_handle //
        ///////////////////

        device device_handle::get_parent() { return impl->device; }

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
                reinterpret_cast<device_handle_impl *>(user)->callback(frame->data, frame->width, frame->height, (frame_format)frame->frame_format);
            }, impl.get(), 0));
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

        int device_handle::claim_interface(int interface_number)
        {
            int status = libusb_claim_interface(impl->handle->usb_devh, interface_number);
            if(status >= 0) impl->claimed_interfaces.push_back(interface_number);
            return status;
        }

        int device_handle::bulk_transfer(unsigned char endpoint, unsigned char *data, int length, int *actual_length, unsigned int timeout)
        {
            return libusb_bulk_transfer(impl->handle->usb_devh, endpoint, data, length, actual_length, timeout);
        }

        ////////////
        // device //
        ////////////

        int device::get_vendor_id() const { return impl->desc->idVendor; }
        int device::get_product_id() const { return impl->desc->idProduct; }
        const char * device::get_product_name() const { return impl->desc->product; }
        context device::get_parent() { return impl->context; }

        device_handle device::claim_subdevice(int subdevice_index)
        {
            return std::make_shared<device_handle_impl>(impl, subdevice_index);
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

        std::vector<device> context::query_devices()
        {
            uvc_device_t ** list;
            check("uvc_get_device_list", uvc_get_device_list(impl->ctx, &list));
            std::vector<device> devices;
            for(auto it = list; *it; ++it)
            {
                devices.push_back(std::make_shared<device_impl>(impl, *it));
            }
            uvc_free_device_list(list, 1);
            return devices;
        }
    }
}
