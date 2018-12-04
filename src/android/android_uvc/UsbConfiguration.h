//
// Created by daniel on 10/16/2018.
//

#pragma once


#include "UsbHost.h"
#include "UsbInterface.h"
#include "UsbInterfaceAssociation.h"
#include <vector>

class UsbConfiguration {
protected:
    UsbConfiguration() {}
    std::vector<UsbInterface> _interfaces;
    std::vector<UsbInterfaceAssociation> _interface_assocs;
    const usb_config_descriptor *_desc;

public:
    UsbConfiguration(usb_config_descriptor *pDescriptor) {
        _desc = pDescriptor;
    }

    void AddInterface(const UsbInterface &usbInterface) {
        _interfaces.push_back(usbInterface);
    }

    int GetNumInterfaces() { return _interfaces.size(); }

    const UsbInterface & GetInterface(int interface_index)const {
        return _interfaces.at(interface_index);
    }
    void AddInterfaceAssociation(const UsbInterfaceAssociation &ia) {
        _interface_assocs.push_back(ia);
    }

    int GetNumInterfaceAssociations() { return _interface_assocs.size(); }

    const UsbInterfaceAssociation & GetInterfaceAssociation(int ia_index)const {
        return _interface_assocs.at(ia_index);
    }

    const usb_config_descriptor * GetDescriptor() const { return _desc; }

};


