//
// Created by daniel on 10/22/2018.
//

#pragma once
class UsbInterfaceAssociation {
    int _mi;

private:
    usb_interface_assoc_descriptor *_desc;
    std::vector< unsigned char > _interfaces;

public:
    UsbInterfaceAssociation(int interface_index,usb_interface_assoc_descriptor *pDescriptor) {
        _desc = pDescriptor;
        _mi=interface_index;
    }

    void AddInterface(__u8 usbInterface) {
        _interfaces.push_back(usbInterface);
    }

    int GetNumInterfaces() { return _interfaces.size(); }

    const unsigned char & GetInterface(int interface_index)const {
        return _interfaces.at(interface_index);
    }

    const usb_interface_assoc_descriptor * GetDescriptor() const { return _desc; }

    int GetMi() const {
        return _mi;
    }

};


