// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS_USE_LIBUVC_BACKEND

//#define ENABLE_DEBUG_SPAM

#include "uvc.h"
#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h" // For LibUSB punchthrough
#include <thread>

namespace rsimpl
{
    namespace uvc
    {
        static void check(const char * call, uvc_error_t status)
        {
            if (status < 0) throw std::runtime_error(to_string() << call << "(...) returned " << uvc_strerror(status));
        }
        #define CALL_UVC(name, ...) check(#name, name(__VA_ARGS__))

        struct context
        {
            uvc_context_t * ctx;
            libusb_context * usb_context;

            context() : ctx()
            {
                check("uvc_init", uvc_init(&ctx, nullptr));

                int status = libusb_init(&usb_context);
                if (status < 0) throw std::runtime_error(to_string() << "libusb_init(...) returned " << libusb_error_name(status));
            }
            ~context()
            {
                libusb_exit(usb_context);
                if (ctx) uvc_exit(ctx);
            }
        };

        struct subdevice
        {
            uvc_device_handle_t * handle = nullptr;
            uvc_stream_ctrl_t ctrl;
            uint8_t unit;
            video_channel_callback callback = nullptr;
            data_channel_callback  channel_data_callback = nullptr;

            void set_data_channel_cfg(data_channel_callback callback)
            {
                this->channel_data_callback = callback;
            }

            static void poll_interrupts(libusb_device_handle *handle, const std::vector<subdevice *> & subdevices, uint16_t timeout)
            {
                static const unsigned short interrupt_buf_size = 0x400;
                uint8_t buffer[interrupt_buf_size];           /* 64 byte transfer buffer - dedicated channel */
                int num_bytes = 0;                            /* Actual bytes transferred */

                // TODO - replace hard-coded values : 0x82 and 1000
                int res = libusb_interrupt_transfer(handle, 0x84, buffer, interrupt_buf_size, &num_bytes, timeout);
                if (0 == res)
                {
                    // Propagate the data to device layer
                    for (auto & sub : subdevices)
                        if (sub->channel_data_callback)
                            sub->channel_data_callback(buffer, num_bytes);
                }
                else
                {
                    if (res == LIBUSB_ERROR_TIMEOUT) 
                        LOG_WARNING("interrupt e.p. timeout");
                    else 
                        throw std::runtime_error(to_string() << "USB Interrupt end-point error " << libusb_strerror((libusb_error)res));
                }
            }

        };

        struct device
        {
            const std::shared_ptr<context> parent;
            uvc_device_t * uvcdevice;
            int vid, pid;
            std::vector<subdevice> subdevices;
            std::vector<int> claimed_interfaces;

            std::thread data_channel_thread;
            volatile bool data_stop;


            std::shared_ptr<device> aux_device;

            libusb_device_handle * usb_handle;

            device(std::shared_ptr<context> parent, uvc_device_t * uvcdevice) : parent(parent), uvcdevice(uvcdevice), usb_handle()
            {
                get_subdevice(0);
                
                uvc_device_descriptor_t * desc;
                CALL_UVC(uvc_get_device_descriptor, uvcdevice, &desc);
                vid = desc->idVendor;
                pid = desc->idProduct;
                uvc_free_device_descriptor(desc);
            }
            ~device()
            {
                for(auto interface_number : claimed_interfaces)
                {
                    int status = libusb_release_interface(get_subdevice(0).handle->usb_devh, interface_number);
                    if(status < 0) LOG_ERROR("libusb_release_interface(...) returned " << libusb_error_name(status));
                }

                for(auto & sub : subdevices) if(sub.handle) uvc_close(sub.handle);
                if(claimed_interfaces.size()) if(uvcdevice) uvc_unref_device(uvcdevice);
            }

            subdevice & get_subdevice(int subdevice_index)
            {
                if (subdevice_index >= subdevices.size()) subdevices.resize(subdevice_index + 1);

                if (subdevice_index == 3 && aux_device)
                {
                    auto& sub = aux_device->get_subdevice(0);
                    subdevices[subdevice_index] = sub;
                    return sub;
                }

                if (!subdevices[subdevice_index].handle) check("uvc_open2", uvc_open2(uvcdevice, &subdevices[subdevice_index].handle, subdevice_index));
                return subdevices[subdevice_index];
            }

            void start_data_acquisition()
            {
                data_stop = false;
                std::vector<subdevice *> data_channel_subs;
                for (auto & sub : subdevices)
                {
                    if (sub.channel_data_callback)
                    {
                        data_channel_subs.push_back(&sub);
                    }
                }

                // Motion events polling pipe
                if (claimed_interfaces.size())
                {
                    data_channel_thread = std::thread([this, data_channel_subs]()
                    {
                        // Polling 100ms timeout
                        while (!data_stop)
                        {
                            subdevice::poll_interrupts(this->usb_handle, data_channel_subs, 100);
                        }
                    });
                }
            }

            void stop_data_acquisition()
            {
                if (data_channel_thread.joinable())
                {
                    data_stop = true;
                    data_channel_thread.join();
                    data_stop = false;
                }
            }
        };

        ////////////
        // device //
        ////////////

        int get_vendor_id(const device & device) { return device.vid; }
        int get_product_id(const device & device) { return device.pid; }

        std::string get_usb_port_id(const device & device)
        {
            std::string usb_port = std::to_string(libusb_get_bus_number(device.uvcdevice->usb_dev)) + "-" +
                std::to_string(libusb_get_port_number(device.uvcdevice->usb_dev));
            return usb_port;
        }

        void get_control(const device & dev, const extension_unit & xu, uint8_t ctrl, void * data, int len)
        {
            int status = uvc_get_ctrl(const_cast<device &>(dev).get_subdevice(xu.subdevice).handle, xu.unit, ctrl, data, len, UVC_GET_CUR);
            if(status < 0) throw std::runtime_error(to_string() << "uvc_get_ctrl(...) returned " << libusb_error_name(status));
        }

        void set_control(device & device, const extension_unit & xu, uint8_t ctrl, void * data, int len)
        {
            int status = uvc_set_ctrl(device.get_subdevice(xu.subdevice).handle, xu.unit, ctrl, data, len);
            if(status < 0) throw std::runtime_error(to_string() << "uvc_set_ctrl(...) returned " << libusb_error_name(status));
        }

        void claim_interface(device & device, const guid & interface_guid, int interface_number)
        {
            libusb_device_handle* dev_h = nullptr;

            if (device.pid == ZR300_CX3_PID)
            {
                dev_h = device.usb_handle;
            }
            else
            {
                dev_h = device.get_subdevice(0).handle->usb_devh;
            }
            int status = libusb_claim_interface(dev_h, interface_number);
            if (status < 0) throw std::runtime_error(to_string() << "libusb_claim_interface(...) returned " << libusb_error_name(status));
            device.claimed_interfaces.push_back(interface_number);
        }

        void claim_aux_interface(device & device, const guid & interface_guid, int interface_number)
        {
            claim_interface(device, interface_guid, interface_number);
        }

        void bulk_transfer(device & device, unsigned char endpoint, void * data, int length, int *actual_length, unsigned int timeout)
        {

            libusb_device_handle* dev_h = nullptr;

            if (device.pid == ZR300_CX3_PID) // W/A for ZR300 fish-eye
            {
                dev_h = device.usb_handle;
            }
            else
            {
                dev_h = device.get_subdevice(0).handle->usb_devh;
            }

            int status = libusb_bulk_transfer(dev_h, endpoint, (unsigned char *)data, length, actual_length, timeout);

            if (status < 0) throw std::runtime_error(to_string() << "libusb_bulk_transfer(...) returned " << libusb_error_name(status));

        }

        void set_subdevice_mode(device & device, int subdevice_index, int width, int height, uint32_t fourcc, int fps, video_channel_callback callback)
        {
            auto & sub = device.get_subdevice(subdevice_index);
            check("get_stream_ctrl_format_size", uvc_get_stream_ctrl_format_size(sub.handle, &sub.ctrl, reinterpret_cast<const big_endian<uint32_t> &>(fourcc), width, height, fps));
            sub.callback = callback;
        }

        void set_subdevice_data_channel_handler(device & device, int subdevice_index, data_channel_callback callback)
        {
            device.subdevices[subdevice_index].set_data_channel_cfg(callback);
        }

        void start_streaming(device & device, int num_transfer_bufs)
        {
            for(auto i = 0; i < device.subdevices.size(); i++)
            {
                auto& sub = device.get_subdevice(i);
                if (sub.callback)
                {
                    #if defined (ENABLE_DEBUG_SPAM)
                    uvc_print_stream_ctrl(&sub.ctrl, stdout);
                    #endif

                    check("uvc_start_streaming", uvc_start_streaming(sub.handle, &sub.ctrl, [](uvc_frame * frame, void * user)
                    {
                        reinterpret_cast<subdevice *>(user)->callback(frame->data, []{});
                    }, &sub, 0, num_transfer_bufs));
                }
            }
        }

        void stop_streaming(device & device)
        {
            // Stop all streaming
            for(auto & sub : device.subdevices)
            {
                if(sub.handle) uvc_stop_streaming(sub.handle);
                sub.ctrl = {};
                sub.callback = {};
            }
        }

        void start_data_acquisition(device & device)
        {
            device.start_data_acquisition();
        }

        void stop_data_acquisition(device & device)
        {
            device.stop_data_acquisition();
        }

        template<class T> void set_pu(uvc_device_handle_t * devh, int subdevice, uint8_t unit, uint8_t control, int value)
        {
            const int REQ_TYPE_SET = 0x21;
            unsigned char buffer[4];
            if(sizeof(T)==1) buffer[0] = value;
            if(sizeof(T)==2) SHORT_TO_SW(value, buffer);
            if(sizeof(T)==4) INT_TO_DW(value, buffer);
            int status = libusb_control_transfer(devh->usb_devh, REQ_TYPE_SET, UVC_SET_CUR, control << 8, unit << 8 | (subdevice * 2), buffer, sizeof(T), 0);
            if(status < 0) throw std::runtime_error(to_string() << "libusb_control_transfer(...) returned " << libusb_error_name(status));
            if(status != sizeof(T)) throw std::runtime_error("insufficient data written to usb");
        }

        template<class T> int get_pu(uvc_device_handle_t * devh, int subdevice, uint8_t unit, uint8_t control, int uvc_get_thing)
        {
            const int REQ_TYPE_GET = 0xa1;
            unsigned char buffer[4];
            int status = libusb_control_transfer(devh->usb_devh, REQ_TYPE_GET, uvc_get_thing, control << 8, unit << 8 | (subdevice * 2), buffer, sizeof(T), 0);
            if(status < 0) throw std::runtime_error(to_string() << "libusb_control_transfer(...) returned " << libusb_error_name(status));
            if(status != sizeof(T)) throw std::runtime_error("insufficient data read from usb");
            if(sizeof(T)==1) return buffer[0];
            if(sizeof(T)==2) return SW_TO_SHORT(buffer);
            if(sizeof(T)==4) return DW_TO_INT(buffer);
        }
        template<class T> void get_pu_range(uvc_device_handle_t * devh, int subdevice, uint8_t unit, uint8_t control, int * min, int * max, int * step, int * def)
        {
            if(min)     *min    = get_pu<T>(devh, subdevice, unit, control, UVC_GET_MIN);
            if(max)     *max    = get_pu<T>(devh, subdevice, unit, control, UVC_GET_MAX);
            if(step)    *step   = get_pu<T>(devh, subdevice, unit, control, UVC_GET_RES);
            if(def)     *def    = get_pu<T>(devh, subdevice, unit, control, UVC_GET_DEF);
        }

        void get_pu_control_range(const device & device, int subdevice, rs_option option, int * min, int * max, int * step, int * def)
        {
            auto handle = const_cast<uvc::device &>(device).get_subdevice(subdevice).handle;
            int ct_unit = 0, pu_unit = 0;
            for(auto ct = uvc_get_input_terminals(handle); ct; ct = ct->next) ct_unit = ct->bTerminalID; // todo - Check supported caps
            for(auto pu = uvc_get_processing_units(handle); pu; pu = pu->next) pu_unit = pu->bUnitID; // todo - Check supported caps

            switch(option)
            {
            case RS_OPTION_COLOR_BACKLIGHT_COMPENSATION: return get_pu_range<uint16_t>(handle, subdevice, pu_unit, UVC_PU_BACKLIGHT_COMPENSATION_CONTROL, min, max, step, def);
            case RS_OPTION_COLOR_BRIGHTNESS: return get_pu_range<int16_t>(handle, subdevice, pu_unit, UVC_PU_BRIGHTNESS_CONTROL, min, max, step, def);
            case RS_OPTION_COLOR_CONTRAST: return get_pu_range<uint16_t>(handle, subdevice, pu_unit, UVC_PU_CONTRAST_CONTROL, min, max, step, def);
            case RS_OPTION_COLOR_EXPOSURE: return get_pu_range<uint32_t>(handle, subdevice, ct_unit, UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL, min, max, step, def);
            case RS_OPTION_COLOR_GAIN: return get_pu_range<uint16_t>(handle, subdevice, pu_unit, UVC_PU_GAIN_CONTROL, min, max, step, def);
            case RS_OPTION_COLOR_GAMMA: return get_pu_range<uint16_t>(handle, subdevice, pu_unit, UVC_PU_GAMMA_CONTROL, min, max, step, def);
            case RS_OPTION_COLOR_HUE: if(min) *min = 0; if(max) *max = 0; return; //return get_pu_range<int16_t>(handle, subdevice, pu_unit, UVC_PU_HUE_CONTROL, min, max, step, def);
            case RS_OPTION_COLOR_SATURATION: return get_pu_range<uint16_t>(handle, subdevice, pu_unit, UVC_PU_SATURATION_CONTROL, min, max, step, def);
            case RS_OPTION_COLOR_SHARPNESS: return get_pu_range<uint16_t>(handle, subdevice, pu_unit, UVC_PU_SHARPNESS_CONTROL, min, max, step, def);
            case RS_OPTION_COLOR_WHITE_BALANCE: return get_pu_range<uint16_t>(handle, subdevice, pu_unit, UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL, min, max, step, def);
            case RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE: if(min) *min = 0; if(max) *max = 1; if (step) *step = 1; if (def) *def = 1; return; // The next 2 options do not support range operations
            case RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE: if(min) *min = 0; if(max) *max = 1; if (step) *step = 1; if (def) *def = 1; return;
            default: throw std::logic_error("invalid option");
            }
        }        

        void get_extension_control_range(const device & device, const extension_unit & xu, char control, int * min, int * max, int * step, int * def)
        {
            throw std::logic_error("get_extension_control_range(...) is not implemented for this backend");
        }

        void set_pu_control(device & device, int subdevice, rs_option option, int value)
        {            
            auto handle = device.get_subdevice(subdevice).handle;
            int ct_unit = 0, pu_unit = 0;
            for(auto ct = uvc_get_input_terminals(handle); ct; ct = ct->next) ct_unit = ct->bTerminalID; // todo - Check supported caps
            for(auto pu = uvc_get_processing_units(handle); pu; pu = pu->next) pu_unit = pu->bUnitID; // todo - Check supported caps

            switch(option)
            {
            case RS_OPTION_COLOR_BACKLIGHT_COMPENSATION: return set_pu<uint16_t>(handle, subdevice, pu_unit, UVC_PU_BACKLIGHT_COMPENSATION_CONTROL, value);
            case RS_OPTION_COLOR_BRIGHTNESS: return set_pu<int16_t>(handle, subdevice, pu_unit, UVC_PU_BRIGHTNESS_CONTROL, value);
            case RS_OPTION_COLOR_CONTRAST: return set_pu<uint16_t>(handle, subdevice, pu_unit, UVC_PU_CONTRAST_CONTROL, value);
            case RS_OPTION_COLOR_EXPOSURE: return set_pu<uint32_t>(handle, subdevice, ct_unit, UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL, value);
            case RS_OPTION_COLOR_GAIN: return set_pu<uint16_t>(handle, subdevice, pu_unit, UVC_PU_GAIN_CONTROL, value);
            case RS_OPTION_COLOR_GAMMA: return set_pu<uint16_t>(handle, subdevice, pu_unit, UVC_PU_GAMMA_CONTROL, value);
            case RS_OPTION_COLOR_HUE: return; // set_pu<int16_t>(handle, subdevice, pu_unit, UVC_PU_HUE_CONTROL, value); // Causes LIBUSB_ERROR_PIPE, may be related to not being able to set UVC_PU_HUE_AUTO_CONTROL
            case RS_OPTION_COLOR_SATURATION: return set_pu<uint16_t>(handle, subdevice, pu_unit, UVC_PU_SATURATION_CONTROL, value);
            case RS_OPTION_COLOR_SHARPNESS: return set_pu<uint16_t>(handle, subdevice, pu_unit, UVC_PU_SHARPNESS_CONTROL, value);
            case RS_OPTION_COLOR_WHITE_BALANCE: return set_pu<uint16_t>(handle, subdevice, pu_unit, UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL, value);
            case RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE: return set_pu<uint8_t>(handle, subdevice, ct_unit, UVC_CT_AE_MODE_CONTROL, value ? 2 : 1); // Modes - (1: manual) (2: auto) (4: shutter priority) (8: aperture priority)
            case RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE: return set_pu<uint8_t>(handle, subdevice, pu_unit, UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL, value);
            case RS_OPTION_FISHEYE_GAIN: {
                assert(subdevice == 3);
                return set_pu<uint16_t>(handle, 0, pu_unit, UVC_PU_GAIN_CONTROL, value);
            }
            default: throw std::logic_error("invalid option");
            }
        }

        int get_pu_control(const device & device, int subdevice, rs_option option)
        {
            auto handle = const_cast<uvc::device &>(device).get_subdevice(subdevice).handle;
            int ct_unit = 0, pu_unit = 0;
            for(auto ct = uvc_get_input_terminals(handle); ct; ct = ct->next) ct_unit = ct->bTerminalID; // todo - Check supported caps
            for(auto pu = uvc_get_processing_units(handle); pu; pu = pu->next) pu_unit = pu->bUnitID; // todo - Check supported caps

            switch(option)
            {
            case RS_OPTION_COLOR_BACKLIGHT_COMPENSATION: return get_pu<uint16_t>(handle, subdevice, pu_unit, UVC_PU_BACKLIGHT_COMPENSATION_CONTROL, UVC_GET_CUR);
            case RS_OPTION_COLOR_BRIGHTNESS: return get_pu<int16_t>(handle, subdevice, pu_unit, UVC_PU_BRIGHTNESS_CONTROL, UVC_GET_CUR);
            case RS_OPTION_COLOR_CONTRAST: return get_pu<uint16_t>(handle, subdevice, pu_unit,UVC_PU_CONTRAST_CONTROL, UVC_GET_CUR);
            case RS_OPTION_COLOR_EXPOSURE: return get_pu<uint32_t>(handle, subdevice, ct_unit, UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL, UVC_GET_CUR);
            case RS_OPTION_COLOR_GAIN: return get_pu<uint16_t>(handle, subdevice, pu_unit, UVC_PU_GAIN_CONTROL, UVC_GET_CUR);
            case RS_OPTION_COLOR_GAMMA: return get_pu<uint16_t>(handle, subdevice, pu_unit, UVC_PU_GAMMA_CONTROL, UVC_GET_CUR);
            case RS_OPTION_COLOR_HUE: return 0; //get_pu<int16_t>(handle, subdevice, pu_unit, UVC_PU_HUE_CONTROL, UVC_GET_CUR);
            case RS_OPTION_COLOR_SATURATION: return get_pu<uint16_t>(handle, subdevice, pu_unit, UVC_PU_SATURATION_CONTROL, UVC_GET_CUR);
            case RS_OPTION_COLOR_SHARPNESS: return get_pu<uint16_t>(handle, subdevice, pu_unit, UVC_PU_SHARPNESS_CONTROL, UVC_GET_CUR);
            case RS_OPTION_COLOR_WHITE_BALANCE: return get_pu<uint16_t>(handle, subdevice, pu_unit, UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL, UVC_GET_CUR);
            case RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE: return get_pu<uint8_t>(handle, subdevice, ct_unit, UVC_CT_AE_MODE_CONTROL, UVC_GET_CUR) > 1; // Modes - (1: manual) (2: auto) (4: shutter priority) (8: aperture priority)
            case RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE: return get_pu<uint8_t>(handle, subdevice, pu_unit, UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL, UVC_GET_CUR);
            case RS_OPTION_FISHEYE_GAIN: return get_pu<uint16_t>(handle, subdevice, pu_unit, UVC_PU_GAIN_CONTROL, UVC_GET_CUR);
            default: throw std::logic_error("invalid option");
            }
        }

        /////////////
        // context //
        /////////////

        std::shared_ptr<context> create_context()
        {
            return std::make_shared<context>();
        }

        bool is_device_connected(device & device, int vid, int pid)
        {
            return true;
        }

        std::vector<std::shared_ptr<device>> query_devices(std::shared_ptr<context> context)
        {
            std::vector<std::shared_ptr<device>> devices;
            
            uvc_device_t ** list;
            CALL_UVC(uvc_get_device_list, context->ctx, &list);
            for (auto it = list; *it; ++it)
                try
            {
                auto dev = std::make_shared<device>(context, *it);
                dev->usb_handle = libusb_open_device_with_vid_pid(context->usb_context, VID_INTEL_CAMERA, ZR300_FISHEYE_PID);
                devices.push_back(dev);
            }
            catch (std::runtime_error &e)
            {
                LOG_WARNING("usb:" << (int)uvc_get_bus_number(*it) << ':' << (int)uvc_get_device_address(*it) << ": " << e.what());
            }
            uvc_free_device_list(list, 1);

            std::shared_ptr<device> fisheye = nullptr; // Currently ZR300 supports only a single device on OSX

            for (auto& dev : devices)
            {
                if (dev->pid == ZR300_FISHEYE_PID)
                {
                    fisheye = dev;
                }
            }

            for (auto& dev : devices)
            {
                dev->aux_device = fisheye;
            }

            return devices;
        }
    }
}

#endif
