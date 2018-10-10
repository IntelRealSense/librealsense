/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/

#pragma once
#include "TrackingDevice.h"

namespace perc
{
    class DLL_EXPORT TrackingRecorder : public TrackingDevice::Listener
    {
    public:
        virtual ~TrackingRecorder() = default;
        static TrackingRecorder* CreateInstance(TrackingDevice* device, TrackingDevice::Listener* listener, const char* file, bool stereoMode = false);
    };

    class DLL_EXPORT TrackingPlayer
    {
    public:
        virtual ~TrackingPlayer() = default;
        /**
        * @brief start
        *        start sending packets from the file to the listener, starts from the beginning of the file.
        */
        virtual void start(bool start_after_calibration = true) = 0;

        /**
        * @brief device_configure
        *        configure and the player to send packets to the device, and to start device streaming
        */
        virtual bool device_configure(TrackingDevice* device, TrackingDevice::Listener* listener, TrackingData::Profile* profile = nullptr) = 0;

        /**
        * @brief isStreaming
        *        Return if the player is in the middle of streaming from the file
        */
        virtual bool isStreaming() = 0;
        /**
        * @brief stop
        *        Stop streaming and close the file.
        */
        virtual void stop() = 0;

        static TrackingPlayer* CreateInstance(TrackingDevice::Listener* listener, const char* file);
    };
} 
