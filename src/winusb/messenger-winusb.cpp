// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

//#define HW_MONITOR_BUFFER_SIZE   (1024)

#if (_MSC_FULL_VER < 180031101)
#error At least Visual Studio 2013 Update 4 is required to compile this backend
#endif

#include "messenger-winusb.h"
#include "device-winusb.h"
#include "usb/usb-enumerator.h"
#include "types.h"

#include <atlstr.h>
#include <Windows.h>
#include <Sddl.h>
#include <string>
#include <regex>
#include <sstream>
#include <mutex>

#pragma comment(lib, "winusb.lib")

namespace librealsense
{
    namespace platform
    {
        usb_messenger_winusb::usb_messenger_winusb(const std::shared_ptr<usb_device_winusb> device)
            : _device(device)
        {

        }

        usb_messenger_winusb::~usb_messenger_winusb()
        {

        }

        std::shared_ptr<usb_interface_winusb> usb_messenger_winusb::get_interface(int number)
        {
            auto intfs = _device->get_interfaces();
            auto it = std::find_if(intfs.begin(), intfs.end(),
                [&](const rs_usb_interface& i) { return i->get_number() == number; });
            if (it == intfs.end())
                return nullptr;
            return std::static_pointer_cast<usb_interface_winusb>(*it);
        }

        usb_status usb_messenger_winusb::control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms)
        {
            WINUSB_SETUP_PACKET setupPacket;
            ULONG length_transferred;

            setupPacket.RequestType = request_type;
            setupPacket.Request = request;
            setupPacket.Value = value;
            setupPacket.Index = index;
            setupPacket.Length = length;

            auto intf = get_interface(0xFF & index);
            if (!intf)
                return RS2_USB_STATUS_INVALID_PARAM;
            auto path = intf->get_device_path();

            handle_winusb dh;
            auto sts = dh.open(path);
            if (sts != RS2_USB_STATUS_SUCCESS)
                return sts;
            auto h = dh.get_first_interface();

            sts = set_timeout_policy(h, 0, timeout_ms);
            if (sts != RS2_USB_STATUS_SUCCESS)
                return sts;

            if (!WinUsb_ControlTransfer(h, setupPacket, buffer, length, &length_transferred, NULL))
            {
                auto lastResult = GetLastError();
                LOG_ERROR("control_transfer failed, error: " << lastResult);
                return winusb_status_to_rs(lastResult);
            }
            transferred = length_transferred;
            return RS2_USB_STATUS_SUCCESS;
        }

        usb_status usb_messenger_winusb::reset_endpoint(const rs_usb_endpoint& endpoint, uint32_t timeout_ms)
        {
            auto intf = get_interface(endpoint->get_interface_number());
            if (!intf)
                return RS2_USB_STATUS_INVALID_PARAM;
            handle_winusb dh;
            auto sts = dh.open(intf->get_device_path());
            if (sts != RS2_USB_STATUS_SUCCESS)
                return sts;
            auto h = dh.get_first_interface();

            if (!WinUsb_ResetPipe(h, endpoint->get_address()))
            {
                auto lastResult = GetLastError();
                LOG_ERROR("control_transfer failed, error: " << lastResult);
                return winusb_status_to_rs(lastResult);
            }
            return RS2_USB_STATUS_SUCCESS;
        }

        usb_status usb_messenger_winusb::bulk_transfer(const rs_usb_endpoint& endpoint, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms)
        {
            ULONG length_transferred;

            auto intf = get_interface(endpoint->get_interface_number());
            if (!intf)
                return RS2_USB_STATUS_INVALID_PARAM; 
            handle_winusb dh;
            auto sts = dh.open(intf->get_device_path());
            if (sts != RS2_USB_STATUS_SUCCESS)
                return sts;
            auto h = dh.get_first_interface();

            sts = set_timeout_policy(h, endpoint->get_address(), timeout_ms);
            if (sts != RS2_USB_STATUS_SUCCESS)
                return sts;

            bool res;
            if (USB_ENDPOINT_DIRECTION_IN(endpoint->get_address()))
                res = WinUsb_ReadPipe(h, endpoint->get_address(), const_cast<unsigned char*>(buffer), length, &length_transferred, NULL);
            else
                res = WinUsb_WritePipe(h, endpoint->get_address(), const_cast<unsigned char*>(buffer), length, &length_transferred, NULL);
            if(!res)
            {
                auto lastResult = GetLastError();
                LOG_ERROR("bulk_transfer failed, error: " << lastResult);
                return winusb_status_to_rs(lastResult);
            }
            transferred = length_transferred;
            return RS2_USB_STATUS_SUCCESS;
        }

        usb_status usb_messenger_winusb::set_timeout_policy(WINUSB_INTERFACE_HANDLE handle, uint8_t endpoint, uint32_t timeout_ms)
        {
            // WinUsb_SetPipePolicy function sets the policy for a specific pipe associated with an endpoint on the device
            // PIPE_TRANSFER_TIMEOUT: Waits for a time-out interval before canceling the request
            auto sts = WinUsb_SetPipePolicy(handle, endpoint, PIPE_TRANSFER_TIMEOUT, sizeof(timeout_ms), &timeout_ms);
            if (!sts)
            {
                auto lastResult = GetLastError();
                LOG_ERROR("failed to set timeout policy, error: " << lastResult);
                return winusb_status_to_rs(lastResult);
            }
            return RS2_USB_STATUS_SUCCESS;
        }
    }
}
