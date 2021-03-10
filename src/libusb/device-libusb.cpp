// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device-libusb.h"
#include "types.h"


namespace librealsense
{
    namespace platform
    {
        usb_device_libusb::usb_device_libusb(libusb_device* device, const libusb_device_descriptor& desc, const usb_device_info& info, std::shared_ptr<usb_context> context) :
                _device(device), _usb_device_descriptor(desc), _info(info), _context(context)
        {
            usb_descriptor ud = {desc.bLength, desc.bDescriptorType, std::vector<uint8_t>(desc.bLength)};
            memcpy(ud.data.data(), &desc, desc.bLength);
            _descriptors.push_back(ud);

            for (uint8_t c = 0; c < desc.bNumConfigurations; ++c)
            {
                libusb_config_descriptor *config{};
                auto ret = libusb_get_config_descriptor(device, c, &config);
                if (LIBUSB_SUCCESS != ret)
                {
                    LOG_WARNING("failed to read USB config descriptor: error = " << std::dec << ret);
                    continue;
                }

                std::shared_ptr<usb_interface_libusb> curr_ctrl_intf;
                for (uint8_t i = 0; i < config->bNumInterfaces; ++i)
                {
                    auto inf = config->interface[i];
                    auto curr_inf = std::make_shared<usb_interface_libusb>(inf);
                    _interfaces.push_back(curr_inf);
                    switch (inf.altsetting->bInterfaceClass)
                    {
                        case RS2_USB_CLASS_VIDEO:
                        {
                            if(inf.altsetting->bInterfaceSubClass == RS2_USB_SUBCLASS_VIDEO_CONTROL)
                                curr_ctrl_intf = curr_inf;
                            if(inf.altsetting->bInterfaceSubClass == RS2_USB_SUBCLASS_VIDEO_STREAMING)
                                curr_ctrl_intf->add_associated_interface(curr_inf);
                            break;
                        }
                        default:
                            break;
                    }
                    for(int j = 0; j < inf.num_altsetting; j++)
                    {
                        auto d = inf.altsetting[j];
                        usb_descriptor ud = {d.bLength, d.bDescriptorType, std::vector<uint8_t>(d.bLength)};
                        memcpy(ud.data.data(), &d, d.bLength);
                        _descriptors.push_back(ud);
                        for(int k = 0; k < d.extra_length; )
                        {
                            auto l = d.extra[k];
                            auto dt = d.extra[k+1];
                            usb_descriptor ud = {l, dt, std::vector<uint8_t>(l)};
                            memcpy(ud.data.data(), &d.extra[k], l);
                            _descriptors.push_back(ud);
                            k += l;
                        }
                    }
                }

                libusb_free_config_descriptor(config);
            }
            libusb_ref_device(_device);
        }

        usb_device_libusb::~usb_device_libusb()
        {
            libusb_unref_device(_device);
        }

        const rs_usb_interface usb_device_libusb::get_interface(uint8_t interface_number) const
        {
            auto it = std::find_if(_interfaces.begin(), _interfaces.end(),
                                   [interface_number](const rs_usb_interface& i) { return interface_number == i->get_number(); });
            if (it == _interfaces.end())
                return nullptr;
            return *it;
        }

        std::shared_ptr<handle_libusb> usb_device_libusb::get_handle(uint8_t interface_number)
        {
            try
            {
                auto i = get_interface(interface_number);
                if (!i)
                    return nullptr;
                auto intf = std::dynamic_pointer_cast<usb_interface_libusb>(i);
                return std::make_shared<handle_libusb>(_context, _device, intf);
            }
            catch(const std::exception& e)
            {
                return nullptr;
            }
        }

        const std::shared_ptr<usb_messenger> usb_device_libusb::open(uint8_t interface_number)
        {
            auto h = get_handle(interface_number);
            if(!h)
                return nullptr;
            return std::make_shared<usb_messenger_libusb>(shared_from_this(), h);
        }
    }
}
