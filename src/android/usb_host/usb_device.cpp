// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "usb_device.h"
#include "../../types.h"

using namespace librealsense::usb_host;

device::device(usb_device *device) :
        _pull_thread(nullptr),
        _pull_requests(true)
{
    if (device != NULL)
    {
        int TIMEOUT = 10;
        this->_handle = device;
        _usb_device_descriptor = usb_device_get_device_descriptor(device);
        _sProduct = toString(usb_device_get_product_name(_handle, TIMEOUT));
        _sManufacturer = toString(usb_device_get_manufacturer_name(_handle, TIMEOUT));
        _sSerialNumber = toString(usb_device_get_serial(_handle, TIMEOUT));
        _name = std::string(usb_device_get_name(_handle));
        _desc_length = usb_device_get_descriptors_length(_handle);
        build_tree();
        start();
    }
}

void device::claim_interface(int interface)
{
    usb_device_connect_kernel_driver(_handle, interface, false);
    usb_device_claim_interface(_handle, interface);
}

device::~device() {
    stop();
    _pipes.clear();
}

bool device::add_configuration(usb_configuration &usbConfiguration) {
    //LOG_DEBUG("Usb config type: %s",descriptorTypeToString(usbConfiguration.get_descriptor()->bDescriptorType).c_str());
    _configurations.push_back(usbConfiguration);
    return true;
}

void device::add_pipe(uint8_t ep_address, std::shared_ptr<usb_pipe> pipe) {
    LOG_DEBUG("Adding pipe at address " << (int) ep_address);
    if (pipe != nullptr) {
        _pipes[ep_address] = pipe;
    }
}

void device::start()
{
    _pull_requests = true;
    if (_pull_thread == nullptr)
    {
        _pull_thread = std::shared_ptr<std::thread>(new std::thread([&] {
            do {
                auto response = usb_request_wait(_handle, 100);
                if(response == nullptr)
                    continue;
                auto p = _pipes[response->endpoint];
                if (p != nullptr) {
                    p->queue_finished_request(response);
                }
            } while (_pull_requests == true);
        }));
    }
}

void device::stop()
{
    _pull_requests = false;
    if(_pull_thread != nullptr && _pull_thread->joinable())
        _pull_thread->join();
}


void device::build_tree() {
    usb_descriptor_iter it;
    usb_descriptor_iter_init(_handle, &it);

    usb_descriptor_header *h = usb_descriptor_iter_next(&it);
    usb_descriptor_header *prev;
    if (h != nullptr && h->bDescriptorType == USB_DT_DEVICE) {
        usb_device_descriptor *device_descriptor = (usb_device_descriptor *) h;
        _usb_conn_spec = device_descriptor->bcdUSB;
        h = usb_descriptor_iter_next(&it);
        if (h != nullptr && h->bDescriptorType == USB_DT_CONFIG) {
            do {
                usb_config_descriptor *config_descriptor = (usb_config_descriptor *) h;
                usb_configuration configuration(config_descriptor);
                for (int i = 0; i < config_descriptor->bNumInterfaces;) {
                    if (h->bDescriptorType == USB_DT_INTERFACE_ASSOCIATION) {
                        usb_interface_association a(i, (usb_interface_assoc_descriptor *) h);
                        configuration.add_interface_association(a);
                    } else if (h->bDescriptorType == (USB_DT_INTERFACE & ~USB_TYPE_MASK)) {
                        i++;
                        usb_interface_descriptor *interface_descriptor = (usb_interface_descriptor *) h;
                        usb_interface usbInterface(*interface_descriptor);
                        for (int e = 0; e < interface_descriptor->bNumEndpoints;) {
                            h = usb_descriptor_iter_next(&it);
                            if (h->bDescriptorType == (USB_DT_ENDPOINT & ~USB_TYPE_MASK)) {
                                e++;
                                usb_endpoint usb_endpoint(*(usb_endpoint_descriptor *) h);
                                usbInterface.add_endpoint(usb_endpoint);
                                auto pipe = std::make_shared<usb_pipe>(_handle, usb_endpoint);
                                add_pipe(usb_endpoint.get_endpoint_address(), pipe);
                            }
                        }
                        configuration.add_interface(usbInterface);
                    }
                    h = usb_descriptor_iter_next(&it);
                }

                add_configuration(configuration);

            } while (h != nullptr && (h->bDescriptorType == USB_DT_CONFIG));
        } else {
            return;
        }
    }
}

const std::string& device::get_device_description() const {
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
       << get_product() << ")\n"
       << "   Manufacturer: " << (int) desc->iManufacturer << " ("
       << get_manufacturer() << ")\n"
       << "   Serial num: " << (int) desc->iSerialNumber << " ("
       << get_serial_number() << ")\n\n";
    return ss.str();
}
    
std::string device::toString(char *cstr) {
    if (cstr == nullptr)
        return "null";
    std::string str(cstr);
    free(cstr);
    return str;
}


std::string device::descriptorTypeToString(uint8_t dt) {
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

std::string device::classToString(uint8_t cls) {
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
