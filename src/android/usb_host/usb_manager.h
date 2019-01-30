#pragma once

#include "usb_host.h"
#include "usb_device.h"
#include <memory>

namespace librealsense
{
    namespace usb_host
    {
        class usb_manager 
        {
        public:

            usb_manager(const usb_manager &arg) = delete; // Copy constructor
            usb_manager(const usb_manager &&arg) = delete;  // Move constructor
            usb_manager &operator=(const usb_manager &arg) = delete; // Assignment operator
            usb_manager &operator=(const usb_manager &&arg) = delete; // Move operator

            static std::vector<std::shared_ptr<usb_device>> get_device_list();

            static std::shared_ptr<usb_device> get_usb_device_from_handle(usb_device_handle *handle);
        };
    }
}
