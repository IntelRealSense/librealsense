// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "Utils.h"
#include "TrackingCommon.h"
#include "TrackingDevice.h"
#include "TrackingManager.h"
#include <vector>
#include <cstring>
#include <stddef.h>

#define LATENCY_MAX_TIMEDIFF_MSEC 1000 /* Latency warning print is aggregated per this diff  */
#define LATENCY_MAX_EVENTS_PER_SEC 10  /* Latency warning print is printed if exceeding this */

perc::Status fwToHostStatus(perc::MESSAGE_STATUS status);

namespace perc {
    enum
    {
        POSE_TASK = 1,
        FRAME_TASK = 2,
        USB_DEVICE_TASK = 3,
        VIDEO_FRAME_COMPLETE_TASK = 4,
        GYRO_FRAME_COMPLETE_TASK = 5,
        ACCEL_FRAME_COMPLETE_TASK = 6,
        VELOCIMETER_FRAME_COMPLETE_TASK = 7,
        RSSI_COMPLETE_TASK = 8,
        CONTROLLER_DISCOVERY_EVENT_FRAME_COMPLETE_TASK = 9,
        CONTROLLER_EVENT_FRAME_COMPLETE_TASK = 10,
        CONTROLLER_CONNECT_EVENT_FRAME_COMPLETE_TASK = 11,
        CONTROLLER_DISCONNECT_EVENT_FRAME_COMPLETE_TASK = 12,
        CONTROLLER_CALIBRATION_EVENT_FRAME_COMPLETE_TASK = 13,
        LOCALIZATION_DATA_STREAM_EVENT_FRAME_COMPLETE_TASK = 14,
        ERROR_COMPLETE_TASK = 15,
        CONTROLLER_LED_COMPLETE_TASK = 16,
        RELOCALIZATION_EVENT_COMPLETE_TASK = 17,
    };

    class CompleteTask
    {
    public:
        CompleteTask(int type, void* owner, bool _mustComplete = false) : Type(type), mOwner(owner), mMustComplete(_mustComplete) {}
        virtual ~CompleteTask() {}
        virtual void complete() = 0;
        virtual bool mustComplete() { return mMustComplete; }
        virtual void* getOwner() { return mOwner; }
    private:
        int Type;
        void* mOwner;
        bool mMustComplete; /* Tasks that must be completed upon STOP */
    };

    class PoseCompleteTask : public CompleteTask
    {
    public:
        PoseCompleteTask(TrackingDevice::Listener* l, TrackingData::PoseFrame& message, TrackingDevice* owner) : CompleteTask(POSE_TASK, owner), mListener(l), mOwner(owner), mMessage(message){}
        virtual ~PoseCompleteTask() {}
        virtual void complete() override
        {
            if (mListener)
            {
                static uint32_t latencyEventPerSec = 0;
                auto start = systemTime();
                static nsecs_t prevStart = start;
                mListener->onPoseFrame(mMessage);
                auto finish = systemTime();

                auto diff = ns2ms(finish - start);
                if (diff > 0)
                {
                    /* Latency event occur */
                    latencyEventPerSec++;
                }
                
                if (ns2ms(start - prevStart) > LATENCY_MAX_TIMEDIFF_MSEC)
                {
                    /* 1 sec passed between previous latency check */
                    if (latencyEventPerSec > LATENCY_MAX_EVENTS_PER_SEC)
                    {
                        LOG(mOwner, LOG_WARN, LOG_TAG, __LINE__, "High latency warning (%d msec): %d Pose latency events occurred in the last second, consider onPoseFrame callback optimization to avoid frame drops", diff, latencyEventPerSec);
                    }
                    latencyEventPerSec = 0;
                    prevStart = start;
                }
            }
        }
    private:
        TrackingDevice::Listener* mListener;
        TrackingData::PoseFrame mMessage;
        TrackingDevice* mOwner;
    };

    class UsbCompleteTask : public CompleteTask
    {
    public:
        UsbCompleteTask(TrackingManager::Listener* l, TrackingDevice* dev, TrackingData::DeviceInfo* deviceInfo, TrackingManager::EventType e, TrackingManager* owner) :
            CompleteTask(USB_DEVICE_TASK, owner, true), mDevice(dev), mDeviceInfo(*deviceInfo), mEventType(e), mListener(l) {}
        virtual ~UsbCompleteTask() {}
        virtual void complete() override
        {
            if (mListener)
            {
                mListener->onStateChanged(mEventType, mDevice, mDeviceInfo);
        }
        }
    private:
        TrackingManager::Listener* mListener;
        TrackingDevice* mDevice;
        TrackingManager::EventType mEventType;
        TrackingData::DeviceInfo mDeviceInfo;
    };

    class ErrorTask : public CompleteTask
    {
    public:
        ErrorTask(TrackingManager::Listener* l, TrackingDevice* dev, Status e, TrackingManager* owner) :
            CompleteTask(ERROR_COMPLETE_TASK, owner, true), mDevice(dev), mErrorStatus(e), mListener(l) {}
        virtual ~ErrorTask() {}
        virtual void complete() override
        {
            if (mListener)
            {
                mListener->onError(mErrorStatus, mDevice);
            }
        }
    private:
        TrackingManager::Listener* mListener;
        TrackingDevice* mDevice;
        Status mErrorStatus;

    };

    class FrameBuffersOwner
    {
    public: 
        virtual void putBufferBack(SensorId id, std::shared_ptr<uint8_t>& frame) = 0;
    };

    class VideoFrameCompleteTask : public CompleteTask
    {
    public:
        VideoFrameCompleteTask(TrackingDevice::Listener* l, std::shared_ptr<uint8_t>& frame, FrameBuffersOwner* frameBufferOwner, TrackingDevice* owner, nsecs_t systemTimeStamp, uint16_t width, uint16_t height) :
            mFrame(frame), mFrameBufferOwner(frameBufferOwner), mListener(l), mOwner(owner), CompleteTask(VIDEO_FRAME_COMPLETE_TASK, owner)
        {
            bulk_message_video_stream* frameBuffer = (bulk_message_video_stream*)mFrame.get();
            mVideoFrame.data = (uint8_t*)mFrame.get() + sizeof(bulk_message_video_stream);
            mVideoFrame.profile.set(width, height, width, PixelFormat::Y8);
            mVideoFrame.systemTimestamp = systemTimeStamp;
            mVideoFrame.timestamp = frameBuffer->rawStreamHeader.llNanoseconds;
            mVideoFrame.arrivalTimeStamp = frameBuffer->rawStreamHeader.llArrivalNanoseconds;
            mVideoFrame.sensorIndex = GET_SENSOR_INDEX(frameBuffer->rawStreamHeader.bSensorID);
            mVideoFrame.frameId = frameBuffer->rawStreamHeader.dwFrameId;
            mVideoFrame.exposuretime = frameBuffer->metadata.dwExposuretime;
            mVideoFrame.gain = frameBuffer->metadata.fGain;
            mVideoFrame.frameLength = frameBuffer->metadata.dwFrameLength;

            mId = frameBuffer->rawStreamHeader.bSensorID;            
        }
        virtual ~VideoFrameCompleteTask() 
        {
            mFrameBufferOwner->putBufferBack(mId, mFrame);
        }
        virtual void complete() override
        {
            if (mListener)
            {
                static uint32_t latencyEventPerSec = 0;
                auto start = systemTime();
                static nsecs_t prevStart = start;
                mListener->onVideoFrame(mVideoFrame);
                auto finish = systemTime();

                auto diff = ns2ms(finish - start);
                if (diff > 0)
                {
                    /* Latency event occur */
                    latencyEventPerSec++;
                }

                if (ns2ms(start - prevStart) > LATENCY_MAX_TIMEDIFF_MSEC)
                {
                    /* 1 sec passed between previous latency check */
                    if (latencyEventPerSec > LATENCY_MAX_EVENTS_PER_SEC)
                    {
                        LOG(mOwner, LOG_WARN, LOG_TAG, __LINE__, "High latency warning (%d msec): %d Video latency events occurred in the last second, consider onVideoFrame callback optimization to avoid frame drops", diff, latencyEventPerSec);
                    }
                    latencyEventPerSec = 0;
                    prevStart = start;
                }
            }
        }

    public:
        TrackingData::VideoFrame mVideoFrame;

    private:
        TrackingDevice::Listener* mListener;
        std::shared_ptr<uint8_t> mFrame;
        FrameBuffersOwner* mFrameBufferOwner;
        SensorId mId;
        TrackingDevice* mOwner;
    };

    class AccelerometerFrameCompleteTask : public CompleteTask
    {
    public:
        AccelerometerFrameCompleteTask(TrackingDevice::Listener* l, interrupt_message_raw_stream_header* imuHeader, int64_t timeStampShift, TrackingDevice* owner) :
            mId(imuHeader->bSensorID), mListener(l), mOwner(owner), CompleteTask(ACCEL_FRAME_COMPLETE_TASK, owner)
        {
            interrupt_message_accelerometer_stream_metadata metaData = ((interrupt_message_accelerometer_stream*)imuHeader)->metadata;

            mFrame.sensorIndex = GET_SENSOR_INDEX(imuHeader->bSensorID);
            mFrame.frameId = imuHeader->dwFrameId;
            mFrame.acceleration.set(metaData.flAx, metaData.flAy, metaData.flAz);
            mFrame.temperature = metaData.flTemperature;
            mFrame.timestamp = imuHeader->llNanoseconds;
            mFrame.arrivalTimeStamp = imuHeader->llArrivalNanoseconds;
            mFrame.systemTimestamp = imuHeader->llNanoseconds + timeStampShift;
        }

        virtual ~AccelerometerFrameCompleteTask() {}
        virtual void complete() override
        {
            if (mListener)
            {
                static uint32_t latencyEventPerSec = 0;
                auto start = systemTime();
                static nsecs_t prevStart = start;
                mListener->onAccelerometerFrame(mFrame);
                auto finish = systemTime();

                auto diff = ns2ms(finish - start);
                if (diff > 0)
                {
                    /* Latency event occur */
                    latencyEventPerSec++;
                }

                if (ns2ms(start - prevStart) > LATENCY_MAX_TIMEDIFF_MSEC)
                {
                    /* 1 sec passed between previous latency check */
                    if (latencyEventPerSec > LATENCY_MAX_EVENTS_PER_SEC)
                    {
                        LOG(mOwner, LOG_WARN, LOG_TAG, __LINE__, "High latency warning (%d msec): %d Accelerometer latency events occurred in the last second, consider onAccelerometerFrame callback optimization to avoid frame drops", diff, latencyEventPerSec);
                    }
                    latencyEventPerSec = 0;
                    prevStart = start;
                }
            }
        }

    public:
        TrackingData::AccelerometerFrame mFrame;
    
    private:
        TrackingDevice::Listener* mListener;
        SensorId mId;
        TrackingDevice* mOwner;
    };

    class GyroFrameCompleteTask : public CompleteTask
    {
    public:
        GyroFrameCompleteTask(TrackingDevice::Listener* l, interrupt_message_raw_stream_header* imuHeader, int64_t timeStampShift, TrackingDevice* owner) : mListener(l), mOwner(owner), CompleteTask(GYRO_FRAME_COMPLETE_TASK, owner)
        {
            interrupt_message_gyro_stream_metadata metaData = ((interrupt_message_gyro_stream*)imuHeader)->metadata;
            mFrame.sensorIndex = GET_SENSOR_INDEX(imuHeader->bSensorID);
            mFrame.frameId = imuHeader->dwFrameId;
            mFrame.angularVelocity.set(metaData.flGx, metaData.flGy, metaData.flGz);
            mFrame.temperature = metaData.flTemperature;
            mFrame.timestamp = imuHeader->llNanoseconds;
            mFrame.arrivalTimeStamp = imuHeader->llArrivalNanoseconds;
            mFrame.systemTimestamp = imuHeader->llNanoseconds + timeStampShift;
        }
        virtual ~GyroFrameCompleteTask() {}
        virtual void complete() override
        {
            if (mListener)
            {
                static uint32_t latencyEventPerSec = 0;
                auto start = systemTime();
                static nsecs_t prevStart = start;
                mListener->onGyroFrame(mFrame);
                auto finish = systemTime();

                auto diff = ns2ms(finish - start);
                if (diff > 0)
                {
                    /* Latency event occur */
                    latencyEventPerSec++;
                }

                if (ns2ms(start - prevStart) > LATENCY_MAX_TIMEDIFF_MSEC)
                {
                    /* 1 sec passed between previous latency check */
                    if (latencyEventPerSec > LATENCY_MAX_EVENTS_PER_SEC)
                    {
                        LOG(mOwner, LOG_WARN, LOG_TAG, __LINE__, "High latency warning (%d msec): %d Gyro latency events occurred in the last second, consider onGyroFrame callback optimization to avoid frame drops", diff, latencyEventPerSec);
                    }
                    latencyEventPerSec = 0;
                    prevStart = start;
                }
            }
        }

    public:         
        TrackingData::GyroFrame mFrame;

    private:
        TrackingDevice::Listener* mListener;
        TrackingDevice* mOwner;
    };

    class VelocimeterFrameCompleteTask : public CompleteTask
    {
    public:
        VelocimeterFrameCompleteTask(TrackingDevice::Listener* l, interrupt_message_raw_stream_header* imuHeader, int64_t timeStampShift, TrackingDevice* owner) : mListener(l), mOwner(owner), CompleteTask(VELOCIMETER_FRAME_COMPLETE_TASK, owner)
        {
            interrupt_message_velocimeter_stream_metadata metaData = ((interrupt_message_velocimeter_stream*)imuHeader)->metadata;
            mFrame.sensorIndex = GET_SENSOR_INDEX(imuHeader->bSensorID);
            mFrame.frameId = imuHeader->dwFrameId;
            mFrame.translationalVelocity.set(metaData.flVx, metaData.flVy, metaData.flVz);
            mFrame.temperature = metaData.flTemperature;
            mFrame.timestamp = imuHeader->llNanoseconds;
            mFrame.arrivalTimeStamp = imuHeader->llArrivalNanoseconds;
            mFrame.systemTimestamp = imuHeader->llNanoseconds + timeStampShift;
        }
        virtual ~VelocimeterFrameCompleteTask() {}
        virtual void complete() override
        {
            if (mListener)
            {
                static uint32_t latencyEventPerSec = 0;
                auto start = systemTime();
                static nsecs_t prevStart = start;
                mListener->onVelocimeterFrame(mFrame);
                auto finish = systemTime();

                auto diff = ns2ms(finish - start);
                if (diff > 0)
                {
                    /* Latency event occur */
                    latencyEventPerSec++;
                }

                if (ns2ms(start - prevStart) > LATENCY_MAX_TIMEDIFF_MSEC)
                {
                    /* 1 sec passed between previous latency check */
                    if (latencyEventPerSec > LATENCY_MAX_EVENTS_PER_SEC)
                    {
                        LOG(mOwner, LOG_WARN, LOG_TAG, __LINE__, "High latency warning (%d msec): %d Velocimeter latency events occurred in the last second, consider onVelocimeterFrame callback optimization to avoid frame drops", diff, latencyEventPerSec);
                    }
                    latencyEventPerSec = 0;
                    prevStart = start;
                }
            }
        }

    public:
        TrackingData::VelocimeterFrame mFrame;

    private:
        TrackingDevice::Listener* mListener;
        TrackingDevice* mOwner;
    };

    class ControllerFrameCompleteTask : public CompleteTask
    {
    public:
        ControllerFrameCompleteTask(TrackingDevice::Listener* l, bulk_message_raw_stream_header* header, int64_t timeStampShift, TrackingDevice* owner) :
            mListener(l), mOwner(owner), CompleteTask(CONTROLLER_EVENT_FRAME_COMPLETE_TASK, owner)
        {
            bulk_message_controller_stream_metadata metaData = ((bulk_message_controller_stream*)header)->metadata;
            mFrame.sensorIndex = GET_SENSOR_INDEX(header->bSensorID);
            mFrame.frameId = header->dwFrameId;
            mFrame.timestamp = header->llNanoseconds;
            mFrame.arrivalTimeStamp = header->llArrivalNanoseconds;
            mFrame.systemTimestamp = header->llNanoseconds + timeStampShift;
            mFrame.eventId = metaData.bEventID;
            mFrame.instanceId = metaData.bInstanceId;
            perc::copy(mFrame.sensorData, metaData.bSensorData, sizeof(mFrame.sensorData));
        }
        virtual ~ControllerFrameCompleteTask() {}
        virtual void complete() override
        {
            if (mListener)
            {
                static uint32_t latencyEventPerSec = 0;
                auto start = systemTime();
                static nsecs_t prevStart = start;
                mListener->onControllerFrame(mFrame);
                auto finish = systemTime();

                auto diff = ns2ms(finish - start);
                if (diff > 0)
                {
                    /* Latency event occur */
                    latencyEventPerSec++;
                }

                if (ns2ms(start - prevStart) > LATENCY_MAX_TIMEDIFF_MSEC)
                {
                    /* 1 sec passed between previous latency check */
                    if (latencyEventPerSec > LATENCY_MAX_EVENTS_PER_SEC)
                    {
                        LOG(mOwner, LOG_WARN, LOG_TAG, __LINE__, "High latency warning (%d msec): %d Controller latency events occurred in the last second, consider onControllerFrame callback optimization to avoid frame drops", diff, latencyEventPerSec);
                    }
                    latencyEventPerSec = 0;
                    prevStart = start;
                }
            }
        }
    private:
        TrackingDevice::Listener* mListener;
        TrackingData::ControllerFrame mFrame;
        TrackingDevice* mOwner;
    };


    class RssiFrameCompleteTask : public CompleteTask
    {
    public:
        RssiFrameCompleteTask(TrackingDevice::Listener* l, bulk_message_raw_stream_header* header, int64_t timeStampShift, TrackingDevice* owner) : mListener(l), mOwner(owner), CompleteTask(RSSI_COMPLETE_TASK, owner)
        {
            bulk_message_rssi_stream_metadata metaData = ((bulk_message_rssi_stream*)header)->metadata;
            mFrame.sensorIndex = GET_SENSOR_INDEX(header->bSensorID);
            mFrame.frameId = header->dwFrameId;
            mFrame.timestamp = header->llNanoseconds;
            mFrame.arrivalTimeStamp = header->llArrivalNanoseconds;
            mFrame.systemTimestamp = header->llNanoseconds + timeStampShift;
            mFrame.signalStrength = metaData.flSignalStrength;
        }
        virtual ~RssiFrameCompleteTask() {}
        virtual void complete() override
        {
            if (mListener)
            {
                static uint32_t latencyEventPerSec = 0;
                auto start = systemTime();
                static nsecs_t prevStart = start;
                mListener->onRssiFrame(mFrame);
                auto finish = systemTime();

                auto diff = ns2ms(finish - start);
                if (diff > 0)
                {
                    /* Latency event occur */
                    latencyEventPerSec++;
                }

                if (ns2ms(start - prevStart) > LATENCY_MAX_TIMEDIFF_MSEC)
                {
                    /* 1 sec passed between previous latency check */
                    if (latencyEventPerSec > LATENCY_MAX_EVENTS_PER_SEC)
                    {
                        LOG(mOwner, LOG_WARN, LOG_TAG, __LINE__, "High latency warning (%d msec): %d Rssi latency events occurred in the last second, consider onRssiFrame callback optimization to avoid frame drops", diff, latencyEventPerSec);
                    }
                    latencyEventPerSec = 0;
                    prevStart = start;
                }
            }
        }
    private:
        TrackingDevice::Listener* mListener;
        TrackingData::RssiFrame mFrame;
        TrackingDevice* mOwner;
    };

    class ControllerDiscoveryEventFrameCompleteTask : public CompleteTask
    {
    public:
        ControllerDiscoveryEventFrameCompleteTask(TrackingDevice::Listener* l, interrupt_message_controller_device_discovery* controllerDeviceDiscovery, TrackingDevice* owner) :
            mListener(l), CompleteTask(CONTROLLER_DISCOVERY_EVENT_FRAME_COMPLETE_TASK, owner)
        {
            perc::copy(mFrame.macAddress, controllerDeviceDiscovery->bMacAddress, sizeof(controllerDeviceDiscovery->bMacAddress));
            mFrame.addressType = (AddressType)controllerDeviceDiscovery->bAddressType;
            mFrame.manufacturerId = controllerDeviceDiscovery->info.wManufacturerId;
            mFrame.vendorData = controllerDeviceDiscovery->info.bVendorData;
            mFrame.protocol.set(controllerDeviceDiscovery->info.bProtocolVersion);
            mFrame.app.set(controllerDeviceDiscovery->info.bAppVersionMajor, controllerDeviceDiscovery->info.bAppVersionMinor, controllerDeviceDiscovery->info.bAppVersionPatch);
            mFrame.bootLoader.set(controllerDeviceDiscovery->info.bBootloaderVersionMajor, controllerDeviceDiscovery->info.bBootloaderVersionMinor, controllerDeviceDiscovery->info.bBootloaderVersionPatch);
            mFrame.softDevice.set(controllerDeviceDiscovery->info.bSoftdeviceVersion);
        }
        virtual ~ControllerDiscoveryEventFrameCompleteTask() {}
        virtual void complete() override
        {
            if (mListener)
            {
                mListener->onControllerDiscoveryEventFrame(mFrame);
            }
        }
    private:
        TrackingDevice::Listener* mListener;
        TrackingData::ControllerDiscoveryEventFrame mFrame;
    };

    class ControllerConnectEventFrameCompleteTask : public CompleteTask
    {
    public:
        ControllerConnectEventFrameCompleteTask(TrackingDevice::Listener* l, interrupt_message_controller_connected* controllerConnected, TrackingDevice* owner) :
            mListener(l), CompleteTask(CONTROLLER_CONNECT_EVENT_FRAME_COMPLETE_TASK, owner)
        {
            mFrame.controllerId = controllerConnected->bControllerID;
            mFrame.manufacturerId = controllerConnected->info.wManufacturerId;
            mFrame.protocol.set(controllerConnected->info.bProtocolVersion);
            mFrame.app.set(controllerConnected->info.bAppVersionMajor, controllerConnected->info.bAppVersionMinor, controllerConnected->info.bAppVersionPatch);
            mFrame.bootLoader.set(controllerConnected->info.bBootloaderVersionMajor, controllerConnected->info.bBootloaderVersionMinor, controllerConnected->info.bBootloaderVersionPatch);
            mFrame.softDevice.set(controllerConnected->info.bSoftdeviceVersion);
            mFrame.status = fwToHostStatus((MESSAGE_STATUS)controllerConnected->wStatus);
        }
        virtual ~ControllerConnectEventFrameCompleteTask() {}
        virtual void complete() override
        {
            if (mListener)
            {
                mListener->onControllerConnectedEventFrame(mFrame);
            }
        }
    private:
        TrackingDevice::Listener* mListener;
        TrackingData::ControllerConnectedEventFrame mFrame;
    };

    class ControllerDisconnectEventFrameCompleteTask : public CompleteTask
    {
    public:
        ControllerDisconnectEventFrameCompleteTask(TrackingDevice::Listener* l, interrupt_message_controller_disconnected* controllerDisconnected, TrackingDevice* owner) :
            mListener(l), CompleteTask(CONTROLLER_DISCONNECT_EVENT_FRAME_COMPLETE_TASK, owner)
        {
            mFrame.controllerId = controllerDisconnected->bControllerID;
        }
        virtual ~ControllerDisconnectEventFrameCompleteTask() {}
        virtual void complete() override
        {
            if (mListener)
            {
                mListener->onControllerDisconnectedEventFrame(mFrame);
            }
        }
    private:
        TrackingDevice::Listener* mListener;
        TrackingData::ControllerDisconnectedEventFrame mFrame;
    };
    
    class ControllerCalibrationEventFrameCompleteTask : public CompleteTask
    {
    public:
        ControllerCalibrationEventFrameCompleteTask(TrackingDevice::Listener* l, uint8_t controllerID, Status status, TrackingDevice* owner) :
            mListener(l), CompleteTask(CONTROLLER_CALIBRATION_EVENT_FRAME_COMPLETE_TASK, owner)
        {
            mFrame.controllerId = controllerID;
            mFrame.status = status;
        }
        virtual ~ControllerCalibrationEventFrameCompleteTask() {}
        virtual void complete() override
        {
            if (mListener)
            {
                mListener->onControllerCalibrationEventFrame(mFrame);
            }
        }
    private:
        TrackingDevice::Listener* mListener;
        TrackingData::ControllerCalibrationEventFrame mFrame;
    };

    class LocalizationDataEventFrameCompleteTask : public CompleteTask
    {
    public:
        /* Get localization data stream */
        LocalizationDataEventFrameCompleteTask(TrackingDevice::Listener* l, std::shared_ptr<uint8_t>& frame, FrameBuffersOwner* frameBufferOwner, TrackingDevice* owner) :
            mListener(l), mFrameBufferOwner(frameBufferOwner), mFrame(frame), CompleteTask(LOCALIZATION_DATA_STREAM_EVENT_FRAME_COMPLETE_TASK, owner, true)
        {

            interrupt_message_get_localization_data_stream* localizationData = (interrupt_message_get_localization_data_stream*)mFrame.get();
            mLocalizationFrame.status = fwToHostStatus((MESSAGE_STATUS)localizationData->wStatus);
            if ((MESSAGE_STATUS)localizationData->wStatus == MESSAGE_STATUS::MORE_DATA_AVAILABLE)
            {
                mLocalizationFrame.moreData = true;
            }
            
            mLocalizationFrame.buffer = localizationData->bLocalizationData;
            mLocalizationFrame.chunkIndex = localizationData->wIndex;
            mLocalizationFrame.length = localizationData->header.dwLength - offsetof(interrupt_message_get_localization_data_stream, bLocalizationData);
        }

        /* Set localization data stream */
        LocalizationDataEventFrameCompleteTask(TrackingDevice::Listener* l, interrupt_message_set_localization_data_stream* localizationData, TrackingDevice* owner) :
            mListener(l), mFrame(NULL), mFrameBufferOwner(NULL), CompleteTask(LOCALIZATION_DATA_STREAM_EVENT_FRAME_COMPLETE_TASK, owner, true)
        {
            mLocalizationFrame.status = fwToHostStatus((MESSAGE_STATUS)localizationData->wStatus);
            mLocalizationFrame.buffer = NULL;
            mLocalizationFrame.length = 0;
            mLocalizationFrame.chunkIndex = 0;
            mLocalizationFrame.moreData = false;
        }

        virtual ~LocalizationDataEventFrameCompleteTask() 
        {
            if (mLocalizationFrame.buffer != NULL)
            {
                mFrameBufferOwner->putBufferBack(0, mFrame);
            }
        }

        virtual void complete() override
        {
            if (mListener)
            {
                mListener->onLocalizationDataEventFrame(mLocalizationFrame);
            }
        }
    private:
        TrackingDevice::Listener* mListener;
        std::shared_ptr<uint8_t> mFrame;
        FrameBuffersOwner* mFrameBufferOwner;
        TrackingData::LocalizationDataFrame mLocalizationFrame;
    };

    class RelocalizationEventFrameCompleteTask : public CompleteTask
    {
    public:
        RelocalizationEventFrameCompleteTask(TrackingDevice::Listener* l, const interrupt_message_slam_relocalization_event* relocalizationEvent, TrackingDevice* owner) :
            mListener(l), CompleteTask(RELOCALIZATION_EVENT_COMPLETE_TASK, owner)
        {
            mRelocalizationEvent.timestamp = relocalizationEvent->llNanoseconds;
            mRelocalizationEvent.sessionId = relocalizationEvent->wSessionId;
        }

        virtual void complete() override
        {
            if (mListener)
            {
                mListener->onRelocalizationEvent(mRelocalizationEvent);
            }
        }
    private:
        TrackingDevice::Listener* mListener;
        TrackingData::RelocalizationEvent mRelocalizationEvent;
    };

    class FWUpdateCompleteTask : public CompleteTask
    {
    public:

        FWUpdateCompleteTask(TrackingDevice::Listener* l, interrupt_message_fw_update_stream* FWImage, TrackingDevice* owner) :
            mListener(l), CompleteTask(DEV_FIRMWARE_UPDATE, owner, true)
        {
            mFrame.status = fwToHostStatus((MESSAGE_STATUS)FWImage->wStatus);
            mFrame.progress = FWImage->bProgress;
            perc::copy(mFrame.macAddress, FWImage->bMacAddress, sizeof(FWImage->bMacAddress));
        }

        virtual ~FWUpdateCompleteTask() = default;

        virtual void complete() override
        {
            if (mListener)
            {
                mListener->onFWUpdateEvent(mFrame);
            }
        }
    private:
        TrackingDevice::Listener* mListener;
        TrackingData::ControllerFWEventFrame mFrame;
    };

    class StatusEventFrameCompleteTask : public CompleteTask
    {
    public:

        StatusEventFrameCompleteTask(TrackingDevice::Listener* l, perc::Status status, TrackingDevice* owner) : mListener(l), CompleteTask(DEV_STATUS, owner, true)
        {
            mFrame.status = status;
        }

        virtual ~StatusEventFrameCompleteTask() = default;

        virtual void complete() override
        {
            if (mListener)
            {
                mListener->onStatusEvent(mFrame);
            }
        }
    private:
        TrackingDevice::Listener* mListener;
        TrackingData::StatusEventFrame mFrame;
    };

    class CompleteQueueHandler
    {
    public:
        virtual void addTask(std::shared_ptr<CompleteTask>&) = 0;
        virtual void removeTasks(void* owner, bool completeTasks) = 0;
    };

    class ControllerLedFrameCompleteTask : public CompleteTask
    {
    public:
        ControllerLedFrameCompleteTask(TrackingDevice::Listener* l, interrupt_message_controller_led_intensity* led, int64_t timeStampShift, TrackingDevice* owner) : mListener(l), mOwner(owner), CompleteTask(CONTROLLER_LED_COMPLETE_TASK, owner)
        {
            mFrame.controllerId = GET_SENSOR_INDEX(led->rawStreamHeader.bSensorID);
            mFrame.timestamp = led->rawStreamHeader.llNanoseconds;
            mFrame.arrivalTimeStamp = led->rawStreamHeader.llArrivalNanoseconds;
            mFrame.systemTimestamp = led->rawStreamHeader.llNanoseconds + timeStampShift;
            mFrame.intensity = led->intensity;
            mFrame.ledId = led->ledId;
        }
        virtual ~ControllerLedFrameCompleteTask() {}
        virtual void complete() override
        {
            if (mListener)
            {
                static uint32_t latencyEventPerSec = 0;
                auto start = systemTime();
                static nsecs_t prevStart = start;
                mListener->onControllerLedEvent(mFrame);
                auto finish = systemTime();

                auto diff = ns2ms(finish - start);
                if (diff > 0)
                {
                    /* Latency event occur */
                    latencyEventPerSec++;
                }

                if (ns2ms(start - prevStart) > LATENCY_MAX_TIMEDIFF_MSEC)
                {
                    /* 1 sec passed between previous latency check */
                    if (latencyEventPerSec > LATENCY_MAX_EVENTS_PER_SEC)
                    {
                        LOG(mOwner, LOG_WARN, LOG_TAG, __LINE__, "High latency warning (%d msec): %d led latency events occurred in the last second, consider onled callback optimization to avoid frame drops", diff, latencyEventPerSec);
                    }
                    latencyEventPerSec = 0;
                    prevStart = start;
                }
            }
        }

    public:
        TrackingData::ControllerLedEventFrame mFrame;

    private:
        TrackingDevice::Listener* mListener;
        TrackingDevice* mOwner;
    };


}
