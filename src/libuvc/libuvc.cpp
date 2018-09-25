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

#include <fcntl.h>
#include <limits.h>
#include <cmath>
#include <errno.h>
#include <sys/stat.h>
#include <regex>
#include <list>
#include <unordered_map>

#include <signal.h>

#include "libuvc.h"
#include "libuvc_internal.h"

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
        // the table provides the substitution 4CC used when firmware-published formats
        // differ from the recognized scheme
        const std::unordered_map<uint32_t, uint32_t> fourcc_map = {
            { 0x32000000, 0x47524559 },    /* 'GREY' from 'L8  ' */
            { 0x52415738, 0x47524559 },    /* 'GREY' from 'RAW8' */
        };

        static void internal_uvc_callback(uvc_frame_t *frame, void *ptr);

        static std::tuple<std::string,uint16_t>  get_usb_descriptors(libusb_device* usb_device)
        {
            auto usb_bus = std::to_string(libusb_get_bus_number(usb_device));

            // As per the USB 3.0 specs, the current maximum limit for the depth is 7.
            const auto max_usb_depth = 8;
            uint8_t usb_ports[max_usb_depth] = {};
            std::stringstream port_path;
            auto port_count = libusb_get_port_numbers(usb_device, usb_ports, max_usb_depth);
            auto usb_dev = std::to_string(libusb_get_device_address(usb_device));
            auto speed = libusb_get_device_speed(usb_device);
            libusb_device_descriptor dev_desc;
            auto r= libusb_get_device_descriptor(usb_device,&dev_desc);

            for (size_t i = 0; i < port_count; ++i)
            {
                port_path << std::to_string(usb_ports[i]) << (((i+1) < port_count)?".":"");
            }

            return std::make_tuple(usb_bus + "-" + port_path.str() + "-" + usb_dev,dev_desc.bcdUSB);
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
                auto devices = libusb_get_device_list(usb_context, &list);

                if(devices < 0)
                    throw linux_backend_exception(to_string() << "libusb_get_device_list(...) returned " << libusb_error_name(devices));

                for(int i=0; i < devices; ++i)
                {
                    libusb_device * usb_device = list[i];
                    libusb_config_descriptor *config;
                    auto status = libusb_get_active_config_descriptor(usb_device, &config);
                    if(status == 0)
                    {
                        auto parent_device = libusb_get_parent(usb_device);
                        //if (parent_device)
                        {
                            usb_device_info info{};
                            std::stringstream ss;
                            auto usb_params = get_usb_descriptors(usb_device);
                            info.unique_id = std::get<0>(usb_params);
                            info.conn_spec = static_cast<usb_spec>(std::get<1>(usb_params));
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
                    auto usb_params = get_usb_descriptors(dev->usb_dev);
                    info.unique_id = std::get<0>(usb_params);
                    info.conn_spec = static_cast<usb_spec>(std::get<1>(usb_params));

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

                    i++;

                }
                uvc_free_device_list(device_list, 0);

            }

            /* responsible for a specific uvc device.*/
            libuvc_uvc_device(const uvc_device_info& info)
                    : _info(),
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
                                           _device_usb_spec = i.conn_spec;
                                       }
                                   });
                if (_name == "") {
                    throw linux_backend_exception("device is no longer connected!");
                }

                _state_change_time = 0;
                _is_power_thread_alive = true;
                _thread_handle = std::thread(std::bind(&libuvc_uvc_device::power_thread,this));
            }

            ~libuvc_uvc_device()
            {
                _is_power_thread_alive = false;
                _thread_handle.join();
                _is_capturing = false;
                uvc_exit(_ctx);
            }

            /* request to set up a streaming profile and its calback */
            void probe_and_commit(stream_profile profile, frame_callback callback, int buffers) override
            {
                uvc_error_t res;
                uvc_stream_ctrl_t ctrl;

                // Assume that the base and the substituted codes do not co-exist
                if (_substitute_4cc.count(profile.format))
                    profile.format = _substitute_4cc.at(profile.format);

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
                    throw linux_backend_exception("Could not get stream format.");
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
                for (auto i=0; i < _profiles.size(); ++i) {
                    callback_context *context = new callback_context();
                    context->_callback = _callbacks[i];
                    context->_this = this;
                    context->_profile = _profiles[i];

                    res = uvc_start_streaming(_device_handle,
                                              &_stream_ctrls[i],
                                              internal_uvc_callback,
                                              context,
                                              0);

                    if (res < 0) throw linux_backend_exception("fail to start streaming.");
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
                uvc_stop_streaming(_device_handle);
                _stream_ctrls.clear();
                _profiles.clear();
                _callbacks.clear();
                
            }

            void power_D0() {
                uvc_error_t res;
                uvc_format_t *formats;

                res = uvc_find_device(_ctx, &_device, _info.vid, _info.pid, NULL,
                                      [&](uvc_device_t* device){
                    auto dev = (uvc_device_internal *) device;
                    auto usb_params = get_usb_descriptors(dev->usb_dev);
                    return _info.unique_id == std::get<0>(usb_params);
                });

                if (res < 0)
                    throw linux_backend_exception("Could not find the device.");
                res = uvc_open2(_device, &_device_handle, _interface);

                if (res < 0) {
                    uvc_unref_device(_device);
                    _device = NULL;
                    throw linux_backend_exception("Could not open device.");
                }

                for(auto ct = uvc_get_input_terminals(_device_handle);
                    ct; ct = ct->next) {
                    _input_terminal = ct->bTerminalID;
                }

                for(auto pu = uvc_get_processing_units(_device_handle);
                    pu; pu = pu->next) {
                    _processing_unit = pu->bUnitID;
                }

                for(auto eu = uvc_get_extension_units(_device_handle);
                    eu; eu = eu->next) {
                    _extension_unit = eu->bUnitID;
                }

                _real_state = D0;
            }

            void power_D3() {
                uvc_unref_device(_device);
                //uvc_stop_streaming(_device_handle);
                //_profiles.clear();
                uvc_close(_device_handle);
                _device = NULL;
                _device_handle = NULL;
                _real_state = D3;
            }

            void set_power_state(power_state state) override {
                std::lock_guard<std::mutex> lock(_power_mutex);

                /* if power became on and it was originally off. open the uvc device. */
                if (state == D0 && _state == D3) {

                    // disable change state aggregation in case exists at the moment.
                    _state_change_time = 0;

                    if ( _real_state == D3) {
                        power_D0();
                    }
                }
                else {
                    // we have been asked to close the device. queue the request for several seconds
                    // just in case a quick turn on come right over.
                    _state_change_time = std::clock();
                }

              _state = state;

            }
            power_state get_power_state() const override { return _state; }

            void init_xu(const extension_unit& xu) override {
                // TODO : implement.
            };
            bool set_xu(const extension_unit& xu, uint8_t control, const uint8_t* data, int size) override
            {
                int status =
                    uvc_set_ctrl(_device_handle, xu.unit, control, (void *)data, size);

                if ( status >= 0) {
                    return true;
                }
                return false;

            }
            bool get_xu(const extension_unit& xu, uint8_t control, uint8_t* data, int size) const override
            {
                int status =
                        uvc_get_ctrl(_device_handle, xu.unit, control, data, size, UVC_GET_CUR);

                if ( status >= 0) {
                    return true;
                }
                return false;
            }
            control_range get_xu_range(const extension_unit& xu, uint8_t control, int len) const override
            {
                int status;
                int32_t value;
                int max;
                int min;
                int step;
                int def;
                status = uvc_get_ctrl(_device_handle, xu.unit, control, &max, sizeof(int32_t), UVC_GET_MAX);
                if ( status < 0) {
                    throw std::runtime_error(
                            to_string() << "uvc_get_ctrl(...) returned for UVC_GET_MAX "
                                        << status);
                }

                status =uvc_get_ctrl(_device_handle, xu.unit, control, &min, sizeof(int32_t), UVC_GET_MIN);
                if ( status < 0) {
                    throw std::runtime_error(
                            to_string() << "uvc_get_ctrl(...) returned for UVC_GET_MIN"
                                        << status);
                }

                status = uvc_get_ctrl(_device_handle, xu.unit, control, &step, sizeof(int32_t), UVC_GET_RES);
                if ( status < 0) {
                    throw std::runtime_error(
                            to_string() << "uvc_get_ctrl(...) returned for UVC_GET_RES"
                                        << status);
                }

                status = uvc_get_ctrl(_device_handle, xu.unit, control, &def, sizeof(int32_t), UVC_GET_DEF);
                if ( status < 0) {
                    throw std::runtime_error(
                            to_string() << "uvc_get_ctrl(...) returned for UVC_GET_DEF"
                                        << status);
                }

                control_range result(min, max, step, def);
                return result;
            }

            int rs2_option_to_ctrl_selector(rs2_option option, int &unit) const {
                // chances are that we will need to adjust some of these operation to the
                // processing unit anod some to the camera terminal.
                unit = _processing_unit;
                switch(option) {
                    case RS2_OPTION_BACKLIGHT_COMPENSATION:
                        return UVC_PU_BACKLIGHT_COMPENSATION_CONTROL;
                    case RS2_OPTION_BRIGHTNESS:
                        return UVC_PU_BRIGHTNESS_CONTROL;
                    case RS2_OPTION_CONTRAST:
                        return UVC_PU_CONTRAST_CONTROL;
                    case RS2_OPTION_EXPOSURE:
                        unit = _input_terminal;
                        return UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL;
                    case RS2_OPTION_GAIN:
                        return UVC_PU_GAIN_CONTROL;
                    case RS2_OPTION_GAMMA:
                        return UVC_PU_GAMMA_CONTROL;
                    case RS2_OPTION_HUE:
                        return UVC_PU_HUE_CONTROL;
                    case RS2_OPTION_SATURATION:
                        return UVC_PU_SATURATION_CONTROL;
                    case RS2_OPTION_SHARPNESS:
                        return UVC_PU_SHARPNESS_CONTROL;
                    case RS2_OPTION_WHITE_BALANCE:
                        return UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL;
                    case RS2_OPTION_ENABLE_AUTO_EXPOSURE:
                        unit = _input_terminal;
                        return UVC_CT_AE_MODE_CONTROL; // Automatic gain/exposure control
                    case RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE:
                        return UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL;
                    case RS2_OPTION_POWER_LINE_FREQUENCY :
                        return UVC_PU_POWER_LINE_FREQUENCY_CONTROL;
                    case RS2_OPTION_AUTO_EXPOSURE_PRIORITY:
                        unit = _input_terminal;
                        return UVC_CT_AE_PRIORITY_CONTROL;
                    default:
                        throw linux_backend_exception(to_string() << "invalid option : " << option);
                }
            }

            int32_t get_data_usb( uvc_req_code action, int control, int unit) const {
                unsigned char buffer[4];

                int status = libusb_control_transfer(_device_handle->usb_devh,
                                                     UVC_REQ_TYPE_GET,
                                                     action,
                                                     control << 8,
                                                     unit << 8 | (_interface),
                                                     buffer,
                                                     sizeof(int32_t), 0);

                if (status < 0) throw std::runtime_error(
                            to_string() << "libusb_control_transfer(...) returned "
                                        << libusb_error_name(status));

                if (status != sizeof(int32_t))
                    throw std::runtime_error("insufficient data read from USB");

                return DW_TO_INT(buffer);
            }

            void set_data_usb( uvc_req_code action, int control, int unit, int value) const {
                unsigned char buffer[4];

                INT_TO_DW(value, buffer);

                int status = libusb_control_transfer(_device_handle->usb_devh,
                                                     UVC_REQ_TYPE_SET,
                                                     action,
                                                     control << 8,
                                                     unit << 8 | (_interface),
                                                     buffer,
                                                     sizeof(int32_t), 0);

                if (status < 0) throw std::runtime_error(
                            to_string() << "libusb_control_transfer(...) returned "
                                        << libusb_error_name(status));

                if (status != sizeof(int32_t))
                    throw std::runtime_error("insufficient data writen to USB");
            }

            bool get_pu(rs2_option opt, int32_t& value) const override
            {
                int unit;
                int control = rs2_option_to_ctrl_selector(opt, unit);

                value = get_data_usb( UVC_GET_CUR, control, unit);
                return true;
            }

            bool set_pu(rs2_option opt, int32_t value) override
            {
                int unit;
                int control = rs2_option_to_ctrl_selector(opt, unit);

                set_data_usb( UVC_SET_CUR, control, unit, value);
                return true;
            }

            control_range get_pu_range(rs2_option option) const override
            {
                int unit;
                int control = rs2_option_to_ctrl_selector(option, unit);
                int min = get_data_usb( UVC_GET_MIN, control, unit);
                int max = get_data_usb( UVC_GET_MAX, control, unit);
                int step = get_data_usb( UVC_GET_RES, control, unit);
                int def = get_data_usb( UVC_GET_DEF, control, unit);

                control_range result(min, max, step, def);

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
                    // Substitude HW profiles with 4CC codes recognized by the core
                    uint32_t device_fourcc = cur_format->fourcc;
                    if (fourcc_map.count(device_fourcc))
                    {
                        _substitute_4cc[fourcc_map.at(device_fourcc)] = device_fourcc;
                        device_fourcc = fourcc_map.at(device_fourcc);
                    }

                    stream_profile p{};
                    p.format = device_fourcc;
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

            usb_spec get_usb_specification() const override { return _device_usb_spec; }

            /* received a frame and call the callback. */
            void uvc_callback(uvc_frame_t *frame, frame_callback callback, stream_profile profile) {
                frame_object fo{ frame->data_bytes,
                                 frame->metadata_bytes,
                                 frame->data,
                                 frame->metadata };

                callback(profile, fo,
                          []() mutable {} );
            }

          void power_thread() {
              do {
                std::this_thread::sleep_for(std::chrono::seconds(1));

                std::lock_guard<std::mutex> lock(_power_mutex);

                if (_state_change_time != 0) {
                    clock_t now_time = std::clock();

                    if (now_time - _state_change_time > 1000 ) {

                        // power state should change.
                        _state_change_time = 0;

                        if (_real_state == D0) {
                            power_D3();
                            _real_state = D3;

                        }
                    }
                }
            } while(_is_power_thread_alive);
          }

        private:

            std::mutex _power_mutex;
            std::thread _thread_handle;
            std::atomic<bool> _is_power_thread_alive;
            power_state _real_state = D3;
            std::clock_t _state_change_time;

            power_state _state = D3;
            std::string _name = "";
            std::string _device_path = "";
            usb_spec _device_usb_spec = usb_undefined;
            uvc_device_info _info;

            std::vector<stream_profile> _profiles;
            std::vector<frame_callback> _callbacks;
            std::vector<uvc_stream_ctrl_t> _stream_ctrls;
            mutable std::unordered_map<uint32_t, uint32_t> _substitute_4cc;
            std::atomic<bool> _is_capturing;
            std::atomic<bool> _is_alive;
            std::atomic<bool> _is_started;
            uvc_context_t *_ctx;
            uvc_device_t *_device;
            uvc_device_handle_t *_device_handle;
            int _input_terminal;
            int _processing_unit;
            int _extension_unit;
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
                return std::make_shared<retry_controls_work_around>(
                    std::make_shared<libuvc_uvc_device>(info));
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
