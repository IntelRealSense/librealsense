// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "TrackingCommon.h"
#include "TrackingDevice.h"
#include "TrackingData.h"

namespace perc
{
    class DLL_EXPORT TrackingManager
    {
    public:
        enum EventType
        {
            ATTACH = 0,
            DETACH
        };

        class Listener
        {
        public:
            // state : enum ATTACH, DETACH....
            virtual void onStateChanged(EventType state, TrackingDevice* device, TrackingData::DeviceInfo deviceInfo) = 0;
            virtual void onError(Status, TrackingDevice* device)= 0;
        };

        // factory
        static TrackingManager* CreateInstance(Listener*, void* param = 0);
        // release existing manager and clean the pointer
        static void ReleaseInstance(TrackingManager*& manager);
        // interface
        virtual Handle getHandle() = 0;
        virtual Status handleEvents(bool blocking = true) = 0;
        virtual size_t getDeviceList(TrackingDevice** list, unsigned int maxListSize) = 0;
        virtual Status setHostLogControl(IN const TrackingData::LogControl& logControl) = 0;
        virtual Status getHostLog(OUT TrackingData::Log* log) = 0;

        /**
        *   libtm version
        *   MAJOR = 0xFF00000000000000
        *   MINOR = 0x00FF000000000000
        *   PATCH = 0x0000FFFF00000000
        *   Build = 0x00000000FFFFFFFF
        */
        virtual uint64_t version() = 0;
        virtual ~TrackingManager() {}

        virtual bool isInitialized() const = 0;
    };
} 
