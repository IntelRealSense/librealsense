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
        usb_device_info generate_info(::usb_device *handle, int mi, usb_spec conn_spec, usb_class cls)
        {
            usb_device_info rv{};
            rv.vid = usb_device_get_vendor_id(handle);
            rv.pid = usb_device_get_product_id(handle);
            rv.unique_id = std::string(usb_device_get_name(handle));
            rv.mi = mi;
            rv.cls = cls;
            rv.conn_spec = conn_spec;
            return rv;
        }

        usb_device_usbhost::usb_device_usbhost(::usb_device* handle) :
                _handle(handle)
        {
            usb_descriptor_iter it;
            ::usb_descriptor_iter_init(handle, &it);
            usb_descriptor_header *h = usb_descriptor_iter_next(&it);
            librealsense::platform::usb_spec conn_spec;
            if (h != nullptr && h->bDescriptorType == USB_DT_DEVICE) {
                usb_device_descriptor *device_descriptor = (usb_device_descriptor *) h;
                conn_spec = librealsense::platform::usb_spec(device_descriptor->bcdUSB);
            }

            do {
                h = usb_descriptor_iter_next(&it);
                if(h == NULL)
                    break;
                if (h->bDescriptorType == USB_DT_INTERFACE_ASSOCIATION) {
                    auto iad = *(usb_interface_assoc_descriptor *) h;
                    auto info = generate_info(_handle, iad.bFirstInterface, conn_spec,
                                              static_cast<usb_class>(iad.bFunctionClass));
                    _infos.push_back(info);
                }
                if (h->bDescriptorType == USB_DT_INTERFACE) {
                    auto id = *(usb_interface_descriptor *) h;
                    _interfaces.push_back(std::make_shared<usb_interface_usbhost>(id, it));
                    switch (id.bInterfaceClass)
                    {
                        case RS2_USB_CLASS_VENDOR_SPECIFIC:
                        case RS2_USB_CLASS_HID:
                        {
                            auto info = generate_info(_handle, id.bInterfaceNumber, conn_spec,
                                                      static_cast<usb_class>(id.bInterfaceClass));
                            _infos.push_back(info);
                            break;
                        }
                        default:
                            break;
                    }
                }
            } while (h != nullptr);

            _usb_device_descriptor = usb_device_get_device_descriptor(_handle);
        }

        void usb_device_usbhost::release()
        {
            _messenger.reset();
            LOG_DEBUG("usb device: " << get_info().unique_id << ", released");
        }

        const std::shared_ptr<usb_messenger> usb_device_usbhost::open()
        {
            if(!_messenger)
                _messenger = std::make_shared<usb_messenger_usbhost>(shared_from_this());
            return _messenger;
        }
    }
}
#endif