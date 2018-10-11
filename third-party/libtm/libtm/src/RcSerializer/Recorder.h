/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/

#pragma once

#include <stdint.h>
#include <math.h>
#include <fstream>
#include "TrackingDevice.h"
#include "latency_queue.h"
#include "concurrency.h"
#include "TrackingSerializer.h"

namespace perc
{
    class RecorderImpl : public TrackingRecorder
    {
    public:

        RecorderImpl(TrackingDevice* device, Listener* listener, const char* file, bool stereoMode);

        virtual ~RecorderImpl();
        
        virtual void onPoseFrame(OUT TrackingData::PoseFrame& pose) override;

        virtual void onVideoFrame(OUT TrackingData::VideoFrame& frame) override;

        virtual void onAccelerometerFrame(OUT TrackingData::AccelerometerFrame& frame) override;

        virtual void onGyroFrame(OUT TrackingData::GyroFrame& frame) override;

        virtual void onVelocimeterFrame(OUT TrackingData::VelocimeterFrame& frame) override;

        virtual void onControllerDiscoveryEventFrame(OUT TrackingData::ControllerDiscoveryEventFrame& frame) override;

        virtual void onControllerFrame(OUT TrackingData::ControllerFrame& frame) override;

        virtual void onControllerConnectedEventFrame(OUT TrackingData::ControllerConnectedEventFrame& frame) override;

        virtual void onControllerDisconnectedEventFrame(OUT TrackingData::ControllerDisconnectedEventFrame& frame) override;

        virtual void onRssiFrame(OUT TrackingData::RssiFrame& frame) override;

        virtual void onLocalizationDataEventFrame(OUT TrackingData::LocalizationDataFrame& frame) override;

        virtual void onFWUpdateEvent(OUT TrackingData::ControllerFWEventFrame& frame) override;

        virtual void onStatusEvent(OUT TrackingData::StatusEventFrame& frame) override;

        virtual void onControllerLedEvent(OUT TrackingData::ControllerLedEventFrame& frame) override;


    private:
        TrackingDevice * mDevice;
        TrackingDevice::Listener * mListener;
        latency_queue  mLatencyQueue;
        single_consumer_queue<std::shared_ptr<Packet>> mWritingQueue;
        std::ofstream mOutputFile;
        std::thread mWritingThread;
        std::atomic<bool> mThreadIsAlive;
        std::map<uint32_t, StereoPacket> mVideoFrames;
        std::atomic<bool> mStereoMode;
    };


}