//
// Created by daniel on 10/16/2018.
//

#ifndef LIBUSBHOST_USBDEVICE_H
#define LIBUSBHOST_USBDEVICE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <regex>
#include <iostream>
#include <random>
#include "UsbHost.h"
#include "android_debug.h"
#include "UsbStringHelpers.h"
#include "UsbConfiguration.h"
#include <memory>

#include <string>
#include "UsbHost.h"
#include "UsbPipe.h"
#include <string>
#include <regex>
#include <sstream>




class UsbDevice {
private:


    std::string toString(char *cstr) {
        if (cstr == nullptr)
            return "null";
        std::string str(cstr);
        free(cstr);
        return str;
    }

    std::string descriptorTypeToString(uint8_t dt) const {
        switch (dt) {
            case USB_DT_DEVICE:
                return "Device";
            case USB_DT_CONFIG:
                return "Config";
            case USB_DT_INTERFACE:
                return "Interface";
            case USB_DT_ENDPOINT:
                return "Endpoint";
            case USB_DT_STRING:
                return "String";
            case USB_DT_CS_DEVICE:
                return "Class-specific device";
            case USB_DT_CS_CONFIG:
                return "Class-specific config";
            case USB_DT_CS_INTERFACE:
                return "Class-specific interface";
            case USB_DT_CS_ENDPOINT:
                return "Class-specific endpoint";
            case USB_DT_CS_STRING:
                return "Class-specific string";
            case USB_DT_DEVICE_QUALIFIER:
                return "Device qualifier";
            case USB_DT_OTHER_SPEED_CONFIG:
                return "Other speed config";
            case USB_DT_INTERFACE_POWER:
                return "Interface power";
            case USB_DT_OTG:
                return "OTG";
            case USB_DT_DEBUG:
                return "Debug";
            case USB_DT_INTERFACE_ASSOCIATION:
                return "Interface association";
            case USB_DT_SECURITY:
                return "Security";
            case USB_DT_KEY:
                return "Key";
            case USB_DT_ENCRYPTION_TYPE:
                return "Enrcyption type";
            case USB_DT_BOS:
                return "BOS";
            case USB_DT_DEVICE_CAPABILITY:
                return "Capability";
            case USB_DT_WIRELESS_ENDPOINT_COMP:
                return "Wireless endpoint comp";
            case USB_DT_SS_ENDPOINT_COMP:
                return "Endpoint comp";
            default:
                return "Other/Unknown";
        }
    }

    std::string classToString(uint8_t cls) const {
        switch (cls) {
            case USB_CLASS_APP_SPEC:
                return "App. specific";
            case USB_CLASS_AUDIO:
                return "Audio";
            case USB_CLASS_CDC_DATA:
                return "CDC Data";
            case USB_CLASS_COMM:
                return "Comm";
            case USB_CLASS_CONTENT_SEC:
                return "Content Sec.";
            case USB_CLASS_CSCID:
                return "CSCID";
            case USB_CLASS_HID:
                return "HID";
            case USB_CLASS_HUB:
                return "HUB";
            case USB_CLASS_MASS_STORAGE:
                return "Mass storage";
            case USB_CLASS_MISC:
                return "Misc";
            case USB_CLASS_PER_INTERFACE:
                return "Per interface";
            case USB_CLASS_PHYSICAL:
                return "Physical";
            case USB_CLASS_PRINTER:
                return "Printer";
            case USB_CLASS_STILL_IMAGE:
                return "Still image";
            case USB_CLASS_VENDOR_SPEC:
                return "Vendor specific";
            case USB_CLASS_VIDEO:
                return "Video";
            case USB_CLASS_WIRELESS_CONTROLLER:
                return "Wireless controller";
            default:
                return "Other/Unknown";
        }
    }

protected:

    usb_device_handle *_handle;
    std::vector<UsbConfiguration> _configurations;
    const usb_device_descriptor *_usb_device_descriptor;
    std::string _sProduct;
    std::string _sManufacturer;
    std::string _sSerialNumber;
    unique_ptr<std::thread> _pull_thread;
    unordered_map<int, shared_ptr<UsbPipe>> _pipes;

public:
    UsbDevice() :
            _pull_thread(nullptr) {
        _pull_requests=true;

    }

    ~UsbDevice() {
        _pipes.clear();
        _pull_requests=false;


    }

    shared_ptr<UsbPipe> GetPipe(int ep_address) {
        return _pipes[ep_address];
    }

    UsbDevice(usb_device_handle *usb_device) {
        if (usb_device != NULL) {
            int TIMEOUT=10;
            this->_handle = usb_device;
            _usb_device_descriptor = usb_device_get_device_descriptor(usb_device);
            _sProduct = toString(
                    usb_device_get_string(usb_device, _usb_device_descriptor->iProduct,TIMEOUT));
            _sManufacturer = toString(
                    usb_device_get_string(usb_device, _usb_device_descriptor->iManufacturer,TIMEOUT));
            _sSerialNumber = toString(
                    usb_device_get_string(usb_device, _usb_device_descriptor->iSerialNumber,TIMEOUT));
        }
    }

    bool AddConfiguration(UsbConfiguration &usbConfiguration) {
        //LOGD("Usb config type: %s",descriptorTypeToString(usbConfiguration.GetDescriptor()->bDescriptorType).c_str());
        _configurations.push_back(usbConfiguration);
        return true;
    }

    const usb_device_descriptor *GetDescriptor() const { return _usb_device_descriptor; };

    int GetNumConfigurations() { return _configurations.size(); }

    const UsbConfiguration &GetConfiguration(int config_index) {
        return _configurations.at(config_index);
    }

    usb_device_handle *GetHandle() const { return _handle; }


    const std::string &GetStringProduct() const {
        return _sProduct;
    }

    const std::string &GetStringManufacturer() const {
        return _sManufacturer;
    }

    const std::string &GetStringSerialNumber() const {
        return _sSerialNumber;
    }

    std::string GetDeviceDescription() const {
        auto desc = _usb_device_descriptor;
        std::stringstream ss;

        ss << "Type: " << (int) desc->bDescriptorType << " ("
           << descriptorTypeToString(desc->bDescriptorType) << ")\n"
           << "   Length: " << (int) desc->bLength << std::endl
           << "   USB version: " << std::hex << (int) __le16_to_cpu(desc->bcdUSB) << std::dec
           << std::endl
           << "   Device version: " << (int) __le16_to_cpu(desc->bcdDevice) << std::endl
           << "   Class: " << (int) desc->bDeviceClass << " ("
           << classToString(desc->bDeviceClass) << ")\n"
           << "   Subclass: " << (int) desc->bDeviceSubClass << std::endl
           << "   Protocol: " << std::hex << (int) desc->bDeviceProtocol << std::dec
           << std::endl
           << "   Num configs: " << (int) desc->bNumConfigurations << std::endl
           << "   Product ID: " << (int) __le16_to_cpu(desc->idProduct) << std::endl
           << "   Vendor ID: " << (int) __le16_to_cpu(desc->idVendor) << std::endl
           << "   Product: " << (int) desc->iProduct << " ("
           << GetStringProduct() << ")\n"
           << "   Manufacturer: " << (int) desc->iManufacturer << " ("
           << GetStringManufacturer() << ")\n"
           << "   Serial num: " << (int) desc->iSerialNumber << " ("
           << GetStringSerialNumber() << ")\n\n";
        return ss.str();
    }

    int GetVid() {
        return usb_device_get_vendor_id(_handle);
    }

    int GetPid() {
        return usb_device_get_product_id(_handle);
    }


    void AddPipe(uint8_t ep_address, std::shared_ptr<UsbPipe> pipe) {
        LOGD("Adding pipe at address %d ", (int) ep_address);
        if (pipe != nullptr) {
            _pipes[ep_address] = pipe;
            if (_pull_thread == nullptr)
                _pull_thread = std::unique_ptr<std::thread>(new thread([&] {
                    do {
                        usb_request *req = usb_request_wait(_handle,1000);
                        if (req != nullptr) {
                            auto p = _pipes[req->endpoint];
                            if (p != nullptr) {
                                p->QueueFinishedRequest(req);
                            }
                        } else {
                            sleep(1);
                        }
                    } while (_pull_requests == true);
                }));
        }
    }

private:
    bool _pull_requests = true;

    mutex m;
};

#endif




