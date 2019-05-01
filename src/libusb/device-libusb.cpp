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
        std::string get_usb_descriptors(libusb_device* usb_device)
        {
            auto usb_bus = std::to_string(libusb_get_bus_number(usb_device));

            // As per the USB 3.0 specs, the current maximum limit for the depth is 7.
            const auto max_usb_depth = 8;
            uint8_t usb_ports[max_usb_depth] = {};
            std::stringstream port_path;
            auto port_count = libusb_get_port_numbers(usb_device, usb_ports, max_usb_depth);
            auto usb_dev = std::to_string(libusb_get_device_address(usb_device));
            auto speed = libusb_get_device_speed(usb_device);
            libusb_device_descriptor dev_desc;
            auto r= libusb_get_device_descriptor(usb_device,&dev_desc);

            for (size_t i = 0; i < port_count; ++i)
            {
                port_path << std::to_string(usb_ports[i]) << (((i+1) < port_count)?".":"");
            }

            return usb_bus + "-" + port_path.str() + "-" + usb_dev;
        }

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
                    if(inf.altsetting->bInterfaceSubClass != USB_SUBCLASS_CONTROL && inf.altsetting->bInterfaceSubClass != USB_SUBCLASS_HWM)
                        continue;
                    usb_device_info info{};
                    info.id = get_usb_descriptors(device);
                    info.unique_id = get_usb_descriptors(device);
                    info.conn_spec = usb_spec(desc.bcdUSB);
                    info.vid = desc.idVendor;
                    info.pid = desc.idProduct;
                    info.mi = i;
                    _infos.push_back(info);
                }

                libusb_free_config_descriptor(config);
            }
        }

        void usb_device_libusb::release()
        {
            LOG_DEBUG("usb device: " << get_info().unique_id << ", released");
        }

        const std::shared_ptr<usb_messenger> usb_device_libusb::open()
        {
            return std::make_shared<usb_messenger_libusb>(shared_from_this());
        }

        const std::vector<std::shared_ptr<usb_interface>> usb_device_libusb::get_interfaces(usb_subclass filter) const
        {
            std::vector<std::shared_ptr<usb_interface>> rv;
            for (auto&& entry : _interfaces)
            {
                auto i = entry.second;
                if(filter == USB_SUBCLASS_ANY ||
                    i->get_subclass() & filter ||
                    (filter == 0 && i->get_subclass() == 0))
                    rv.push_back(i);
            }
            return rv;
        }
    }
}
