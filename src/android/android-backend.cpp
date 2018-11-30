// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#ifdef RS2_USE_USBHOST_UVC_BACKEND


#include "android-backend.h"
#include "android-uvc.h"
#include "android-usb.h"
#include "android-hid.h"
#include "../types.h"
#include <chrono>
#include <cctype> // std::tolower
#include <android/usbhost_uvc/usbmanager.h>


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

            LOGD("Creating UVC Device from path: %s",info.device_path.c_str());
            return std::make_shared<retry_controls_work_around>(
                    std::make_shared<android_uvc_device>(info, shared_from_this()));
        }

        std::shared_ptr<backend> create_backend() {

            return std::make_shared<android_backend>();
        }

        std::vector<uvc_device_info> android_backend::query_uvc_devices() const {
            std::vector<uvc_device_info> devices;
            LOGD("Querying USB Devices...");
            UsbManager &usb_manager = UsbManager::getInstance();
            auto dev_list = usb_manager.GetDeviceList();
            for (auto dev: dev_list) {
                UsbConfiguration config = dev->GetConfiguration(0);
                for (int ia = 0; ia < config.GetNumInterfaceAssociations(); ia++) {
                    auto iad = config.GetInterfaceAssociation(ia);
                    if (iad.GetDescriptor()->bFunctionClass == USB_CLASS_VIDEO) {
                        uvc_device_info device_info;
                        device_info.vid = dev->GetVid();
                        device_info.pid = dev->GetPid();
                        device_info.mi = iad.GetMi();
                        device_info.unique_id = dev->GetHandle()->fd;
                        device_info.device_path = std::string(dev->GetHandle()->dev_name);
                        LOGD("Found UVC Device vid:%04x pid:%04x mi:%02x path: %s",device_info.vid,device_info.pid,device_info.mi,device_info.device_path.c_str());
                        devices.push_back(device_info);
                    }
                }
/*                uvc_device_info device_info;
                device_info.vid = dev.GetVid();
                device_info.pid = dev.GetPid();
                device_info.mi = 5;
                device_info.unique_id = dev.GetHandle()->fd;
                device_info.device_path = std::string(dev.GetHandle()->dev_name);
                LOGD("Found UVC Device vid:%04x pid:%04x mi:%02x path: %s",device_info.vid,device_info.pid,device_info.mi,device_info.device_path.c_str());
                devices.push_back(device_info);*/
            }
            LOGD("UVC Devices found: %d",devices.size());
            return devices;
        }

        std::shared_ptr<usb_device> android_backend::create_usb_device(usb_device_info info) const {
            throw std::runtime_error("create_hid_device Not supported");
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
            auto dw=std::make_shared<polling_device_watcher>(this);
            dw->stop();

            return dw;
        }
    }
}

#endif
