// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "context-libusb.h"

#include <chrono>

#include <libusb.h>

namespace librealsense
{
    namespace platform
    {
        static usb_status libusb_status_to_rs(int sts)
        {
            switch (sts)
            {
                case LIBUSB_SUCCESS: return RS2_USB_STATUS_SUCCESS;
                case LIBUSB_ERROR_IO: return RS2_USB_STATUS_IO;
                case LIBUSB_ERROR_INVALID_PARAM: return RS2_USB_STATUS_INVALID_PARAM;
                case LIBUSB_ERROR_ACCESS: return RS2_USB_STATUS_ACCESS;
                case LIBUSB_ERROR_NO_DEVICE: return RS2_USB_STATUS_NO_DEVICE;
                case LIBUSB_ERROR_NOT_FOUND: return RS2_USB_STATUS_NOT_FOUND;
                case LIBUSB_ERROR_BUSY: return RS2_USB_STATUS_BUSY;
                case LIBUSB_ERROR_TIMEOUT: return RS2_USB_STATUS_TIMEOUT;
                case LIBUSB_ERROR_OVERFLOW: return RS2_USB_STATUS_OVERFLOW;
                case LIBUSB_ERROR_PIPE: return RS2_USB_STATUS_PIPE;
                case LIBUSB_ERROR_INTERRUPTED: return RS2_USB_STATUS_INTERRUPTED;
                case LIBUSB_ERROR_NO_MEM: return RS2_USB_STATUS_NO_MEM;
                case LIBUSB_ERROR_NOT_SUPPORTED: return RS2_USB_STATUS_NOT_SUPPORTED;
                case LIBUSB_ERROR_OTHER: return RS2_USB_STATUS_OTHER;
                default: return RS2_USB_STATUS_OTHER;
            }
        }

        class handle_libusb
        {
        private:
            class safe_handle
            {
            public:
                safe_handle(libusb_device* device) : _handle(nullptr)
                {
                    auto sts = libusb_open(device, &_handle);
                    if(sts != LIBUSB_SUCCESS)
                    {
                        auto rs_sts =  libusb_status_to_rs(sts);
                        std::stringstream msg;
                        msg << "failed to open usb interface, error: " << usb_status_to_string.at(rs_sts);
                        LOG_ERROR(msg.str());
                        throw std::runtime_error(msg.str());
                    }
                }

                ~safe_handle()
                {
                    if (_first_interface)
                    {
                        uninit(_first_interface);
                    }

                    libusb_close(_handle);
                }

                void init(std::shared_ptr<usb_interface_libusb> interface)
                {
                    usb_status status;
                    int failed_interface;
                    do
                    {
                        status = claim_interface(interface->get_number());
                        if (status != RS2_USB_STATUS_SUCCESS)
                        {
                            failed_interface = interface->get_number();
                            break;
                        }
                        for (auto&& i : interface->get_associated_interfaces())
                        {
                            status = claim_interface(i->get_number());
                            if (status != RS2_USB_STATUS_SUCCESS)
                            {
                                failed_interface = i->get_number();
                                break;
                            }
                        }
                        _first_interface = interface;
                        return;
                    } while (0);
                    
                    uninit(interface);
                    throw std::runtime_error(to_string() << "Unable to claim interface " << failed_interface << ", error: " << usb_status_to_string.at(rs_sts));
                }

                operator libusb_device_handle* () const
                {
                    return _handle;
                }

            private:
                usb_status claim_interface(uint8_t interface)
                {
                    //libusb_set_auto_detach_kernel_driver(h, true);

                    if (libusb_kernel_driver_active(_handle, interface) == 1)//find out if kernel driver is attached
                        if (libusb_detach_kernel_driver(_handle, interface) == 0)// detach driver from device if attached.
                            LOG_DEBUG("handle_libusb - detach kernel driver");

                    auto sts = libusb_claim_interface(_handle, interface);
                    if(sts != LIBUSB_SUCCESS)
                    {
                        auto rs_sts =  libusb_status_to_rs(sts);
                        LOG_ERROR("failed to claim usb interface: " << (int)interface << ", error: " << usb_status_to_string.at(rs_sts));
                        return rs_sts;
                    }

                    return RS2_USB_STATUS_SUCCESS;
                }

                void uninit(std::shared_ptr<usb_interface_libusb> interface)
                {
                    for(auto&& i : interface->get_associated_interfaces())
                        libusb_release_interface(_handle, i->get_number());
                    libusb_release_interface(_handle, interface->get_number());
                }
                    
                libusb_device_handle* _handle;
                std::shared_ptr<usb_interface_libusb> _first_interface;
            };
        public:
            handle_libusb(std::shared_ptr<usb_context> context, libusb_device* device, std::shared_ptr<usb_interface_libusb> interface) :
                    _handle(device), _context(context)
            {
                _handle.init(interface);
                _context->start_event_handler();
            }

            ~handle_libusb()
            {
                _context->stop_event_handler();
            }

            libusb_device_handle* get()
            {
                return _handle;
            }

        private:
            safe_handle _handle;
            std::shared_ptr<usb_context> _context;
        };
    }
}
