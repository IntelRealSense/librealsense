// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#ifdef RS2_USE_ANDROID_BACKEND


#include "android-backend.h"
#include "android-uvc.h"
#include "android-hid.h"
#include "../types.h"
#include "usb_host/device_watcher.h"
#include <chrono>
#include <cctype> // std::tolower

namespace librealsense {
    namespace platform {
        android_backend::android_backend() {
        }

        android_backend::~android_backend() {
            try {

            }
            catch (...) {
                // TODO: Write to log
            }
        }

        std::shared_ptr<uvc_device> android_backend::create_uvc_device(uvc_device_info info) const {

            LOG_DEBUG("Creating UVC Device from path: " << info.device_path.c_str());
            return std::make_shared<retry_controls_work_around>(
                    std::make_shared<android_uvc_device>(info, shared_from_this()));
        }

        std::shared_ptr<backend> create_backend()
        {
            return std::make_shared<android_backend>();
        }

        std::vector<uvc_device_info> android_backend::query_uvc_devices() const {
            return usb_host::device_watcher::query_uvc_devices();
        }

        std::shared_ptr<usb_device> android_backend::create_usb_device(usb_device_info info) const {
            throw std::runtime_error("create_usb_device Not supported");
        }

        std::vector<usb_device_info> android_backend::query_usb_devices() const {

            std::vector<usb_device_info> result;
            // Not supported
            return result;
        }

        std::shared_ptr<hid_device> android_backend::create_hid_device(hid_device_info info) const {
            throw std::runtime_error("create_hid_device Not supported");
        }

        std::vector<hid_device_info> android_backend::query_hid_devices() const {
            std::vector<hid_device_info> devices;
            // Not supported 
            return devices;
        }

        std::shared_ptr<time_service> android_backend::create_time_service() const {
            return std::make_shared<os_time_service>();
        }


        std::shared_ptr<device_watcher> android_backend::create_device_watcher() const {
            return std::make_shared<usb_host::device_watcher>();
        }
    }
}

#endif
