//
// Created by daniel on 10/16/2018.
//

#pragma once


#include <vector>
#include "UsbEndpoint.h"
using namespace std;
class UsbInterface {
    usb_interface_descriptor _desc;
    std::vector<UsbEndpoint> _endpoints;
public:
    UsbInterface(usb_interface_descriptor Descriptor) {
        _desc = Descriptor;
    }
    ~UsbInterface(){
    }

    void AddEndpoint(const UsbEndpoint &usbEndpoint) {
        _endpoints.push_back(usbEndpoint);
    }

    int GetNumEndpoints() { return _endpoints.size(); }

    const UsbEndpoint & GetEndpoint(int endpoint_index)const {
        return _endpoints.at(endpoint_index);
    }

    const usb_interface_descriptor&  GetDescriptor() const { return _desc; }

};


