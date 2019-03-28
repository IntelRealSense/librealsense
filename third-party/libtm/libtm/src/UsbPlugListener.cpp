// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#define LOG_TAG "UsbListener"
#define LOG_NDEBUG 0 /* Enable LOGV */

#include "UsbPlugListener.h"
#include <chrono>
#include <list>
#include "Manager.h"

using namespace std::chrono;

const size_t ENUMERATE_TIMEOUT_NSEC         = 500000000; // 500 msec periodic query
const size_t ENUMERATE_TIMEOUT_STARTUP_NSEC = 10000000;  // 10 msec at startup

typedef enum {
    USB_1_0 = 0x0100, // USB 1.0 (Low Speed 1.5 Mbps)
    USB_1_1 = 0x0110, // USB 1.1 (Full Speed 12 Mbps)
    USB_2_0 = 0x0200, // USB 2.0 (High Speed 480 Mbps)
    USB_2_1 = 0x0210, // USB 2.1 (High Speed 480 Mbps) 
    USB_3_0 = 0x0300, // USB 3.0 (Super Speed 5.0 Gbps)
    USB_3_1 = 0x0310, // USB 3.1 (Super Speed + 10.0 Gbps)
} USB_SPECIFICIATION_VERSION;

perc::UsbPlugListener::UsbPlugListener(Owner& owner) : mOwner(owner), mMessage(0),
    mInitialized(usb_setup_init), mNextScanIntervalMs(ENUMERATE_TIMEOUT_STARTUP_NSEC), mDevicesToProcess(0)
{
    // schedule the listener to query USB devices according to the initialization phase: 10 msec during boot, then 500 msec
    mOwner.dispatcher().scheduleTimer(this, mNextScanIntervalMs, mMessage);
}

void perc::UsbPlugListener::onTimeout(uintptr_t timerId, const Message & msg)
{
    EnumerateDevices();

    /* Reschedule the listener itself */
    mOwner.dispatcher().scheduleTimer(this, mNextScanIntervalMs, mMessage);
}

bool perc::UsbPlugListener::identifyDevice(libusb_device_descriptor* desc)
{
    if ((desc->idProduct == TM2_T260_PID || desc->idProduct == TM2_T265_PID) && (desc->idVendor == TM2_DEV_VID) && (desc->bcdUSB >= USB_2_0))
    {
        return true;
    }

    return false;
}

bool perc::UsbPlugListener::identifyDFUDevice(libusb_device_descriptor* desc)
{
    if ((desc->idVendor == TM2_DFU_DEV_VID) && (desc->idProduct == TM2_DFU_DEV_PID) && (desc->bcdUSB >= USB_2_0))
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
    size_t bootLoader_count = 0;
    size_t TM_count = 0;

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

        auto tm2_dev = identifyDevice(&desc);
        auto dfu_dev = identifyDFUDevice(&desc);
        if (tm2_dev || dfu_dev)
        {
            std::lock_guard<std::mutex> lk(mDeviceToPortMapLock);
            DeviceToPortMap devicePort(device);
            Status st = Status::SUCCESS;

            if (mDeviceToPortMap.find(devicePort) == mDeviceToPortMap.end())
            {
                LOGD("Found USB device %s on port %d: VID 0x%04X, PID 0x%04X, %s",(desc.idVendor == TM2_DFU_DEV_VID)?"Movidius":(desc.idProduct == TM2_T265_PID)?"T265":"T260", devicePort.portChain[0], desc.idVendor, desc.idProduct, usbSpeed(desc.bcdUSB));
                if (dfu_dev) bootLoader_count++;
                if (tm2_dev) TM_count++;

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

    switch (mInitialized.load())
    {
    case usb_setup_init:
        mInitialized.store(usb_setup_progress);
        // One-time overhead: schedule 1 sec for loading if DFU device(s) were present, otherwise 300 msec. 
        mUSBMinTimeout = mUSBSetupTimeout = systemTime() + (bootLoader_count ? 1000000000 : 300000000);
        mDevicesToProcess = bootLoader_count;
        LOGT("usb_setup_init, %d dfu devices, %d TM2 devices, time=%lu , min timeout =%lu", bootLoader_count, TM_count, systemTime(), mUSBSetupTimeout);
        break;
    case usb_setup_progress:
        // Devices discovered during the setup phase will add 1 sec
        if (bootLoader_count)
        {
            mUSBSetupTimeout = systemTime() + 1000000000;
            mDevicesToProcess += bootLoader_count;
            LOGT("usb_setup_progress, %d new dfu devices, time=%lu, new timeout=%lu", bootLoader_count, systemTime(), mUSBSetupTimeout);
        }

        if (TM_count)
        {
            LOGT("New TM2 discovered were  %d time=%lu, timeout=%lu", TM_count, systemTime(), mUSBSetupTimeout);
        }
        mDevicesToProcess -= TM_count;

        if (systemTime() >= mUSBSetupTimeout)
        {
            mInitialized.store(usb_setup_timeout);
            LOGT("EnumerateDevices: ,timeout occurred time=%lu, timeout=%lu", systemTime(), mUSBSetupTimeout);
        }
        else
        {
            if ((systemTime() >= mUSBMinTimeout) && (!mDevicesToProcess))
            {
                mInitialized.store(usb_setup_success);
                LOGT("EnumerateDevices: ,usb_setup_success time=%lu, timeout=%lu", systemTime(), mUSBSetupTimeout);
            }
            // else wait for process co complete
        }
        break;
    default: // Init completed
        break;
    }

    if (mInitialized.load() & (usb_setup_success | usb_setup_timeout))
        mNextScanIntervalMs = ENUMERATE_TIMEOUT_NSEC;
}
