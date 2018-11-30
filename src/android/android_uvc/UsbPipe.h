//
// Created by daniel on 10/29/2018.
//

#ifndef LIBUSBHOST_USBPIPE_H
#define LIBUSBHOST_USBPIPE_H


#include <cstdlib>
#include <cstring>
#include <zconf.h>
#include "usbhost.h"
#include "UsbEndpoint.h"
#include <linux/usbdevice_fs.h>
#include <unordered_map>
#include "Queue.h"
#include <chrono>


using namespace std;

class UsbPipe {


    void submit_request(uint8_t *buffer, size_t buffer_len) {
        auto req = usb_request_new(_device, _endpoint.GetDescriptor());
        req->buffer = buffer;
        req->buffer_length = buffer_len;
        usb_request_queue(req);
    }


public:

    UsbPipe(usb_device_handle *usb_device, UsbEndpoint endpoint, int buffer_size = 32) :
            _endpoint(endpoint), _device(usb_device) {
        _device = usb_device;
        _endpoint = endpoint;

    }

    virtual ~UsbPipe() {

    }

    size_t ReadPipe(uint8_t *buffer, size_t buffer_len,unsigned int timeout_ms=0) {
        using namespace std::chrono;
        int bytes_copied = 0;
        // Wait until pipe gets data
        std::unique_lock<std::mutex> lk(m);
        submit_request(buffer, buffer_len);
        high_resolution_clock::time_point t1 = high_resolution_clock::now();




        cv.wait(lk, [&] {
            if(timeout_ms>0){
                high_resolution_clock::time_point t2 = high_resolution_clock::now();
                duration<double, std::milli> time_span = t2 - t1;

                if(time_span.count() > timeout_ms)
                    return false;
            }
            auto itInner = _requests_filled.find(buffer);
            return (itInner != _requests_filled.end());
        });
        auto req = _requests_filled[buffer];
        if (req != nullptr) {
            bytes_copied = req->actual_length;
            _requests_filled.erase(buffer);
        }
        //TODO: check this
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
