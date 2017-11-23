// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS2_USE_LIBUVC_BACKEND
#include "../include/librealsense2/h/rs_types.h"     // Inherit all type definitions in the public API
#include "backend.h"
#include "types.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <fstream>
#include <regex>
#include <thread>
#include <utility> // for pair
#include <chrono>
#include <thread>
#include <atomic>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <cmath>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fts.h>
#include <regex>
#include <list>

#include <signal.h>

#include "libuvc.h"

#pragma GCC diagnostic ignored "-Wpedantic"
#include "../third-party/libusb/libusb/libusb.h"
#pragma GCC diagnostic pop

#pragma GCC diagnostic ignored "-Woverflow"

struct uvc_device_internal {
    struct uvc_context *ctx;
    int ref;
    libusb_device *usb_dev;
    int interface;
};

namespace librealsense
{
    namespace platform
    {
        static void internal_uvc_callback(uvc_frame_t *frame, void *ptr);

        static std::string get_usb_port_id(libusb_device* usb_device)
        {
            auto usb_bus = std::to_string(libusb_get_bus_number(usb_device));

            // As per the USB 3.0 specs, the current maximum limit for the depth is 7.
            const auto max_usb_depth = 8;
            uint8_t usb_ports[max_usb_depth] = {};
            std::stringstream port_path;
            auto port_count = libusb_get_port_numbers(usb_device, usb_ports, max_usb_depth);
            auto usb_dev = std::to_string(libusb_get_device_address(usb_device));

            for (size_t i = 0; i < port_count; ++i)
            {
                port_path << std::to_string(usb_ports[i]) << (((i+1) < port_count)?".":"");
            }

            return usb_bus + "-" + port_path.str() + "-" + usb_dev;
        }

        class uvclib_usb_device : public usb_device
        {
        public:
            uvclib_usb_device(const usb_device_info& info)
            {
                int status = libusb_init(&_usb_context);
                if(status < 0)
                    throw linux_backend_exception(to_string() << "libusb_init(...) returned " << libusb_error_name(status));


                std::vector<usb_device_info> results;
                uvclib_usb_device::foreach_usb_device(_usb_context,
                                                   [&results, info, this](const usb_device_info& i, libusb_device* dev)
                                                   {
                                                       if (i.unique_id == info.unique_id)
                                                       {
                                                           _usb_device = dev;
                                                           libusb_ref_device(dev);
                                                       }
                                                   });

                _mi = info.mi;
            }

            ~uvclib_usb_device()
            {
                if(_usb_device) libusb_unref_device(_usb_device);
                libusb_exit(_usb_context);
            }

            static void foreach_usb_device(libusb_context* usb_context, std::function<void(
                    const usb_device_info&,
                    libusb_device*)> action)
            {
                // Obtain libusb_device_handle for each device
                libusb_device ** list = nullptr;
                int status = libusb_get_device_list(usb_context, &list);

                if(status < 0)
                    throw linux_backend_exception(to_string() << "libusb_get_device_list(...) returned " << libusb_error_name(status));

                for(int i=0; list[i]; ++i)
                {
                    libusb_device * usb_device = list[i];
                    libusb_config_descriptor *config;
                    status = libusb_get_active_config_descriptor(usb_device, &config);
                    if(status == 0)
                    {
                        auto parent_device = libusb_get_parent(usb_device);
                        //if (parent_device)
                        {
                            usb_device_info info{};
                            std::stringstream ss;
                            info.unique_id = get_usb_port_id(usb_device);
                            info.mi = config->bNumInterfaces - 1; // The hardware monitor USB interface is expected to be the last one
                            action(info, usb_device);
                        }

                        libusb_free_config_descriptor(config);
                    }
                }
                libusb_free_device_list(list, 1);
            }

            std::vector<uint8_t> send_receive(
                    const std::vector<uint8_t>& data,
                    int timeout_ms = 5000,
                    bool require_response = true) override
            {
                libusb_device_handle* usb_handle = nullptr;
                int status = libusb_open(_usb_device, &usb_handle);
                if(status < 0)
                    throw linux_backend_exception(to_string() << "libusb_open(...) returned " << libusb_error_name(status));
                status = libusb_claim_interface(usb_handle, _mi);
                if(status < 0)
                    throw linux_backend_exception(to_string() << "libusb_claim_interface(...) returned " << libusb_error_name(status));

                int actual_length;
                status = libusb_bulk_transfer(usb_handle, 1, const_cast<uint8_t*>(data.data()), data.size(), &actual_length, timeout_ms);
                if(status < 0)
                    throw linux_backend_exception(to_string() << "libusb_bulk_transfer(...) returned " << libusb_error_name(status));

                std::vector<uint8_t> result;


                if (require_response)
                {
                    result.resize(1024);
                    status = libusb_bulk_transfer(usb_handle, 0x81, const_cast<uint8_t*>(result.data()), result.size(), &actual_length, timeout_ms);
                    if(status < 0)
                        throw linux_backend_exception(to_string() << "libusb_bulk_transfer(...) returned " << libusb_error_name(status));

                    result.resize(actual_length);
                }

                libusb_close(usb_handle);

                return result;
            }

        private:
            libusb_context* _usb_context;
            libusb_device* _usb_device;
            int _mi;
        };

        class libuvc_uvc_device;

        /* callback context send for each frame for its specific profile */
        struct callback_context {
          frame_callback _callback;
          stream_profile _profile;
          libuvc_uvc_device *_this;
        };

        /* implements uvc_device for libUVC support */
        class libuvc_uvc_device : public uvc_device
        {
        public:
            static void foreach_uvc_device(uvc_context_t *ctx,
                    std::function<void(
                            const uvc_device_info &info,
                                       const std::string&)> action)
            {
                uvc_error_t res;
                uvc_device_t **device_list;
                uvc_device_internal *dev;
                uvc_device_info info;
                uvc_device_descriptor_t *device_desc;

                // get all uvc devices.
                res = uvc_get_device_list(ctx, &device_list);

                if (res < 0) {
                    throw linux_backend_exception("fail to get uvc device list.");
                }

                int i = 0;
                // iterate over all devices.
                while (device_list[i] != NULL) {
                    // get the internal device object so we can use the libusb handle.
                    dev = (uvc_device_internal *) device_list[i];
                    // set up the unique id for the device.
                    info.unique_id = get_usb_port_id(dev->usb_dev);

                    // get device descriptor.
                    res = uvc_get_device_descriptor((uvc_device_t *)dev, &device_desc);
                    if ( res < 0) {
                        uvc_free_device_list(device_list, 0);
                        throw linux_backend_exception("fail to get device descriptor");
                    }

                    // set up device definitions.
                    info.pid = device_desc->idProduct;
                    info.vid = device_desc->idVendor;
                    info.mi = dev->interface-1;

                    uvc_free_device_descriptor(device_desc);

                    // declare this device to the caller.
                    action(info, "Intel Camera");

                    // this code was used for the sr300 version.
/*                    libusb_config_descriptor *config;
                    int status = libusb_get_active_config_descriptor(dev->usb_dev, &config);

                    if (config->bNumInterfaces >= 2)
                    {
                        info.mi = 2;
                        action(info, "depth");
                    }

                    libusb_free_config_descriptor(config);*/

                    i++;

                }
                uvc_free_device_list(device_list, 0);

            }

            /* responsible for a specific uvc device.*/
            libuvc_uvc_device(const uvc_device_info& info)
                    : _name(""), _info(),
                      _is_capturing(false)
            {
                uvc_error_t res;
                // init libUVC
                res = uvc_init(&_ctx, NULL);

                if (res < 0) {
                    throw linux_backend_exception("fail to initialize libuvc");
                }

                // search the device in device list.
                foreach_uvc_device(_ctx, [&info, this](const uvc_device_info& i, const std::string& name)
                                   {
                                       if (i == info)
                                       {
                                           _name = name;
                                           _info = i;
                                           _device_path = i.device_path;
                                           _interface = i.mi;
                                       }
                                   });
                if (_name == "")
                    throw linux_backend_exception("device is no longer connected!");
            }

            ~libuvc_uvc_device()
            {
                _is_capturing = false;
                uvc_exit(_ctx);
            }

            /* request to set up a streaming profile and its calback */
            void probe_and_commit(stream_profile profile, frame_callback callback, int buffers) override
            {
                uvc_error_t res;
                uvc_stream_ctrl_t ctrl;

                // request all formats for all pins in the device.
                res = uvc_get_stream_ctrl_format_size_all(
                        _device_handle, &ctrl,
                        profile.format,
                        profile.width,
                        profile.height,
                        profile.fps);

                if (res < 0) {
                    uvc_close(_device_handle);
                    uvc_unref_device(_device);
                    throw linux_backend_exception(
                            "Could not get stream format.");
                }

                // add to the vector of profiles.
                _profiles.push_back(profile);
                _callbacks.push_back(callback);
                _stream_ctrls.push_back(ctrl);
            }

            /* request to start streaming*/
            void stream_on(std::function<void(const notification& n)> error_handler) override
            {
              uvc_error_t res;
              // loop over each prfile and start streaming.
              for (auto i=0;i< _profiles.size(); ++i) {
                callback_context *context = new callback_context();
                context->_callback = _callbacks[i];
                context->_this = this;
                context->_profile = _profiles[i];

                res = uvc_start_streaming(_device_handle,
                                          &_stream_ctrls[i],
                                          internal_uvc_callback,
                                          context,
                                          0);

                if (res < 0) {
                  throw linux_backend_exception(
                          "fail to start streaming.");

                }
              }
            }

            void start_callbacks() override
            {
                _is_started = true;
            }

            void stop_callbacks() override
            {
                _is_started = false;
            }

            void close(stream_profile) override
            {
                // TODO : support close of a stream.
                if(_is_capturing)
                {
                    _is_capturing = false;
                    _is_started = false;
                }

            }

            void set_power_state(power_state state) override {
                uvc_error_t res;
                uvc_format_t *formats;
                /* if power became on and it was originally off. open the uvc device. */
                if (state == D0 && _state == D3) {
                    res = uvc_find_device(_ctx, &_device, _info.vid, _info.pid, NULL);

                    if (res < 0) {
                        throw linux_backend_exception(
                                "Could not find the device.");
                    }
                    res = uvc_open2(_device, &_device_handle, _interface);

                    if (res < 0) {
                        uvc_unref_device(_device);
                        _device = NULL;
                        throw linux_backend_exception(
                                "Could not open device.");
                    }
                }
                else {
                    // we have been asked to close the device.
                    uvc_unref_device(_device);
                    uvc_stop_streaming(_device_handle);
                    _profiles.clear();
                    uvc_close(_device_handle);
                    _device = NULL;
                    _device_handle = NULL;


                }

                _state = state;
            }
            power_state get_power_state() const override { return _state; }

            void init_xu(const extension_unit& xu) override {
              // TODO : implement.
            }
            bool set_xu(const extension_unit& xu, uint8_t control, const uint8_t* data, int size) override
            {
              // TODO : implement.
                return true;
            }
            bool get_xu(const extension_unit& xu, uint8_t control, uint8_t* data, int size) const override
            {
              // TODO : implement.
                return true;
            }
            control_range get_xu_range(const extension_unit& xu, uint8_t control, int len) const override
            {
              // TODO : implement.
                control_range result{};
                return result;
            }

            bool get_pu(rs2_option opt, int32_t& value) const override
            {
              // TODO : implement.
                return true;
            }

            bool set_pu(rs2_option opt, int32_t value) override
            {
              // TODO : implement.
                return true;
            }

            control_range get_pu_range(rs2_option option) const override
            {
              // TODO : implement.
                control_range result{};

                return result;
            }

            std::vector<stream_profile> get_profiles() const override
            {
                std::vector<stream_profile> results;
                uvc_error_t res;
                uvc_format_t *formats;

               // receive all available formats from the device.
                res = uvc_get_available_formats_all(_device_handle, &formats);

                if (res < 0) {
                    throw linux_backend_exception(
                            "Couldn't get available formats from device");
                }

                uvc_format_t *cur_format = formats;
              // build a list of all stream profiles and return to the caller.
                while ( cur_format != NULL) {
                    stream_profile p{};
                    p.format = cur_format->fourcc;
                    p.fps = cur_format->fps;
                    p.width = cur_format->width;
                    p.height = cur_format->height;
                    results.push_back(p);
                    cur_format = cur_format->next;
                }

                uvc_free_formats(formats);

                return results;
            }

            void lock() const override
            {
            }
            void unlock() const override
            {
            }

            std::string get_device_location() const override { return _device_path; }

            /* recieved a frame and call the callback. */
            void uvc_callback(uvc_frame_t *frame, frame_callback callback, stream_profile profile) {
                frame_object fo{ frame->data_bytes,
                                 0,
                                 frame->data,
                                 nullptr };

                callback(profile, fo,
                          []() mutable {} );
            }

        private:
            power_state _state = D3;
            std::string _name;
            std::string _device_path;
            uvc_device_info _info;

            std::vector<stream_profile> _profiles;
            std::vector<frame_callback> _callbacks;
            std::vector<uvc_stream_ctrl_t> _stream_ctrls;
            std::atomic<bool> _is_capturing;
            std::atomic<bool> _is_alive;
            std::atomic<bool> _is_started;
            uvc_context_t *_ctx;
            uvc_device_t *_device;
            uvc_device_handle_t *_device_handle;
            int _interface;
        };

        // receive the original callback and pass it to the right device. this is registered by libUVC.
        static void internal_uvc_callback(uvc_frame_t *frame, void *ptr)
        {
            callback_context *context = (callback_context *) ptr;
            libuvc_uvc_device *device = context->_this;


            device->uvc_callback(frame, context->_callback, context->_profile);
        }

        /* implements backend. provide a libuvc backend. */
        class libuvc_backend : public backend
        {
        public:
            std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const override
            {
                return std::make_shared<libuvc_uvc_device>(info);
            }

            /* query UVC devices on the system */
            std::vector<uvc_device_info> query_uvc_devices() const override
            {
                uvc_error_t res;
                uvc_context_t *ctx;

                // init libUVC.
                res = uvc_init(&ctx, NULL);

                if (res < 0) {
                    throw linux_backend_exception("fail to initialize libuvc");
                }
                // build a list of uvc devices.
                std::vector<uvc_device_info> results;
                libuvc_uvc_device::foreach_uvc_device(ctx,
                        [&results](const uvc_device_info& i, const std::string&)
                        {
                            results.push_back(i);
                        });

                uvc_exit(ctx);
                return results;
            }

            std::shared_ptr<usb_device> create_usb_device(usb_device_info info) const override
            {
                return std::make_shared<uvclib_usb_device>(info);
            }
            /* query USB devices on the system */
            std::vector<usb_device_info> query_usb_devices() const override
            {
                libusb_context * usb_context = nullptr;
                int status = libusb_init(&usb_context);
                if(status < 0)
                    throw linux_backend_exception(to_string() << "libusb_init(...) returned " << libusb_error_name(status));

                std::vector<usb_device_info> results;
                uvclib_usb_device::foreach_usb_device(usb_context,
                                                   [&results](const usb_device_info& i, libusb_device* dev)
                                                   {
                                                       results.push_back(i);
                                                   });
                libusb_exit(usb_context);

                return results;
            }

            std::shared_ptr<hid_device> create_hid_device(hid_device_info info) const override
            {
                return nullptr;
            }

            std::vector<hid_device_info> query_hid_devices() const override
            {
                std::vector<hid_device_info> devices;
                return devices;
            }
            std::shared_ptr<time_service> create_time_service() const override
            {
                return std::make_shared<os_time_service>();
            }

            std::shared_ptr<device_watcher> create_device_watcher() const
            {
                return std::make_shared<polling_device_watcher>(this);
            }
        };


        // create the singleton backend.
        std::shared_ptr<backend> create_backend()
        {
            return std::make_shared<libuvc_backend>();
        }

    }
}

#endif
