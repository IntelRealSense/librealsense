#ifndef LIBUSBHOST_USBMANAGER_H
#define LIBUSBHOST_USBMANAGER_H
#include <string>
#include <unordered_map>
#include <vector>
#include <regex>
#include "usbhost.h"
#include "Android.h"
#include "UsbConfiguration.h"
#include "UsbDevice.h"
using namespace std;
class UsbManager {
protected:
    usb_host_context *_context;
    unordered_map<int,std::shared_ptr<UsbDevice>> _devices;

    UsbManager() {
        _context = usb_host_init();
    };

    virtual ~UsbManager() {
        usb_host_cleanup(_context);
        _devices.clear();
    }


    void BuildDeviceTree(shared_ptr<UsbDevice> device) {
        usb_descriptor_iter it;
        usb_descriptor_iter_init(device->GetHandle(), &it);

        usb_descriptor_header *h = usb_descriptor_iter_next(&it);
        usb_descriptor_header *prev;
        if (h->bDescriptorType == USB_DT_DEVICE && h != nullptr) {
            h = usb_descriptor_iter_next(&it);
            if (h->bDescriptorType == USB_DT_CONFIG && h != nullptr) {
                do {
                    usb_config_descriptor *config_descriptor = (usb_config_descriptor *) h;
                    UsbConfiguration configuration(config_descriptor);
                    for (int i = 0; i < config_descriptor->bNumInterfaces;) {
                        if (h->bDescriptorType == USB_DT_INTERFACE_ASSOCIATION) {
                            UsbInterfaceAssociation a(i, (usb_interface_assoc_descriptor *) h);
                            configuration.AddInterfaceAssociation(a);
                        } else if (h->bDescriptorType == (USB_DT_INTERFACE & ~USB_TYPE_MASK)) {
                            i++;
                            usb_interface_descriptor *interface_descriptor = (usb_interface_descriptor *) h;
                            UsbInterface usbInterface(*interface_descriptor);
                            for (int e = 0; e < interface_descriptor->bNumEndpoints;) {
                                h = usb_descriptor_iter_next(&it);
                                if (h->bDescriptorType == (USB_DT_ENDPOINT & ~USB_TYPE_MASK)) {
                                    e++;
                                    UsbEndpoint usbEndpoint(*(usb_endpoint_descriptor *) h);
                                    usbInterface.AddEndpoint(usbEndpoint);
                                    device->AddPipe(usbEndpoint.GetEndpointAddress(),make_shared<UsbPipe>(device->GetHandle(),usbEndpoint));
                                }
                            }
                            configuration.AddInterface(usbInterface);
                        }
                        h = usb_descriptor_iter_next(&it);
                    }

                    device->AddConfiguration(configuration);

                } while ((h->bDescriptorType == USB_DT_CONFIG) && h != nullptr);
            } else {
                return;
            }
        }
    }

public:
//Singleton methods
    static UsbManager &getInstance() {
        static UsbManager instance;
        return instance;
    }

    UsbManager(const UsbManager &arg) = delete; // Copy constructor
    UsbManager(const UsbManager &&arg) = delete;  // Move constructor
    UsbManager &operator=(const UsbManager &arg) = delete; // Assignment operator
    UsbManager &operator=(const UsbManager &&arg) = delete; // Move operator
    //Device list methods

    bool AddDevice(std::string dev_name, int file_descriptor) {
        if(_devices.find(file_descriptor) != _devices.end())
            return false;
        auto handle = usb_device_new(dev_name.c_str(), file_descriptor);
        usb_device_reset(handle);
        if (handle != NULL) {
            _devices[file_descriptor] = std::make_shared<UsbDevice>(handle);
            BuildDeviceTree(_devices[file_descriptor]);
            LOGD("Added USB device with dev name: %s and FD: %d", dev_name.c_str(),
                 file_descriptor);
            return true;
        } else {
            LOGD("Failed to add USB device with dev name: %s and FD: %d", dev_name.c_str(),
                 file_descriptor);
            return false;
        }
    }


    bool RemoveDevice(int file_descriptor) {
        auto handle = _devices[file_descriptor]->GetHandle();

        if (handle != NULL) {
            usb_device_close(handle);
            _devices.erase(file_descriptor);
            LOGD("Removed USB device with FD: %d", file_descriptor);
            return true;
        } else {
            LOGE("Cannot remove USB device with FD %d ", file_descriptor);
            return false;

        }
    }


    vector<shared_ptr<UsbDevice>> GetDeviceList() {
        std::vector<shared_ptr<UsbDevice>> list;
        for (auto pd: _devices)
            list.push_back(pd.second);
        return list;
    }

    shared_ptr<UsbDevice> GetUsbDeviceFromHandle(usb_device_handle* handle){
       auto list=GetDeviceList();
       for(auto &d: list){
           if(d->GetHandle() == handle)
               return d;
       }
       return nullptr;
    }

};

#endif