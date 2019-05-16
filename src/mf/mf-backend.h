// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "../backend.h"

namespace librealsense
{
    namespace platform
    {
        class wmf_backend : public std::enable_shared_from_this<wmf_backend>, public backend
        {
        public:
            wmf_backend();
            ~wmf_backend();

            std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const override;
            std::vector<uvc_device_info> query_uvc_devices() const override;

            std::shared_ptr<command_transfer> create_usb_device(usb_device_info info) const override;
            std::vector<usb_device_info> query_usb_devices() const override;

            std::shared_ptr<hid_device> create_hid_device(hid_device_info info) const override;
            std::vector<hid_device_info> query_hid_devices() const override;
            virtual std::shared_ptr<time_service> create_time_service() const override;
            std::shared_ptr<device_watcher> create_device_watcher() const override;
            std::string get_device_serial(uint16_t device_vid, uint16_t device_pid, const std::string& device_uid) const override;

        private:
            std::chrono::high_resolution_clock::time_point _start_time;
        };
    }
}
