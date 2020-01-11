// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <queue>
#include "hid-device.h"

const int USB_REQUEST_COUNT = 1;

namespace librealsense
{
    namespace platform
    {
        std::vector<hid_device_info> query_hid_devices_info()
        {
            std::vector<std::string> hid_sensors = { gyro, accel, custom };

            std::vector<hid_device_info> rv;
            auto usb_devices = platform::usb_enumerator::query_devices_info();
            for (auto&& info : usb_devices) {
                if(info.cls != RS2_USB_CLASS_HID)
                    continue;
                platform::hid_device_info device_info;
                device_info.vid = hexify(info.vid);
                device_info.pid = hexify(info.pid);
                device_info.unique_id = info.unique_id;
                device_info.device_path = info.unique_id;//the device unique_id is the USB port

                for(auto&& hs : hid_sensors)
                {
                    device_info.id = hs;
                    // LOG_INFO("Found HID device: " << std::string(device_info).c_str());
                    rv.push_back(device_info);
                }
            }
            return rv;
        }

        std::shared_ptr<hid_device> create_rshid_device(hid_device_info info)
        {
            auto devices = usb_enumerator::query_devices_info();
            for (auto&& usb_info : devices)
            {
                if(usb_info.unique_id != info.unique_id || usb_info.cls != RS2_USB_CLASS_HID)
                    continue;

                auto dev = usb_enumerator::create_usb_device(usb_info);
                return std::make_shared<rs_hid_device>(dev);
            }

            return nullptr;
        }

        rs_hid_device::rs_hid_device(rs_usb_device usb_device)
            : _usb_device(usb_device),
              _action_dispatcher(10)
        {
            _id_to_sensor[REPORT_ID_GYROMETER_3D] = gyro;
            _id_to_sensor[REPORT_ID_ACCELEROMETER_3D] = accel;
            _id_to_sensor[REPORT_ID_CUSTOM] = custom;
            _sensor_to_id[gyro] = REPORT_ID_GYROMETER_3D;
            _sensor_to_id[accel] = REPORT_ID_ACCELEROMETER_3D;
            _sensor_to_id[custom] = REPORT_ID_CUSTOM;
            _action_dispatcher.start();
        }

        rs_hid_device::~rs_hid_device()
        {
            _action_dispatcher.stop();
        }

        std::vector<hid_sensor> rs_hid_device::get_sensors()
        {
            std::vector<hid_sensor> sensors;

            for (auto& sensor : _hid_profiles)
                sensors.push_back({ sensor.sensor_name });

            return sensors;
        }

        void rs_hid_device::open(const std::vector<hid_profile>& hid_profiles)
        {
            for(auto&& p : hid_profiles)
            {
                set_feature_report(DEVICE_POWER_D0, _sensor_to_id[p.sensor_name], p.frequency);
            }
            _configured_profiles = hid_profiles;
        }

        void rs_hid_device::close()
        {
            set_feature_report(DEVICE_POWER_D4, REPORT_ID_ACCELEROMETER_3D);
            set_feature_report(DEVICE_POWER_D4, REPORT_ID_GYROMETER_3D);
        }

        void rs_hid_device::stop_capture()
        {
            _action_dispatcher.invoke_and_wait([this](dispatcher::cancellable_timer c)
            {
                if(!_running)
                    return;

                _request_callback->cancel();

                _queue.clear();

                for (auto&& r : _requests)
                    _messenger->cancel_request(r);

                _requests.clear();

                _handle_interrupts_thread->stop();

                _messenger.reset();

               _running = false;
            }, [this](){ return !_running; });
        }

        void rs_hid_device::start_capture(hid_callback callback)
        {
            _action_dispatcher.invoke_and_wait([this, callback](dispatcher::cancellable_timer c)
            {
                if(_running)
                    return;

                _callback = callback;

                auto in = get_hid_interface()->get_number();
                _messenger = _usb_device->open(in);

                _handle_interrupts_thread = std::make_shared<active_object<>>([this](dispatcher::cancellable_timer cancellable_timer)
                {
                    handle_interrupt();
                });

                _handle_interrupts_thread->start();

                _request_callback = std::make_shared<usb_request_callback>([&](platform::rs_usb_request r)
                    {
                        _action_dispatcher.invoke([this, r](dispatcher::cancellable_timer c)
                        {
                            if(!_running)
                                return;
                            if(r->get_actual_length() == sizeof(REALSENSE_HID_REPORT))
                            {
                                REALSENSE_HID_REPORT report;
                                memcpy(&report, r->get_buffer().data(), r->get_actual_length());
                                _queue.enqueue(std::move(report));
                            }
                            auto sts = _messenger->submit_request(r);
                            if (sts != platform::RS2_USB_STATUS_SUCCESS)
                                LOG_ERROR("failed to submit UVC request");
                        });

                    });

                _requests = std::vector<rs_usb_request>(USB_REQUEST_COUNT);
                for(auto&& r : _requests)
                {
                    r = _messenger->create_request(get_hid_endpoint());
                    r->set_buffer(std::vector<uint8_t>(sizeof(REALSENSE_HID_REPORT)));
                    r->set_callback(_request_callback);
                }

                _running = true;

                for(auto&& r : _requests)
                    _messenger->submit_request(r);

            }, [this](){ return _running; });
        }

        void rs_hid_device::handle_interrupt()
        {
            REALSENSE_HID_REPORT report;

            if(_queue.dequeue(&report, 10))
            {
                if(std::find_if(_configured_profiles.begin(), _configured_profiles.end(), [&](hid_profile& p)
                {
                    return _id_to_sensor[report.reportId].compare(p.sensor_name) == 0;

                }) != _configured_profiles.end())
                {
                    sensor_data data{};
                    data.sensor = { _id_to_sensor[report.reportId] };

                    hid_data hid{};
                    hid.x = report.x;
                    hid.y = report.y;
                    hid.z = report.z;

                    data.fo.pixels = &(hid.x);
                    data.fo.metadata = &(report.timeStamp);
                    data.fo.frame_size = sizeof(REALSENSE_HID_REPORT);
                    data.fo.metadata_size = sizeof(report.timeStamp);

                    _callback(data);
                }
            }
        }

        usb_status rs_hid_device::set_feature_report(unsigned char power, int report_id, int fps)
        {
            uint32_t transferred;


            int value = (HID_REPORT_TYPE_FEATURE << 8) + report_id;

            FEATURE_REPORT featureReport;
            auto hid_interface = get_hid_interface()->get_number();

            auto dev = _usb_device->open(hid_interface);

            if (!dev)
                return RS2_USB_STATUS_NO_DEVICE;

            auto res = dev->control_transfer(USB_REQUEST_CODE_GET,
                HID_REQUEST_GET_REPORT,
                value,
                hid_interface,
                (uint8_t*) &featureReport,
                sizeof(featureReport),
                transferred,
                1000);

            if(res != RS2_USB_STATUS_SUCCESS)
            {
                LOG_WARNING("control_transfer of USB_REQUEST_CODE_GET failed return value is: " << res);
                return res;
            }

            featureReport.power = power;

            if(fps > 0)
                featureReport.report = (1000 / fps);

            res = dev->control_transfer(USB_REQUEST_CODE_SET,
                HID_REQUEST_SET_REPORT,
                value,
                hid_interface,
                (uint8_t*) &featureReport,
                sizeof(featureReport),
                transferred,
                1000);

            if(res != RS2_USB_STATUS_SUCCESS)
            {
                LOG_WARNING("control_transfer of USB_REQUEST_CODE_SET failed return value is: " << res);
                return res;
            }

            res = dev->control_transfer(USB_REQUEST_CODE_GET,
                HID_REQUEST_GET_REPORT,
                value,
                hid_interface,
                (uint8_t*) &featureReport,
                sizeof(featureReport),
                transferred,
                1000);

            if(res != RS2_USB_STATUS_SUCCESS)
            {
                LOG_WARNING("control_transfer of USB_REQUEST_CODE_GET failed return value is: " << res);
                return res;
            }

            if(featureReport.power != power)
            {
                LOG_WARNING("faild to set power: " << power);
                return RS2_USB_STATUS_OTHER;
            }
            return res;
        }

        rs_usb_interface rs_hid_device::get_hid_interface()
        {
            auto intfs = _usb_device->get_interfaces();

            auto it = std::find_if(intfs.begin(), intfs.end(),
                [](const rs_usb_interface& i) { return i->get_class() == RS2_USB_CLASS_HID; });

            if (it == intfs.end())
                throw std::runtime_error("can't find HID interface of device: " + _usb_device->get_info().id);

            return *it;
        }

        rs_usb_endpoint rs_hid_device::get_hid_endpoint()
        {
            auto hid_interface = get_hid_interface();

            auto ep = hid_interface->first_endpoint(RS2_USB_ENDPOINT_DIRECTION_READ, RS2_USB_ENDPOINT_INTERRUPT);
            if(!ep)
                throw std::runtime_error("can't find HID endpoint of device: " + _usb_device->get_info().id);

            return ep;
        }
    }
}