// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "../backend.h"
#include "win7-helpers.h"

#include <Sensorsapi.h>
#include <atlcomcli.h>

namespace librealsense
{
    namespace platform
    {
        class wmf_hid_device : public hid_device
        {
        public:
            static void foreach_hid_device(std::function<void(hid_device_info, CComPtr<ISensor>)> action);

            explicit wmf_hid_device(CComPtr<ISensor> sensor) : _sensor(sensor) {}

            void open(const std::vector<hid_profile>&iio_profiles) override;
            void close() override;
            void stop_capture() override;
            void start_capture(hid_callback callback) override;
            std::vector<hid_sensor> get_sensors() override;
            std::vector<uint8_t> get_custom_report_data(const std::string& custom_sensor_name,
                const std::string& report_name,
                custom_sensor_report_field report_field) override;

        private:
            CComPtr<ISensor> _sensor = nullptr;
            CComPtr<ISensorEvents> _cb = nullptr;
        };
    }
}
