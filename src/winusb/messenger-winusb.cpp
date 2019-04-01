// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef WIN32

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
        usb_messenger_winusb::usb_messenger_winusb(std::shared_ptr<device_handle> handle, std::shared_ptr<usb_interface_winusb> inf)
            : _handle(handle), _interface(inf)
        {
            for (auto&& ep : _interface->get_endpoints())
            {
                switch (ep->get_type())
                {
                case RS_USB_ENDPOINT_BULK:
                    if (ep->get_address() < 0x80)
                        _write_endpoint = ep;
                    else
                        _read_endpoint = ep;
                    break;
                case RS_USB_ENDPOINT_INTERRUPT:
                    _interrupt_endpoint = ep;
                    break;
                default: break;
                }
            }

            init_winusb_pipe();
        }

        usb_messenger_winusb::~usb_messenger_winusb()
        {
            try
            {
                release();
            }
            catch (...)
            {

            }
        }

        void usb_messenger_winusb::release()
        {
            //_handle.reset();//TODO:MK
        }

        int usb_messenger_winusb::control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t timeout_ms)
        {
            WINUSB_SETUP_PACKET setupPacket;
            ULONG lengthOutput;

            setupPacket.RequestType = request_type;
            setupPacket.Request = request;
            setupPacket.Value = value;
            setupPacket.Index = index;// | _interface_descriptor.bInterfaceNumber; //TODO_MK: check what is the required value
            setupPacket.Length = length;

            auto handle_index = index & 0xFF;
            if (!WinUsb_ControlTransfer(_interface->get_handle(), setupPacket, buffer, length, &lengthOutput, NULL))
            {
                auto lastResult = GetLastError();
                LOG_ERROR("control_transfer failed, error: " << lastResult);
                return -1;
            }
            return lengthOutput;
        }

        bool usb_messenger_winusb::reset_pipe(uint8_t endpoint)
        {
            return WinUsb_ResetPipe(_interface->get_handle(), endpoint);
        }

        bool usb_messenger_winusb::wait_for_async_op(OVERLAPPED &hOvl, ULONG &lengthTransferred, DWORD timeout_ms, uint8_t endpoint)
        {
            if (GetOverlappedResult(_interface->get_handle(), &hOvl, &lengthTransferred, FALSE))
                return true;

            auto lastResult = GetLastError();
            if (lastResult == ERROR_IO_PENDING || lastResult == ERROR_IO_INCOMPLETE)
            {
                auto hResult = WaitForSingleObject(hOvl.hEvent, timeout_ms);
                if (hResult == WAIT_TIMEOUT)
                {
                    lengthTransferred = 0;  //WinUsb failure - reset winusb

                    reset_pipe(_read_endpoint->get_address());
                    reset_pipe(_write_endpoint->get_address());
                    return false;
                }
                else
                {
                    if (GetOverlappedResult(_interface->get_handle(), &hOvl, &lengthTransferred, FALSE) != 1)
                        throw winapi_error("GetOverlappedResult failed.");
                }
            }
            else
            {
                lengthTransferred = 0;
                reset_pipe(endpoint);
                throw std::runtime_error(winapi_error::last_error_string(lastResult).c_str());
            }
        }

        int usb_messenger_winusb::bulk_transfer(uint8_t endpoint_id, uint8_t* buffer, uint32_t length, uint32_t timeout_ms)
        {
            // LPOVERLAPPED structure is used for asynchronous operations.
            // If this parameter is specified, WinUsb_ReadPipe/WinUsb_WritePipe returns immediately rather than waiting synchronously
            // for the operation to complete before returning.An event is signaled when the operation is complete.
            OVERLAPPED ovl;

            auto_reset_event evnt;
            ovl.hEvent = evnt.get_handle();

            ULONG length_transfered;
            auto isExitOnTimeout = false;
            int sts;
            if (USB_ENDPOINT_DIRECTION_IN(endpoint_id))
                sts = WinUsb_ReadPipe(_interface->get_handle(), endpoint_id, const_cast<unsigned char*>(buffer), length, &length_transfered, &ovl);
            else
                sts = WinUsb_WritePipe(_interface->get_handle(), endpoint_id, const_cast<unsigned char*>(buffer), length, &length_transfered, &ovl);
            if (sts == 0)
            {
                auto lastError = GetLastError();
                if (lastError == ERROR_IO_PENDING)
                {
                    wait_for_async_op(ovl, length_transfered, timeout_ms, endpoint_id);
                }
                else
                {
                    reset_pipe(endpoint_id);
                    throw std::runtime_error(winapi_error::last_error_string(lastError).c_str());
                }
            }
            return length_transfered;
        }

        std::vector<uint8_t> usb_messenger_winusb::send_receive_transfer(std::vector<uint8_t> data, int timeout_ms)
        {
            int transfered_count = bulk_transfer(_write_endpoint->get_address(), data.data(), data.size(), timeout_ms);

            if (transfered_count < 0)
                throw std::runtime_error("USB command timed-out!");

            std::vector<uint8_t> output(1024);
            transfered_count = bulk_transfer(_read_endpoint->get_address(), output.data(), output.size(), timeout_ms);

            if (transfered_count < 0)
                throw std::runtime_error("USB command timed-out!");

            output.resize(transfered_count);
            return output;
        }

        void usb_messenger_winusb::set_timeout_policy(uint8_t endpoint, uint32_t timeout_ms)
        {
            // WinUsb_SetPipePolicy function sets the policy for a specific pipe associated with an endpoint on the device
            // PIPE_TRANSFER_TIMEOUT: Waits for a time-out interval before canceling the request
            auto bRetVal = WinUsb_SetPipePolicy(_interface->get_handle(), endpoint, PIPE_TRANSFER_TIMEOUT, sizeof(timeout_ms), &timeout_ms);
            if (!bRetVal)
            {
                throw winapi_error("SetPipeTimeOutPolicy failed.");
            }
        }

        void usb_messenger_winusb::init_winusb_pipe()
        {
            if (_read_endpoint != 0) 
                set_timeout_policy(_read_endpoint->get_address(), _in_out_pipe_timeout_ms);

            if (_write_endpoint != 0)
                set_timeout_policy(_write_endpoint->get_address(), _in_out_pipe_timeout_ms);


            //auto sts = WinUsb_ResetPipe(_handle->get_winusb_handle(), _read_endpoint); // WinUsb_ResetPipe function resets the data toggle and clears the stall condition on a pipe (is this necessary?)
            //if (!sts)
            //    throw winapi_error("WinUsb_ResetPipe failed.");
        }
    }
}

#endif
