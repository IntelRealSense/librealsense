/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/
#define LOG_TAG "Recorder"
#include <chrono>
#include "Recorder.h"
#include "Packet.h"
#include "Utils.h"

using namespace perc;
const uint64_t MAX_LATENCY = 1000LL * 1000LL * 1000LL;

TrackingRecorder* TrackingRecorder::CreateInstance(TrackingDevice* device, TrackingDevice::Listener* listener, const char* file, bool stereoMode)
{
    if (device == nullptr || listener == nullptr || file == nullptr)
    {
        LOGE("TrackingRecorder::CreateInstance : Invalid parameters");
        return nullptr;
    }
    try
    {
        return new RecorderImpl(device, listener, file, stereoMode);
    }
    catch (std::exception& e)
    {
        std::string error = std::string("Failed to create Recorder : ") + e.what();
        LOGE(error.c_str());
        return nullptr;
    }
}

RecorderImpl::RecorderImpl(TrackingDevice* device, TrackingDevice::Listener* listener, const char* file, bool stereoMode) : mDevice(device), mListener(listener), mThreadIsAlive(true), mWritingQueue(0), mVideoFrames(), mStereoMode(stereoMode),
mLatencyQueue(MAX_LATENCY, [&](uint64_t arrivalTime, std::shared_ptr<Packet> packet) -> bool
{
    switch (packet->getType())
    {
    case packet_image_raw:
    {
        ImagePacket * imagePacket = (ImagePacket*)packet.get();
        if (mStereoMode == false)
        {
            mWritingQueue.enqueue(std::make_shared<ArrivalTimePacket>(arrivalTime));
            mWritingQueue.enqueue(std::make_shared<ExposurePacket>(imagePacket->getExposurePacket()));
            mWritingQueue.enqueue(std::make_shared<ArrivalTimePacket>(arrivalTime));
            mWritingQueue.enqueue(std::shared_ptr<Packet>(packet));
        }
        else
        {
            uint16_t stereoSensorId = imagePacket->getSensorId() / 2;
            StereoPacket& _packet = mVideoFrames[stereoSensorId];
            if (_packet.addPacket(imagePacket) == false)
            {
                LOGE("Failed to record an image packet");
                return false;
            }
            if (_packet.isComplete())
            {
                mWritingQueue.enqueue(std::make_shared<ArrivalTimePacket>(arrivalTime));
                mWritingQueue.enqueue(std::make_shared<ExposurePacket>(_packet.getExposurePacket()));
                mWritingQueue.enqueue(std::make_shared<ArrivalTimePacket>(arrivalTime));
                mWritingQueue.enqueue(std::make_shared<StereoPacket>(_packet));
                mVideoFrames[stereoSensorId].clear();
            }
        }
        break;
    }
    case packet_gyroscope:
    {
        GyroPacket * _packet = (GyroPacket*)packet.get();
        mWritingQueue.enqueue(std::make_shared<ArrivalTimePacket>(arrivalTime));
        mWritingQueue.enqueue(std::make_shared<ThermometerPacket>(_packet->getThermometerPacket()));
        mWritingQueue.enqueue(std::make_shared<ArrivalTimePacket>(arrivalTime));
        mWritingQueue.enqueue(std::shared_ptr<Packet>(packet));
        break;
    }
    case packet_accelerometer:
    {
        AcclPacket * _packet = (AcclPacket*)packet.get();
        mWritingQueue.enqueue(std::make_shared<ArrivalTimePacket>(arrivalTime));
        mWritingQueue.enqueue(std::make_shared<ThermometerPacket>(_packet->getThermometerPacket()));
        mWritingQueue.enqueue(std::make_shared<ArrivalTimePacket>(arrivalTime));
        mWritingQueue.enqueue(std::shared_ptr<Packet>(packet));
        break;
    }
    case packet_velocimeter:
    {
        VelocimeterPacket * _packet = (VelocimeterPacket*)packet.get();
        mWritingQueue.enqueue(std::make_shared<ArrivalTimePacket>(arrivalTime));
        mWritingQueue.enqueue(std::make_shared<ThermometerPacket>(_packet->getThermometerPacket()));
        mWritingQueue.enqueue(std::make_shared<ArrivalTimePacket>(arrivalTime));
        mWritingQueue.enqueue(std::shared_ptr<Packet>(packet));
        break;
    }
    case packet_led_intensity:
    {
        mWritingQueue.enqueue(std::make_shared<ArrivalTimePacket>(arrivalTime));
        mWritingQueue.enqueue(std::shared_ptr<Packet>(packet));
        break;
    }
    default: LOGW("Invalid packet type : %d", packet->getType());
    }
    return true;
})
{
    LOGD("Set recording to file %s", file);
    uint16_t actualSize = 0;

    if (!setProcessPriorityToRealtime())
    {
        throw std::runtime_error("Failed to set process priority");
    }

    uint8_t buffer[MAX_CONFIGURATION_SIZE] = { 0 };
    const static int CALIBRATION_TABLE_TYPE = 0x0;
    if (mDevice->ReadConfiguration(CALIBRATION_TABLE_TYPE, MAX_CONFIGURATION_SIZE, buffer, &actualSize) != Status::SUCCESS)
    {
        throw std::runtime_error("Failed to read calibration table from device");
    }
    mWritingQueue.enqueue(std::make_shared<CalibrationPacket>(buffer, actualSize));

    mOutputFile.open(file, std::fstream::out | std::ofstream::binary);

    mWritingThread = std::thread([&]() -> void {
        while (mThreadIsAlive || mWritingQueue.size() > 0)
        {
            if (mWritingQueue.size() == 0)
            {
                static uint32_t WAIT_FOR_PACKETS = 10;
                std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_FOR_PACKETS));
                continue;
            }
            std::shared_ptr<Packet> packet;
            mWritingQueue.dequeue(&packet); //check output
            mOutputFile.write((const char*)packet->getBytes(), packet->getSize());
            if (packet->getType() == packet_stereo_raw)
            {
                StereoPacket * _packet = (StereoPacket*)packet.get();
                mOutputFile.write((const char*)_packet->getImageBytes(0), _packet->getImageSize(0));
                mOutputFile.write((const char*)_packet->getImageBytes(1), _packet->getImageSize(1));
            }
        }
    });
}

RecorderImpl::~RecorderImpl()
{
    mLatencyQueue.drain();
    mThreadIsAlive = false;
    if (mWritingThread.joinable())
    {
        mWritingThread.join();
    }
    mOutputFile.close();
    mWritingQueue.clear();
}

void RecorderImpl::onPoseFrame(OUT TrackingData::PoseFrame& pose)
{
    mListener->onPoseFrame(pose);
}

void RecorderImpl::onVideoFrame(OUT TrackingData::VideoFrame& frame)
{
    mLatencyQueue.push(frame.arrivalTimeStamp, std::make_shared<ImagePacket>(frame));
    mListener->onVideoFrame(frame);
}

void RecorderImpl::onAccelerometerFrame(OUT TrackingData::AccelerometerFrame& frame)
{
    mLatencyQueue.push(frame.arrivalTimeStamp, std::make_shared<AcclPacket>(frame));
    mListener->onAccelerometerFrame(frame);
}

void RecorderImpl::onGyroFrame(OUT TrackingData::GyroFrame& frame)
{
    mLatencyQueue.push(frame.arrivalTimeStamp, std::make_shared<GyroPacket>(frame));
    mListener->onGyroFrame(frame);
}

void RecorderImpl::onVelocimeterFrame(OUT TrackingData::VelocimeterFrame& frame)
{
    mLatencyQueue.push(frame.arrivalTimeStamp, std::make_shared<VelocimeterPacket>(frame));
    mListener->onVelocimeterFrame(frame);
}

void RecorderImpl::onControllerDiscoveryEventFrame(OUT TrackingData::ControllerDiscoveryEventFrame& frame)
{
    mListener->onControllerDiscoveryEventFrame(frame);
}

void RecorderImpl::onControllerFrame(OUT TrackingData::ControllerFrame& frame)
{
    mListener->onControllerFrame(frame);
}

void RecorderImpl::onControllerConnectedEventFrame(OUT TrackingData::ControllerConnectedEventFrame& frame)
{
    uint16_t actualSize = 0;
    if (frame.controllerId != 1 && frame.controllerId != 2)
    {
        LOGE("Invalid controller id onControllerConnectedEventFrame %d", frame.controllerId);
        return;
    }

    int tableType = 0x100 + frame.controllerId;
    uint8_t buffer[MAX_CONFIGURATION_SIZE] = { 0 };
    if (mDevice->ReadConfiguration(tableType, MAX_CONFIGURATION_SIZE, buffer, &actualSize) == Status::SUCCESS)
    {
        mWritingQueue.enqueue(std::make_shared<PhysicalInfoPacket>(frame.controllerId, buffer, actualSize));
    }
    else
    {
        LOGE("Failed to read physical info table from device");
    }
    mListener->onControllerConnectedEventFrame(frame);
}

void RecorderImpl::onControllerDisconnectedEventFrame(OUT TrackingData::ControllerDisconnectedEventFrame& frame)
{
    mListener->onControllerDisconnectedEventFrame(frame);
}

void RecorderImpl::onRssiFrame(OUT TrackingData::RssiFrame& frame)
{
    mListener->onRssiFrame(frame);
}

void RecorderImpl::onLocalizationDataEventFrame(OUT TrackingData::LocalizationDataFrame& frame)
{
    mListener->onLocalizationDataEventFrame(frame);
}

void RecorderImpl::onFWUpdateEvent(OUT TrackingData::ControllerFWEventFrame& frame)
{
    mListener->onFWUpdateEvent(frame);
}

void RecorderImpl::onStatusEvent(OUT TrackingData::StatusEventFrame& frame)
{
    mListener->onStatusEvent(frame);
}

void RecorderImpl::onControllerLedEvent(OUT TrackingData::ControllerLedEventFrame& frame)
{
    mLatencyQueue.push(frame.arrivalTimeStamp, std::make_shared<LedPacket>(frame));
    mListener->onControllerLedEvent(frame);

}
