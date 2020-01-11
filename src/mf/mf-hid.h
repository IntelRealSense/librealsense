// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "../backend.h"
#include "win/win-helpers.h"

#include <Sensorsapi.h>
#include <atlcomcli.h>

namespace librealsense
{
    namespace platform
    {

        class wmf_hid_sensor {
        public:
            wmf_hid_sensor(const hid_device_info& device_info, CComPtr<ISensor> pISensor) :
                _hid_device_info(device_info), _pISensor(pISensor) 
            {
                BSTR fName{};

                auto res = pISensor->GetFriendlyName(&fName);
                if (FAILED(res))
                {
                    fName = L"Unidentified HID Sensor";
                }

                _name = CW2A(fName);
                SysFreeString(fName);
            };

            const std::string& get_sensor_name() const { return _name; }
            const CComPtr<ISensor>& get_sensor() const { return _pISensor; }

            HRESULT start_capture(ISensorEvents* sensorEvents)
            {
                return _pISensor->SetEventSink(sensorEvents);
            }

            HRESULT stop_capture()
            {
                return _pISensor->SetEventSink(NULL);
            }

        private:

            hid_device_info _hid_device_info;
            CComPtr<ISensor> _pISensor;
            std::string _name;
        };

        class wmf_hid_device : public hid_device
        {
        public:
            static void foreach_hid_device(std::function<void(hid_device_info, CComPtr<ISensor>)> action);
            wmf_hid_device(const hid_device_info& info);

            void register_profiles(const std::vector<hid_profile>& hid_profiles) override { _hid_profiles = hid_profiles;}
            void open(const std::vector<hid_profile>&iio_profiles) override;
            void close() override;
            void stop_capture() override;
            void start_capture(hid_callback callback) override;
            std::vector<hid_sensor> get_sensors() override; // Get opened sensors
            std::vector<uint8_t> get_custom_report_data(const std::string& custom_sensor_name, const std::string& report_name, custom_sensor_report_field report_field) override;

        private:

            std::vector<std::shared_ptr<wmf_hid_sensor>> _connected_sensors; // Vector of all connected sensors of this device
            std::vector<std::shared_ptr<wmf_hid_sensor>> _opened_sensors;    // Vector of all opened sensors of this device (subclass of _connected_sensors)
            std::vector<std::shared_ptr<wmf_hid_sensor>> _streaming_sensors; // Vector of all streaming sensors of this device (subclass of _connected_sensors)

            CComPtr<ISensorEvents> _cb = nullptr;
            std::vector<hid_profile> _hid_profiles;
        };
    }
}
