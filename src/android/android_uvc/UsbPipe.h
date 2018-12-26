//
// Created by daniel on 10/29/2018.
//

#ifndef LIBUSBHOST_USBPIPE_H
#define LIBUSBHOST_USBPIPE_H


#include <cstdlib>
#include <cstring>
#include <zconf.h>
#include "UsbHost.h"
#include "UsbEndpoint.h"
#include <linux/usbdevice_fs.h>
#include <unordered_map>
#include "Queue.h"
#include <chrono>

using namespace std::chrono;


using namespace std;

class UsbPipe {


    usb_request *submit_request(uint8_t *buffer, size_t buffer_len) {
        auto req = usb_request_new(_device, _endpoint.GetDescriptor());
        req->buffer = buffer;
        req->buffer_length = buffer_len;
        int res=usb_request_queue(req);
        if(res<0) {
            LOGE("Cannot queue request: %s", strerror(errno));
            return nullptr;
        }
        return req;
    }


public:

    UsbPipe(usb_device_handle *usb_device, UsbEndpoint endpoint, int buffer_size = 32) :
            _endpoint(endpoint), _device(usb_device) {
        _device = usb_device;
        _endpoint = endpoint;

    }

    virtual ~UsbPipe() {

    }

    size_t ReadPipe(uint8_t *buffer, size_t buffer_len, unsigned int timeout_ms = 1000) {
        using namespace std::chrono;
        int bytes_copied = 0;
        // Wait until pipe gets data
        std::unique_lock<std::mutex> lk(m);
        auto request = submit_request(buffer, buffer_len);
        if(request== nullptr){
            lk.unlock();
            return -1;
        }
        auto res = cv.wait_for(lk, chrono::milliseconds(timeout_ms), [&] {
            auto itInner = _requests_filled.find(buffer);
            return (itInner != _requests_filled.end());
        });
        if (res == true) {
            auto req = _requests_filled[buffer];
            bytes_copied = req->actual_length;
            _requests_filled.erase(buffer);
        } else {
            LOGE("Timeout reached waiting for response!");
            usb_request_cancel(request);
            bytes_copied = -1;
        }
        lk.unlock();

        return bytes_copied;
    }

    void QueueFinishedRequest(usb_request *request) {
        if (request->endpoint == _endpoint.GetEndpointAddress()) {
            std::unique_lock<std::mutex> lk(m);
            _requests_filled[request->buffer] = request;
            lk.unlock();
            cv.notify_all();
        }
    }

private:
    std::mutex m;
    std::condition_variable cv;
    std::string data;
    bool ready = false;
    usb_device_handle *_device;
    UsbEndpoint _endpoint;
    std::unordered_map<void *, usb_request *> _requests_filled;
};


#endif //LIBUSBHOST_USBPIPE_H
