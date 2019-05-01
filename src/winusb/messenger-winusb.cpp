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

        int usb_messenger_winusb::control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t timeout_ms)
        {
            WINUSB_SETUP_PACKET setupPacket;
            ULONG lengthOutput;

            setupPacket.RequestType = request_type;
            setupPacket.Request = request;
            setupPacket.Value = value;
            setupPacket.Index = index;
            setupPacket.Length = length;

            auto intf = std::static_pointer_cast<usb_interface_winusb>(_device->get_interface(index));//TODO_MK index?
            auto path = intf->get_device_path();
            handle_winusb dh(path);
            auto h = dh.get_first_interface();
            if (!WinUsb_ControlTransfer(h, setupPacket, buffer, length, &lengthOutput, NULL))
            {
                auto lastResult = GetLastError();
                LOG_ERROR("control_transfer failed, error: " << lastResult);
                return -1;
            }
            return lengthOutput;
        }

        bool usb_messenger_winusb::reset_endpoint(std::shared_ptr<usb_endpoint> endpoint)
        {
            auto path = get_interface_by_endpoint(endpoint)->get_device_path();
            handle_winusb dh(path);
            auto h = dh.get_first_interface();
            return WinUsb_ResetPipe(h, endpoint->get_address());
        }

        std::shared_ptr<usb_interface_winusb> usb_messenger_winusb::get_interface_by_endpoint(const std::shared_ptr<usb_endpoint>& endpoint)
        {
            auto i = _device->get_interface(endpoint->get_interface_number());
            return std::static_pointer_cast<usb_interface_winusb>(i);
        }

        int usb_messenger_winusb::bulk_transfer(const std::shared_ptr<usb_endpoint>& endpoint, uint8_t* buffer, uint32_t length, uint32_t timeout_ms)
        {
            ULONG length_transfered;
            int sts;

            auto start = std::chrono::system_clock::now();
            auto path = get_interface_by_endpoint(endpoint)->get_device_path();
            handle_winusb dh(path, timeout_ms);//this call may wait for other user to finish
            auto h = dh.get_interface_handle(get_interface_by_endpoint(endpoint)->get_number());
            auto now = std::chrono::system_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            set_timeout_policy(h, endpoint->get_address(), timeout_ms - diff);

            if (USB_ENDPOINT_DIRECTION_IN(endpoint->get_address()))
                sts = WinUsb_ReadPipe(h, endpoint->get_address(), const_cast<unsigned char*>(buffer), length, &length_transfered, NULL);
            else
                sts = WinUsb_WritePipe(h, endpoint->get_address(), const_cast<unsigned char*>(buffer), length, &length_transfered, NULL);
            return length_transfered;
        }

        void usb_messenger_winusb::set_timeout_policy(WINUSB_INTERFACE_HANDLE handle, uint8_t endpoint, uint32_t timeout_ms)
        {
            // WinUsb_SetPipePolicy function sets the policy for a specific pipe associated with an endpoint on the device
            // PIPE_TRANSFER_TIMEOUT: Waits for a time-out interval before canceling the request
            auto bRetVal = WinUsb_SetPipePolicy(handle, endpoint, PIPE_TRANSFER_TIMEOUT, sizeof(timeout_ms), &timeout_ms);
            if (!bRetVal)
            {
                throw winapi_error("SetPipeTimeOutPolicy failed.");
            }
        }
    }
}
