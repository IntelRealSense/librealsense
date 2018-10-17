// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#define LOG_TAG "UsbListener"
#define LOG_NDEBUG 0 /* Enable LOGV */

#include "UsbPlugListener.h"
#include <chrono>
#include <list>
#include "Manager.h"

using namespace std::chrono;

typedef enum {
    USB_1_0 = 0x0100, // USB 1.0 (Low Speed 1.5 Mbps)
    USB_1_1 = 0x0110, // USB 1.1 (Full Speed 12 Mbps)
    USB_2_0 = 0x0200, // USB 2.0 (High Speed 480 Mbps)
    USB_2_1 = 0x0210, // USB 2.1 (High Speed 480 Mbps) 
    USB_3_0 = 0x0300, // USB 3.0 (Super Speed 5.0 Gbps)
    USB_3_1 = 0x0310, // USB 3.1 (Super Speed + 10.0 Gbps)
} USB_SPECIFICIATION_VERSION;

perc::UsbPlugListener::UsbPlugListener(Owner& owner) : mOwner(owner), mMessage(0)
{
    // schedule the listener itself for every 100 milliseconds
    mOwner.dispatcher().scheduleTimer(this, ENUMERATE_TIMEOUT_NSEC, mMessage);
}

void perc::UsbPlugListener::onTimeout(uintptr_t timerId, const Message & msg)
{
    EnumerateDevices();

    /* Reschedule the listener itself for every 100 milliseconds */
    mOwner.dispatcher().scheduleTimer(this, ENUMERATE_TIMEOUT_NSEC, mMessage);
}

bool perc::UsbPlugListener::identifyDevice(libusb_device_descriptor* desc)
{
    if ((desc->idProduct == TM2_DEV_PID) && (desc->idVendor == TM2_DEV_VID) && (desc->bcdUSB >= USB_2_0))
    {
        return true;
    }

    return false;
}

bool perc::UsbPlugListener::identifyUDFDevice(libusb_device_descriptor* desc)
{
    if ((desc->idVendor == TM2_UDF_DEV_VID) && (desc->idProduct == TM2_UDF_DEV_PID) && (desc->bcdUSB >= USB_2_0))
    {
        return true;
    }

    return false;
}

perc::UsbPlugListener::~UsbPlugListener()
{
    // todo
}

const char* perc::UsbPlugListener::usbSpeed(uint16_t bcdUSB)
{
    switch (bcdUSB)
    {
        case USB_1_0: return "USB 1.0 (Low Speed 1.5 Mbps)";
        case USB_1_1: return "USB 1.1 (Full Speed 12 Mbps)";
        case USB_2_0: return "USB 2.0 (High Speed 480 Mbps)";
        case USB_2_1: return "USB 2.1 (High Speed 480 Mbps)"; /* USB 3.0 device connected to USB 2.0 HUB */
        case USB_3_0: return "USB 3.0 (Super Speed 5.0 Gbps)";
        case USB_3_1: return "USB 3.1 (Super Speed+ 10.0 Gbps)";
    }
    return "Unknown USB speed";
}


void perc::UsbPlugListener::EnumerateDevices()
{
    libusb_device **list = NULL;
    int rc = 0;
    ssize_t count = 0;

    count = libusb_get_device_list(NULL, &list);

    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    // mark all devices as not found to track removed devices
    for (auto it = mDeviceToPortMap.begin(); it != mDeviceToPortMap.end(); it++)
    {
        it->second = false;
    }

    for (ssize_t idx = 0; idx < count; ++idx)
    {
        libusb_device *device = list[idx];
        libusb_device_descriptor desc = { 0 };

        rc = libusb_get_device_descriptor(device, &desc);

        LOGV("%d: VID 0x%04X, PID 0x%04X, bcdUSB 0x%x, iSerialNumber %d", idx, desc.idVendor, desc.idProduct, desc.bcdUSB, desc.iSerialNumber);

        if ((identifyDevice(&desc) == true) || (identifyUDFDevice(&desc) == true))
        {
            std::lock_guard<std::mutex> lk(mDeviceToPortMapLock);
            DeviceToPortMap devicePort(device);
            Status st = Status::SUCCESS;

            if (mDeviceToPortMap.find(devicePort) == mDeviceToPortMap.end())
            {
                LOGD("Found USB device %s on port %d: VID 0x%04X, PID 0x%04X, %s",(desc.idVendor == TM2_UDF_DEV_VID)?"Movidius":"T250", devicePort.portChain[0], desc.idVendor, desc.idProduct, usbSpeed(desc.bcdUSB));
                st = mOwner.onAttach(device);
            }

            // just mark the device as found in this list
            if (st == Status::SUCCESS)
            {
                mDeviceToPortMap[devicePort] = true;
            }
        }
    }

    // look for unplugged devices (==not found in the list)
    std::list<DeviceToPortMap> devicesToDelete;
    for (auto item : mDeviceToPortMap)
    {
        if (item.second == false)   // mark the device for deletion
        {
            devicesToDelete.push_back(item.first);
        }
    }

    for (auto devicePort : devicesToDelete) // ... and delete it
    {
        // send message to client handler ON_DETACH
        std::lock_guard<std::mutex> lock(mDeviceToPortMapLock);
        mDeviceToPortMap.erase(devicePort);
        mOwner.onDetach(devicePort.libusbDevice);
    }
    
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(t2 - t1).count();

    LOGV("Devices connected: %d. Time: %d micro-seconds", mDeviceToPortMap.size(), duration);

    // free the list
    libusb_free_device_list(list, 1);
}
