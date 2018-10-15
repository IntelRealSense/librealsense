/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/

#pragma once

#include <stdint.h>
#include <math.h>
#include <fstream>
#include <thread>
#include <atomic>

#include "Packet.h"
#include "TrackingDevice.h"
#include "TrackingSerializer.h"

namespace perc
{
    class PlayerImpl : public TrackingPlayer
    {
    public:

        PlayerImpl(TrackingDevice::Listener* listener, const char* file);
        virtual ~PlayerImpl();
        virtual bool device_configure(TrackingDevice* device, TrackingDevice::Listener* listener, TrackingData::Profile* profile = nullptr) override;
        virtual void start(bool start_after_calibration = true) override;
        virtual bool isStreaming() override;
        virtual void stop() override;

    private:
        void sendImageFrame(std::shared_ptr<packet_t> packet, float gain, uint64_t exposureTime, uint64_t arrivalTime);
        void sendStereoFrame(std::shared_ptr<packet_t> packet, float gain, uint64_t exposureTime, uint64_t arrivalTime, uint32_t index);
        void sendAccelFrame(std::shared_ptr<packet_t> packet, uint64_t arrival_time, float temperature);
        void sendGyroFrame(std::shared_ptr<packet_t> packet, uint64_t arrival_time, float temperature);
        void writeCalibration(uint8_t* buffer, size_t size);
        void writePhysicalInfo(uint16_t sensorIndex, uint8_t* buffer, size_t size);
        bool startDevice();

    private:
        TrackingDevice * mDevice;
        TrackingDevice::Listener * mListener;
        std::thread mReadingThread;
        std::atomic<bool> mThreadIsAlive;
        std::atomic<bool> mIsStreaming;
        std::string mfilePath;
        TrackingDevice::Listener* mDeviceListener;
        TrackingData::Profile* mProfile;
    };
}