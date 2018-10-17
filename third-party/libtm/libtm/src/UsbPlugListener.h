// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <memory>
#include <functional>
#include <thread>
#include "Dispatcher.h"
#include "Fsm.h"
#include "Device.h"
#include "TrackingCommon.h"
#include "TrackingData.h"
#include <set>
#include <mutex>
#include <map>

#define TM2_DEV_VID 0x8087
#define TM2_DEV_PID 0x0AF3
#define TM2_UDF_DEV_VID 0x03E7
#define TM2_UDF_DEV_PID 0x2150
#define ENUMERATE_TIMEOUT_NSEC 500000000


namespace perc
{
    class UsbPlugListener : public EventHandler
    {
    public:
        class Owner {
        public:
            virtual ~Owner() {};
            virtual Status onAttach(libusb_device*) = 0;
            virtual void onDetach(libusb_device*) = 0;
            virtual libusb_context* context() = 0;
            virtual Dispatcher& dispatcher() = 0;
        };

    public:
        UsbPlugListener(Owner& owner);
        ~UsbPlugListener();
        
        bool identifyDevice(libusb_device_descriptor* desc);
        bool identifyUDFDevice(libusb_device_descriptor* desc);

        // [interface] EventHandler
        virtual void onTimeout(uintptr_t timerId, const Message &msg);
        static const char *usbSpeed(uint16_t bcdUSB);

    private:
        void EnumerateDevices();
        Message mMessage;
        Owner& mOwner;
        class DeviceToPortMap
        {
        public:
            DeviceToPortMap() : libusbDevice(NULL), portChainDepth(0), portChain{0} {};
            DeviceToPortMap(libusb_device* _libusbDevice) : portChain{ 0 } {
                libusbDevice = _libusbDevice;
                portChainDepth = libusb_get_port_numbers(libusbDevice, portChain, MAX_USB_TREE_DEPTH);
            };


            libusb_device* libusbDevice;
            uint32_t portChainDepth;
            uint8_t portChain[MAX_USB_TREE_DEPTH];

            bool operator<(const DeviceToPortMap &ref) const
            {
                if (this->libusbDevice < ref.libusbDevice) 
                {
                    return true;
                }
                else if (this->libusbDevice > ref.libusbDevice)
                {
                    return false;
                }

                /* this->libusbDevice == ref.libusbDevice */
                if (this->portChainDepth < ref.portChainDepth)
                {
                    return true;
                }
                else if (this->portChainDepth > ref.portChainDepth)
                {
                    return false;
                }

                /* this->libusbDevice == ref.libusbDevice    */
                /* this->portChainDepth == ref.portChainDepth */
                for (uint32_t i = 0; i < this->portChainDepth; i++)
                {
                    if (this->portChain[i] < ref.portChain[i])
                    {
                        return true;
                    }
                    else if (this->portChain[i] > ref.portChain[i])
                    {
                        return false;
                    }
                }

                return false;
            }

        };
        std::map<DeviceToPortMap, bool> mDeviceToPortMap;
        std::mutex mDeviceToPortMapLock;
    };
}
