// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include "../backend.h"
#include "hid-types.h"
#include "../usb/usb-messenger.h"
#include "../usb/usb-enumerator.h"
#include "../concurrency.h"
#include "stdio.h"
#include "stdlib.h"

#include <cstring>
#include <string>
#include <chrono>
#include <thread>
#include "../types.h"

namespace librealsense
{
    namespace platform
    {
        std::vector<hid_device_info> query_hid_devices_info();
        std::shared_ptr<hid_device> create_rshid_device(hid_device_info info);

        class rs_hid_device : public hid_device
        {
        public:
            rs_hid_device(rs_usb_device usb_device);
            virtual ~rs_hid_device();

            void register_profiles(const std::vector<hid_profile>& hid_profiles) override { _hid_profiles = hid_profiles; }
            void open(const std::vector<hid_profile>& hid_profiles) override;
            void close() override;
            void stop_capture() override;
            void start_capture(hid_callback callback) override;
            std::vector<hid_sensor> get_sensors() override;
            virtual std::vector<uint8_t> get_custom_report_data(const std::string& custom_sensor_name,
                                                                const std::string& report_name,
                                                                custom_sensor_report_field report_field) override { return {}; }

        private:
            void handle_interrupt();
            rs_usb_endpoint get_hid_endpoint();
            rs_usb_interface get_hid_interface();
            usb_status set_feature_report(unsigned char power, int report_id, int fps = 0);

            bool _running = false;
            dispatcher _action_dispatcher;

            hid_callback _callback;
            rs_usb_device _usb_device;
            rs_usb_messenger _messenger;
            std::vector<rs_usb_request> _requests;
            std::shared_ptr<platform::usb_request_callback> _request_callback;

            std::vector<hid_profile> _hid_profiles;
            std::map<int, std::string> _id_to_sensor;
            std::map<std::string, int> _sensor_to_id;
            std::vector<hid_profile> _configured_profiles;
            single_consumer_queue<REALSENSE_HID_REPORT> _queue;
            std::shared_ptr<active_object<>> _handle_interrupts_thread;
        };
    }
}