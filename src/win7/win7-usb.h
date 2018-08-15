// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "../backend.h"
#include "win7-helpers.h"

#include <winusb.h>
#include <SetupAPI.h>

#define HW_MONITOR_BUFFER_SIZE   (1024)

namespace librealsense
{
    namespace platform
    {
        enum class pipe_direction
        {
            InPipe,
            OutPipe
        };

        class usb_interface
        {
        public:
            explicit usb_interface(HANDLE& deviceHandle);
            ~usb_interface();

            void wait_for_async_op(OVERLAPPED &hOvl, ULONG &lengthTransferred, DWORD TimeOut, pipe_direction pipeDirection, bool* isExitOnTimeout) const;
            bool wait_for_async_op_interrupt(OVERLAPPED &hOvl, ULONG &lengthTransferred) const;
            bool read_pipe(unsigned char* buffer, ULONG bufferLength, PULONG lengthTransferred, LPOVERLAPPED hOvl) const;
            bool write_pipe(const unsigned char* buffer, ULONG bufferLength, PULONG lengthTransferred, LPOVERLAPPED hOvl) const;
            void reset_pipe(pipe_direction outPipe) const;
            bool read_interupt_pipe(unsigned char* buffer, ULONG bufferLength, PULONG lengthTransferred, LPOVERLAPPED hOvl) const;
            void reset_interrupt_pipe() const;

        private:
            void init_interface(HANDLE deviceHandle);
            void release();
            void init_winusb_pipe();
            void query_endpoints();
            void set_timeout_policy(unsigned char pipeID, unsigned long timeOutInMs) const;

            unsigned long _in_out_pipe_timeout_ms = 7000;
            WINUSB_INTERFACE_HANDLE _interface_handle;
            UCHAR _out_pipe_id = 0;
            UCHAR _in_pipe_id = 0;
            UCHAR _interrupt_pipe_id = 0;
        };


        class winusb_device
        {
        public:
            explicit winusb_device(std::wstring lpDevicePath);
            ~winusb_device();
            usb_interface& get_interface() const;
            void recreate_interface();

        private:
            void open(std::wstring lpDevicePath);
            void release();

            std::wstring _lp_device_path;
            HANDLE _device_handle;
            std::unique_ptr<usb_interface> _usb_interface;
        };

        class usb_enumerate {
        public:
            static std::vector<std::wstring> query_by_interface(const std::string& guid, const std::string& vid, const std::string& pid);
            static std::string get_camera_id(const wchar_t* device_id);
        private:
            static bool get_device_data(_GUID guid, int i, const HDEVINFO& device_info, std::wstring& lp_device_path);

        };

        class winusb_bulk_transfer : public usb_device
        {
        public:
            std::vector<uint8_t> send_receive(
                const std::vector<uint8_t>& data,
                int timeout_ms = 5000,
                bool require_response = true) override;

            explicit winusb_bulk_transfer(const usb_device_info& info);
            const wchar_t* get_path() const;

        private:
            void write_to_pipe_and_read_response(const std::vector<uint8_t>& input,
                std::vector<uint8_t>&  output,
                DWORD TimeOut, bool readResponse = true);

            static bool read_pipe_sync(winusb_device* usbDevice, unsigned char* buffer, int bufferLength, ULONG& lengthTransferred, DWORD TimeOut);

            std::wstring _lp_device_path = L"";
            named_mutex _named_mutex;
        };

        class winapi_error : public std::runtime_error
        {
        public:
            explicit winapi_error(const char* message);

            static std::string last_error_string(DWORD lastError);
            static std::string generate_message(const std::string& message);
        };


    }
}
