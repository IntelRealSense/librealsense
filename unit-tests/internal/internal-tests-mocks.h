// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <vector>
#include <memory>

#include "../../src/backend.h"
#include "../include/librealsense2/h/rs_types.h"     // Inherit all type definitions in the public API
#include "../include/librealsense2/h/rs_option.h"
#include "../src/usb/usb-types.h"
#include "../src/usb/usb-device.h"
#include "../src/hid/hid-types.h"
#include "../src/command_transfer.h"

namespace librealsense
{
    namespace platform
    {
        class mock_uvc_device : public uvc_device
        {
        public:
            mock_uvc_device() {};

            void probe_and_commit(stream_profile profile, frame_callback callback, int buffers) override {};
            void stream_on(std::function<void(const notification & n)> error_handler = [](const notification& n) {}) override {};
            void start_callbacks() override {};
            void stop_callbacks() override {};
            void close(stream_profile profile) override {};
            void set_power_state(power_state state) override {};
            power_state get_power_state() const override { return power_state::D0; }
            std::vector<stream_profile> get_profiles() const override { return std::vector<stream_profile>(); };

            void init_xu(const extension_unit& xu) override {};
            bool set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len) override { return true; };
            bool get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const override { return true; };
            control_range get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const override { return control_range(); };

            bool get_pu(rs2_option opt, int32_t& value) const override { return true; };
            bool set_pu(rs2_option opt, int value) override { return true; };
            control_range get_pu_range(rs2_option opt) const override { return control_range(); };

            void lock() const override {};
            void unlock() const override {};

            std::string get_device_location() const override { return ""; }
            usb_spec get_usb_specification() const override { return usb_spec::usb_undefined; }
        };

        class mock_hid_device : public hid_device
        {
        public:
            void register_profiles(const std::vector<hid_profile>& hid_profiles) override {};
            void open(const std::vector<hid_profile>& hid_profiles) override {};
            void close() override {};
            void stop_capture() override {};
            void start_capture(hid_callback callback) override {};
            std::vector<hid_sensor> get_sensors() override { return std::vector<hid_sensor>(); }
            std::vector<uint8_t> get_custom_report_data(const std::string& custom_sensor_name,
                const std::string& report_name,
                custom_sensor_report_field report_field) override { return std::vector<uint8_t>(); }
        };

        class mock_device_watcher : public device_watcher
        {
        public:
            virtual void start(device_changed_callback callback) override {};
            virtual void stop() override {};
        };

        class mock_command_transfer : public command_transfer
        {
        public:
            std::vector<uint8_t> send_receive(
                const std::vector<uint8_t>& data,
                int timeout_ms = 5000,
                bool require_response = true) override { return std::vector<uint8_t>(); }
        };

        class mock_backend : public backend
        {
            std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const override
            {
                return std::make_shared<mock_uvc_device>();
            }

            std::vector<uvc_device_info> query_uvc_devices() const override
            {
                return std::vector<uvc_device_info>();
            }

            std::shared_ptr<command_transfer> create_usb_device(usb_device_info info) const override
            {
                return std::make_shared<mock_command_transfer>();
            }

            std::vector<usb_device_info> query_usb_devices() const override
            {
                return std::vector<usb_device_info>();
            }

            std::shared_ptr<hid_device> create_hid_device(hid_device_info info) const override
            {
                return std::make_shared<mock_hid_device>();
            }

            std::vector<hid_device_info> query_hid_devices() const override
            {
                return std::vector<hid_device_info>();
            }

            std::shared_ptr<time_service> create_time_service() const override
            {
                return std::make_shared<os_time_service>();
            }

            std::shared_ptr<device_watcher> create_device_watcher() const override
            {
                return std::make_shared<mock_device_watcher>();
            }

        };

        class mock_context : public context
        {
        public:
            mock_context() : context(backend_type::standard) {}

            const backend& get_backend() const override
            {
                return backend;
            }

        protected:
            mock_backend backend;
        };
    }
}
