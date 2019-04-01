// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device-libusb.h"
#include "endpoint-libusb.h"
#include "interface-libusb.h"
#include "types.h"

#include <string>
#include <regex>
#include <sstream>
#include <mutex>

namespace librealsense
{
    namespace platform
    {
        usb_device_libusb::usb_device_libusb(libusb_device* device, libusb_device_descriptor desc) :
            _device(device), _usb_device_descriptor(desc)
        {
            for (ssize_t c = 0; c < desc.bNumConfigurations; ++c)
            {
                libusb_config_descriptor *config;
                auto rc = libusb_get_config_descriptor(device, c, &config);

                for (ssize_t i = 0; i < config->bNumInterfaces; ++i)
                {
                    auto inf = config->interface[i];
                    _interfaces[i] = std::make_shared<usb_interface_libusb>(inf);
                    _messengers[i] = std::make_shared<usb_messenger_libusb>(device, _interfaces.at(i));
                }

                libusb_free_config_descriptor(config);
            }

            _info.conn_spec = usb_spec(desc.bcdUSB);
            _info.vid = desc.idVendor;
            _info.pid = desc.idProduct;
            auto bus = libusb_get_bus_number(device);
            auto port = libusb_get_port_number(device);
            _info.unique_id = _info.id = std::to_string(bus) + "-" + std::to_string(port);
            //_info.serial = std::string(usb_device_get_serial(_handle, 10));//TODO:MK
            //_info.id = std::string(usb_device_get_name(_handle));
            //_info.unique_id = std::string(usb_device_get_name(_handle));
//            _desc_length = usb_device_get_descriptors_length(_handle);
        }

        void usb_device_libusb::release()
        {
            for(auto&& m : _messengers)
                m.second->release();
            LOG_DEBUG("usb device: " << get_info().unique_id << ", released");
        }
    }
}
