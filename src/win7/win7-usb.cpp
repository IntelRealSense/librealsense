// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS2_USE_WINUSB_UVC_BACKEND

#if (_MSC_FULL_VER < 180031101)
#error At least Visual Studio 2013 Update 4 is required to compile this backend
#endif

#include "win7-usb.h"
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
        winusb_device::winusb_device(std::wstring lpDevicePath)
            : _lp_device_path(lpDevicePath),
              _device_handle(nullptr),
              _usb_interface(nullptr)
        {
            open(lpDevicePath);
        }

        winusb_device::~winusb_device()
        {
            try
            {
                release();
            }
            catch (...)
            {

            }
        }

        void winusb_device::recreate_interface()
        {
            if (_usb_interface)
                _usb_interface.reset();

            if(_device_handle)
            {
                CloseHandle(_device_handle);
                _device_handle = nullptr;
            }

            open(_lp_device_path);
            _usb_interface = std::make_unique<usb_interface>(_device_handle);
        }

        void winusb_device::release()
        {
            if (_device_handle)
            {
                CloseHandle(_device_handle);
                _device_handle = nullptr;
            }
        }

        void winusb_device::open(std::wstring lpDevicePath)
        {
            if (_device_handle)
                return;

            auto deviceID = usb_enumerate::get_camera_id(lpDevicePath.c_str());

            if(strcmp(deviceID.c_str(), deviceID.c_str()))
                throw std::runtime_error("deviceID is different!");

            _device_handle = CreateFile(lpDevicePath.c_str(), GENERIC_WRITE | GENERIC_READ,
                FILE_SHARE_WRITE | FILE_SHARE_READ, nullptr,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);

            if (_device_handle == INVALID_HANDLE_VALUE)
            {
                _device_handle = nullptr;
                throw std::runtime_error("CreateFile failed! Probably camera not connected.");
            }

            _usb_interface = std::make_unique<usb_interface>(_device_handle);
        }

        usb_interface& winusb_device::get_interface() const
        {
            return *_usb_interface;
        }

        std::vector<std::wstring> usb_enumerate::query_by_interface(const std::string& guidStr,
            const std::string& vid, const std::string& pid)
        {
            GUID guid;
            std::wstring guidWStr(guidStr.begin(), guidStr.end());
            CHECK_HR(CLSIDFromString(guidWStr.c_str(), static_cast<LPCLSID>(&guid)));

            HDEVINFO deviceInfo;
            std::vector<std::wstring> retVec;

            deviceInfo = SetupDiGetClassDevs(&guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
            int i;
            if (deviceInfo != INVALID_HANDLE_VALUE)
            {
                for (i = 0; i < 256; i++)
                {
                    std::wstring lpDevicePath;

                    if (!get_device_data(guid, i, deviceInfo, lpDevicePath))
                        continue;

                    std::wstring wVid(vid.begin(), vid.end());
                    std::wstring wPid(pid.begin(), pid.end());

                    auto vidPos = lpDevicePath.find(std::wstring(L"vid_") + std::wstring(vid.begin(), vid.end()));
                    if (vidPos == std::wstring::npos)
                    {
                        continue;
                    }

                    auto pidPos = lpDevicePath.find(std::wstring(L"pid_") + std::wstring(pid.begin(), pid.end()));
                    if (pidPos == std::wstring::npos)
                    {
                        continue;
                    }

                    retVec.push_back(lpDevicePath);
                }
            }
            else
            {
                throw winapi_error("SetupDiGetClassDevs failed.");
            }

            if (!SetupDiDestroyDeviceInfoList(deviceInfo))
            {
                throw winapi_error("Failed to clean-up SetupDiDestroyDeviceInfoList.");
            }

            return retVec;
        }

        std::string usb_enumerate::get_camera_id(const wchar_t* deviceID)
        {
            std::wsmatch matches;
            std::wstring deviceIDstr(deviceID);

            std::wregex camIdReg(L"\\b(mi_)(.*#\\w&)(.*)(&.*&)", std::regex_constants::icase);
            if (!std::regex_search(deviceIDstr, matches, camIdReg) || matches.size() < 5)
                throw std::runtime_error("regex_search failed!");

            std::wstring wdevID = matches[3];
            return std::string(wdevID.begin(), wdevID.end());
        }

        bool usb_enumerate::get_device_data(_GUID guid, int i, const HDEVINFO& deviceInfo, std::wstring& lpDevicePath)
        {
            SP_DEVICE_INTERFACE_DATA interfaceData;
            unsigned long length;
            unsigned long requiredLength = 0;

            std::shared_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA> detailData;

            // Obtain information on your device interface
            //Enumerate all the device interfaces in the device information set.
            memset(&interfaceData, 0, sizeof(interfaceData));
            interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

            if (!SetupDiEnumDeviceInterfaces(deviceInfo, nullptr, &guid, i, &interfaceData))
            {
                return false;
            }

            // Get detailed data for the device interface
            //Interface data is returned in SP_DEVICE_INTERFACE_DETAIL_DATA
            //which we need to allocate, so we have to call this function twice.
            //First to get the size so that we know how much to allocate
            //Second, the actual call with the allocated buffer
            if (!SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, nullptr, 0, &requiredLength, nullptr))
            {
                if ((ERROR_INSUFFICIENT_BUFFER == GetLastError()) && (requiredLength > 0))
                {
                    //we got the size, allocate buffer
                    auto * detailData1 = static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(LocalAlloc(LMEM_FIXED, requiredLength));
                    if (!detailData1)
                        return false;

                    // TODO: Make proper PSP_DEVICE_INTERFACE_DETAIL_DATA wrapper
                    detailData = std::shared_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA>(detailData1,
                        [=](SP_DEVICE_INTERFACE_DETAIL_DATA*)
                    {
                        if (detailData1 != nullptr)
                        {
                            LocalFree(detailData1);
                        }
                    });
                }
                else
                {
                    return false;
                }
            }

            if (nullptr == detailData)
                throw winapi_error("SP_DEVICE_INTERFACE_DETAIL_DATA object is null!");

            detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            length = requiredLength;

            if (!SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, detailData.get(), length, &requiredLength, nullptr))
            {
                return false;
            }

            // Get the device path
            lpDevicePath = detailData->DevicePath;

            return true;
        }


        usb_interface::usb_interface(HANDLE& deviceHandle)
            : _interface_handle(nullptr)
        {
            init_interface(deviceHandle);
            init_winusb_pipe();
        }

        usb_interface::~usb_interface()
        {
            try {
                release();
            }
            catch (...)
            {
                // TODO: Write to log
            }
        }

        void usb_interface::init_interface(HANDLE deviceHandle)
        {
            if (_interface_handle)
                return;

            auto bRetval = WinUsb_Initialize(IN deviceHandle, OUT &_interface_handle); // WinUsb_Initialize function creates a WinUSB handle for the device specified by a file handle
            if (!bRetval)
            {
                release();
                throw winapi_error("WinUsb_Initialize failed.");
            }
        }

        void usb_interface::release()
        {
            if (!WinUsb_Free(_interface_handle)) // WinUsb_Free function releases all of the resources that WinUsb_Initialize allocated
                throw winapi_error("WinUsb_Free failed.");

            _interface_handle = nullptr;
        }

        void usb_interface::set_timeout_policy(unsigned char pipeID, unsigned long timeOutInMs) const
        {
            // WinUsb_SetPipePolicy function sets the policy for a specific pipe associated with an endpoint on the device
            // PIPE_TRANSFER_TIMEOUT: Waits for a time-out interval before canceling the request
            auto bRetVal = WinUsb_SetPipePolicy(_interface_handle, pipeID, PIPE_TRANSFER_TIMEOUT, sizeof(timeOutInMs), &timeOutInMs);
            if (!bRetVal)
            {
                throw winapi_error("SetPipeTimeOutPolicy failed.");
            }
        }

        void usb_interface::wait_for_async_op(OVERLAPPED &hOvl, ULONG &lengthTransferred, DWORD TimeOut, pipe_direction pipeDirection, bool* isExitOnTimeout) const
        {
            *isExitOnTimeout = false;
            if (GetOverlappedResult(_interface_handle, &hOvl, &lengthTransferred, FALSE))
                return;

            auto lastResult = GetLastError();
            if (lastResult == ERROR_IO_PENDING || lastResult == ERROR_IO_INCOMPLETE)
            {
                auto hResult = WaitForSingleObject(hOvl.hEvent, TimeOut);
                if (hResult == WAIT_TIMEOUT)
                {
                    lengthTransferred = 0;  //WinUsb failure - reset winusb

                    reset_pipe(pipe_direction::InPipe);
                    reset_pipe(pipe_direction::OutPipe);
                    *isExitOnTimeout = true;
                }
                else
                {
                    if (GetOverlappedResult(_interface_handle, &hOvl, &lengthTransferred, FALSE) != 1)
                        throw winapi_error("GetOverlappedResult failed.");
                }
            }
            else
            {
                lengthTransferred = 0;
                reset_pipe(pipeDirection);
                throw std::runtime_error(winapi_error::last_error_string(lastResult).c_str());
            }
        }

        bool usb_interface::wait_for_async_op_interrupt(OVERLAPPED &hOvl, ULONG &lengthTransferred) const
        {
            if (GetOverlappedResult(_interface_handle, &hOvl, &lengthTransferred, FALSE))
                return true;

            auto lastResult = GetLastError();
            if (lastResult == ERROR_IO_PENDING || lastResult == ERROR_IO_INCOMPLETE)
            {
                WaitForSingleObject(hOvl.hEvent, 20);
                auto res = GetOverlappedResult(_interface_handle, &hOvl, &lengthTransferred, FALSE);
                if (res != 1)
                {
                    return false;
                }
            }
            else
            {
                lengthTransferred = 0;
                reset_interrupt_pipe();
                return false;
            }

            return true;
        }

        void usb_interface::query_endpoints()
        {
            if (_interface_handle == INVALID_HANDLE_VALUE)
                throw std::runtime_error("Interface handle is INVALID_HANDLE_VALUE!");
            if (_interface_handle == nullptr)
                throw std::runtime_error("Interface handle is NULL!");

            USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
            ZeroMemory(&InterfaceDescriptor, sizeof(USB_INTERFACE_DESCRIPTOR));

            WINUSB_PIPE_INFORMATION  Pipe;
            ZeroMemory(&Pipe, sizeof(WINUSB_PIPE_INFORMATION));

            // WinUsb_QueryInterfaceSettings function retrieves the interface descriptor for the specified alternate interface settings for a particular interface handle
            if (!WinUsb_QueryInterfaceSettings(_interface_handle, 0, &InterfaceDescriptor))
                throw winapi_error("WinUsb_QueryInterfaceSettings failed.");


            for (auto index = static_cast<UCHAR>(0); index < InterfaceDescriptor.bNumEndpoints; ++index)
            {
                auto sts = WinUsb_QueryPipe(_interface_handle, 0, index, &Pipe); // WinUsb_QueryPipe function retrieves information about the specified endpoint and the associated pipe for an interface
                if (!sts)
                {
                    continue;
                }

                if (Pipe.PipeType == UsbdPipeTypeBulk) // Bulk Transfer
                {
                    if (USB_ENDPOINT_DIRECTION_IN(Pipe.PipeId))
                    {
                        _in_pipe_id = Pipe.PipeId;
                    }
                    else if (USB_ENDPOINT_DIRECTION_OUT(Pipe.PipeId))
                    {
                        _out_pipe_id = Pipe.PipeId;
                    }
                }
                else if (Pipe.PipeType == UsbdPipeTypeInterrupt) // Interrupt Transfer
                {
                    _interrupt_pipe_id = Pipe.PipeId;
                }
                // TODO: Isochronous Transfer and Control Transfer
            }
        }

        void usb_interface::init_winusb_pipe()
        {
            // initialize _dataInPipeID and _dataOutPipeID that will be used below.
            query_endpoints();

            if (_in_pipe_id != 0)
                set_timeout_policy(_in_pipe_id, _in_out_pipe_timeout_ms);

            if (_out_pipe_id != 0)
                set_timeout_policy(_out_pipe_id, _in_out_pipe_timeout_ms);


            auto sts = WinUsb_ResetPipe(_interface_handle, _in_pipe_id); // WinUsb_ResetPipe function resets the data toggle and clears the stall condition on a pipe (is this necessary?)
            if (!sts)
                throw winapi_error("WinUsb_ResetPipe failed.");
        }

        bool usb_interface::read_pipe(unsigned char* buffer, ULONG bufferLength, PULONG lengthTransferred, LPOVERLAPPED hOvl) const
        {
            auto sts = WinUsb_ReadPipe(_interface_handle, _in_pipe_id, buffer, bufferLength, lengthTransferred, hOvl);
            return (sts) ? true : false;
        }

        bool usb_interface::read_interupt_pipe(unsigned char* buffer, ULONG bufferLength, PULONG lengthTransferred, LPOVERLAPPED hOvl) const
        {
            auto sts = WinUsb_ReadPipe(_interface_handle, _interrupt_pipe_id, buffer, bufferLength, lengthTransferred, hOvl);
            return (sts) ? true : false;
        }

        bool usb_interface::write_pipe(const unsigned char* buffer, ULONG bufferLength, PULONG lengthTransferred, LPOVERLAPPED hOvl) const
        {
            auto sts = WinUsb_WritePipe(_interface_handle, _out_pipe_id, const_cast<unsigned char*>(buffer), bufferLength, lengthTransferred, hOvl);
            return (sts) ? true : false;
        }

        void usb_interface::reset_interrupt_pipe() const
        {
            auto sts = WinUsb_ResetPipe(_interface_handle, _interrupt_pipe_id);
            if (!sts)
                throw winapi_error("WinUsb_ResetPipe failed!");
        }

        void usb_interface::reset_pipe(pipe_direction pipeDirection) const
        {
            BOOL sts;

            if (pipeDirection == pipe_direction::InPipe)
            {
                sts = WinUsb_ResetPipe(_interface_handle, _in_pipe_id);
            }
            else
            {
                sts = WinUsb_ResetPipe(_interface_handle, _out_pipe_id);
            }
            if (!sts)
                throw winapi_error("WinUsb_ResetPipe failed!");
        }

        winapi_error::winapi_error(const char* message)
            : runtime_error(generate_message(message).c_str())
        { }

        std::string winapi_error::generate_message(const std::string& message)
        {
            std::stringstream ss;
            ss << message << " Last Error: " << last_error_string(GetLastError()) << std::endl;
            return ss.str();
        }

        std::string winapi_error::last_error_string(DWORD lastError)
        {
            // TODO: Error handling
            LPSTR messageBuffer = nullptr;
            size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&messageBuffer), 0, nullptr);

            std::string message(messageBuffer, size);

            LocalFree(messageBuffer);

            return message;
        }

        winusb_bulk_transfer::winusb_bulk_transfer(const usb_device_info& info)
            : _lp_device_path(std::wstring(info.id.begin(), info.id.end())),
              _named_mutex(usb_enumerate::get_camera_id(_lp_device_path.c_str()).c_str(), WAIT_FOR_MUTEX_TIME_OUT)
        { }

        void winusb_bulk_transfer::write_to_pipe_and_read_response(const std::vector<uint8_t>& input,
            std::vector<uint8_t>&  output, DWORD TimeOut, bool isWriteOnly)
        {
            std::lock_guard<named_mutex> lock(_named_mutex);
            winusb_device usbDevice(_lp_device_path);


            // LPOVERLAPPED structure is used for asynchronous operations.
            // If this parameter is specified, WinUsb_ReadPipe/WinUsb_WritePipe returns immediately rather than waiting synchronously
            // for the operation to complete before returning.An event is signaled when the operation is complete.
            OVERLAPPED hOvl;

            auto_reset_event evnt;
            hOvl.hEvent = evnt.get_handle();

            ULONG lengthWritten{};
            auto isExitOnTimeout = false;
            auto res = usbDevice.get_interface().write_pipe(input.data(), static_cast<ULONG>(input.size()), &lengthWritten, &hOvl);
            if (!res)
            {
                auto lastError = GetLastError();
                if (lastError == ERROR_IO_PENDING)
                {
                    usbDevice.get_interface().wait_for_async_op(hOvl, lengthWritten, TimeOut, pipe_direction::OutPipe, &isExitOnTimeout);
                }
                else
                {
                    usbDevice.get_interface().reset_pipe(pipe_direction::OutPipe);
                    throw std::runtime_error(winapi_error::last_error_string(lastError).c_str());
                }
            }

            if (isWriteOnly)
                return;

            DWORD timeSlice = 4000;
            ULONG bytesReceived = 0;
            auto readRes = false;
            if (TimeOut > timeSlice)
            {
                auto timeLeft = TimeOut;

                while (timeLeft > timeSlice && !readRes)
                {
                    readRes = read_pipe_sync(&usbDevice, output.data(), HW_MONITOR_BUFFER_SIZE, bytesReceived, timeSlice);

                    // Passed 5 sec on long-running HW-Monitor command... Keep waiting...
                    timeLeft -= timeSlice;
                    if (!readRes)
                    {
                        usbDevice.recreate_interface();
                    }
                }

                if (!readRes)
                {
                    usbDevice.recreate_interface();
                    readRes = read_pipe_sync(&usbDevice, output.data(), HW_MONITOR_BUFFER_SIZE, bytesReceived, timeLeft);
                }
            }
            else
            {
                readRes = read_pipe_sync(&usbDevice, output.data(), HW_MONITOR_BUFFER_SIZE, bytesReceived, TimeOut);
            }

            if (!readRes)
                throw std::runtime_error("USB command timed-out!");

            output.resize(bytesReceived);
        }

        bool winusb_bulk_transfer::read_pipe_sync(winusb_device* usbDevice, unsigned char* buffer, int bufferLength, ULONG& lengthTransferred, DWORD TimeOut)
        {
            OVERLAPPED hOvl;

            buffer[0] = buffer[1] = 0;

            auto_reset_event evnt;
            hOvl.hEvent = evnt.get_handle();

            auto res = usbDevice->get_interface().read_pipe(buffer, bufferLength, &lengthTransferred, &hOvl);
            if (!res)
            {
                auto lastError = GetLastError();
                if (lastError == ERROR_IO_PENDING)
                {
                    auto isExitOnTimeout = false;
                    usbDevice->get_interface().wait_for_async_op(hOvl, lengthTransferred, TimeOut, pipe_direction::InPipe, &isExitOnTimeout);
                    if (isExitOnTimeout)
                        return false;
                }
                else
                {
                    usbDevice->get_interface().reset_pipe(pipe_direction::InPipe);
                    return false;
                }
            }

            return true;
        }


        std::vector<uint8_t> winusb_bulk_transfer::send_receive(const std::vector<uint8_t>& data, int timeout_ms, bool require_response)
        {
            ULONG buffSize = 0;
            std::vector<uint8_t> output(HW_MONITOR_BUFFER_SIZE);

            write_to_pipe_and_read_response(data, output, timeout_ms, !require_response);

            return output;
        }

        const wchar_t* winusb_bulk_transfer::get_path() const
        {
            return _lp_device_path.c_str();
        }
    }
}

#endif
