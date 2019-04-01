// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS2_USE_ANDROID_BACKEND

#include "device-usbhost.h"
#include "endpoint-usbhost.h"
#include "interface-usbhost.h"
#include "usbhost.h"
#include "../types.h"

#include <string>
#include <regex>
#include <sstream>
#include <mutex>

namespace librealsense
{
    namespace platform
    {
        usb_device_usbhost::usb_device_usbhost(::usb_device* handle) :
            _handle(handle)
        {
            usb_descriptor_iter it;
            ::usb_descriptor_iter_init(handle, &it);
            usb_descriptor_header *h = usb_descriptor_iter_next(&it);
            if (h != nullptr && h->bDescriptorType == USB_DT_DEVICE) {
                usb_device_descriptor *device_descriptor = (usb_device_descriptor *) h;
                _info.conn_spec = librealsense::platform::usb_spec(device_descriptor->bcdUSB);
            }

            do {
                h = usb_descriptor_iter_next(&it);
                if(h == NULL)
                    break;
                if (h->bDescriptorType == USB_DT_INTERFACE_ASSOCIATION) {
                    auto iad = *(usb_interface_assoc_descriptor *) h;
                    _control_interfaces_numbers.insert(iad.bFirstInterface);
                }
                if(h->bDescriptorType == USB_DT_INTERFACE)
                {
                    auto id = *(usb_interface_descriptor *) h;
                    auto in = id.bInterfaceNumber;
                    if(_control_interfaces_numbers.find(in) == _control_interfaces_numbers.end())
                        _stream_interfaces_numbers.insert(in);
                    _interfaces[in] = std::make_shared<usb_interface_usbhost>(id, it);
                    _messengers[in] = std::make_shared<usb_messenger_usbhost>(_handle, _interfaces.at(in));
                }
            } while (h != nullptr);

            _usb_device_descriptor = usb_device_get_device_descriptor(_handle);
            _info.vid = usb_device_get_vendor_id(_handle);
            _info.pid = usb_device_get_product_id(_handle);
            _info.id = std::string(usb_device_get_name(_handle));
            _info.unique_id = std::string(usb_device_get_name(_handle));
//            _info.serial = std::string(usb_device_get_serial(_handle, 10));
//            _desc_length = usb_device_get_descriptors_length(_handle);
        }

        void usb_device_usbhost::release()
        {
            for(auto&& m : _messengers)
                m.second->release();
            LOG_DEBUG("usb device: " << get_info().unique_id << ", released");
        }
    }
}
#endif