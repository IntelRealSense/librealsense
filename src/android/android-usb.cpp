// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS2_USE_USBHOST_UVC_BACKEND

#include <mutex>
#include <android/usbhost_uvc/usbmanager.h>
#include "android-usb.h"





namespace librealsense {
    namespace platform {
        android_usb_device::android_usb_device(usb_device_handle *usbDevice)
                : _device_handle(usbDevice),
                  _usb_interface(nullptr) {
            open();
        }

        android_usb_device::~android_usb_device() {
            try {
                release();
            }
            catch (...) {

            }
        }

        void android_usb_device::release() {
            if (_device_handle) {
                _device_handle = nullptr;
            }
        }

        void android_usb_device::open() {
            if (_device_handle)
                return;
            _usb_device = UsbManager::getInstance().GetUsbDeviceFromHandle(_device_handle);
            _usb_interface = std::unique_ptr<usb_interface>(new usb_interface(_device_handle, 0));
        }


        usb_device_handle *android_usb_device::get_device_handle() const {
            return _device_handle;
        }

        usb_interface &android_usb_device::get_usb_interface() const {
            return *_usb_interface;
        }

        shared_ptr<UsbDevice> android_usb_device::get_usb_device() const {
            return _usb_device;
        }


        usb_interface::~usb_interface() {
            try {
                release();
            }
            catch (...) {
                // TODO: Write to log
            }
        }

        usb_interface::usb_interface(usb_device_handle *deviceHandle, int interface_index) {
            init_interface(deviceHandle, interface_index);
            init_usb_pipe();
        }

        void usb_interface::init_interface(usb_device_handle *deviceHandle, int interface_index) {
            if (_interface_handle.device != nullptr)
                return;
            _interface_handle.device = deviceHandle;
            _interface_handle.interface_index = interface_index;
            _usbDevice = UsbManager::getInstance().GetUsbDeviceFromHandle(deviceHandle);
        }

        void usb_interface::release() {
            _interface_handle.device = nullptr;
        }

        void
        usb_interface::set_timeout_policy(unsigned char pipeID, unsigned long timeOutInMs) const {
            //TODO: do we need this?
        }


        bool usb_interface::wait_for_async_op_interrupt(
                ULONG &lengthTransferred) const {
            lengthTransferred = static_cast<ULONG>(usb_request_wait(
                    _interface_handle.device)->actual_length);
            return true;
        }

        void usb_interface::query_endpoints() {

            if (_interface_handle.device == nullptr)
                throw std::runtime_error("Interface handle is NULL!");

            usb_interface_descriptor InterfaceDescriptor;
            const UsbInterface &interface = _usbDevice->GetConfiguration(0).GetInterface(
                    _interface_handle.interface_index);


            for (auto index = static_cast<UCHAR>(0);
                 index < InterfaceDescriptor.bNumEndpoints; ++index) {
                UsbEndpoint endpoint = interface.GetEndpoint(index);
                const usb_endpoint_descriptor *ep_desc = endpoint.GetDescriptor();
                int xfer_type = ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
                if (xfer_type == USB_ENDPOINT_XFER_BULK) //BULK transfer
                {
                    int direction = (ep_desc->bEndpointAddress & USB_ENDPOINT_DIR_MASK);
                    if (direction == USB_DIR_IN) // Bulk Transfer In
                    {
                        _in_pipe_id = ep_desc->bEndpointAddress;
                    } else { //Bulk transfer out
                        _out_pipe_id = ep_desc->bEndpointAddress;
                    }
                } else if (xfer_type == USB_ENDPOINT_XFER_INT) // Interrupt Transfer
                {
                    _interrupt_pipe_id = ep_desc->bEndpointAddress;
                }
                _ep_desc=*ep_desc;
                // TODO: Isochronous Transfer and Control Transfer
            }
        }

        void usb_interface::init_usb_pipe() {
            // initialize _dataInPipeID and _dataOutPipeID that will be used below.
            query_endpoints();

            if (_in_pipe_id != 0)
                set_timeout_policy(_in_pipe_id, _in_out_pipe_timeout_ms);

            if (_out_pipe_id != 0)
                set_timeout_policy(_out_pipe_id, _in_out_pipe_timeout_ms);

        }

        bool usb_interface::read_pipe(unsigned char *buffer, ULONG bufferLength,
                                      PULONG lengthTransferred) const {
            int ret = usb_device_bulk_transfer(_interface_handle.device, _in_pipe_id, buffer,
                                               bufferLength, IN_PIPE_TIMEOUT);
            if (ret < 0) {
                *lengthTransferred = 0;
                return false;
            }
            *lengthTransferred = static_cast<ULONG>(ret);
            return true;
        }

        bool usb_interface::read_interrupt_pipe(unsigned char *buffer, ULONG bufferLength,
                                                PULONG lengthTransferred) const {
            int ret = usb_device_bulk_transfer(_interface_handle.device, _interrupt_pipe_id, buffer,
                                               bufferLength, IN_PIPE_TIMEOUT);
            if (ret < 0) {
                *lengthTransferred = 0;
                return false;
            }
            *lengthTransferred = static_cast<ULONG>(ret);
            return true;
        }

        bool usb_interface::write_pipe(const unsigned char *buffer, ULONG bufferLength,
                                       PULONG lengthTransferred) const {
            int ret = usb_device_bulk_transfer(_interface_handle.device, _out_pipe_id,
                                               (void *) buffer,
                                               bufferLength, IN_PIPE_TIMEOUT);
            if (ret < 0) {
                *lengthTransferred = 0;
                return false;
            }
            *lengthTransferred = static_cast<ULONG>(ret);
            return true;
        }

        void usb_interface::reset_interrupt_pipe() const {
            //TODO: do we need this?
        }

        void usb_interface::reset_pipe(pipe_direction pipeDirection) const {
            //TODO: do we need this?
        }


        android_bulk_transfer::android_bulk_transfer(const usb_device_info &info) {
        }

        void
        android_bulk_transfer::write_to_pipe_and_read_response(const std::vector<uint8_t> &input,
                                                               std::vector<uint8_t> &output,
                                                               int TimeOut, bool readResponse) {
            std::lock_guard<std::mutex> lock(_named_mutex);
            //TODO: fill this with bulk transfer?
        }

        bool android_bulk_transfer::read_pipe_sync(android_usb_device *androidUsbDevice,
                                                   unsigned char *buffer, int bufferLength,
                                                   int &lengthTransferred, int TimeOut) {

            //TODO: fill this with bulk transfer?



            return true;
        }

        std::vector<uint8_t>
        android_bulk_transfer::send_receive(const std::vector<uint8_t> &data, int timeout_ms,
                                            bool require_response) {
            return std::vector<uint8_t>();
        }


    }
}

#endif
