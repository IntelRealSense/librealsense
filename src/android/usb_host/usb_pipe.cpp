// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "usb_pipe.h"

#include <cstdlib>
#include <cstring>
#include <zconf.h>
#include "usbhost.h"
#include "usb_endpoint.h"
#include <linux/usbdevice_fs.h>
#include "android_uvc.h"
#include <chrono>
#include "../../types.h"

#define INTERRUPT_BUFFER_SIZE 1024

using namespace librealsense::usb_host;

usb_pipe::usb_pipe(usb_device *usb_device, usb_endpoint endpoint) :
        _endpoint(endpoint),
        _device(usb_device)
{
    _request = std::shared_ptr<usb_request>(usb_request_new(_device, _endpoint.get_descriptor()), [](usb_request* req) {usb_request_free(req);});
    auto attr = _endpoint.get_descriptor()->bmAttributes;
    if(attr == USB_ENDPOINT_XFER_INT)
    {
        _interrupt_buffer = std::vector<uint8_t>(INTERRUPT_BUFFER_SIZE);
        queue_interrupt_request();
    }
}

usb_pipe::~usb_pipe()
{
    usb_request_cancel(_request.get());
}

bool usb_pipe::reset()
{
    return usb_device_control_transfer(_device,
                                       0x02, //UVC_FEATURE,
                                       0x01, //CLEAR_FEATURE
                                       0, _endpoint.get_endpoint_address(), NULL, 0, 10) == UVC_SUCCESS;
}

size_t usb_pipe::read_pipe(uint8_t *buffer, size_t buffer_len, unsigned int timeout_ms) {
    using namespace std::chrono;
    int bytes_copied = -1;
    // Wait until pipe gets data
    std::unique_lock<std::mutex> lk(_mutex);
    submit_request(buffer, buffer_len);

    auto res = _cv.wait_for(lk, std::chrono::milliseconds(timeout_ms), [&] { return _received; });
    _received = false;
    if (res == true) {
        bytes_copied = _request->actual_length;
    }
    else {
        LOG_WARNING("Timeout reached waiting for response!");
        usb_request_cancel(_request.get());
    }
    lk.unlock();

    return bytes_copied;
}

void usb_pipe::queue_finished_request(usb_request* response) {
    assert (response->endpoint == _endpoint.get_endpoint_address());

    std::unique_lock<std::mutex> lk(_mutex);
    _received = true;
    lk.unlock();
    _cv.notify_all();
    if(_interrupt_buffer.size() > 0)
        queue_interrupt_request();
}
            
void usb_pipe::submit_request(uint8_t *buffer, size_t buffer_len) {
    _request->buffer = buffer;
    _request->buffer_length = buffer_len;
    int res = usb_request_queue(_request.get());
    if(res < 0) {
        LOG_ERROR("Cannot queue request: " << strerror(errno));
    }
}

void usb_pipe::queue_interrupt_request()
{
    std::lock_guard<std::mutex> lock(_mutex);
    submit_request(_interrupt_buffer.data(), _interrupt_buffer.size());
}