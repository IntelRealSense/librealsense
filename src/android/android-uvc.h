// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "../backend.h"
#include "usb_host/android_uvc.h"
#include <unordered_map>
#include <mutex>
#include <atomic>

#define UVC_AE_MODE_D0_MANUAL   ( 1 << 0 )
#define UVC_AE_MODE_D1_AUTO     ( 1 << 1 )
#define UVC_AE_MODE_D2_SP       ( 1 << 2 )
#define UVC_AE_MODE_D3_AP       ( 1 << 3 )

struct usbhost_uvc_device;

namespace librealsense
{
    namespace platform
    {
        class android_backend;

        struct profile_and_callback
        {
            stream_profile profile;
            frame_callback callback = nullptr;
        };

        typedef std::function<void(const uvc_device_info&)> uvc_enumeration_callback;

        class android_uvc_device : public std::enable_shared_from_this<android_uvc_device>,
                               public uvc_device
        {
        public:
            android_uvc_device(const uvc_device_info& info, std::shared_ptr<const android_backend> backend);
            ~android_uvc_device();

            //std::vector<uvc_device_info> query_uvc_devices() const;

            void probe_and_commit(stream_profile profile, frame_callback callback, int buffers) override;

            // open thread and take frames
            void stream_on(std::function<void(const notification& n)> error_handler = [](const notification& n){}) override;

            void start_callbacks() override;
            void stop_callbacks() override;
            void close(stream_profile profile) override;
            void set_power_state(power_state state) override;
            power_state get_power_state() const override { return _power_state; }
            std::vector<stream_profile> get_profiles() const override;

            static bool is_connected(const uvc_device_info& info);

            void init_xu(const extension_unit& xu) override;
            bool set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len) override;
            bool get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const override;
            control_range get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const override;

            bool get_pu(rs2_option opt, int32_t& value) const override;
            bool set_pu(rs2_option opt, int value) override;
            control_range get_pu_range(rs2_option opt) const override;

            void lock() const override { _systemwide_lock.lock(); }
            void unlock() const override {_systemwide_lock.unlock(); }

            std::string get_device_location() const override { return _location; }
            usb_spec get_usb_specification() const override { return _info.conn_spec; }

        private:
            friend class source_reader_callback;

            int32_t rs2_value_translate(uvc_req_code action, rs2_option option, int32_t value) const;
            void play_profile(stream_profile profile, frame_callback callback);
            void stop_stream_cleanup(const stream_profile& profile, std::vector<profile_and_callback>::iterator& elem);
            void flush(int sIndex);
            void check_connection() const;

            int rs2_option_to_ctrl_selector(rs2_option option, int &unit) const;
            int32_t get_data_usb(uvc_req_code action, int control, int unit, unsigned int length = sizeof(uint32_t)) const;
            void set_data_usb(uvc_req_code action, int control, int unit, int value) const;

            const uvc_device_info                   _info;
            power_state                             _power_state = D3; // power state change is unsupported

            uint16_t                                _streamIndex;
            std::vector<profile_and_callback>       _streams;
            std::mutex                              _streams_mutex;

            std::shared_ptr<const android_backend>      _backend;

            std::string                             _location;
            std::vector<stream_profile>             _profiles;
            std::vector<frame_callback>             _frame_callbacks;
            bool                                    _streaming = false;
            std::atomic<bool>                       _is_started = {false};

            std::shared_ptr<usbhost_uvc_device>     _device = nullptr;

            int _input_terminal = 0;
            int _processing_unit = 0;
            int _extension_unit = 0;
            mutable std::recursive_mutex            _systemwide_lock;
            mutable std::mutex                      _power_mutex;
        };
    }
}
