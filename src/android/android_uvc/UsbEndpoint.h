//
// Created by daniel on 10/16/2018.
//

#pragma once

#include "ch9_custom.h"

class UsbEndpoint {
    usb_endpoint_descriptor _desc;
public:
    UsbEndpoint(usb_endpoint_descriptor Descriptor) {
        _desc = Descriptor;
    }

    const usb_endpoint_descriptor * GetDescriptor() const { return &_desc; }

    int GetMaxPacketSize() {
        return __le16_to_cpu(_desc.wMaxPacketSize);
    }

    uint8_t GetEndpointAddress() {
        return _desc.bEndpointAddress;
    }

};


