// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "../backend.h"

#include "android_uvc/UsbHost.h"
#include "android_uvc/android_uvc.h"

#define HW_MONITOR_BUFFER_SIZE   (1024)
typedef unsigned long ULONG;
typedef ULONG *PULONG;
typedef unsigned int DWORD;
namespace librealsense {
    namespace platform {
        enum class pipe_direction {
            InPipe,
            OutPipe
        };
        typedef struct interface_handle {
            usb_device_handle *device;
            int interface_index;
        } interface_handle;

        class usb_interface {
        public:
            usb_interface(usb_device_handle *deviceHandle, int interface_index);

            ~usb_interface();

            bool wait_for_async_op_interrupt(ULONG &lengthTransferred) const;

            bool
            read_pipe(unsigned char *buffer, ULONG bufferLength, PULONG lengthTransferred) const;

            bool write_pipe(const unsigned char *buffer, ULONG bufferLength,
                            PULONG lengthTransferred) const;

            void reset_pipe(pipe_direction outPipe) const;

            bool read_interrupt_pipe(unsigned char *buffer, ULONG bufferLength,
                                     PULONG lengthTransferred) const;

            void reset_interrupt_pipe() const;



        private:
            void init_interface(usb_device_handle *deviceHandle, int interface_index);

            void release();

            void init_usb_pipe();

            void query_endpoints();

            void set_timeout_policy(unsigned char pipeID, unsigned long timeOutInMs) const;

            unsigned long _in_out_pipe_timeout_ms = 7000;
            interface_handle _interface_handle;
            UCHAR _out_pipe_id = 0;
            UCHAR _in_pipe_id = 0;
            UCHAR _interrupt_pipe_id = 0;
            shared_ptr<UsbDevice> _usbDevice;
            const unsigned int IN_PIPE_TIMEOUT = USB3_LPM_U2_MAX_TIMEOUT;
            const unsigned int OUT_PIPE_TIMEOUT = USB3_LPM_U2_MAX_TIMEOUT;
            Queue<usb_request*> _requests_finished;
            std::vector<usb_request*> _requests;
            usb_endpoint_descriptor _ep_desc;
        };

        class android_usb_device {
        public:
            explicit android_usb_device(usb_device_handle *device);

            ~android_usb_device();


        private:
            void open();

            void release();

            usb_device_handle * _device_handle;
            std::unique_ptr<usb_interface> _usb_interface;
            shared_ptr<UsbDevice> _usb_device;
        public:
            usb_device_handle *get_device_handle() const;

            usb_interface &get_usb_interface() const;

            shared_ptr<UsbDevice> get_usb_device() const;
        };


        class android_bulk_transfer : public usb_device {
        public:
            std::vector<uint8_t> send_receive(
                    const std::vector<uint8_t> &data,
                    int timeout_ms = 5000,
                    bool require_response = true) override;

            explicit android_bulk_transfer(const usb_device_info &info);

        private:
            void write_to_pipe_and_read_response(const std::vector<uint8_t> &input,
                                                 std::vector<uint8_t> &output,
                                                 int TimeOut, bool readResponse = true);

            static bool read_pipe_sync(android_usb_device *androidUsbDevice, unsigned char *buffer,
                                       int bufferLength, int &lengthTransferred, int TimeOut);

            std::mutex _named_mutex;
        };


    }
}
