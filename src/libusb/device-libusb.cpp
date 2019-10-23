// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device-libusb.h"
#include "types.h"


namespace librealsense
{
    namespace platform
    {
        usb_device_libusb::usb_device_libusb(libusb_device* device, const libusb_device_descriptor& desc, const usb_device_info& info) :
            _device(device), _usb_device_descriptor(desc), _info(info)
        {
            for (uint8_t c = 0; c < desc.bNumConfigurations; ++c)
            {
                libusb_config_descriptor *config{};
                auto ret = libusb_get_config_descriptor(device, c, &config);
                if (LIBUSB_SUCCESS==ret)
                {
                    for (uint8_t i = 0; i < config->bNumInterfaces; ++i)
                    {
                        auto inf = config->interface[i];
                        _interfaces.push_back(std::make_shared<usb_interface_libusb>(inf));
                    }

                    libusb_free_config_descriptor(config);
                }
                else
                    LOG_WARNING("failed to read USB config descriptor: error = " << std::dec << ret);
            }
        }

        usb_device_libusb::~usb_device_libusb()
        {
            if(_device)
                libusb_unref_device(_device);
        }

        void usb_device_libusb::release()
        {
            LOG_DEBUG("usb device: " << get_info().unique_id << ", released");
        }

        const std::shared_ptr<usb_messenger> usb_device_libusb::open()
        {
            return std::make_shared<usb_messenger_libusb>(shared_from_this());
        }
    }
}
