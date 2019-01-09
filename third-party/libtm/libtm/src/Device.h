// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "TrackingCommon.h"
#include "TrackingDevice.h"
#include "TrackingData.h"
#include "Fsm.h"
#include "Message.h"
#include <atomic>
#include "CompleteTask.h"
#include <map>
#include <list>
#include <memory>
#include <mutex>

#define INTERFACE_INDEX 0 /* TM2 device interface descriptor index */

#ifdef _WIN32
#pragma warning (push)
#pragma warning (disable : 4200)
#endif
#include "libusb.h"
#ifdef _WIN32
#pragma warning (pop)
#endif

namespace perc
{
    class Device : public TrackingDevice,
        public EventHandler,
        public FrameBuffersOwner
    {
    public:
        Device(libusb_device*, Dispatcher* dispatcher, EventHandler* owner, CompleteQueueHandler* taskHandler);
        virtual ~Device();

        // [interface] - TrackingDevice
        virtual Status GetSupportedProfile(TrackingData::Profile& profile);
        virtual Status Start(Listener*, TrackingData::Profile* = NULL);
        virtual Status Stop();
        virtual Status GetDeviceInfo(TrackingData::DeviceInfo& info);
        virtual Status GetSupportedRawStreams(TrackingData::VideoProfile* videoProfiles, TrackingData::GyroProfile* gyroProfiles, TrackingData::AccelerometerProfile* accelerometerProfiles, TrackingData::VelocimeterProfile* velocimeterProfiles = nullptr);
        virtual Status SetFWLogControl(const TrackingData::LogControl& logControl);
        virtual Status GetFWLog(TrackingData::Log& log);
        virtual Status GetCameraIntrinsics(SensorId id, TrackingData::CameraIntrinsics& intrinsics);
        virtual Status SetCameraIntrinsics(SensorId id, const TrackingData::CameraIntrinsics& intrinsics);
        virtual Status GetMotionModuleIntrinsics(SensorId id, TrackingData::MotionIntrinsics& intrinsics);
        virtual Status SetMotionModuleIntrinsics(SensorId id, const TrackingData::MotionIntrinsics& intrinsics);
        virtual Status GetExtrinsics(SensorId id, TrackingData::SensorExtrinsics& extrinsics);
        virtual Status SetOccupancyMapControl(uint8_t enable);
        virtual Status GetPose(TrackingData::PoseFrame& pose, uint8_t sourceIndex);
        virtual Status SetExposureModeControl(const TrackingData::ExposureModeControl& mode);
        virtual Status SetExposure(const TrackingData::Exposure& exposure);
        virtual Status GetTemperature(TrackingData::Temperature& temperature);
        virtual Status SetTemperatureThreshold(const TrackingData::Temperature& temperature, uint32_t token);
        virtual Status LockConfiguration(LockType type, bool lock, uint16_t tableType);
        virtual Status PermanentLockConfiguration(LockType type, uint32_t token, uint16_t tableType);
        virtual Status ReadConfiguration(uint16_t tableType, uint16_t size, uint8_t* buffer, uint16_t* actualSize = nullptr);
        virtual Status WriteConfiguration(uint16_t tableType, uint16_t size, uint8_t* buffer);
        virtual Status DeleteConfiguration(uint16_t tableType);
        virtual Status GetLocalizationData(Listener* listener);
        virtual Status SetLocalizationData(Listener* listener, uint32_t length, const uint8_t* buffer);
        virtual Status ResetLocalizationData(uint8_t flag);
        virtual Status SetStaticNode(const char* guid, const TrackingData::RelativePose& relativePose);
        virtual Status GetStaticNode(const char* guid, TrackingData::RelativePose& relativePose);
        virtual Status SetGeoLocation(const TrackingData::GeoLocalization& geoLocation);
        virtual Status EepromRead(uint16_t offset, uint16_t size, uint8_t* buffer, uint16_t& actual);
        virtual Status EepromWrite(uint16_t offset, uint16_t size, uint8_t* buffer, uint16_t& actual, bool verify = false);
        virtual Status Reset(void);
        virtual Status SetCalibration(const TrackingData::CalibrationData& calibrationData);
        virtual Status SendFrame(const TrackingData::VelocimeterFrame& frame);
        virtual Status SendFrame(const TrackingData::VideoFrame& frame);
        virtual Status SendFrame(const TrackingData::GyroFrame& frame);
        virtual Status SendFrame(const TrackingData::AccelerometerFrame& frame);
        virtual Status ControllerConnect(const TrackingData::ControllerDeviceConnect& device, uint8_t& controllerId);
        virtual Status ControllerDisconnect(uint8_t controllerId);
        virtual Status ControllerStartCalibration(uint8_t controllerId);
        virtual Status GetAssociatedDevices(TrackingData::ControllerAssociatedDevices& devices);
        virtual Status SetAssociatedDevices(const TrackingData::ControllerAssociatedDevices& devices);
        virtual Status ControllerSendData(const TrackingData::ControllerData& controllerData);
        virtual Status ControllerRssiTestControl(uint8_t controllerId, bool testControl);
        virtual Status SetGpioControl(uint8_t gpioControl);
        virtual Status ControllerFWUpdate(const TrackingData::ControllerFW& FW) override;
        // [interface] EventHandler
        virtual void onTimeout(uintptr_t timerId, const Message &msg);

        bool IsDeviceReady() { return (mUsbState == DEVICE_USB_STATE_READY);  }

    protected:
        // [interface] - EventHandler
        virtual void onMessage(const Message &) override {};
        virtual void onExit() override;
        virtual Status DevConfigurationLock(uint32_t lockValue, uint16_t tableType);
        virtual Status DevEepromLock(uint32_t lockValue);

        virtual void putBufferBack(SensorId id, std::shared_ptr<uint8_t>& frame);

        // [USB States]
        typedef enum {
            DEVICE_USB_STATE_INIT = 0,
            DEVICE_USB_STATE_OPENED,
            DEVICE_USB_STATE_CLAIMED,
            DEVICE_USB_STATE_READY,
            DEVICE_USB_STATE_MAX
        } DEVICE_USB_STATE;

        // [FSM] declaration
        Fsm mFsm;
        DECLARE_FSM(main);

        // [States]
        enum {
            IDLE_STATE = FSM_STATE_USER_DEFINED,
            ASYNC_STATE,
            ACTIVE_STATE,
            ERROR_STATE,
        };

        // [Messages]
        enum {
            ON_INIT = FSM_EVENT_USER_DEFINED,
            ON_DONE,
            ON_START,
            ON_STOP,
            ON_DETACH,
            ON_ERROR,
            ON_BULK_MESSAGE,
            ON_CONTROL_MESSAGE,
            ON_PLAYBACK_SET,
            ON_SET_ENABLED_STREAMS,
            ON_ASYNC_START,
            ON_ASYNC_STOP,
            ON_LARGE_MESSAGE
        };

        /* The timeout that USB transfer should wait before giving up due to no response being received */
        enum {
            USB_TRANSFER_FAST_TIMEOUT_MS = 100,    /* Most of the messages uses fast timeout                                               */
            USB_TRANSFER_MEDIUM_TIMEOUT_MS = 5000, /* All Control messages + Start/Stop/GetTemperature may need medium timeout to complete */
            USB_TRANSFER_SLOW_TIMEOUT_MS = 16000,  /* Controller connect/disconnect may need slow timeout to complete                      */
        };

        // [State] IDLE
        DECLARE_FSM_STATE(IDLE_STATE);
        DECLARE_FSM_STATE_ENTRY(IDLE_STATE);
        DECLARE_FSM_ACTION(IDLE_STATE, ON_INIT);
        DECLARE_FSM_ACTION(IDLE_STATE, ON_START);
        DECLARE_FSM_ACTION(IDLE_STATE, ON_DETACH);
        DECLARE_FSM_ACTION(IDLE_STATE, ON_PLAYBACK_SET);
        DECLARE_FSM_ACTION(IDLE_STATE, ON_STOP);
        DECLARE_FSM_ACTION(IDLE_STATE, ON_ERROR);
        DECLARE_FSM_ACTION(IDLE_STATE, ON_BULK_MESSAGE);
        DECLARE_FSM_ACTION(IDLE_STATE, ON_LARGE_MESSAGE);
        DECLARE_FSM_ACTION(IDLE_STATE, ON_CONTROL_MESSAGE);
        DECLARE_FSM_ACTION(IDLE_STATE, ON_SET_ENABLED_STREAMS);
        DECLARE_FSM_ACTION(IDLE_STATE, ON_ASYNC_START);
        DECLARE_FSM_ACTION(IDLE_STATE, ON_ASYNC_STOP);
        DECLARE_FSM_GUARD(IDLE_STATE, ON_START);
        DECLARE_FSM_GUARD(IDLE_STATE, ON_ASYNC_START);


        // [State] ASYNC
        DECLARE_FSM_STATE(ASYNC_STATE);
        DECLARE_FSM_STATE_ENTRY(ASYNC_STATE);
        DECLARE_FSM_STATE_EXIT(ASYNC_STATE);
        DECLARE_FSM_ACTION(ASYNC_STATE, ON_INIT);
        DECLARE_FSM_ACTION(ASYNC_STATE, ON_DONE);
        DECLARE_FSM_ACTION(ASYNC_STATE, ON_START);
        DECLARE_FSM_ACTION(ASYNC_STATE, ON_STOP);
        DECLARE_FSM_ACTION(ASYNC_STATE, ON_DETACH);
        DECLARE_FSM_ACTION(ASYNC_STATE, ON_ERROR);
        DECLARE_FSM_ACTION(ASYNC_STATE, ON_BULK_MESSAGE);
        DECLARE_FSM_ACTION(ASYNC_STATE, ON_LARGE_MESSAGE);
        DECLARE_FSM_ACTION(ASYNC_STATE, ON_CONTROL_MESSAGE);
        DECLARE_FSM_ACTION(ASYNC_STATE, ON_PLAYBACK_SET);
        DECLARE_FSM_ACTION(ASYNC_STATE, ON_SET_ENABLED_STREAMS);
        DECLARE_FSM_ACTION(ASYNC_STATE, ON_ASYNC_START);
        DECLARE_FSM_ACTION(ASYNC_STATE, ON_ASYNC_STOP);


        // [State] ACTIVE
        DECLARE_FSM_STATE(ACTIVE_STATE);
        DECLARE_FSM_STATE_ENTRY(ACTIVE_STATE);
        DECLARE_FSM_STATE_EXIT(ACTIVE_STATE);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_DETACH);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_INIT);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_DONE);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_START);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_PLAYBACK_SET);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_ASYNC_START);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_ASYNC_STOP);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_ERROR);
        DECLARE_FSM_GUARD(ACTIVE_STATE, ON_STOP);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_STOP);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_BULK_MESSAGE);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_LARGE_MESSAGE);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_CONTROL_MESSAGE);


        // [State] ERROR
        DECLARE_FSM_STATE(ERROR_STATE);
        DECLARE_FSM_STATE_ENTRY(ERROR_STATE);
        DECLARE_FSM_ACTION(ERROR_STATE, ON_STOP);
        DECLARE_FSM_ACTION(ERROR_STATE, ON_BULK_MESSAGE);
        DECLARE_FSM_ACTION(ERROR_STATE, ON_LARGE_MESSAGE);
        DECLARE_FSM_ACTION(ERROR_STATE, ON_CONTROL_MESSAGE);
        //DECLARE_FSM_ACTION(ERROR_STATE, ON_DONE);

        class CentralListener : public TrackingDevice::Listener
        {
            virtual void onFWUpdateEvent(OUT TrackingData::ControllerFWEventFrame& frame) override {}
        };

        class Bulk_Message : public Message {
        public:
            Bulk_Message(unsigned char* request, unsigned int sizeOfRequest, unsigned char* response, unsigned int sizeOfResponse, int endpointOut, int endpointIn, int timeoutInMs = USB_TRANSFER_FAST_TIMEOUT_MS) :
                Message(ON_BULK_MESSAGE), srcSize(sizeOfRequest), dstSize(sizeOfResponse), mSrc(request), mDst(response), mEndpointIn(endpointIn), mEndpointOut(endpointOut), mTimeoutInMs(timeoutInMs)
            { }
            unsigned char* mSrc;
            unsigned char* mDst;
            int srcSize;
            int dstSize;
            int mEndpointIn;
            int mEndpointOut;
            int mTimeoutInMs;
        };

        class Large_Message : public Message {
        public:
            Large_Message(TrackingDevice::Listener *listener, const uint16_t messageId, const uint32_t length, const uint8_t* buffer) :
                Message(ON_LARGE_MESSAGE), mListener(listener), mMessageId(messageId), mLength(length), mBuffer(buffer) {}
            TrackingDevice::Listener *mListener;
            const uint16_t mMessageId;
            const uint32_t mLength;
            const uint8_t* mBuffer;
        };

        class Control_Message : public Message {
        public:
            Control_Message(unsigned char* request, unsigned int sizeOfRequest, unsigned char* response = NULL, unsigned int sizeOfResponse = 0, uint16_t value = 0, uint16_t index = 0, unsigned int timeoutInMs = USB_TRANSFER_MEDIUM_TIMEOUT_MS) :
                Message(ON_CONTROL_MESSAGE), srcSize(sizeOfRequest), dstSize(sizeOfResponse), mSrc(request), mDst(response), mValue(value), mIndex(index), mTimeoutInMs(timeoutInMs)
            { }
            unsigned char* mSrc;
            unsigned char* mDst;
            int srcSize;
            int dstSize;
            int mTimeoutInMs;
            uint16_t mValue;
            uint16_t mIndex;
        };

        // private messages
        class MessageON_START : public Message {
        public:
            MessageON_START(TrackingDevice::Listener *l) : Message(ON_START),
                listener(l){}
            TrackingDevice::Listener *listener;
        };

        class MessageON_SET_ENABLED_STREAMS : public Message {
        public:
            MessageON_SET_ENABLED_STREAMS(supported_raw_stream_libtm_message * message, uint16_t wNumEnabledStreams) : Message(ON_SET_ENABLED_STREAMS),
               mMessage(message), mNumEnabledStreams(wNumEnabledStreams) {}
            supported_raw_stream_libtm_message * mMessage;
            uint16_t mNumEnabledStreams;
        };

        class MessageON_PLAYBACK : public Message {
        public:
            MessageON_PLAYBACK(bool on) : Message(ON_PLAYBACK_SET),
                set(on) {}
            bool set;
        };

        class MessageON_ASYNC_START : public Message {
        public:
            MessageON_ASYNC_START(TrackingDevice::Listener *listener, const uint16_t messageId, const uint32_t length, const uint8_t* buffer) :
                Message(ON_ASYNC_START), mListener(listener), mMessageId(messageId), mLength(length), mBuffer(buffer) {}
            TrackingDevice::Listener *mListener;
            const uint16_t mMessageId;
            const uint32_t mLength;
            const uint8_t* mBuffer;
        };

        class MessageON_ASYNC_STOP : public Message {
        public:
            MessageON_ASYNC_STOP() : Message(ON_ASYNC_STOP) {}
        };

    private:
        CentralListener mCentralListener;
        TrackingDevice::Listener* mListener; 
        libusb_device* mLibusbDevice;
        libusb_device_handle* mDevice;

        Dispatcher* mDispatcher;
        EventHandler* mOwner;
        CompleteQueueHandler* mTaskHandler;
        std::condition_variable mAsyncCV;
        void interruptEndpointThread();
        void streamEndpointThread();
        std::thread mInterruptEPThread;
        std::thread mBulkEPThread;
        std::atomic<bool> mStreamEndpointThreadStop;
        std::atomic<bool> mInterruptEndpointThreadStop;
        std::atomic<bool> mStreamEndpointThreadActive;
        std::atomic<bool> mInterruptEndpointThreadActive;

        std::mutex streamThreadMutex;
        std::mutex eventThreadMutex;

        class PrevFrameData {
        public:
            PrevFrameData() : prevFrameTimeStamp(0), prevFrameId(0) {};
            PrevFrameData(int64_t _prevFrameTimeStamp, uint32_t _prevFrameId) : prevFrameTimeStamp(_prevFrameTimeStamp), prevFrameId(_prevFrameId) {};
            void Reset()
            {
                prevFrameTimeStamp = 0;
                prevFrameId = 0;
            };

            int64_t prevFrameTimeStamp;
            uint32_t prevFrameId;
        };

        void ResetStatistics()
        {
            for (uint8_t i = 0; i < SixDofProfileMax; i++)
            {
                sixdofPrevFrame[i].Reset();
            }

            for (uint8_t i = 0; i < VideoProfileMax; i++)
            {
                videoPrevFrame[i].Reset();
            }

            for (uint8_t i = 0; i < GyroProfileMax; i++)
            {
                gyroPrevFrame[i].Reset();
            }

            for (uint8_t i = 0; i < AccelerometerProfileMax; i++)
            {
                accelerometerPrevFrame[i].Reset();
            }
        }

        PrevFrameData sixdofPrevFrame[SixDofProfileMax];
        PrevFrameData videoPrevFrame[VideoProfileMax];
        PrevFrameData gyroPrevFrame[GyroProfileMax];
        PrevFrameData velocimeterPrevFrame[VelocimeterProfileMax];
        PrevFrameData accelerometerPrevFrame[AccelerometerProfileMax]; 

        int mEndpointInterrupt;
        int mEndpointBulkMessages;
        int mStreamEndpoint;
        int mEepromChunkSize;

        int FindInterruptEndpoint();
        Status SetPlayback(bool on);
        Status Set6DofInterruptRate(sixdof_interrupt_rate_libtm_message message);

        void printSupportedRawStreams(supported_raw_stream_libtm_message* pRawStreams, uint32_t rawStreamsCount);
        Status SetEnabledRawStreams(supported_raw_stream_libtm_message * message, uint16_t wNumEnabledStreams);
        Status GetSupportedRawStreamsInternal(supported_raw_stream_libtm_message * message, uint16_t wBufferSize, uint16_t& wNumSupportedStreams);
        Status SetTimeoutConfiguration(uint16_t timeoutMsec);
        void onBulkMessage(const Message& msg);
        void onControlMessage(const Message& msg);
        void SendLargeMessage(const Message& msg);
        Status CentralFWUpdate();
        Status CentralLoadFW(uint8_t* buffer);
        std::mutex mDeletionMutex;
        bool mCleared;
        unsigned int mFrameTempBufferSize;

        device_info_libtm_message mDeviceInfo;

        Status WriteEepromChunk(uint16_t offset, uint16_t size, uint8_t* buffer, uint16_t& actual, bool verify = false);
        Status ReadEepromChunk(uint16_t offset, uint16_t size, uint8_t * buffer, uint16_t& actual);
        Status GetUsbConnectionDescriptor();
        Status GetDeviceInfoInternal();
        Status DeviceFlush();
        Status SetDeviceStreamConfig(uint32_t maxSize);
        Status GetInterfaceVersionInternal();
        interface_version_libtm_message mFWInterfaceVersion;
        TrackingData::DeviceInfo::UsbConnectionDescriptor mUsbDescriptor;
        Status SyncTime();
        void AllocateBuffers();
        Status Set6DoFControl(TrackingData::SixDofProfile& profile);
        Status SetController6DoFControl(bool enabled, uint8_t numOfControllers);
        ProfileType getProfileType(uint8_t sensorID);

        long long  mTM2CorrelatedTimeStampShift;
        nsecs_t mSyncTimeout;

        void StartThreads(bool interruptThread, bool frameThread);
        void StopThreads(bool force, bool interruptThread, bool frameThread);

        std::recursive_mutex mFramesBuffersMutex; /* This mutex can be acquired several times by the same thread */
        std::list<std::shared_ptr<uint8_t>> mFramesBuffersLists;
        std::map<SensorId, uint16_t> mWidthsMap;
        std::map<SensorId, uint16_t> mHeightsMap;

        bool mPlaybackIsOn;
        bool mSyncTimeEnabled;
        DEVICE_USB_STATE mUsbState;
        Status mDeviceStatus;

        std::vector<TrackingData::GyroProfile> mGyroProfiles;
        std::vector<TrackingData::VelocimeterProfile> mVelocimeterProfiles;
        std::vector<TrackingData::AccelerometerProfile> mAccelerometerProfiles;
        std::vector<TrackingData::VideoProfile> mVideoProfiles;
    };
}
