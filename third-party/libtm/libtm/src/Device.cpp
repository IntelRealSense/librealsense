// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#define LOG_TAG "Device"
#define LOG_NDEBUG 0 /* Enable LOGV */

#include <algorithm>
#include <memory>
#include <cstring>
#include <cinttypes>
#include <cstddef>
#include <vector>
#include <sstream> 
#include "Device.h"
#include "Common.h"
#include "CompleteTask.h"
#include "Version.h"
#include "Utils.h"
#include "UsbPlugListener.h"
#include "TrackingData.h"
#include "CentralAppFw.h"
#include "CentralBlFw.h"

#define CHUNK_SIZE 512
#define BUFFER_SIZE 1024
#define SYNC_TIME_FREQUENCY_NS 500000000 // in nano seconds, every 500 msec
#define SYNC_TIME_DIFF_MSEC 1
#define MAX_6DOF_TIMEDIFF_MSEC 5 /* 6dof must arrive every 5 msec */
#define WAIT_FOR_STOP_STATUS_MSEC 200 /* Time to wait before closing interrupt/event threads - give opportunity for the FW to send the DEVICE_STOPPED status */
#define MAX_FRAME_HEIGHT 1080
#define MAX_FRAME_WIDTH 1920
#define MAX_BIG_DATA_MESSAGE_LENGTH 10250 //(10K + large message header size) 
#define FRAMES_PER_STREAM 100
#define PERMANENT_LOCK_CONFIGURATION_TOKEN 0xDEAD10CC
#define BULK_ENDPOINT_MESSAGES 2 /* Endpoint address for sending/receiving all bulk messages */
#define BULK_ENDPOINT_FRAMES 1 /* Endpoint address for sending/receiving all frames (Fisheye, Gyro, Accelerometer) */
#define TO_HOST LIBUSB_ENDPOINT_IN /* to host */
#define TO_DEVICE LIBUSB_ENDPOINT_OUT /* to device */

namespace perc {
    // ----------------------------------------------------------------------------
    Device::Device(libusb_device* dev, Dispatcher* dispatcher, EventHandler* owner, CompleteQueueHandler* taskHandler) :
        mListener(nullptr), mLibusbDevice(dev), mDevice(nullptr), mDispatcher(dispatcher),
        mStreamEndpointThreadStop(false), mInterruptEndpointThreadStop(false), mStreamEndpointThreadActive(false), mInterruptEndpointThreadActive(false), mOwner(owner), mTaskHandler(taskHandler), mCleared(false),
        mEndpointBulkMessages(BULK_ENDPOINT_MESSAGES), mStreamEndpoint(BULK_ENDPOINT_FRAMES), mEepromChunkSize(CHUNK_SIZE), mSyncTimeout(SYNC_TIME_FREQUENCY_NS),
        mPlaybackIsOn(false), mSyncTimeEnabled(false), mUsbState(DEVICE_USB_STATE_INIT), mDeviceStatus(Status::SUCCESS), mFWInterfaceVersion{0}
    {
        memset(&mDeviceInfo, 0, sizeof(mDeviceInfo));
        supported_raw_stream_libtm_message streams[MAX_SUPPORTED_STREAMS];
        uint16_t actual = 0;
        Status status = Status::SUCCESS;

        DEVICELOGV("Creating Device");

        ASSERT(mDispatcher);
        mFsm.init(FSM(main), this, mDispatcher, LOG_TAG);

        auto rc = libusb_open(mLibusbDevice, &mDevice);
        if (rc != 0)
        {
            if (rc == LIBUSB_ERROR_NOT_SUPPORTED)
            {
                DEVICELOGE("Error while opening device (%s), Please install driver for Intel(R) RealSense(TM) T265 device", libusb_error_name(rc));
            }
            else
            {
                DEVICELOGE("Error while opening device. LIBUSB_ERROR_CODE: %d (%s)", rc, libusb_error_name(rc));
            }

            return;
        }

        /* Device USB marked as open - to close it on destructor */
        mUsbState = DEVICE_USB_STATE_OPENED;

        mEndpointInterrupt = FindInterruptEndpoint();
        if (mEndpointInterrupt == -1)
        {
            DEVICELOGE("Interrupt endpoint not found! ");
            return;
        }

        rc = libusb_claim_interface(mDevice, INTERFACE_INDEX);
        if (rc != 0)
        {
            DEVICELOGE("Error: Failed to claim USB interface. LIBUSB_ERROR_CODE: 0x%X (%s)", rc, libusb_error_name(rc));
            return;
        }

        /* Device USB marked as claimed - to release it on destructor */
        mUsbState = DEVICE_USB_STATE_CLAIMED;

        status = GetUsbConnectionDescriptor();
        if (status != Status::SUCCESS)
        {
            DEVICELOGE("Error: Failed to get USB connection descriptor (0x%X)", status);
            return;
        }

        // Pending for FW
        //status = DeviceFlush();
        //if (status != Status::SUCCESS)
        //{
        //    DEVICELOGE("Error: Failed to flush device (0x%X)", status);
        //    return;
        //}

        status = GetInterfaceVersionInternal();
        if (status != Status::SUCCESS)
        {
            DEVICELOGE("Error: Failed to get interface (0x%X)", status);
            return;
        }

        /* Device USB marked as ready - There are no fatal errors on device creation, from this point every error will be forwarded to the device state machine (to support user reset) */
        mUsbState = DEVICE_USB_STATE_READY;

        if ((mFWInterfaceVersion.dwMajor != LIBTM_API_VERSION_MAJOR) || (mFWInterfaceVersion.dwMinor < LIBTM_API_VERSION_MINOR))
        {
            DEVICELOGE("Error: Interface version mismatch: Host %d.%d, FW %d.%d", LIBTM_API_VERSION_MAJOR, LIBTM_API_VERSION_MINOR, mFWInterfaceVersion.dwMajor, mFWInterfaceVersion.dwMinor);
            mDispatcher->postMessage(&mFsm, Message(ON_ERROR));
            mDeviceStatus = Status::INIT_FAILED;
        }

        /* Enable low power mode */
        status = SetLowPowerModeInternal(true);
        if (status != Status::SUCCESS)
        {
            DEVICELOGE("Error: Failed to enable low power mode (0x%X)", status);
        }

        status = GetDeviceInfoInternal();
        if (status != Status::SUCCESS)
        {
            DEVICELOGE("Error: Failed to get device info (0x%X)", status);
            mDispatcher->postMessage(&mFsm, Message(ON_ERROR));
            mDeviceStatus = Status::INIT_FAILED;
            return;
        }

        // Pending for FW
        //status = SetDeviceStreamConfig(MAX_BIG_DATA_MESSAGE_LENGTH);
        //if (status != Status::SUCCESS)
        //{
        //    DEVICELOGE("Error: Set device stream config (0x%X)", status);
        //    mDispatcher->postMessage(&mFsm, Message(ON_ERROR));
        //    mDeviceStatus = Status::INIT_FAILED;
        //    return;
        //}

        mFrameTempBufferSize = (MAX_FRAME_HEIGHT * MAX_FRAME_WIDTH) + sizeof(supported_raw_stream_libtm_message);
        AllocateBuffers();

        status = GetSupportedRawStreamsInternal(streams, MAX_SUPPORTED_STREAMS, actual);
        if (status != Status::SUCCESS)
        {
            DEVICELOGE("Error: Failed to get supported raw stream (0x%X)", status);
            mDispatcher->postMessage(&mFsm, Message(ON_ERROR));
            mDeviceStatus = Status::INIT_FAILED;
            return;
        }

        for (int i = 0; i < actual; i++)
        {
            switch (GET_SENSOR_TYPE(streams[i].bSensorID))
            {
                case SensorType::Fisheye:
                {
                    TrackingData::VideoProfile p;
                    p.set(true, false, streams[i].wFramesPerSecond, GET_SENSOR_INDEX(streams[i].bSensorID));
                    p.profile.set(streams[i].wWidth, streams[i].wHeight, streams[i].wStride, static_cast<PixelFormat>(streams[i].bPixelFormat));
                    mVideoProfiles.push_back(p);
                    break;
                }

                case SensorType::Gyro:
                {
                    TrackingData::GyroProfile p;
                    p.set(true, false, streams[i].wFramesPerSecond, GET_SENSOR_INDEX(streams[i].bSensorID));
                    mGyroProfiles.push_back(p);
                    break;
                }

                case SensorType::Accelerometer:
                {
                    TrackingData::AccelerometerProfile p;
                    p.set(true, false, streams[i].wFramesPerSecond, GET_SENSOR_INDEX(streams[i].bSensorID));
                    mAccelerometerProfiles.push_back(p);
                    break;
                }

                case SensorType::Velocimeter:
                {
                    TrackingData::VelocimeterProfile p;
                    p.set(true, false, streams[i].wFramesPerSecond, GET_SENSOR_INDEX(streams[i].bSensorID));
                    mVelocimeterProfiles.push_back(p);
                    break;
                }

                case SensorType::Rssi:
                {
                    break;
                }

                default:
                    DEVICELOGE("Unknown SensorType support (0x%X)", GET_SENSOR_TYPE(streams[i].bSensorID));
                    break;
            } // end of switch
        }

        if (mDeviceInfo.bSKUInfo == SKU_INFO_TYPE::SKU_INFO_TYPE_WITH_BLUETOOTH)
        {
            status = CentralFWUpdate();
            if (status != Status::SUCCESS)
            {
                DEVICELOGE("Central firmware update failed with error (0x%X)", status);
                mDispatcher->postMessage(&mFsm, Message(ON_ERROR));
                mDeviceStatus = Status::INIT_FAILED;
                return;
            }
        }
        SyncTime();

        // schedule the listener itself for every 500 msec
        mDispatcher->scheduleTimer(this, mSyncTimeout, Message(0));

        return;
    }

    Status Device::SyncTime()
    {
        bulk_message_request_get_time req = {0};
        bulk_message_response_get_time res = {0};
        nsecs_t start;
        nsecs_t finish = systemTime();
        uint32_t syncTry = 1;

        req.header.wMessageID = DEV_GET_TIME;
        req.header.dwLength = sizeof(req);
        res.header.wStatus = 0xFF;

        while (true)
        {
            start = systemTime();

            Bulk_Message msg((uint8_t*)&req, sizeof(req), (uint8_t*)&res, sizeof(res), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

            mFsm.fireEvent(msg);

            if (msg.Result != 0)
                break;

            finish = systemTime();

            auto diff = ns2ms(finish - start);
            if ((diff) <= SYNC_TIME_DIFF_MSEC)
            {
                mTM2CorrelatedTimeStampShift = start + ((finish-start)/2) - res.llNanoseconds;
                break;
            }
            syncTry++;
        }

        if (res.header.wStatus == 0)
        {
            DEVICELOGV("TM2 and Host clocks were synced in %d nanosec after %d tries", ns2ms(finish - start), syncTry);
        }
        else
        {
            DEVICELOGE("Error syncing TM2 and Host clocks");
        }
        
        return Status::SUCCESS;
    }

    const char* temperatureSensor(uint8_t index)
    {
        switch (index)
        {
            case 0x00: return "VPU";
            case 0x01: return "IMU";
            case 0x02: return "BLE";
        }
        return "Unknown";
    }

    const char* hwVersion(uint8_t hwVersion)
    {
        switch (hwVersion)
        {
            case 0x00: return "ES0";
            case 0x01: return "ES1";
            case 0x02: return "ES2";
            case 0x03: return "ES3";
            case 0x04: return "ES4";
        }
        return "Unknown";
    }

    const char* fwLogVerbosityLevel(LogVerbosityLevel level)
    {
        switch (level)
        {
            case None:    return "[N]";
            case Error:   return "[E]";
            case Info:    return "[I]";
            case Warning: return "[W]";
            case Debug:   return "[D]";
            case Verbose: return "[V]";
            case Trace:   return "[T]";
        }
        return "[Unknown]";
    }

    enum module_id {
        MODULE_UNDEFINED = 0x0000,
        MODULE_MAIN = 0x0001,
        MODULE_USB = 0x0002,
        MODULE_IMU_DRV = 0x0003,
        MODULE_VIDEO = 0x0004,
        MODULE_MEMPOOL = 0x0005,
        MODULE_MESSAGE_IO = 0X0006,
        MODULE_LOGGER = 0x0007,
        MODULE_LOG_TESTS = 0x0008,
        MODULE_CENTRAL_MGR = 0x000A,
        MODULE_HMD_TRACKING = 0x000B,
        MODULE_CONTROLLERS = 0x000C,
        MODULE_PACKET_IO = 0x000D,
        MODULE_DFU = 0x000E,
        MODULE_CONFIG_TABLES = 0x000F,
        MODULE_LAST_VALUE = 0xFFFF
    };

    const char* fwModuleID(module_id module)
    {
        switch (module)
        {
            case MODULE_UNDEFINED:       return "UNDEFINED";
            case MODULE_MAIN:            return "MAIN";
            case MODULE_USB:             return "USB";
            case MODULE_IMU_DRV:         return "IMU_DRV";
            case MODULE_VIDEO:           return "VIDEO";
            case MODULE_MEMPOOL:         return "MEMPOOL";
            case MODULE_MESSAGE_IO:      return "MESSAGE_IO";
            case MODULE_LOGGER:          return "LOGGER";
            case MODULE_LOG_TESTS:       return "LOG_TESTS";
            case MODULE_CENTRAL_MGR:     return "CENTRAL_MGR";
            case MODULE_HMD_TRACKING:    return "HMD_TRACKING";
            case MODULE_CONTROLLERS:     return "CONTROLLERS";
            case MODULE_PACKET_IO:       return "PACKET_IO";
            case MODULE_DFU:             return "DFU";
            case MODULE_CONFIG_TABLES:   return "CONFIG_TABLE";
            case MODULE_LAST_VALUE:      break;
        }
        return "UNKNOWN";
    }
 
    Status Device::GetFWLog(TrackingData::Log& log)
    {
        bulk_message_request_get_and_clear_event_log request = {0};
        bulk_message_response_get_and_clear_event_log response = {0};
        uint8_t outputMode;
        uint8_t verbosity;
        uint8_t rolloverMode;

        request.header.wMessageID = DEV_GET_AND_CLEAR_EVENT_LOG;
        request.header.dwLength = sizeof(request);

        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);
        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            return Status::ERROR_USB_TRANSFER;
        }

        if (response.header.dwLength > sizeof(bulk_message_response_get_and_clear_event_log))
        {
            DEVICELOGE("Error: Log size (%u) is too big for bulk_message_response_get_and_clear_event_log (%u) - Host->FW API issue", response.header.dwLength, sizeof(bulk_message_response_get_and_clear_event_log));
            return Status::BUFFER_TOO_SMALL;
        }

        auto logSize = (response.header.dwLength - sizeof(bulk_message_response_header));
        log.entries = (uint32_t)(logSize / sizeof(device_event_log));
        log.maxEntries = MAX_FW_LOG_BUFFER_ENTRIES;
        DEVICELOGV("log size = %d, entries = %d, entry size = %d", logSize, log.entries, sizeof(device_event_log));

        if (logSize > (MAX_LOG_BUFFER_ENTRIES * MAX_LOG_BUFFER_ENTRY_SIZE))
        {
            DEVICELOGE("Error: Log size (%u) is too big for TrackingData::Log (%u)", logSize, (MAX_LOG_BUFFER_ENTRIES * MAX_LOG_BUFFER_ENTRY_SIZE));
            return Status::BUFFER_TOO_SMALL;
        }

        __perc_Log_Get_Configuration(LogSourceFW, &outputMode, &verbosity, &rolloverMode);

        if (log.entries == log.maxEntries)
        {
            DEVICELOGW("Got full FW log buffer (%d entries) - increase GetFWLog call rate to retrieve more FW logs", log.entries);
        }

        if (outputMode == LogOutputModeScreen)
        {
            if (log.entries > 0)
            {
                DEVICELOGD("Received %d FW log entries", log.entries);
                DEVICELOGD("FW: <FWTimeStamp> [Verbosity] [Thread] [Function] [Module](Line): <Payload>");
            }
        }

        HostLocalTime localTime = getLocalTime();
        TrackingData::Log::LocalTime localTimeStamp;

        localTimeStamp.year = localTime.year;
        localTimeStamp.month = localTime.month;
        localTimeStamp.dayOfWeek = localTime.dayOfWeek;
        localTimeStamp.day = localTime.day;
        localTimeStamp.hour = localTime.hour;
        localTimeStamp.minute = localTime.minute;
        localTimeStamp.second = localTime.second;
        localTimeStamp.milliseconds = localTime.milliseconds;

        for (uint32_t i = 0; i < log.entries; i++)
        {
            /* remove new line character */
            std::remove(response.bEventLog[i].payload, response.bEventLog[i].payload + 44, '\n');

            /* Build 8 bytes timestamp from 7 bytes FW timestamp */
            perc::copy(&log.entry[i].timeStamp, response.bEventLog[i].timestamp, sizeof(response.bEventLog[i].timestamp));
            log.entry[i].localTimeStamp = localTimeStamp;
            log.entry[i].functionSymbol = response.bEventLog[i].functionSymbol;
            log.entry[i].lineNumber = response.bEventLog[i].lineNumber;
            perc::copy(&log.entry[i].moduleID, fwModuleID((module_id)response.bEventLog[i].moduleID), MAX_LOG_BUFFER_MODULE_SIZE);
            log.entry[i].threadID = response.bEventLog[i].threadID;
            log.entry[i].verbosity = (LogVerbosityLevel)response.bEventLog[i].source;
            log.entry[i].deviceID = (uint64_t)this;
            log.entry[i].payloadSize = response.bEventLog[i].payloadSize;
            perc::copy(&log.entry[i].payload, response.bEventLog[i].payload, response.bEventLog[i].payloadSize);

            if (outputMode == LogOutputModeScreen)
            {
                DEVICELOGD("FW: %012llu %s [%02d] [0x%X] [%s](%d): %s",
                    log.entry[i].timeStamp,
                    fwLogVerbosityLevel((LogVerbosityLevel)response.bEventLog[i].source),
                    response.bEventLog[i].threadID,
                    response.bEventLog[i].functionSymbol,
                    fwModuleID((module_id)response.bEventLog[i].moduleID),
                    response.bEventLog[i].lineNumber,
                    response.bEventLog[i].payload
                );
            }
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    void Device::onTimeout(uintptr_t timerId, const Message & msg)
    {
        if (mSyncTimeEnabled)
        {
            SyncTime();
        }

        // Schedule the listener itself for every 500 msec
        mDispatcher->scheduleTimer(this, mSyncTimeout, Message(0));
    }

    Status Device::Set6DoFControl(TrackingData::SixDofProfile& profile)
    {
        bulk_message_request_6dof_control req = {0};
        bulk_message_response_6dof_control res = {0};

        req.header.wMessageID = SLAM_6DOF_CONTROL;
        req.header.dwLength = sizeof(req);
        req.bEnable = profile.enabled;
        req.bMode = profile.mode;

        DEVICELOGD("Set 6Dof Control %s, Mode 0x%X", (req.bEnable) ? "Enabled" : "Disabled", req.bMode);
        Bulk_Message msg((uint8_t*)&req, sizeof(req), (uint8_t*)&res, sizeof(res), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);
        mDispatcher->sendMessage(&mFsm, msg);

        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("Error Transferring SLAM_6DOF_CONTROL");
            return Status::ERROR_USB_TRANSFER;
        }

        return Status::SUCCESS;
    }

    Status Device::SetController6DoFControl(bool enabled, uint8_t numOfControllers)
    {
        if (mDeviceInfo.bSKUInfo == SKU_INFO_TYPE::SKU_INFO_TYPE_WITH_BLUETOOTH && enabled)
        {
            DEVICELOGE("cannot SetController6DoFControl with controllers enabled, there is no bluetooth in the device");
            return Status::FEATURE_UNSUPPORTED;
        }
        bulk_message_request_controller_pose_control req;
        bulk_message_response_controller_pose_control res = {0};

        req.header.wMessageID = CONTROLLER_POSE_CONTROL;
        req.header.dwLength = sizeof(req);
        req.bEnable = enabled;
        req.bMode = 0;
        req.bNumControllers = numOfControllers;

        DEVICELOGD("Set Controller 6Dof Control %s for %d controllers", (req.bEnable) ? "Enabled" : "Disabled", req.bNumControllers);
        Bulk_Message msg((uint8_t*)&req, sizeof(req), (uint8_t*)&res, sizeof(res), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);
        mDispatcher->sendMessage(&mFsm, msg);

        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("Error Transferring CONTROLLER_POSE_CONTROL");
            return Status::ERROR_USB_TRANSFER;
        }
        return Status::SUCCESS;
    }

    ProfileType Device::getProfileType(uint8_t sensorID)
    {
        ProfileType profileType = ProfileTypeMax;

        switch (GET_SENSOR_TYPE(sensorID))
        {
            case SensorType::Fisheye:
            {
                profileType = ProfileType::HMD;
            }
            break;

            case SensorType::Gyro:
            {
                if (GET_SENSOR_INDEX(sensorID) == GyroProfile0)
                {
                    profileType = ProfileType::HMD;
                }

                if (GET_SENSOR_INDEX(sensorID) == GyroProfile1)
                {
                    profileType = ProfileType::Controller1;
                }

                if (GET_SENSOR_INDEX(sensorID) == GyroProfile2)
                {
                    profileType = ProfileType::Controller2;
                }
            }
            break;

            case SensorType::Accelerometer:
            {
                if (GET_SENSOR_INDEX(sensorID) == AccelerometerProfile0)
                {
                    profileType = ProfileType::HMD;
                }

                if (GET_SENSOR_INDEX(sensorID) == AccelerometerProfile1)
                {
                    profileType = ProfileType::Controller1;
                }

                if (GET_SENSOR_INDEX(sensorID) == AccelerometerProfile2)
                {
                    profileType = ProfileType::Controller2;
                }
            }
            break;

            default:
                break;
        }

        return profileType;
    }

    Status Device::Reset(void)
    {
        control_message_request_reset request;
        request.header.bmRequestType = (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE);
        request.header.bRequest = CONTROL_USB_RESET;

        Control_Message msg((uint8_t*)&request, sizeof(request));
     
        DEVICELOGD("Reseting device");

        /* Calling onControlMessage */
        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("Error Transferring CONTROL_USB_RESET");
            return Status::ERROR_USB_TRANSFER;
        }

        return Status::SUCCESS;
    }

    void Device::onExit()
    {
        std::lock_guard<std::mutex> lock(mDeletionMutex);
        if (mCleared)
            return;

        mFsm.fireEvent(Message(ON_STOP));

        if (mUsbState >= DEVICE_USB_STATE_CLAIMED)
        {
            libusb_release_interface(mDevice, INTERFACE_INDEX);
        }

        if (mUsbState >= DEVICE_USB_STATE_OPENED)
        {
            libusb_close(mDevice);
        }

        mDispatcher->removeHandler(&mFsm);
        mFsm.done();
        mCleared = true;
    }

    // ----------------------------------------------------------------------------
    Device::~Device()
    {
        onExit();
    }

    // ----------------------------------------------------------------------------
    Status Device::GetSupportedProfile(TrackingData::Profile& profile)
    {
        Status st = Status::SUCCESS;
        TrackingData::DeviceInfo deviceInfo;
        st = this->GetDeviceInfo(deviceInfo);
        if (st != Status::SUCCESS)
        {
            DEVICELOGE("Error: Failed getting device info (0x%X)", st);
            return st;
        }

        std::vector<TrackingData::VideoProfile> videoProfiles;
        std::vector<TrackingData::GyroProfile> gyroProfiles;
        std::vector<TrackingData::VelocimeterProfile> velocimeterProfiles;
        std::vector<TrackingData::AccelerometerProfile> accelerometerProfiles;
        TrackingData::SixDofProfile sixDofProfile;

        videoProfiles.resize(deviceInfo.numVideoProfiles);
        gyroProfiles.resize(deviceInfo.numGyroProfile);
        velocimeterProfiles.resize(deviceInfo.numVelocimeterProfile);
        accelerometerProfiles.resize(deviceInfo.numAccelerometerProfiles);

        st = this->GetSupportedRawStreams(videoProfiles.data(), gyroProfiles.data(), accelerometerProfiles.data(), velocimeterProfiles.data());
        if (st != Status::SUCCESS)
        {
            DEVICELOGE("Error: Failed getting supported stream (0x%X)", st);
            return st;
        }

        for (uint8_t i = 0; i < GyroProfileMax; i++)
        {
            for (auto it = gyroProfiles.begin(); it != gyroProfiles.end(); ++it)
            {
                if ((*it).sensorIndex == i) 
                {
                    /* Adding the first Gyro profile on every Gyro index or the requested gyro profile according to the FPS */
                    if ((profile.gyro[i].fps == 0) || ((*it).fps == profile.gyro[i].fps))
                    {
                        profile.set((*it), false, false);
                        gyroProfiles.erase(it);
                        break;
                    }
                }
            }
        }

        for (uint8_t i = 0; i < AccelerometerProfileMax; i++)
        {
            for (auto it = accelerometerProfiles.begin(); it != accelerometerProfiles.end(); ++it)
            {
                if ((*it).sensorIndex == i) 
                {
                    /* Adding the first Accelerometer profile on every Accelerometer index or the requested Accelerometer profile according to the FPS */
                    if ((profile.accelerometer[i].fps == 0) || ((*it).fps == profile.accelerometer[i].fps))
                    {
                        profile.set((*it), false, false);
                        accelerometerProfiles.erase(it);
                        break;
                    }
                }
            }
        }

        for (uint8_t i = 0; i < VelocimeterProfileMax; i++)
        {
            for (auto it = velocimeterProfiles.begin(); it != velocimeterProfiles.end(); ++it)
            {
                if ((*it).sensorIndex == i)
                {
                    /* Adding the first Velocimeter profile on every Velocimeter index or the requested Velocimeter profile according to the FPS */
                    profile.set((*it), false, false);
                    velocimeterProfiles.erase(it);
                    break;
                }
            }
        }

        for (uint8_t i = 0; i < VideoProfileMax; i++)
        {
            for (auto it = videoProfiles.begin(); it != videoProfiles.end(); ++it)
            {
                /* Choose Video profile from supported list according to: Height+Width+FPS or default (first) profile */
                if ((*it).sensorIndex == i)
                {
                    if ((profile.video[i].profile.height * profile.video[i].profile.width * profile.video[i].fps) != 0)
                    {
                        /* Adding the requested Video profile from this Video index according to the Height, Width, FPS */
                        if ((profile.video[i].profile.height == (*it).profile.height) && (profile.video[i].profile.width == (*it).profile.width) && (profile.video[i].fps == (*it).fps))
                        {
                            profile.set((*it), false, false);
                            videoProfiles.erase(it);
                            break;
                        }
                    }
                    else
                    {
                        /* Adding the default (first) Video profile from this Video index */
                        profile.set((*it), false, false);
                        videoProfiles.erase(it);
                        break;
                    }
                }
            }
        }

        for (uint8_t i = 0; i < SixDofProfileMax; i++)
        {
            sixDofProfile.set(false, (SIXDOF_MODE_ENABLE_MAPPING | SIXDOF_MODE_ENABLE_RELOCALIZATION), SIXDOF_INTERRUPT_RATE::SIXDOF_INTERRUPT_RATE_IMU, (SixDofProfileType)i);
            profile.set(sixDofProfile, false);
        }

        profile.playbackEnabled = false;

        return st;
    }

    // ----------------------------------------------------------------------------
    Status Device::Start(Listener* listener, TrackingData::Profile* profile)
    {
        mSyncTimeEnabled = true;

        if (profile != NULL)
        {
            supported_raw_stream_libtm_message profiles[MAX_SUPPORTED_STREAMS] = { 0 };
            Status status = Status::SUCCESS;
            uint8_t sixdofControllerNum = 0;
            bool sixdofControllerEnabled = false;
            int activeProfiles = 0;

            status = SetPlayback(profile->playbackEnabled);
            if (status != Status::SUCCESS)
            {
                DEVICELOGE("Error: Failed setting playback (0x%X)", status);
                return status;
            }

            /* Video Profiles */
            for (uint8_t i = 0; i < VideoProfileMax; i++)
            {
                if (profile->video[i].enabled)
                {
                    profiles[activeProfiles].wFramesPerSecond = profile->video[i].fps;
                    profiles[activeProfiles].bOutputMode = profile->video[i].outputEnabled;
                    profiles[activeProfiles].wWidth = profile->video[i].profile.width;
                    profiles[activeProfiles].wHeight = profile->video[i].profile.height;
                    profiles[activeProfiles].wStride = profile->video[i].profile.stride;
                    profiles[activeProfiles].bPixelFormat = profile->video[i].profile.pixelFormat;
                    profiles[activeProfiles].bSensorID = SET_SENSOR_ID(SensorType::Fisheye, i);
                    activeProfiles++;
                }
            }

            /* Gyro Profiles */
            for (uint8_t i = 0; i < GyroProfileMax; i++)
            {
                if (profile->gyro[i].enabled)
                {
                    profiles[activeProfiles].bSensorID = SET_SENSOR_ID(SensorType::Gyro, i);
                    profiles[activeProfiles].bOutputMode = profile->gyro[i].outputEnabled;
                    profiles[activeProfiles].wFramesPerSecond = profile->gyro[i].fps;
                    activeProfiles++;
                }
            }

            /* Velocimeter Profiles */
            for (uint8_t i = 0; i < VelocimeterProfileMax; i++)
            {
                if (profile->velocimeter[i].enabled)
                {
                    profiles[activeProfiles].bSensorID = SET_SENSOR_ID(SensorType::Velocimeter, i);
                    profiles[activeProfiles].bOutputMode = profile->velocimeter[i].outputEnabled;
                    profiles[activeProfiles].wFramesPerSecond = 0;
                    activeProfiles++;
                }
            }

            /* Accelerometer Profiles */
            for (uint8_t i = 0; i < AccelerometerProfileMax; i++)
            {
                if (profile->accelerometer[i].enabled)
                {
                    profiles[activeProfiles].bSensorID = SET_SENSOR_ID(SensorType::Accelerometer, i);
                    profiles[activeProfiles].bOutputMode = profile->accelerometer[i].outputEnabled;
                    profiles[activeProfiles].wFramesPerSecond = profile->accelerometer[i].fps;
                    activeProfiles++;
                }
            }

            if (activeProfiles > 0)
            {
                status = SetEnabledRawStreams(profiles, activeProfiles);
                if (status != Status::SUCCESS)
                {
                    DEVICELOGE("Error: Failed setting Supported Raw Streams (0x%X)", status);
                    return status;
                }
            }

            /* 6dof HMD profile */
            status = Set6DoFControl(profile->sixDof[SixDofProfile0]);
            if (status != Status::SUCCESS)
            {
                DEVICELOGE("Error: Failed setting 6dof Control (0x%X)", status);
                return status;
            }

            /* 6dof Controllers profile */
            for (uint8_t i = (SixDofProfile0+1); i < SixDofProfileMax; i++)
            {
                if (profile->sixDof[i].enabled == true)
                {
                    sixdofControllerNum++;
                    sixdofControllerEnabled = true;
                }
            }

            status = SetController6DoFControl(sixdofControllerEnabled, sixdofControllerNum);
            if (status != Status::SUCCESS)
            {
                DEVICELOGE("Error: Failed setting Controller 6dof Control (0x%X)", status);
                return status;
            }
            

            /* Currently FW doesn't support setting interrupt rate */
            //sixdof_interrupt_rate_libtm_message msgIntRate;
            //msgIntRate.bInterruptRate = toUnderlying(profile->sixDof[SixDofHMD].interruptRate);
            //status = Set6DofInterruptRate(msgIntRate);
            //if (status != Status::SUCCESS)
            //    return status;

        }

        MessageON_START msg(listener);
        mDispatcher->sendMessage(&mFsm, msg);

        if (msg.Result != 0)
        {
            mSyncTimeEnabled = false;
        }

        return msg.Result == 0 ? Status::SUCCESS : Status::COMMON_ERROR;
    }

    // ----------------------------------------------------------------------------
    Status Device::Stop(void)
    {
        mSyncTimeEnabled = false;

        Message msg(ON_STOP);
        mDispatcher->sendMessage(&mFsm, msg);

        return msg.Result == 0 ? Status::SUCCESS : Status::COMMON_ERROR;
    }

    Status Device::GetSupportedRawStreams(TrackingData::VideoProfile* videoProfilesBuffer, TrackingData::GyroProfile* gyroProfilesBuffer, TrackingData::AccelerometerProfile* accelerometerProfilesBuffer, TrackingData::VelocimeterProfile* velocimeterProfilesBuffer)
    {
        unsigned int count = 0;
        for (auto p : mVideoProfiles)
        {
            videoProfilesBuffer[count++] = p;
        }

        count = 0;
        for (auto p : mAccelerometerProfiles)
        {
            accelerometerProfilesBuffer[count++] = p;
        }

        count = 0;
        for (auto p : mGyroProfiles)
        {
            gyroProfilesBuffer[count++] = p;
        }

        if (velocimeterProfilesBuffer != nullptr)
        {
            count = 0;
            for (auto p : mVelocimeterProfiles)
            {
                velocimeterProfilesBuffer[count++] = p;
            }
        }

        return Status::SUCCESS;
    }

    void Device::printSupportedRawStreams(supported_raw_stream_libtm_message* pRawStreams, uint32_t rawStreamsCount)
    {
        DEVICELOGD("---+----------------------------+--------+-------+--------+--------+--------+--------+------");
        DEVICELOGD(" # |            Sensor          | Frames | Width | Height | Pixel  | Output | Stride | Rsvd ");
        DEVICELOGD("   |     Name      | Type | Idx | PerSec |       |        | Format | Mode   |        |      ");
        DEVICELOGD("---+---------------+------+-----+--------+-------+--------+--------+--------+--------+------");

        for (uint32_t i = 0; i < rawStreamsCount; i++)
        {
            DEVICELOGD("%02d | %-13s | 0x%02X |  %01d  | %-6d | %-5d | %-5d  |  %-3d   | 0x%01X    | %-3d    | %-3d", i, sensorToString(SensorType(GET_SENSOR_TYPE(pRawStreams[i].bSensorID))).c_str(), GET_SENSOR_TYPE(pRawStreams[i].bSensorID), GET_SENSOR_INDEX(pRawStreams[i].bSensorID),
                pRawStreams[i].wFramesPerSecond, pRawStreams[i].wWidth, pRawStreams[i].wHeight, pRawStreams[i].bPixelFormat, pRawStreams[i].bOutputMode, pRawStreams[i].wStride, pRawStreams[i].bReserved);
        }
        DEVICELOGD("---+---------------+------+-----+--------+-------+--------+--------+--------+--------+------");
        DEVICELOGD("");
    }

    Status Device::GetSupportedRawStreamsInternal(supported_raw_stream_libtm_message * message, uint16_t wBufferSize, uint16_t& wNumSupportedStreams)
    {
        wNumSupportedStreams = 0;

        if (!message)
        {
            return Status::ERROR_PARAMETER_INVALID;
        }

        bulk_message_request_get_supported_raw_streams req;

        req.header.dwLength = sizeof(req);
        req.header.wMessageID = DEV_GET_SUPPORTED_RAW_STREAMS;

        uint8_t responseBuffer[BUFFER_SIZE];

        Bulk_Message msg((uint8_t*)&req, sizeof(req), responseBuffer, BUFFER_SIZE, mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mFsm.fireEvent(msg);

        bulk_message_response_get_supported_raw_streams* res = (bulk_message_response_get_supported_raw_streams*)responseBuffer;

        uint16_t streamsToCopy;
        Status st = Status::SUCCESS;

        if (res->header.wStatus != 0)
            return fwToHostStatus((MESSAGE_STATUS)res->header.wStatus);

        if (wBufferSize > res->wNumSupportedStreams)
        {
            streamsToCopy = res->wNumSupportedStreams;
        }
        else
        {
            streamsToCopy = wBufferSize;
            st = Status::BUFFER_TOO_SMALL;
        }

        wNumSupportedStreams = streamsToCopy;

        perc::copy(message, res->stream, streamsToCopy*sizeof(supported_raw_stream_libtm_message));

        DEVICELOGD("Get Supported RAW Streams (%d)", streamsToCopy);
        printSupportedRawStreams(res->stream, streamsToCopy);

        return st;
    }

    void Device::AllocateBuffers()
    {
        while (mFramesBuffersLists.size() > 0)
        {
            mFramesBuffersLists.pop_front();
        }

        for (int j = 0; j < FRAMES_PER_STREAM; j++)
        {
            mFramesBuffersLists.push_back(std::shared_ptr<uint8_t>(new uint8_t[mFrameTempBufferSize], arrayDeleter<uint8_t>()));
            DEVICELOGV("frame buffers pushed back - %p", mFramesBuffersLists.back().get());
        }
    }

    Status Device::SetEnabledRawStreams(supported_raw_stream_libtm_message * message, uint16_t wNumEnabledStreams)
    {
        if (!message)
        {
            return Status::ERROR_PARAMETER_INVALID;
        }

        MessageON_SET_ENABLED_STREAMS setMsg(message, wNumEnabledStreams);

        mDispatcher->sendMessage(&mFsm, setMsg);

        return setMsg.Result == 0 ? Status::SUCCESS : Status::COMMON_ERROR;
    }

    Status Device::SetFWLogControl(const TrackingData::LogControl& logControl)
    {
        /* FW log control */
        bulk_message_request_log_control request = {0};
        bulk_message_response_log_control response = {0};

        __perc_Log_Set_Configuration(LogSourceFW, logControl.outputMode, logControl.verbosity, logControl.rolloverMode);

        request.header.wMessageID = DEV_LOG_CONTROL;
        request.header.dwLength = sizeof(request);
        request.bVerbosity = logControl.verbosity;
        request.bLogMode = logControl.rolloverMode;

        DEVICELOGD("Set FW Log Control, output to %s, verbosity = 0x%X, Rollover mode = 0x%X", (logControl.outputMode == LogOutputModeScreen) ? "Screen" : "Buffer", request.bVerbosity, request.bLogMode);
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::GetCameraIntrinsics(SensorId id, TrackingData::CameraIntrinsics& intrinsics)
    {
        bulk_message_request_get_camera_intrinsics request = {0};
        bulk_message_response_get_camera_intrinsics response = {0};

        if ((GET_SENSOR_TYPE(id) != SensorType::Color) &&
            (GET_SENSOR_TYPE(id) != SensorType::Depth) &&
            (GET_SENSOR_TYPE(id) != SensorType::IR) &&
            (GET_SENSOR_TYPE(id) != SensorType::Fisheye))
        {
            DEVICELOGE("Unsupported SensorId (0x%X)", id);
            return Status::ERROR_PARAMETER_INVALID;
        }

        request.header.wMessageID = DEV_GET_CAMERA_INTRINSICS;
        request.header.dwLength = sizeof(request);
        request.bCameraID = id;

        DEVICELOGD("Get camera intrinsics for sensor [%d,%d]", GET_SENSOR_TYPE(id), GET_SENSOR_INDEX(id));
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }
        
        if ((MESSAGE_STATUS)response.header.wStatus == MESSAGE_STATUS::SUCCESS)
        {
            intrinsics = cameraIntrinsicsMessageToClass(response.intrinsics);
            DEVICELOGD("Width = %d", intrinsics.width);
            DEVICELOGD("Height = %d", intrinsics.height);
            DEVICELOGD("Ppx = %f", intrinsics.ppx);
            DEVICELOGD("Ppy = %f", intrinsics.ppy);
            DEVICELOGD("Fx = %f", intrinsics.fx);
            DEVICELOGD("Fy = %f", intrinsics.fy);
            DEVICELOGD("DistortionModel = %d", intrinsics.distortionModel);
            for (int i = 0; i < 5; i++)
            {
                DEVICELOGD("Coeffs[%d] = %f", i, intrinsics.coeffs[i]);
            }
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::SetCameraIntrinsics(SensorId id, const TrackingData::CameraIntrinsics& intrinsics)
    {
        bulk_message_request_set_camera_intrinsics request = {0};
        bulk_message_response_set_camera_intrinsics response = {0};

        if ((GET_SENSOR_TYPE(id) != SensorType::Color) &&
            (GET_SENSOR_TYPE(id) != SensorType::Depth) &&
            (GET_SENSOR_TYPE(id) != SensorType::IR) &&
            (GET_SENSOR_TYPE(id) != SensorType::Fisheye))
        {
            DEVICELOGE("Unsupported SensorId (0x%X)", id);
            return Status::ERROR_PARAMETER_INVALID;
        }

        request.header.wMessageID = DEV_SET_CAMERA_INTRINSICS;
        request.header.dwLength = sizeof(request);
        request.bCameraID = id;
        
        request.intrinsics.dwWidth = intrinsics.width;
        request.intrinsics.dwHeight = intrinsics.height;
        request.intrinsics.flPpx = intrinsics.ppx;
        request.intrinsics.flPpy = intrinsics.ppy;
        request.intrinsics.flFx = intrinsics.fx;
        request.intrinsics.flFy = intrinsics.fy;
        request.intrinsics.dwDistortionModel = intrinsics.distortionModel;
        for (uint8_t i = 0; i < 5; i++)
        {
            request.intrinsics.flCoeffs[i] = intrinsics.coeffs[i];
        }

        DEVICELOGD("Set camera intrinsics for sensor [%d,%d]", GET_SENSOR_TYPE(request.bCameraID), GET_SENSOR_INDEX(request.bCameraID));
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        if ((MESSAGE_STATUS)response.header.wStatus == MESSAGE_STATUS::SUCCESS)
        {
            DEVICELOGD("Width = %d", request.intrinsics.dwWidth);
            DEVICELOGD("Height = %d", request.intrinsics.dwHeight);
            DEVICELOGD("Ppx = %f", request.intrinsics.flPpx);
            DEVICELOGD("Ppy = %f", request.intrinsics.flPpy);
            DEVICELOGD("Fx = %f", request.intrinsics.flFx);
            DEVICELOGD("Fy = %f", request.intrinsics.flFy);
            DEVICELOGD("DistortionModel = %d", request.intrinsics.dwDistortionModel);
            for (int i = 0; i < 5; i++)
            {
                DEVICELOGD("Coeffs[%d] = %f", i, request.intrinsics.flCoeffs[i]);
            }
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::GetMotionModuleIntrinsics(SensorId id, TrackingData::MotionIntrinsics& intrinsics)
    {
        bulk_message_request_get_motion_intrinsics request = {0};
        bulk_message_response_get_motion_intrinsics response = {0};

        if ((GET_SENSOR_TYPE(id) != SensorType::Gyro) && (GET_SENSOR_TYPE(id) != SensorType::Accelerometer))
        {
            DEVICELOGE("Unsupported SensorId (0x%X)", id);
            return Status::ERROR_PARAMETER_INVALID;
        }

        request.header.wMessageID = DEV_GET_MOTION_INTRINSICS;
        request.header.dwLength = sizeof(request);
        request.bMotionID = id;

        DEVICELOGD("Get motion module intrinsics for sensor [%d,%d]", GET_SENSOR_TYPE(id), GET_SENSOR_INDEX(id));
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        if ((MESSAGE_STATUS)response.header.wStatus == MESSAGE_STATUS::SUCCESS)
        {
            intrinsics = motionIntrinsicsMessageToClass(response.intrinsics);

            for (uint8_t i = 0; i < 3; i++)
            {
                for (uint8_t j = 0; j < 4; j++)
                {
                    DEVICELOGD("Data[%d][%d] = %f", i, j, intrinsics.data[i][j]);
                }
            }

            for (uint8_t i = 0; i < 3; i++)
            {
                DEVICELOGD("NoiseVariances[%d] = %f", i, intrinsics.noiseVariances[i]);
            }

            for (uint8_t i = 0; i < 3; i++)
            {
                DEVICELOGD("BiasVariances[%d] = %f", i, intrinsics.biasVariances[i]);
            }
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::SetMotionModuleIntrinsics(SensorId id, const TrackingData::MotionIntrinsics& intrinsics)
    {
        bulk_message_request_set_motion_intrinsics request = {0};
        bulk_message_response_set_motion_intrinsics response = {0};

        if ((GET_SENSOR_TYPE(id) != SensorType::Gyro) && (GET_SENSOR_TYPE(id) != SensorType::Accelerometer))
        {
            DEVICELOGE("Unsupported SensorId (0x%X)", id);
            return Status::ERROR_PARAMETER_INVALID;
        }

        request.header.wMessageID = DEV_SET_MOTION_INTRINSICS;
        request.header.dwLength = sizeof(request);
        request.bMotionID = id;
        request.intrinsics = motionIntrinsicsClassToMessage(intrinsics);

        DEVICELOGD("Set motion module intrinsics for sensor [%d,%d]", GET_SENSOR_TYPE(id), GET_SENSOR_INDEX(id));
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        if ((MESSAGE_STATUS)response.header.wStatus == MESSAGE_STATUS::SUCCESS)
        {
            for (uint8_t i = 0; i < 3; i++)
            {
                for (uint8_t j = 0; j < 4; j++)
                {
                    DEVICELOGD("Data[%d][%d] = %f", i, j, request.intrinsics.flData[i][j]);
                }
            }

            for (uint8_t i = 0; i < 3; i++)
            {
                DEVICELOGD("NoiseVariances[%d] = %f", i, request.intrinsics.flNoiseVariances[i]);
            }

            for (uint8_t i = 0; i < 3; i++)
            {
                DEVICELOGD("BiasVariances[%d] = %f", i, request.intrinsics.flBiasVariances[i]);
            }
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::GetExtrinsics(SensorId id, TrackingData::SensorExtrinsics& extrinsics)
    {
        bulk_message_request_get_extrinsics request;
        bulk_message_response_get_extrinsics response = {0};

        if (GET_SENSOR_TYPE(id) >= SensorType::Max)
        {
            DEVICELOGE("Unsupported SensorId (0x%X)", id);
            return Status::ERROR_PARAMETER_INVALID;
        }

        request.header.wMessageID = DEV_GET_EXTRINSICS;
        request.header.dwLength = sizeof(request);
        request.bSensorID = id;

        DEVICELOGD("Get Extrinsics pose for sensor [%d,%d]", GET_SENSOR_TYPE(id), GET_SENSOR_INDEX(id));
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        if ((MESSAGE_STATUS)response.header.wStatus == MESSAGE_STATUS::SUCCESS)
        {
            extrinsics = sensorExtrinsicsMessageToClass(response.extrinsics);

            DEVICELOGD("Reference sensor [%d,%d]", GET_SENSOR_TYPE(extrinsics.referenceSensorId), GET_SENSOR_INDEX(extrinsics.referenceSensorId));

            for (uint8_t i = 0; i < 9; i++)
            {
                DEVICELOGD("Rotation[%d] = %f", i, extrinsics.rotation[i]);
            }

            for (uint8_t i = 0; i < 3; i++)
            {
                DEVICELOGD("Translation[%d] = %f", i, extrinsics.translation[i]);
            }
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::SetOccupancyMapControl(uint8_t enable)
    {
        bulk_message_request_occupancy_map_control request = {0};
        bulk_message_response_occupancy_map_control response = {0};

        request.header.wMessageID = SLAM_OCCUPANCY_MAP_CONTROL;
        request.header.dwLength = sizeof(request);
        request.bEnable = enable;

        DEVICELOGD("Set Occupancy Map Control %s", (request.bEnable) ? "Enabled" : "Disabled");
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::GetPose(TrackingData::PoseFrame& pose, uint8_t sourceIndex)
    {
        bulk_message_request_get_pose request = {0};
        bulk_message_response_get_pose response = {0};

        request.header.wMessageID = DEV_GET_POSE;
        request.header.dwLength = sizeof(request);
        request.bIndex = sourceIndex;

        DEVICELOGD("Get Pose");
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        if ((MESSAGE_STATUS)response.header.wStatus == MESSAGE_STATUS::SUCCESS)
        {
            pose = poseMessageToClass(response.pose, request.bIndex, response.pose.llNanoseconds + mTM2CorrelatedTimeStampShift);
            DEVICELOGD("SourceIndex = 0x%X", pose.sourceIndex);
            DEVICELOGD("Timestamp %" PRId64 "", pose.timestamp);
            DEVICELOGD("SystemTimestamp %" PRId64 "", pose.systemTimestamp);
            DEVICELOGD("ArrivalTimeStamp %" PRId64 "", pose.arrivalTimeStamp);
            DEVICELOGD("translation [X,Y,Z] = [%f,%f,%f]", pose.translation.x, pose.translation.y, pose.translation.z);
            DEVICELOGD("rotation [I,J,K,R] = [%f,%f,%f,%f]", pose.rotation.i, pose.rotation.j, pose.rotation.k, pose.rotation.r);
            DEVICELOGD("velocity [X,Y,Z] = [%f,%f,%f]", pose.velocity.x, pose.velocity.y, pose.velocity.z);
            DEVICELOGD("angularVelocity [X,Y,Z] = [%f,%f,%f]", pose.angularVelocity.x, pose.angularVelocity.y, pose.angularVelocity.z);
            DEVICELOGD("acceleration [X,Y,Z] = [%f,%f,%f]", pose.acceleration.x, pose.acceleration.y, pose.acceleration.z);
            DEVICELOGD("angularAcceleration [X,Y,Z] = [%f,%f,%f]", pose.angularAcceleration.x, pose.angularAcceleration.y, pose.angularAcceleration.z);
            DEVICELOGD("trackerConfidence = 0x%X", pose.trackerConfidence);
            DEVICELOGD("mapperConfidence = 0x%X", pose.mapperConfidence);
            DEVICELOGD("trackerState = 0x%X", pose.trackerState);
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::SetExposureModeControl(const TrackingData::ExposureModeControl& mode)
    {
        bulk_message_request_set_exposure_mode_control request = {0};
        bulk_message_response_set_exposure_mode_control response = {0};

        request.header.wMessageID = DEV_EXPOSURE_MODE_CONTROL;
        request.header.dwLength = sizeof(request);
        request.bAntiFlickerMode = mode.antiFlickerMode;

        for (uint8_t i = 0; i < VideoProfileMax; i++)
        {
            if (mode.videoStreamAutoExposure[i] == true)
            {
                request.bVideoStreamsMask |= (1 << i);
            }
        }

        DEVICELOGD("Set Exposure Mode Control, bStreamsMask = 0x%X, AntiFlickerMode = 0x%X", request.bVideoStreamsMask, request.bAntiFlickerMode);
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            if (msg.Result == toUnderlying(Status::FEATURE_UNSUPPORTED))
            {
                DEVICELOGE("FEATURE_UNSUPPORTED (0x%X)", msg.Result);
                return Status::FEATURE_UNSUPPORTED;
            }
            else
            {
                DEVICELOGE("USB Error (0x%X)", msg.Result);
                return Status::ERROR_USB_TRANSFER;
            }
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::SetExposure(const TrackingData::Exposure& exposure)
    {
        if (exposure.numOfVideoStreams > MAX_VIDEO_STREAMS)
        {
            DEVICELOGE("numOfVideoStreams (%d) too big, max supported %d streams", exposure.numOfVideoStreams, MAX_VIDEO_STREAMS);
            return Status::ERROR_PARAMETER_INVALID;
        }

        bulk_message_request_set_exposure request = {0};
        bulk_message_response_set_exposure response = {0};

        uint32_t messageLength = offsetof(bulk_message_request_set_exposure, stream) + exposure.numOfVideoStreams * sizeof(stream_exposure);

        request.header.wMessageID = DEV_SET_EXPOSURE;
        request.header.dwLength = messageLength;
        request.bNumOfVideoStreams = exposure.numOfVideoStreams;

        for (uint8_t i = 0; i < exposure.numOfVideoStreams; i++)
        {
            if (GET_SENSOR_TYPE(exposure.stream[i].cameraId) > SensorType::Fisheye)
            {
                DEVICELOGE("stream[%d] = Unsupported cameraId (0x%X)", i, exposure.stream[i].cameraId);
                return Status::ERROR_PARAMETER_INVALID;
            }

            request.stream[i].bCameraID = exposure.stream[i].cameraId;
            request.stream[i].dwIntegrationTime = exposure.stream[i].integrationTime;
            request.stream[i].fGain = exposure.stream[i].gain;
        }

        DEVICELOGD("Set Exposure for %d video streams", request.bNumOfVideoStreams);
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        if ((MESSAGE_STATUS)response.header.wStatus == MESSAGE_STATUS::SUCCESS)
        {
            for (uint32_t i = 0; i < request.bNumOfVideoStreams; i++)
            {
                DEVICELOGD("VideoStream [%d]: cameraId 0x%X, dwIntegrationTime %d (usec), Gain %f", i, request.stream[i].bCameraID, request.stream[i].dwIntegrationTime, request.stream[i].fGain);
            }
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::GetTemperature(TrackingData::Temperature& temperature)
    {
        uint8_t responseBuffer[BUFFER_SIZE];
        bulk_message_request_get_temperature request;
        bulk_message_response_get_temperature* response = (bulk_message_response_get_temperature*)responseBuffer;

        request.header.wMessageID = DEV_GET_TEMPERATURE;
        request.header.dwLength = sizeof(request);

        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)response, BUFFER_SIZE, mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST, USB_TRANSFER_MEDIUM_TIMEOUT_MS);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        if ((MESSAGE_STATUS)response->header.wStatus == MESSAGE_STATUS::SUCCESS)
        {
            if (response->dwCount > TemperatureSensorMax)
            {
                DEVICELOGE("Error: Got Temperature for %u sensors, exceeding max sensors (%d)", response->dwCount, TemperatureSensorMax);
                return Status::ERROR_PARAMETER_INVALID;
            }

            for (uint8_t i = 0; i < response->dwCount; i++)
            {
                if (response->temperature[i].dwIndex >= TemperatureSensorMax)
                {
                    DEVICELOGE("Error: Temperature Sensor (%d) is unknown, max temperature sensor = 0x%X", temperature.sensor[i].index, TemperatureSensorMax - 1);
                    return Status::ERROR_PARAMETER_INVALID;
                }
            }

            temperature.numOfSensors = response->dwCount;
            DEVICELOGD("Got Temperature for %u sensors, (Status 0x%X)", temperature.numOfSensors, response->header.wStatus);
            if (temperature.numOfSensors > 0)
            {
                DEVICELOGD("---+-------------+-------------+-----------");
                DEVICELOGD(" # |   Sensor    | Temperature | Threshold ");
                DEVICELOGD("   | Info | Idx  | (Celsius)   | (Celsius) ");
                DEVICELOGD("---+------+------+-------------+-----------");

                for (uint32_t i = 0; i < temperature.numOfSensors; i++)
                {
                    temperature.sensor[i].index = (TemperatureSensorType)response->temperature[i].dwIndex;
                    temperature.sensor[i].temperature = response->temperature[i].fTemperature;
                    temperature.sensor[i].threshold = response->temperature[i].fThreshold;

                    DEVICELOGD("%02d | %s  | 0x%02X | %-11.2f | %.2f", i, temperatureSensor(temperature.sensor[i].index), temperature.sensor[i].index, temperature.sensor[i].temperature, temperature.sensor[i].threshold);
                }
                DEVICELOGD("---+------+------+-------------+-----------");
            }
        }

        return fwToHostStatus((MESSAGE_STATUS)response->header.wStatus);
    }

    Status Device::SetTemperatureThreshold(const TrackingData::Temperature& temperature, uint32_t token)
    {
        if (temperature.numOfSensors > TemperatureSensorMax)
        {
            DEVICELOGE("Error: numOfSensors (%u) is too big, max supported sensors = %d", temperature.numOfSensors, TemperatureSensorMax);
            return Status::ERROR_PARAMETER_INVALID;
        }

        for (uint8_t i = 0; i < temperature.numOfSensors; i++)
        {
            if (temperature.sensor[i].index >= TemperatureSensorMax)
            {
                DEVICELOGE("Error: Temperature Sensor (%d) is unknown, max temperature sensor = 0x%X", temperature.sensor[i].index, TemperatureSensorMax-1);
                return Status::ERROR_PARAMETER_INVALID;
            }
        }

        uint8_t requestBuffer[BUFFER_SIZE];
        bulk_message_request_set_temperature_threshold* request = (bulk_message_request_set_temperature_threshold*)requestBuffer;
        bulk_message_response_set_temperature_threshold response = {0};

        request->header.wMessageID = DEV_SET_TEMPERATURE_THRESHOLD;
        request->header.dwLength = offsetof(bulk_message_request_set_temperature_threshold, temperature) + temperature.numOfSensors * sizeof(sensor_set_temperature);
        request->wForceToken = token;
        request->dwCount = temperature.numOfSensors;

        for (uint8_t i = 0; i < temperature.numOfSensors; i++)
        {
            request->temperature[i].dwIndex = temperature.sensor[i].index;
            request->temperature[i].fThreshold = temperature.sensor[i].threshold;
        }

        DEVICELOGD("Set Temperature threshold for %d sensors, Token = 0x%X", temperature.numOfSensors, request->wForceToken);
        Bulk_Message msg((uint8_t*)request, request->header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        if ((MESSAGE_STATUS)response.header.wStatus == MESSAGE_STATUS::SUCCESS)
        {
            DEVICELOGD("---+-------------+-----------");
            DEVICELOGD(" # |   Sensor    | Threshold ");
            DEVICELOGD("   | Type | Idx  | (Celsius) ");
            DEVICELOGD("---+------+------+-----------");

            for (uint32_t i = 0; i < request->dwCount; i++)
            {
                DEVICELOGD("%02d | %s  | 0x%02X | %.2f", i, temperatureSensor(request->temperature[i].dwIndex), request->temperature[i].dwIndex, request->temperature[i].fThreshold);
            }

            DEVICELOGD("---+------+------+-----------");
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::DevConfigurationLock(uint32_t lockValue, uint16_t tableType)
    {
        bulk_message_request_lock_configuration request = { 0 };
        bulk_message_response_lock_configuration response = { 0 };
        request.header.wMessageID = DEV_LOCK_CONFIGURATION;
        request.header.dwLength = sizeof(request);
        request.wTableType = tableType;
        request.dwLock = lockValue;
        DEVICELOGD("Set %s Configuration Lock, Change Lock Value to be %s, table id is 0x%x", (request.header.wMessageID == DEV_LOCK_EEPROM) ? "Hardware" : "Software", (lockValue == 0) ? "false" : "true", tableType);
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);
        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }
        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::DevEepromLock(uint32_t lockValue)
    {
        bulk_message_request_lock_eeprom request = { 0 };
        bulk_message_response_lock_eeprom response = { 0 };

        request.header.wMessageID = DEV_LOCK_EEPROM;
        request.header.dwLength = sizeof(request);
        request.dwLock = lockValue;

        DEVICELOGD("Set %s Configuration Lock, Change Lock Value to be %s", (request.header.wMessageID == DEV_LOCK_EEPROM) ? "Hardware" : "Software", (lockValue == 0) ? "false" : "true");
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);
        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }
        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::LockConfiguration(LockType type, bool lock, uint16_t tableType)
    {
        switch (type)
        {
        case HardwareLock: return DevEepromLock(uint32_t(lock));
        case SoftwareLock: return DevConfigurationLock(uint32_t(lock), tableType);
        default:
            DEVICELOGE("Error: unknown lock type (0x%X)", type);
            return Status::ERROR_PARAMETER_INVALID;
            break;
        }
    }

    Status Device::PermanentLockConfiguration(LockType type, uint32_t token, uint16_t tableType)
    {
        if (token != PERMANENT_LOCK_CONFIGURATION_TOKEN)
        {
            DEVICELOGE("Error: Bad token (0x%X)", token);
            return Status::ERROR_PARAMETER_INVALID;
        }
        DEVICELOGD("Permanent Lock Configuration...");
        switch (type)
        {
        case HardwareLock: return DevEepromLock(token);
        case SoftwareLock: return DevConfigurationLock(token, tableType);
        default:
            DEVICELOGE("Error: unknown lock type (0x%X)", type);
            return Status::ERROR_PARAMETER_INVALID;
            break;
        }
        
    }


    Status Device::ReadConfiguration(uint16_t configurationId, uint16_t size, uint8_t* buffer, uint16_t* actualSize)
    {
        if ((buffer == NULL) || (size  == 0) || (size > MAX_CONFIGURATION_SIZE))
        {
            DEVICELOGE("Error: Invalid parameters: buffer = 0x%p, size = %d", buffer, size);
            return Status::ERROR_PARAMETER_INVALID;
        }

        uint8_t responseBuffer[BUFFER_SIZE] = {0};
        bulk_message_request_read_configuration request;
        bulk_message_response_read_configuration* response = (bulk_message_response_read_configuration*)responseBuffer;

        request.header.wMessageID = DEV_READ_CONFIGURATION;
        request.header.dwLength = sizeof(request);
        request.wTableId = configurationId;

        DEVICELOGD("Set Read Configuration: configurationId = 0x%X, size = %d", configurationId, size);
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)response, BUFFER_SIZE, mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST, USB_TRANSFER_MEDIUM_TIMEOUT_MS);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        if ((MESSAGE_STATUS)response->header.wStatus == MESSAGE_STATUS::SUCCESS)
        {
            uint32_t bufferSize = response->header.dwLength - offsetof(bulk_message_response_read_configuration, bTable);
            if (size < bufferSize)
            {
                DEVICELOGE("Error: Buffer size %d is too small: EEPROM table %d needs at least %d bytes buffer", size, configurationId, bufferSize);
                return Status::BUFFER_TOO_SMALL;
            }

            perc::copy(buffer, response->bTable, bufferSize);

            if (actualSize != nullptr)
            {
                *actualSize = bufferSize;
            }
        }

        return fwToHostStatus((MESSAGE_STATUS)response->header.wStatus);
    }
    
    Status Device::WriteConfiguration(uint16_t configurationId, uint16_t size, uint8_t* buffer)
    {
        if ((buffer == NULL) || (size == 0) || (size > MAX_CONFIGURATION_SIZE))
        {
            DEVICELOGE("Error: Invalid parameters: buffer = 0x%p, size = %d", buffer, size);
            return Status::ERROR_PARAMETER_INVALID;
        }

        uint8_t requestBuffer[BUFFER_SIZE] = {0};
        bulk_message_request_write_configuration* request = (bulk_message_request_write_configuration*)requestBuffer;
        bulk_message_response_write_configuration response = {0};

        request->header.wMessageID = DEV_WRITE_CONFIGURATION;
        request->header.dwLength = size + offsetof(bulk_message_request_write_configuration, bTable);
        request->wTableId = configurationId;
        perc::copy(request->bTable, buffer, size);

        DEVICELOGD("Set Write Configuration: configurationId = 0x%X, size = %d", configurationId, size);
        Bulk_Message msg((uint8_t*)request, request->header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST, USB_TRANSFER_MEDIUM_TIMEOUT_MS);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::DeleteConfiguration(uint16_t configurationId)
    {
        bulk_message_request_reset_configuration request = {0};
        bulk_message_response_reset_configuration response = {0};

        request.header.wMessageID = DEV_RESET_CONFIGURATION;
        request.header.dwLength = sizeof(request);
        request.wTableId = configurationId;

        DEVICELOGD("Set Delete Configuration: configurationId = 0x%X", configurationId);
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::SetTimeoutConfiguration(uint16_t timeoutMsec)
    {
        bulk_message_request_timeout_configuration request = {0};
        bulk_message_response_timeout_configuration response = {0};

        request.header.wMessageID = DEV_TIMEOUT_CONFIGURATION;
        request.header.dwLength = sizeof(request);
        request.wTimeoutInMillis = timeoutMsec;

        DEVICELOGD("Set Timeout Configuration: timeoutMsec = %d", timeoutMsec);
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    } 

    Status Device::GetLocalizationData(Listener* listener)
    {
        if (listener == NULL)
        {
            DEVICELOGE("Error: Invalid parameters: listener = %p, length = %d", listener);
            return Status::ERROR_PARAMETER_INVALID;
        }

        MessageON_ASYNC_START setMsg(listener, SLAM_GET_LOCALIZATION_DATA, 0, NULL);
        mDispatcher->sendMessage(&mFsm, setMsg);

        return setMsg.Result == 0 ? Status::SUCCESS : Status::COMMON_ERROR;
    }

    Status Device::SetLocalizationData(Listener* listener, uint32_t length, const uint8_t* buffer)
    {
        if ((listener == NULL ) || (length <= 0) || (buffer == NULL))
        {
            DEVICELOGE("Error: Invalid parameters: listener = %p, buffer = %p, length = %d", listener, buffer, length);
            return Status::ERROR_PARAMETER_INVALID;
        }

        MessageON_ASYNC_START setMsg(listener, SLAM_SET_LOCALIZATION_DATA_STREAM, length, buffer);
        mDispatcher->sendMessage(&mFsm, setMsg);

        return setMsg.Result == 0 ? Status::SUCCESS : Status::COMMON_ERROR;
    }

    Status Device::ResetLocalizationData(uint8_t flag)
    {
        bulk_message_request_reset_localization_data request = {0};
        bulk_message_response_reset_localization_data response = {0};

        if (flag > 1)
        {
            DEVICELOGE("Error: Flag (%d) is unknown", flag);
            return Status::ERROR_PARAMETER_INVALID;
        }

        request.header.wMessageID = SLAM_RESET_LOCALIZATION_DATA;
        request.header.dwLength = sizeof(request);
        request.bFlag = flag;

        DEVICELOGD("Set Reset Localization Data - Flag 0x%X", flag);
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::SetStaticNode(const char* guid, const TrackingData::RelativePose& relativePose)
    {
        bulk_message_request_set_static_node request = { 0 };
        bulk_message_response_set_static_node response = { 0 };

        auto length = perc::stringLength(guid, MAX_GUID_LENGTH);
        if (length > (MAX_GUID_LENGTH-1))
        {
            DEVICELOGE("Error: guid length is too big, max length = %d", (MAX_GUID_LENGTH - 1));
            return Status::ERROR_PARAMETER_INVALID;
        }

        request.header.wMessageID = SLAM_SET_STATIC_NODE;
        request.header.dwLength = sizeof(request);
        request.data.flX = relativePose.translation.x;
        request.data.flY = relativePose.translation.y;
        request.data.flZ = relativePose.translation.z;
        request.data.flQi = relativePose.rotation.i;
        request.data.flQj = relativePose.rotation.j;
        request.data.flQk = relativePose.rotation.k;
        request.data.flQr = relativePose.rotation.r;
        snprintf((char*)request.bGuid, length+1, "%s", guid);

        DEVICELOGD("Set Static Node: guid [%s], translation [%f,%f,%f], rotation [%f,%f,%f,%f]", guid, request.data.flX, request.data.flY, request.data.flZ, request.data.flQi, request.data.flQj, request.data.flQk, request.data.flQr);

        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::GetStaticNode(const char* guid, TrackingData::RelativePose& relativePose)
    {
        bulk_message_request_get_static_node request = { 0 };
        bulk_message_response_get_static_node response = { 0 };

        auto length = perc::stringLength(guid, MAX_GUID_LENGTH);
        if (length > (MAX_GUID_LENGTH - 1))
        {
            DEVICELOGE("Error: guid length is too big, max length = %d", (MAX_GUID_LENGTH - 1));
            return Status::ERROR_PARAMETER_INVALID;
        }

        request.header.wMessageID = SLAM_GET_STATIC_NODE;
        request.header.dwLength = sizeof(request);
        snprintf((char*)request.bGuid, length + 1, "%s", guid);

        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        DEVICELOGD("Get Static Node: guid [%s], translation  [%f,%f,%f], rotation [%f,%f,%f,%f]", guid, response.data.flX, response.data.flY, response.data.flZ, response.data.flQi, response.data.flQj, response.data.flQk, response.data.flQr);

        relativePose.translation.x = response.data.flX;
        relativePose.translation.y = response.data.flY;
        relativePose.translation.z = response.data.flZ;
        relativePose.rotation.i = response.data.flQi;
        relativePose.rotation.j = response.data.flQj;
        relativePose.rotation.k = response.data.flQk;
        relativePose.rotation.r = response.data.flQr;

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::SetCalibration(const TrackingData::CalibrationData& calibrationData)
    {
        if (calibrationData.length > MAX_SLAM_CALIBRATION_SIZE)
        {
            DEVICELOGE("Error: Buffer length (%d) is too big, max length = %d", calibrationData.length, MAX_SLAM_CALIBRATION_SIZE);
            return Status::ERROR_PARAMETER_INVALID;
        }

        if (calibrationData.type >= CalibrationTypeMax)
        {
            DEVICELOGE("Error: Calibration type (0x%X) is unsupported", calibrationData.type);
            return Status::ERROR_PARAMETER_INVALID;
        }

        // WA for low power issue - FW may not hear this bulk message on low power, pre-sending control message to wake it
        WakeFW();

        DEVICELOGD("%s calibration (length %d)", (calibrationData.type == CalibrationTypeNew)?"Set new":"Append", calibrationData.length);

        std::vector<uint8_t> buf;
        buf.resize(calibrationData.length + sizeof(bulk_message_request_header));

        bulk_message_request_slam_append_calibration* msg = (bulk_message_request_slam_append_calibration*)buf.data();
        msg->header.dwLength = (uint32_t)buf.size();
        perc::copy(buf.data() + sizeof(bulk_message_request_header), calibrationData.buffer, calibrationData.length);
        
        switch (calibrationData.type)
        {
            case CalibrationTypeNew:
                msg->header.wMessageID = SLAM_CALIBRATION;
                break;
            case CalibrationTypeAppend:
                msg->header.wMessageID = SLAM_APPEND_CALIBRATION;
                break;
        }

        int actual;
        auto rc = libusb_bulk_transfer(mDevice, mStreamEndpoint | TO_DEVICE, buf.data(), (int)buf.size(), &actual, USB_TRANSFER_MEDIUM_TIMEOUT_MS);
        if (rc != 0 || actual == 0)
        {
            DEVICELOGE("Error while sending calibration buffer to device: 0x%X", rc);
            return Status::ERROR_USB_TRANSFER;
        }

        return Status::SUCCESS;
    }

    Status Device::SetGeoLocation(const TrackingData::GeoLocalization& geoLocation)
    {
        bulk_message_request_set_geo_location request = {0};
        bulk_message_response_set_geo_location response = {0}; 

        request.header.wMessageID = DEV_SET_GEO_LOCATION;
        request.header.dwLength = sizeof(request);
        request.dfLatitude = geoLocation.latitude;
        request.dfLongitude = geoLocation.longitude;
        request.dfAltitude = geoLocation.altitude;

        DEVICELOGD("Set Geo Localization - Latitude = %lf, Longitude = %lf, Altitude = %lf", geoLocation.latitude, geoLocation.longitude, geoLocation.altitude);
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::Set6DofInterruptRate(sixdof_interrupt_rate_libtm_message message)
    {
        bulk_message_request_set_6dof_interrupt_rate request = {0};
        bulk_message_response_set_6dof_interrupt_rate response = {0};

        if (message.bInterruptRate >= SIXDOF_INTERRUPT_RATE_MAX)
        {
            DEVICELOGE("Got an invalid 6dof interrupt rate (%d)", message.bInterruptRate);
            return Status::ERROR_PARAMETER_INVALID;
        }

        request.header.wMessageID = SLAM_SET_6DOF_INTERRUPT_RATE;
        request.header.dwLength = sizeof(request);
        request.message.bInterruptRate = message.bInterruptRate;

        DEVICELOGD("Set 6Dof Interrupt rate to %d", message.bInterruptRate);
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    void Device::putBufferBack(SensorId id, std::shared_ptr<uint8_t>& frame)
    {
        std::lock_guard<std::recursive_mutex> lg(mFramesBuffersMutex);
        mFramesBuffersLists.push_back(frame);
        DEVICELOGV("frame buffers increased (%d) - %p", mFramesBuffersLists.size(), mFramesBuffersLists.back().get());

    }

    int Device::FindInterruptEndpoint()
    {
        int result = -1;

        struct libusb_config_descriptor *config;
        int rc = libusb_get_active_config_descriptor(mLibusbDevice, &config);
        if (rc < 0) 
        {
            DEVICELOGE("Error while getting active config descriptor. LIBUSB_ERROR_CODE: %d (%s)", rc, libusb_error_name(rc));
            return rc;
        }

        /* Looping on all interfaces supported by this USB configuration descriptor */
        for (int iface_idx = 0; iface_idx < config->bNumInterfaces; iface_idx++)
        {
            const struct libusb_interface *iface = &config->interface[iface_idx];

            /* Looping on all interface descriptors (alternate settings) for a particular USB interface */
            for (int altsetting_idx = 0; altsetting_idx < iface->num_altsetting; altsetting_idx++)
            {
                const struct libusb_interface_descriptor *altsetting = &iface->altsetting[altsetting_idx];

                /* Looping on all endpoint descriptors for a particular USB interface descriptor */
                for (int ep_idx = 0; ep_idx < altsetting->bNumEndpoints; ep_idx++)
                {
                    const struct libusb_endpoint_descriptor *ep = &altsetting->endpoint[ep_idx];

                    /* Searching for interrupt endpoint directed to host */
                    if ((TO_HOST == (ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK)) && (LIBUSB_TRANSFER_TYPE_INTERRUPT == (ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK)))
                    {
                        result = ep->bEndpointAddress;
                        goto found;
                    }
                }
            }

        }

    found:
        DEVICELOGV("Found interrupt endpoint address: %d", result);
        libusb_free_config_descriptor(config);
        return result;
    }

    Status Device::EepromRead(uint16_t offset, uint16_t size, uint8_t * buffer, uint16_t& actual)
    {
        uint16_t transferred = 0;
        Status st = Status::SUCCESS;
        uint16_t actualChunk = 0;
        actual = 0;

        DEVICELOGV("Reading EEPROM: offset 0x%X, size %d (bytes)", offset, size);
        while (transferred < size)
        {
            unsigned int currentChunkSize;
            currentChunkSize = ((size - transferred) > mEepromChunkSize) ? mEepromChunkSize : (size - transferred);
            st = ReadEepromChunk(offset + transferred, currentChunkSize, buffer + transferred, actualChunk);
            actual += actualChunk;
            transferred += currentChunkSize;
            if (st != Status::SUCCESS)
            {
                DEVICELOGE("Error Reading EEPROM chunk: offset 0x%X, size %d (bytes), status 0x%X", offset, size, st);
                return st;
            }
        }

        return st;
    }

    Status Device::ReadEepromChunk(uint16_t offset, uint16_t size, uint8_t * buffer, uint16_t& actual)
    {
        bulk_message_request_read_eeprom request = {0};
        bulk_message_response_read_eeprom response = {0};

        if (size > MAX_EEPROM_BUFFER_SIZE || !buffer)
        {
            DEVICELOGE("Parameter error: size %d (bytes), buffer %p", size, buffer);
            return Status::ERROR_PARAMETER_INVALID;
        }

        request.header.dwLength = sizeof(bulk_message_request_read_eeprom);
        request.header.wMessageID = DEV_READ_EEPROM;
        request.wSize = size;
        request.wOffset = offset;

        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);
        mDispatcher->sendMessage(&mFsm, msg);

        if (msg.Result == toUnderlying(Status::SUCCESS))
        {
            perc::copy(buffer, &response.bData, size);
        }
        else
        {
            return Status::ERROR_USB_TRANSFER;
        }

        DEVICELOGV("Reading EEPROM chunk: offset 0x%X, size %d (bytes), actual %d, status 0x%X", offset, size, response.wSize, response.header.wStatus);
        if (response.header.wStatus == 0)
        {
            actual = response.wSize;
            return Status::SUCCESS;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::EepromWrite(uint16_t offset, uint16_t size, uint8_t * buffer, uint16_t& actual, bool verify)
    {
        uint16_t transferred = 0;
        Status st = Status::SUCCESS;
        uint16_t actualChunk = 0;
        actual = 0;
        DEVICELOGD("Writing EEPROM: offset 0x%X, size %d (bytes)", offset, size);
        while (transferred < size)
        {
            unsigned int currentChunkSize;
            currentChunkSize = ((size - transferred) > mEepromChunkSize) ? mEepromChunkSize : (size - transferred);
            st = WriteEepromChunk(offset + transferred, currentChunkSize, buffer + transferred, actualChunk, verify);
            actual += actualChunk;
            transferred += currentChunkSize;
            if (st != Status::SUCCESS)
            {
                DEVICELOGE("Error Writing EEPROM chunk: offset 0x%X, size %d (bytes), status 0x%X", offset, size, st);
                return st;
            }
        }
        return st;
    }

    Status Device::SetLowPowerModeInternal(bool enable)
    {
        bulk_message_request_set_low_power_mode request = {0};
        bulk_message_response_set_low_power_mode response = {0};
        request.header.dwLength = sizeof(request);
        request.header.wMessageID = DEV_SET_LOW_POWER_MODE;
        request.bEnabled = enable;

        DEVICELOGD("Set Low Power mode = %s", enable?"Enabled":"Disabled");
        Bulk_Message msg((uint8_t*)&request, sizeof(request), (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST, USB_TRANSFER_MEDIUM_TIMEOUT_MS);

        mFsm.fireEvent(msg);

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::SetLowPowerMode(bool enable)
    {
        bulk_message_request_set_low_power_mode request = { 0 };
        bulk_message_response_set_low_power_mode response = { 0 };
        request.header.dwLength = sizeof(request);
        request.header.wMessageID = DEV_SET_LOW_POWER_MODE;
        request.bEnabled = enable;

        DEVICELOGD("Set Low Power mode = %s", enable?"Enabled":"Disabled");
        Bulk_Message msg((uint8_t*)&request, sizeof(request), (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST, USB_TRANSFER_MEDIUM_TIMEOUT_MS);

        mDispatcher->sendMessage(&mFsm, msg);

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::WriteEepromChunk(uint16_t offset, uint16_t size, uint8_t * buffer, uint16_t& actual, bool verify)
    {
        bulk_message_request_write_eeprom request = {0};
        bulk_message_response_write_eeprom response = {0};

        if (size > mEepromChunkSize)
        {
            DEVICELOGE("Parameter error: size %d > maxChunkSize %d", size, mEepromChunkSize);
            return Status::ERROR_PARAMETER_INVALID;
        }

        request.header.dwLength = sizeof(bulk_message_request_write_eeprom);
        request.header.wMessageID = DEV_WRITE_EEPROM;
        request.wSize = size;
        request.wOffset = offset;
        perc::copy(&request.bData[0], buffer, size);

        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);
        mDispatcher->sendMessage(&mFsm, msg);

        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error Writing EEPROM chunk: offset 0x%X, size %d (bytes), result 0x%X", offset, size, msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        DEVICELOGV("Writing EEPROM chunk: offset 0x%X, size %d (bytes), actual %d, status 0x%X", offset, size, response.wSize, response.header.wStatus);
        if (response.header.wStatus == 0)
        {
            actual = response.wSize;

            if (verify)
            {
                /* This EEPROM has a write time of 5 msec per PAGE, waiting for write to be completed before verifying */
                std::this_thread::sleep_for(std::chrono::milliseconds(5));

                uint16_t tmp;
                std::vector<char> tmpBuf;
                tmpBuf.resize(size);
                DEVICELOGV("Verifing EEPROM chunk: offset 0x%X, size %d (bytes)", offset, size);
                EepromRead(offset, size, (uint8_t*)tmpBuf.data(), tmp);
                int rc = memcmp((uint8_t*)tmpBuf.data(), buffer, size);
                if (rc)
                {
                    DEVICELOGE("Verifing EEPROM chunk failed: offset 0x%X, size %d (bytes)", offset, size);
                    return Status::ERROR_EEPROM_VERIFY_FAIL;
                }
            }
            return Status::SUCCESS;
        }

        return Status::ERROR_PARAMETER_INVALID;
    }

    Status Device::SendFrame(const TrackingData::VelocimeterFrame& frame)
    {
        std::vector<uint8_t> buf;
        buf.resize(sizeof(bulk_message_velocimeter_stream));

        bulk_message_velocimeter_stream* req = (bulk_message_velocimeter_stream*)buf.data();

        req->rawStreamHeader.header.dwLength = sizeof(bulk_message_velocimeter_stream);
        req->rawStreamHeader.header.wMessageID = DEV_SAMPLE;
        req->rawStreamHeader.bSensorID = SET_SENSOR_ID(SensorType::Velocimeter, frame.sensorIndex);
        req->rawStreamHeader.llNanoseconds = frame.timestamp;
        req->rawStreamHeader.bReserved = 0;
        req->rawStreamHeader.llArrivalNanoseconds = frame.arrivalTimeStamp;
        req->rawStreamHeader.dwFrameId = frame.frameId;

        req->metadata.dwMetadataLength = offsetof(bulk_message_velocimeter_stream_metadata, dwFrameLength) - sizeof(req->metadata.dwMetadataLength);
        req->metadata.flTemperature = frame.temperature;
        req->metadata.dwFrameLength = sizeof(bulk_message_velocimeter_stream_metadata) - offsetof(bulk_message_velocimeter_stream_metadata, dwFrameLength) - sizeof(req->metadata.dwFrameLength);
        req->metadata.flVx = frame.translationalVelocity.x;
        req->metadata.flVy = frame.translationalVelocity.y;
        req->metadata.flVz = frame.translationalVelocity.z;

        int actual;
        auto rc = libusb_bulk_transfer(mDevice, mStreamEndpoint | TO_DEVICE, (unsigned char*)req, (int)buf.size(), &actual, 100);
        if (rc != 0 || actual == 0)
        {
            DEVICELOGE("Error while sending velocimeter frame to device: %d", rc);
            return Status::ERROR_USB_TRANSFER;
        }

        return Status::SUCCESS;
    }

    Status Device::SendFrame(const TrackingData::GyroFrame& frame)
    {
        std::vector<uint8_t> buf;
        buf.resize(sizeof(bulk_message_gyro_stream));

        bulk_message_gyro_stream* req = (bulk_message_gyro_stream*)buf.data();

        req->rawStreamHeader.header.dwLength = sizeof(bulk_message_gyro_stream);
        req->rawStreamHeader.header.wMessageID = DEV_SAMPLE;
        req->rawStreamHeader.bSensorID = SET_SENSOR_ID(SensorType::Gyro, frame.sensorIndex);
        req->rawStreamHeader.llNanoseconds = frame.timestamp;
        req->rawStreamHeader.bReserved = 0;
        req->rawStreamHeader.llArrivalNanoseconds = frame.arrivalTimeStamp;
        req->rawStreamHeader.dwFrameId = frame.frameId;

        req->metadata.dwMetadataLength = offsetof(bulk_message_gyro_stream_metadata, dwFrameLength) - sizeof(req->metadata.dwMetadataLength);
        req->metadata.flTemperature = frame.temperature;
        req->metadata.dwFrameLength = sizeof(bulk_message_gyro_stream_metadata) - offsetof(bulk_message_gyro_stream_metadata, dwFrameLength) - sizeof(req->metadata.dwFrameLength);
        req->metadata.flGx = frame.angularVelocity.x;
        req->metadata.flGy = frame.angularVelocity.y;
        req->metadata.flGz = frame.angularVelocity.z;

        int actual;
        auto rc = libusb_bulk_transfer(mDevice, mStreamEndpoint | TO_DEVICE, (unsigned char*)req, (int)buf.size(), &actual, 100);
        if (rc != 0 || actual == 0)
        {
            DEVICELOGE("Error while sending gyro frame to device: %d", rc);
            return Status::ERROR_USB_TRANSFER;
        }

        return Status::SUCCESS;
    }

    Status Device::SendFrame(const TrackingData::AccelerometerFrame& frame)
    {
        std::vector<uint8_t> buf;
        buf.resize(sizeof(bulk_message_accelerometer_stream));

        bulk_message_accelerometer_stream* req = (bulk_message_accelerometer_stream*)buf.data();
        req->rawStreamHeader.header.dwLength = sizeof(bulk_message_accelerometer_stream);
        req->rawStreamHeader.header.wMessageID = DEV_SAMPLE;
        req->rawStreamHeader.bSensorID = SET_SENSOR_ID(SensorType::Accelerometer, frame.sensorIndex);
        req->rawStreamHeader.llNanoseconds = frame.timestamp;
        req->rawStreamHeader.bReserved = 0;
        req->rawStreamHeader.llArrivalNanoseconds = frame.arrivalTimeStamp;
        req->rawStreamHeader.dwFrameId = frame.frameId;

        req->metadata.dwMetadataLength = offsetof(bulk_message_accelerometer_stream_metadata, dwFrameLength) - sizeof(req->metadata.dwMetadataLength);
        req->metadata.flTemperature = frame.temperature;
        req->metadata.dwFrameLength = sizeof(bulk_message_accelerometer_stream_metadata) - offsetof(bulk_message_accelerometer_stream_metadata, dwFrameLength) - sizeof(req->metadata.dwFrameLength);
        req->metadata.flAx = frame.acceleration.x;
        req->metadata.flAy = frame.acceleration.y;
        req->metadata.flAz = frame.acceleration.z;

        int actual;
        auto rc = libusb_bulk_transfer(mDevice, mStreamEndpoint | TO_DEVICE, (unsigned char*)req, (int)buf.size(), &actual, 100);
        if (rc != 0 || actual == 0)
        {
            DEVICELOGE("Error while sending accelerometer frame to device: %d", rc);
            return Status::ERROR_USB_TRANSFER;
        }

        return Status::SUCCESS;
    }

    Status Device::SendFrame(const TrackingData::VideoFrame& frame)
    {
        std::vector<uint8_t> buf;
        buf.resize(frame.profile.stride * frame.profile.height + sizeof(bulk_message_video_stream));

        bulk_message_video_stream* msg = (bulk_message_video_stream*)buf.data();

        msg->rawStreamHeader.header.dwLength = (uint32_t)buf.size();
        msg->rawStreamHeader.header.wMessageID = DEV_SAMPLE;
        msg->rawStreamHeader.bSensorID = SET_SENSOR_ID(SensorType::Fisheye, frame.sensorIndex);
        msg->rawStreamHeader.bReserved = 0;
        msg->rawStreamHeader.llNanoseconds = frame.timestamp;
        msg->rawStreamHeader.llArrivalNanoseconds = frame.arrivalTimeStamp;
        msg->rawStreamHeader.dwFrameId = frame.frameId;
                
        msg->metadata.dwMetadataLength = frame.profile.stride*frame.profile.height + sizeof(bulk_message_video_stream_metadata);
        msg->metadata.dwExposuretime = frame.exposuretime;
        msg->metadata.fGain = frame.gain;
        msg->metadata.dwFrameLength = frame.profile.stride*frame.profile.height;

        perc::copy(buf.data() + sizeof(bulk_message_video_stream), frame.data, frame.profile.stride*frame.profile.height);

        int actual;
        auto rc = libusb_bulk_transfer(mDevice, mStreamEndpoint | TO_DEVICE, buf.data(), (int)buf.size(), &actual, 100);
        if (rc != 0 || actual == 0)
        {
            DEVICELOGE("Error while sending frame to device: %d", rc);
            return Status::ERROR_USB_TRANSFER;
        }

        return Status::SUCCESS;
    }

    Status Device::ControllerConnect(const TrackingData::ControllerDeviceConnect& device, uint8_t& controllerId)
    {
        bulk_message_request_controller_device_connect request = {0};
        bulk_message_response_controller_device_connect response = {0};

        if (device.addressType >= AddressTypeMax)
        {
            DEVICELOGE("Error: Unsupported addressType (0x%X)", device.addressType);
            return Status::ERROR_PARAMETER_INVALID;
        }
        
        request.header.wMessageID = CONTROLLER_DEVICE_CONNECT;
        request.header.dwLength = sizeof(request);
        request.wTimeout = device.timeout;
        request.bAddressType = uint8_t(device.addressType);
        perc::copy(request.bMacAddress, device.macAddress, MAC_ADDRESS_SIZE);

        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST, USB_TRANSFER_SLOW_TIMEOUT_MS);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        if ((MESSAGE_STATUS)response.header.wStatus == MESSAGE_STATUS::SUCCESS)
        {
            DEVICELOGD("Sent Controller Device Connect request for Controller MacAddress = [%02X:%02X:%02X:%02X:%02X:%02X], [AddressType 0x%X], timeout %d (msec), Received ControllerId %d",
                request.bMacAddress[0], request.bMacAddress[1], request.bMacAddress[2], request.bMacAddress[3], request.bMacAddress[4], request.bMacAddress[5], request.bAddressType, request.wTimeout, response.bControllerID);
        }
        else
        {
            DEVICELOGE("Error: Failed to send Controller Device Connect request for Controller MacAddress = [%02X:%02X:%02X:%02X:%02X:%02X], [AddressType 0x%X], timeout %d (msec)",
                request.bMacAddress[0], request.bMacAddress[1], request.bMacAddress[2], request.bMacAddress[3], request.bMacAddress[4], request.bMacAddress[5], request.bAddressType, request.wTimeout);
        }

        controllerId = response.bControllerID;

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::ControllerDisconnect(uint8_t controllerId)
    {
        bulk_message_request_controller_device_disconnect request = {0};
        bulk_message_response_controller_device_disconnect response = {0};

        request.header.wMessageID = CONTROLLER_DEVICE_DISCONNECT;
        request.header.dwLength = sizeof(request);
        request.bControllerID = controllerId;

        DEVICELOGD("Set Controller Device Disconnect request on controllerId %d", controllerId);
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST, USB_TRANSFER_SLOW_TIMEOUT_MS);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::ControllerStartCalibration(uint8_t controllerId)
    {
        bulk_message_request_controller_start_calibration request = { 0 };
        bulk_message_response_controller_start_calibration response = { 0 };

        request.header.wMessageID = CONTROLLER_START_CALIBRATION;
        request.header.dwLength = sizeof(request);
        request.bControllerID = controllerId;

        DEVICELOGD("Set Controller calibration request on controllerId %d", controllerId);
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST, USB_TRANSFER_SLOW_TIMEOUT_MS);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::GetAssociatedDevices(TrackingData::ControllerAssociatedDevices& devices)
    {
        bulk_message_request_controller_read_associated_devices request = {0};
        bulk_message_response_controller_read_associated_devices response = {0};

        request.header.wMessageID = CONTROLLER_READ_ASSOCIATED_DEVICES;
        request.header.dwLength = sizeof(request);

        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        devices.set(response.bMacAddress1, response.bMacAddress2, (AddressType)response.bAddressType1, (AddressType)response.bAddressType2);

        DEVICELOGD("Got Associated Devices from the EEPROM: Device1 %02X:%02X:%02X:%02X:%02X:%02X, Address type 0x%X, Device2 %02X:%02X:%02X:%02X:%02X:%02X, Address type 0x%X (Status 0x%X)",
            devices.macAddress1[0], devices.macAddress1[1], devices.macAddress1[2], devices.macAddress1[3], devices.macAddress1[4], devices.macAddress1[5], devices.addressType1,
            devices.macAddress2[0], devices.macAddress2[1], devices.macAddress2[2], devices.macAddress2[3], devices.macAddress2[4], devices.macAddress2[5], devices.addressType2, response.header.wStatus);

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::SetAssociatedDevices(const TrackingData::ControllerAssociatedDevices& devices)
    {
        bulk_message_request_controller_write_associated_devices request = {0};
        bulk_message_response_controller_write_associated_devices response = {0};

        request.header.wMessageID = CONTROLLER_WRITE_ASSOCIATED_DEVICES;
        request.header.dwLength = sizeof(request);
        
        if ((devices.macAddress1[0] == 0) && (devices.macAddress1[1] == 0) && (devices.macAddress1[2] == 0) && (devices.macAddress1[3] == 0) && (devices.macAddress1[4] == 0) && (devices.macAddress1[5] == 0))
        {
            DEVICELOGE("Error: MacAddress1 can't be zero");
            return Status::ERROR_PARAMETER_INVALID;
        }

        if (devices.addressType1 >= AddressTypeMax)
        {
            DEVICELOGE("Error: Unsupported addressType1 (0x%X)", devices.addressType1);
            return Status::ERROR_PARAMETER_INVALID;
        }

        if (devices.addressType2 >= AddressTypeMax)
        {
            DEVICELOGE("Error: Unsupported addressType2 (0x%X)", devices.addressType2);
            return Status::ERROR_PARAMETER_INVALID;
        }

        perc::copy(&request.bMacAddress1, devices.macAddress1, MAC_ADDRESS_SIZE);
        perc::copy(&request.bMacAddress2, devices.macAddress2, MAC_ADDRESS_SIZE);
        request.bAddressType1 = devices.addressType1;
        request.bAddressType2 = devices.addressType2;

        DEVICELOGD("Set Associated Devices to the EEPROM: Device1 %02X:%02X:%02X:%02X:%02X:%02X, Address type 0x%X, Device2 %02X:%02X:%02X:%02X:%02X:%02X, Address type 0x%X",
            request.bMacAddress1[0], request.bMacAddress1[1], request.bMacAddress1[2], request.bMacAddress1[3], request.bMacAddress1[4], request.bMacAddress1[5], request.bAddressType1,
            request.bMacAddress2[0], request.bMacAddress2[1], request.bMacAddress2[2], request.bMacAddress2[3], request.bMacAddress2[4], request.bMacAddress2[5], request.bAddressType2);

        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::ControllerSendData(const TrackingData::ControllerData& controllerData)
    {
        if (controllerData.bufferSize + offsetof(bulk_message_request_controller_send_data, bControllerData) > BUFFER_SIZE)
        {
            DEVICELOGE("BufferSize (%d bytes) must not exceed %d bytes", controllerData.bufferSize, BUFFER_SIZE - offsetof(bulk_message_request_controller_send_data, bControllerData));
            return Status::COMMON_ERROR;
        }

        if (controllerData.bufferSize == 0)
        {
            DEVICELOGE("BufferSize (%d bytes) too small", controllerData.bufferSize);
            return Status::BUFFER_TOO_SMALL;
        }

        uint8_t requestBuffer[BUFFER_SIZE];
        bulk_message_request_controller_send_data* request = (bulk_message_request_controller_send_data*)requestBuffer;
        bulk_message_response_controller_send_data response = {0};

        request->header.wMessageID = CONTROLLER_SEND_DATA;
        request->header.dwLength = offsetof(bulk_message_request_controller_send_data, bControllerData) + controllerData.bufferSize;
        request->bControllerID = controllerData.controllerId;
        request->bCommandID = controllerData.commandId;
        perc::copy(request->bControllerData, controllerData.buffer, controllerData.bufferSize);

        DEVICELOGD("Set Controller Send Data: controllerId = 0x%02X, commandId = 0x%02X, bufferSize = %02d", controllerData.controllerId, controllerData.commandId, controllerData.bufferSize);
        Bulk_Message msg((uint8_t*)request, request->header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::ControllerRssiTestControl(uint8_t controllerId, bool testControl)
    {
        bulk_message_request_controller_rssi_test_control request = {0};
        bulk_message_response_controller_rssi_test_control response = {0};

        request.header.wMessageID = CONTROLLER_RSSI_TEST_CONTROL;
        request.header.dwLength = sizeof(request);
        request.bControllerID = controllerId;
        request.bTestControl = testControl;

        DEVICELOGD("Set Controller RSSI test Control: controllerId = 0x%X, testControl = %s", controllerId, (testControl == 0)?"False":"True");
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::SetGpioControl(uint8_t gpioControl)
    {
        bulk_message_request_gpio_control request = {0};
        bulk_message_response_gpio_control response = {0};

        request.header.wMessageID = DEV_GPIO_CONTROL;
        request.header.dwLength = sizeof(request);
        request.bGpioControl = gpioControl;

        DEVICELOGD("Set GPIO Control = 0x%X", gpioControl);
        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);

        mDispatcher->sendMessage(&mFsm, msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::CentralLoadFW(uint8_t* buffer)
    {
        if (mDeviceInfo.bSKUInfo == SKU_INFO_TYPE::SKU_INFO_TYPE_WITHOUT_BLUETOOTH)
        {
            DEVICELOGE("cannot CentralLoadFW, there is no bluetooth in the device");
            return Status::FEATURE_UNSUPPORTED;
        }
        uint32_t addressSize = offsetof(message_fw_update_request, bNumFiles);
        std::vector<uint8_t> msgArr(addressSize + CENTRAL_APP_SIZE, 0);
        perc::copy(msgArr.data() + addressSize, buffer, CENTRAL_APP_SIZE);
        message_fw_update_request* msg = (message_fw_update_request*)(msgArr.data());
        MessageON_ASYNC_START setMsg(&mCentralListener, DEV_FIRMWARE_UPDATE, (uint32_t)msgArr.size(), (uint8_t*)msg);
        mFsm.fireEvent(setMsg);
        if (setMsg.Result != 0)
        {
            DEVICELOGE("Failed to start ON_ASYNC state when updating the central fw update : %d", setMsg.Result);
            return Status::COMMON_ERROR;
        }
       
        std::mutex asyncMutex;
        std::unique_lock<std::mutex> lk(asyncMutex);
        mAsyncCV.wait(lk);

        MessageON_ASYNC_STOP stopMsg;
        mFsm.fireEvent(stopMsg);
        if (stopMsg.Result != 0)
        {
            DEVICELOGE("Failed to stop ON_ASYNC state when updating the central fw update : %d", stopMsg.Result);
            return Status::COMMON_ERROR;
        }

        return Status::SUCCESS;
    }

    Status Device::CentralFWUpdate()
    {
        if (mDeviceInfo.bSKUInfo == SKU_INFO_TYPE::SKU_INFO_TYPE_WITHOUT_BLUETOOTH)
        {
            DEVICELOGE("cannot CentralFWUpdate, there is no bluetooth in the device");
            return Status::FEATURE_UNSUPPORTED;
        }

        bool updateApp = false;

        if (CentralBlFw::Version[0] != mDeviceInfo.bCentralBootloaderVersionMajor ||
            CentralBlFw::Version[1] != mDeviceInfo.bCentralBootloaderVersionMinor ||
            CentralBlFw::Version[2] != mDeviceInfo.bCentralBootloaderVersionPatch)
        {
            DEVICELOGD("Updating Central Boot Loader FW [%u.%u.%u] --> [%u.%u.%u]", 
                mDeviceInfo.bCentralBootloaderVersionMajor, mDeviceInfo.bCentralBootloaderVersionMinor, mDeviceInfo.bCentralBootloaderVersionPatch, 
                CentralBlFw::Version[0], CentralBlFw::Version[1], CentralBlFw::Version[2]);
            auto status = CentralLoadFW((uint8_t*)CentralBlFw::Buffer);
            if (status != Status::SUCCESS)
            {
                return status;
            }
            updateApp = true;
        }

        if (updateApp == true ||
            CentralAppFw::Version[0] != mDeviceInfo.bCentralAppVersionMajor ||
            CentralAppFw::Version[1] != mDeviceInfo.bCentralAppVersionMinor ||
            CentralAppFw::Version[2] != mDeviceInfo.bCentralAppVersionPatch ||
            CentralAppFw::Version[3] != mDeviceInfo.dwCentralAppVersionBuild)
        {
            DEVICELOGD("Updating Central Application FW [%u.%u.%u.%u] --> [%u.%u.%u.%u]", 
                mDeviceInfo.bCentralAppVersionMajor, mDeviceInfo.bCentralAppVersionMinor, mDeviceInfo.bCentralAppVersionPatch, mDeviceInfo.dwCentralAppVersionBuild,
                CentralAppFw::Version[0], CentralAppFw::Version[1], CentralAppFw::Version[2], CentralAppFw::Version[3]);
            auto status = CentralLoadFW((uint8_t*)CentralAppFw::Buffer);
            if (status != Status::SUCCESS)
            {
                return status;
            }
        }
        return Status::SUCCESS;
    }

    Status Device::ControllerFWUpdate(const TrackingData::ControllerFW& fw)
    {
        if (mDeviceInfo.bSKUInfo == SKU_INFO_TYPE::SKU_INFO_TYPE_WITHOUT_BLUETOOTH)
        {
            DEVICELOGE("cannot ControllerFWUpdate, there is no bluetooth in the device");
            return Status::FEATURE_UNSUPPORTED;
        }
        if (fw.imageSize == 0)
        {
            DEVICELOGE("FW image size (%d bytes) too small", fw.imageSize);
            return Status::BUFFER_TOO_SMALL;
        }

        uint32_t addressSize = offsetof(message_fw_update_request, bNumFiles);
        std::vector<uint8_t> msgArr(addressSize + fw.imageSize, 0);
        message_fw_update_request* msg = (message_fw_update_request*)(msgArr.data());
        msg->bAddressType = (uint8_t)fw.addressType;

        if (msg->bAddressType >= AddressTypeMax)
        {
            DEVICELOGE("Error: Unsupported addressType (0x%X) in FW image", msg->bAddressType);
            return Status::ERROR_PARAMETER_INVALID;
        }

        perc::copy(msgArr.data(), fw.macAddress, MAC_ADDRESS_SIZE);
        perc::copy(msgArr.data() + addressSize, fw.image, fw.imageSize);

        Large_Message setMsg(mListener, DEV_FIRMWARE_UPDATE, (uint32_t)msgArr.size(), (uint8_t*)msg);
        mDispatcher->sendMessage(&mFsm, setMsg);

        if (setMsg.Result != 0)
        {
            DEVICELOGE("Received error when loading controller image : %d", setMsg.Result);
            return Status::COMMON_ERROR;
        }
        return Status::SUCCESS;
    }

    /* Stream/Frame Endpoint Thread */
    void Device::streamEndpointThread()
    {
        /* Making sure this thread gets high CPU priority */
        std::lock_guard<std::mutex> lk(streamThreadMutex);

        DEVICELOGD("Thread Start - Stream thread (Video/Controllers/Rssi/Localization frames)");

        mStreamEndpointThreadActive = true;

        nsecs_t timeOfFirstByte = 0;
        uint64_t totalBytesReceived = 0;

        while (mStreamEndpointThreadStop == false)
        {
            int actual = 0;
            auto rc = libusb_bulk_transfer(mDevice, mStreamEndpoint | TO_HOST, mFramesBuffersLists.front().get(), mFrameTempBufferSize, &actual, 100);
            if (rc == LIBUSB_ERROR_TIMEOUT)
                continue;

            if (rc != 0 || actual == 0)
            {
                DEVICELOGE("FW crashed - got error in stream endpoint thread function: status = %d (%s), actual = %d", rc, libusb_error_name(rc), actual);
                mDispatcher->postMessage(&mFsm, Message(ON_ERROR));
                break;
            }

            // Add some statistics calculations
            totalBytesReceived += actual;
            if (timeOfFirstByte == 0)
                timeOfFirstByte = systemTime();
            else
            {
                auto currentTime = systemTime();
                double diff = (double)currentTime - timeOfFirstByte;
                diff /= 1000000000;
                if (diff != 0)
                {
                    double speed = ((double)totalBytesReceived) / diff;
                    DEVICELOGV("Current transfer speed on Frame Endpoint is: %.2f bytes per second, Total bytes received %lld, time passed %.2f", speed, totalBytesReceived, diff);
                }
            }

            bulk_message_raw_stream_header* header = (bulk_message_raw_stream_header*)mFramesBuffersLists.front().get();
            switch (header->header.wMessageID)
            {
                case DEV_SAMPLE:
                {
                    switch (GET_SENSOR_TYPE(header->bSensorID))
                    {
                        case SensorType::Fisheye:
                        {
                            /* After calling addTask, VideoFrameCompleteTask ptr can be destructed from whoever come last:                               */
                            /* 1. streamEndpointThread (This function)                                                                                   */
                            /* 2. Manager::handleEvents                                                                                                  */
                            /* in case of 1 - The VideoFrameCompleteTask destructor is called inside the brackets (before releasing mFramesBuffersMutex) */
                            /*                To prevent deadlock we are using recursive_mutex which allows calling lock inside lock in the same thread  */
                            std::lock_guard<std::recursive_mutex> lg(mFramesBuffersMutex);
                            if (mFramesBuffersLists.size() > 1)
                            {
                                std::shared_ptr<VideoFrameCompleteTask> videoFrameCompleteTask = std::make_shared<VideoFrameCompleteTask>(mListener,
                                                                                                             mFramesBuffersLists.front(),
                                                                                                             this,
                                                                                                             this,
                                                                                                             header->llNanoseconds + mTM2CorrelatedTimeStampShift,
                                                                                                             mWidthsMap[header->bSensorID],
                                                                                                             mHeightsMap[header->bSensorID]);
                                std::shared_ptr<CompleteTask> completeTask = videoFrameCompleteTask;

                                TrackingData::VideoFrame* frame = &videoFrameCompleteTask->mVideoFrame;
                                int64_t offset = (int64_t)(frame->frameId) - (int64_t)videoPrevFrame[frame->sensorIndex].prevFrameId;
                                if (videoPrevFrame[frame->sensorIndex].prevFrameId != 0)
                                {
                                    if (offset > 1)
                                    {
                                        DEVICELOGW("Video[%d] frame drops occurred - %lld missing frames [FrameId %d-%d], time diff = %d (msec)",
                                            frame->sensorIndex, offset - 1, videoPrevFrame[frame->sensorIndex].prevFrameId + 1, frame->frameId - 1, ns2ms(frame->timestamp - videoPrevFrame[frame->sensorIndex].prevFrameTimeStamp));
                                    }
                                    else if (offset < 1)
                                    {
                                        DEVICELOGW("Video[%d] frame reorder occurred - prev frameId = %d, new frameId = %d, time diff = %d (msec)",
                                            frame->sensorIndex, videoPrevFrame[frame->sensorIndex].prevFrameId, frame->frameId, ns2ms(frame->timestamp - videoPrevFrame[frame->sensorIndex].prevFrameTimeStamp));
                                    }
                                }

                                videoPrevFrame[frame->sensorIndex].prevFrameTimeStamp = frame->timestamp;
                                videoPrevFrame[frame->sensorIndex].prevFrameId = frame->frameId;

                                DEVICELOGV("frame buffers decreased (%d) - %p", mFramesBuffersLists.size()-1, mFramesBuffersLists.front().get());

                                mFramesBuffersLists.pop_front();
                                mTaskHandler->addTask(completeTask);
                            }
                            else
                            {
                                /* Todo: Consider allocating more buffers */
                                DEVICELOGE("No more frame buffers (%d), dropping frame", mFramesBuffersLists.size());
                            }
                            break;
                        } // end of switch

                        case SensorType::Controller:
                        {
                            std::shared_ptr<CompleteTask> ptr = std::make_shared<ControllerFrameCompleteTask>(mListener, header, mTM2CorrelatedTimeStampShift, this);
                            mTaskHandler->addTask(ptr);
                            break;
                        }

                        case SensorType::Rssi:
                        {
                            std::shared_ptr<CompleteTask> ptr = std::make_shared<RssiFrameCompleteTask>(mListener, header, mTM2CorrelatedTimeStampShift, this);
                            mTaskHandler->addTask(ptr);
                            break;
                        }

                        default:
                            DEVICELOGE("Unsupported sensor (0x%X) on stream endpoint thread function", header->bSensorID);
                            break;
                    }
                    break;
                }

                case SLAM_GET_LOCALIZATION_DATA_STREAM:
                {
                    std::lock_guard<std::recursive_mutex> lg(mFramesBuffersMutex);
                    interrupt_message_get_localization_data_stream* getLocalizationDataStream = (interrupt_message_get_localization_data_stream*)header;
                    uint16_t status = getLocalizationDataStream->wStatus;

                    DEVICELOGV("Got Localization Data frame: status = 0x%X, moredata = %s, chunkIndex = %d, length = %d",
                        status, 
                        (((MESSAGE_STATUS)getLocalizationDataStream->wStatus == MESSAGE_STATUS::MORE_DATA_AVAILABLE)) ? "True" : "False", 
                        getLocalizationDataStream->wIndex, 
                        getLocalizationDataStream->header.dwLength - offsetof(interrupt_message_get_localization_data_stream, bLocalizationData));

                    if (mFramesBuffersLists.size() > 1)
                    {
                        std::shared_ptr<CompleteTask> ptr = std::make_shared<LocalizationDataEventFrameCompleteTask>(mListener, mFramesBuffersLists.front(), this, this);
                        mFramesBuffersLists.pop_front();
                        mTaskHandler->addTask(ptr);
                    }
                    else
                    {
                        /* Todo: Consider allocating more buffers */
                        DEVICELOGE("No more get localization buffers (%d), dropping get localization frame", mFramesBuffersLists.size());
                    }

                    /* Check if this is the last Get localization frame, and exit ASYNC state */
                    if (status == (uint16_t)MESSAGE_STATUS::SUCCESS)
                    {
                        MessageON_ASYNC_STOP setMsg;
                        mDispatcher->postMessage(&mFsm, setMsg);
                    }
                    break;
                }

                case DEV_STATUS:
                {
                    interrupt_message_status* status = (interrupt_message_status*)header;
                    DEVICELOGD("Got DEV status %s (0x%X) on stream endpoint", statusCodeToString((MESSAGE_STATUS)status->wStatus).c_str(), status->wStatus);

                    auto hostStatus = fwToHostStatus((MESSAGE_STATUS)status->wStatus);

                    /* DEVICE_STOPPED is indicated once when device exists from ACTIVE_STATE, all other statuses will be indicated here  */
                    if (hostStatus != Status::DEVICE_STOPPED)
                    {
                        /* Creating onStatusEvent to indicate hostStatus */
                        std::shared_ptr<CompleteTask> ptr = std::make_shared<StatusEventFrameCompleteTask>(mListener, hostStatus, this);
                        mTaskHandler->addTask(ptr);
                    }
                    break;
                }

                default:
                {
                    DEVICELOGE("Unsupported message (0x%X) on stream endpoint thread function", header->header.wMessageID);
                    break;
                }
            }
        } // end of while

        mStreamEndpointThreadActive = false;
        DEVICELOGD("Thread Stop - Stream thread (Video/Controllers/Rssi/Localization frames)");
    }

    /* Event Endpoint Thread */
    void Device::interruptEndpointThread()
    {
        /* Making sure this thread gets high CPU priority */
        std::lock_guard<std::mutex> lk(eventThreadMutex);

        DEVICELOGD("Thread Start - Interrupt thread (Accelerometer/Velocimeter/Gyro/6DOF/Controller/Localization events)");
        mInterruptEndpointThreadActive = true;

        unsigned char msgBuffer[BUFFER_SIZE];
        interrupt_message_header* header = (interrupt_message_header*)msgBuffer;

        while (mInterruptEndpointThreadStop == false)
        {
            int actual = 0;
            int result;

            result = libusb_interrupt_transfer(mDevice, mEndpointInterrupt, msgBuffer, BUFFER_SIZE, &actual, 100);
            if (result == LIBUSB_ERROR_TIMEOUT)
            {
                continue;
            }

            if (result != 0 || actual < sizeof(interrupt_message_header))
            {
                DEVICELOGE("FW crashed - got error in interrupt endpoint thread function: status = %d (%s), actual = %d", result, libusb_error_name(result), actual);
                mDispatcher->postMessage(&mFsm, Message(ON_ERROR));
                break;
            }

            switch (header->wMessageID)
            {
                case DEV_GET_POSE:
                {
                    interrupt_message_get_pose poseMsg = *((interrupt_message_get_pose*)header);
                    auto pose = poseMessageToClass(poseMsg.pose, poseMsg.bIndex, poseMsg.pose.llNanoseconds + mTM2CorrelatedTimeStampShift);
                    std::shared_ptr<CompleteTask> ptr = std::make_shared<PoseCompleteTask>(mListener, pose, this);

                    /* Pose must arrive every 5 msec */
                    int64_t offsetMsec = (pose.timestamp - sixdofPrevFrame[pose.sourceIndex].prevFrameTimeStamp) / ms2ns(MAX_6DOF_TIMEDIFF_MSEC);
                    if (sixdofPrevFrame[pose.sourceIndex].prevFrameTimeStamp != 0)
                    {
                        if (offsetMsec > MAX_6DOF_TIMEDIFF_MSEC)
                        {
                            DEVICELOGW("Pose[%d] frame drops occurred - %lld missing frames, prev pose time = %lld, new pose time = %lld, time diff = %d (msec)",
                                pose.sourceIndex, offsetMsec - 1, sixdofPrevFrame[pose.sourceIndex].prevFrameTimeStamp, pose.timestamp, ns2ms(pose.timestamp - sixdofPrevFrame[pose.sourceIndex].prevFrameTimeStamp));
                        }
                        else if (offsetMsec < 0)
                        {
                            DEVICELOGW("Pose[%d] frame reorder occurred - prev pose time = %lld, new pose time = %lld, time diff = %d (msec)",
                                pose.sourceIndex, sixdofPrevFrame[pose.sourceIndex].prevFrameTimeStamp, pose.timestamp, ns2ms(pose.timestamp - sixdofPrevFrame[pose.sourceIndex].prevFrameTimeStamp));
                        }
                    }

                    sixdofPrevFrame[pose.sourceIndex].prevFrameTimeStamp = pose.timestamp;

                    mTaskHandler->addTask(ptr);
                    break;
                }

                case DEV_SAMPLE:
                {
                    interrupt_message_raw_stream_header* imuHeader = (interrupt_message_raw_stream_header*)header;
                    switch (GET_SENSOR_TYPE(imuHeader->bSensorID))
                    {
                        case SensorType::Gyro:
                        {
                            std::shared_ptr<GyroFrameCompleteTask> gyroFrameCompleteTask = std::make_shared<GyroFrameCompleteTask>(mListener, imuHeader, mTM2CorrelatedTimeStampShift, this);
                            if (gyroFrameCompleteTask != NULL)
                            {
                                TrackingData::GyroFrame* frame = &gyroFrameCompleteTask->mFrame;
                                int64_t offset = (int64_t)(frame->frameId) - (int64_t)gyroPrevFrame[frame->sensorIndex].prevFrameId;
                                if (gyroPrevFrame[frame->sensorIndex].prevFrameId != 0)
                                {
                                    if (offset > 1)
                                    {
                                        DEVICELOGW("Gyro[%d] frame drops occurred - %lld missing frames [FrameId %d-%d], time diff = %d (msec)",
                                            frame->sensorIndex, offset - 1, gyroPrevFrame[frame->sensorIndex].prevFrameId + 1, frame->frameId - 1, ns2ms(frame->timestamp - gyroPrevFrame[frame->sensorIndex].prevFrameTimeStamp));
                                    }
                                    else if (offset < 1)
                                    {
                                        DEVICELOGW("Gyro[%d] frame reorder occurred - prev frameId = %d, new frameId = %d, time diff = %d (msec)",
                                            frame->sensorIndex, gyroPrevFrame[frame->sensorIndex].prevFrameId, frame->frameId, ns2ms(frame->timestamp - gyroPrevFrame[frame->sensorIndex].prevFrameTimeStamp));
                                    }
                                }

                                gyroPrevFrame[frame->sensorIndex].prevFrameTimeStamp = frame->timestamp;
                                gyroPrevFrame[frame->sensorIndex].prevFrameId = frame->frameId;


                                std::shared_ptr<CompleteTask> completeTask = gyroFrameCompleteTask;
                                mTaskHandler->addTask(completeTask);
                                break;
                            }
                        }

                        case SensorType::Velocimeter:
                        {
                            std::shared_ptr<VelocimeterFrameCompleteTask> velocimeterFrameCompleteTask = std::make_shared<VelocimeterFrameCompleteTask>(mListener, imuHeader, mTM2CorrelatedTimeStampShift, this);
                            if (velocimeterFrameCompleteTask != NULL)
                            {
                                TrackingData::VelocimeterFrame* frame = &velocimeterFrameCompleteTask->mFrame;
                                int64_t offset = (int64_t)(frame->frameId) - (int64_t)velocimeterPrevFrame[frame->sensorIndex].prevFrameId;
                                if (velocimeterPrevFrame[frame->sensorIndex].prevFrameId != 0)
                                {
                                    if (offset > 1)
                                    {
                                        DEVICELOGW("Velocimeter[%d] frame drops occurred - %lld missing frames [FrameId %d-%d], time diff = %d (msec)",
                                            frame->sensorIndex, offset - 1, velocimeterPrevFrame[frame->sensorIndex].prevFrameId + 1, frame->frameId - 1, ns2ms(frame->timestamp - velocimeterPrevFrame[frame->sensorIndex].prevFrameTimeStamp));
                                    }
                                    else if (offset < 1)
                                    {
                                        DEVICELOGW("Velocimeter[%d] frame reorder occurred - prev frameId = %d, new frameId = %d, time diff = %d (msec)",
                                            frame->sensorIndex, velocimeterPrevFrame[frame->sensorIndex].prevFrameId, frame->frameId, ns2ms(frame->timestamp - velocimeterPrevFrame[frame->sensorIndex].prevFrameTimeStamp));
                                    }
                                }

                                velocimeterPrevFrame[frame->sensorIndex].prevFrameTimeStamp = frame->timestamp;
                                velocimeterPrevFrame[frame->sensorIndex].prevFrameId = frame->frameId;

                                std::shared_ptr<CompleteTask> completeTask = velocimeterFrameCompleteTask;
                                mTaskHandler->addTask(completeTask);
                                break;
                            }
                        }

                        case SensorType::Accelerometer:
                        {
                            std::shared_ptr<AccelerometerFrameCompleteTask> accelerometerFrameCompleteTask = std::make_shared<AccelerometerFrameCompleteTask>(mListener, imuHeader, mTM2CorrelatedTimeStampShift, this);
                            if (accelerometerFrameCompleteTask != NULL)
                            {
                                TrackingData::AccelerometerFrame* frame = &accelerometerFrameCompleteTask->mFrame;
                                int64_t offset = (int64_t)(frame->frameId) - (int64_t)accelerometerPrevFrame[frame->sensorIndex].prevFrameId;
                                if (accelerometerPrevFrame[frame->sensorIndex].prevFrameId != 0)
                                {
                                    if (offset > 1)
                                    {
                                        DEVICELOGW("Accelerometer[%d] frame drops occurred - %lld missing frames [FrameId %d-%d], time diff = %d (msec)",
                                            frame->sensorIndex, offset - 1, accelerometerPrevFrame[frame->sensorIndex].prevFrameId + 1, frame->frameId - 1, ns2ms(frame->timestamp - accelerometerPrevFrame[frame->sensorIndex].prevFrameTimeStamp));
                                    }
                                    else if (offset < 1)
                                    {
                                        DEVICELOGW("Accelerometer[%d] frame reorder occurred - prev frameId = %d, new frameId = %d, time diff = %d (msec)",
                                            frame->sensorIndex, accelerometerPrevFrame[frame->sensorIndex].prevFrameId, frame->frameId, ns2ms(frame->timestamp - accelerometerPrevFrame[frame->sensorIndex].prevFrameTimeStamp));
                                    }
                                }

                                accelerometerPrevFrame[frame->sensorIndex].prevFrameTimeStamp = frame->timestamp;
                                accelerometerPrevFrame[frame->sensorIndex].prevFrameId = frame->frameId;

                                std::shared_ptr<CompleteTask> completeTask = accelerometerFrameCompleteTask;
                                mTaskHandler->addTask(completeTask);
                                break;
                            }
                        }

                        default:
                            DEVICELOGE("Unsupported sensor (0x%X) on interrupt endpoint thread function", imuHeader->bSensorID);
                            break;
                        }
                        break;
                }

                case DEV_STATUS:
                {
                    interrupt_message_status* status = (interrupt_message_status*)header;
                    DEVICELOGD("Got DEV status %s (0x%X) on interrupt endpoint", statusCodeToString((MESSAGE_STATUS)status->wStatus).c_str(), status->wStatus);

                    auto hostStatus = fwToHostStatus((MESSAGE_STATUS)status->wStatus);

                    /* DEVICE_STOPPED is indicated once when device exists from ACTIVE_STATE, all other statuses will be indicated here  */
                    if (hostStatus != Status::DEVICE_STOPPED)
                    {
                        /* Creating onStatusEvent to indicate hostStatus */
                        std::shared_ptr<CompleteTask> ptr = std::make_shared<StatusEventFrameCompleteTask>(mListener, hostStatus, this);
                        mTaskHandler->addTask(ptr);
                    }

                    break;
                }

                case SLAM_STATUS:
                {
                    interrupt_message_status* status = (interrupt_message_status*)header;
                    DEVICELOGD("Got SLAM status %s (0x%X) on interrupt endpoint", slamStatusCodeToString((SLAM_STATUS_CODE)status->wStatus).c_str(), status->wStatus);

                    if ((SLAM_STATUS_CODE)status->wStatus != SLAM_STATUS_CODE_SUCCESS)
                    {
                        auto hostStatus = slamToHostStatus((SLAM_STATUS_CODE)status->wStatus);
                        std::shared_ptr<CompleteTask> ptr = std::make_shared<StatusEventFrameCompleteTask>(mListener, hostStatus, this);
                        mTaskHandler->addTask(ptr);
                    }

                    break;
                }

                case SLAM_ERROR:
                {
                    interrupt_message_slam_error* error = (interrupt_message_slam_error*)header;
                    DEVICELOGD("Got SLAM error %s (0x%X) on interrupt endpoint", slamErrorCodeToString((SLAM_ERROR_CODE)error->wStatus).c_str(), error->wStatus);

                    if ((SLAM_ERROR_CODE)error->wStatus != SLAM_ERROR_CODE_NONE)
                    {
                        auto hostStatus = slamErrorToHostStatus((SLAM_ERROR_CODE)error->wStatus);
                        std::shared_ptr<CompleteTask> ptr = std::make_shared<StatusEventFrameCompleteTask>(mListener, hostStatus, this);
                        mTaskHandler->addTask(ptr);
                    }

                    break;
                }

                case SLAM_RELOCALIZATION_EVENT:
                {
                    interrupt_message_slam_relocalization_event* msg = (interrupt_message_slam_relocalization_event*)header;
                    DEVICELOGD("Got SLAM relocalization, timestamp %" PRIu64 ", session %" PRIu16, msg->llNanoseconds, msg->wSessionId);

                    std::shared_ptr<CompleteTask> ptr = std::make_shared<RelocalizationEventFrameCompleteTask>(mListener, msg, this);
                    mTaskHandler->addTask(ptr);

                    break;
                }

                case CONTROLLER_DEVICE_DISCOVERY_EVENT:
                {
                    interrupt_message_controller_device_discovery* controllerDeviceDiscovery = (interrupt_message_controller_device_discovery*)header;
                    std::shared_ptr<CompleteTask> ptr = std::make_shared<ControllerDiscoveryEventFrameCompleteTask>(mListener, controllerDeviceDiscovery, this);

                    DEVICELOGD("Controller discovered: Mac Address [%02X:%02X:%02X:%02X:%02X:%02X], AddressType [0x%X], Manufacturer ID [0x%X], Vendor Data [0x%X], App [%u.%u.%u], Boot Loader [%u.%u.%u], Soft Device [%u], Protocol [%u]", 
                        controllerDeviceDiscovery->bMacAddress[0], controllerDeviceDiscovery->bMacAddress[1], controllerDeviceDiscovery->bMacAddress[2], controllerDeviceDiscovery->bMacAddress[3], controllerDeviceDiscovery->bMacAddress[4], controllerDeviceDiscovery->bMacAddress[5],
                        controllerDeviceDiscovery->bAddressType,
                        controllerDeviceDiscovery->info.wManufacturerId,
                        controllerDeviceDiscovery->info.bVendorData,
                        controllerDeviceDiscovery->info.bAppVersionMajor, controllerDeviceDiscovery->info.bAppVersionMinor, controllerDeviceDiscovery->info.bAppVersionPatch, 
                        controllerDeviceDiscovery->info.bBootloaderVersionMajor, controllerDeviceDiscovery->info.bBootloaderVersionMinor, controllerDeviceDiscovery->info.bBootloaderVersionPatch, 
                        controllerDeviceDiscovery->info.bSoftdeviceVersion, 
                        controllerDeviceDiscovery->info.bProtocolVersion);

                    mTaskHandler->addTask(ptr);
                    break;
                }

                case CONTROLLER_CALIBRATION_STATUS_EVENT:
                {
                    interrupt_message_controller_calibration_status* calibrationStatus = (interrupt_message_controller_calibration_status*)header;
                    DEVICELOGD("Got Controller[%d] calibration status %s (0x%X) on interrupt endpoint", calibrationStatus->bControllerID, controllerCalibrationStatusCodeToString((CONTROLLER_CALIBRATION_STATUS_CODE)calibrationStatus->wStatus).c_str(), calibrationStatus->wStatus);

                    auto hostStatus = controllerCalibrationToHostStatus((CONTROLLER_CALIBRATION_STATUS_CODE)calibrationStatus->wStatus);
                    std::shared_ptr<CompleteTask> ptr = std::make_shared<ControllerCalibrationEventFrameCompleteTask>(mListener, calibrationStatus->bControllerID, hostStatus, this);
                    mTaskHandler->addTask(ptr);                    
                    break;
                }

                case CONTROLLER_DEVICE_CONNECTED_EVENT:
                {
                    interrupt_message_controller_connected* controllerDeviceConnect = (interrupt_message_controller_connected*)header;
                    std::shared_ptr<CompleteTask> ptr = std::make_shared<ControllerConnectEventFrameCompleteTask>(mListener, controllerDeviceConnect, this);

                    std::string status = "Error";
                    switch ((MESSAGE_STATUS)controllerDeviceConnect->wStatus)
                    {
                        case MESSAGE_STATUS::SUCCESS:
                            status = "Success";
                            break;
                        case MESSAGE_STATUS::TIMEOUT:
                            status = "Timeout";
                            break;
                        case MESSAGE_STATUS::INCOMPATIBLE:
                            status = "Incompatible";
                            break;
                    }

                    DEVICELOGD("---------------------------------------");
                    DEVICELOGD("Controller %d connection Info", controllerDeviceConnect->bControllerID);
                    DEVICELOGD("--------------------+------------------");
                    DEVICELOGD("Status              | %s (0x%X)", status.c_str(), controllerDeviceConnect->wStatus);
                    DEVICELOGD("Controller ID       | %d", controllerDeviceConnect->bControllerID);
                    DEVICELOGD("Manufacturer ID     | 0x%X", controllerDeviceConnect->info.wManufacturerId);
                    DEVICELOGD("App Version         | %u.%u.%u", controllerDeviceConnect->info.bAppVersionMajor, controllerDeviceConnect->info.bAppVersionMinor, controllerDeviceConnect->info.bAppVersionPatch);
                    DEVICELOGD("Boot Loader Version | %u.%u.%u", controllerDeviceConnect->info.bBootloaderVersionMajor, controllerDeviceConnect->info.bBootloaderVersionMinor, controllerDeviceConnect->info.bBootloaderVersionPatch);
                    DEVICELOGD("Soft Device Version | %u", controllerDeviceConnect->info.bSoftdeviceVersion);
                    DEVICELOGD("Protocol Version    | %u", controllerDeviceConnect->info.bProtocolVersion);
                    DEVICELOGD("--------------------+------------------");

                    mTaskHandler->addTask(ptr);
                    break;
                }

                case CONTROLLER_DEVICE_DISCONNECTED_EVENT:
                {
                    interrupt_message_controller_disconnected* controllerDeviceDisconnect = (interrupt_message_controller_disconnected*)header;
                    std::shared_ptr<CompleteTask> ptr = std::make_shared<ControllerDisconnectEventFrameCompleteTask>(mListener, controllerDeviceDisconnect, this);
                    mTaskHandler->addTask(ptr);
                    break;
                }

                case CONTROLLER_DEVICE_LED_INTENSITY_EVENT:
                {
                    interrupt_message_controller_led_intensity *ledMessage = (interrupt_message_controller_led_intensity*)msgBuffer;
                    std::shared_ptr<CompleteTask> ptr = std::make_shared<ControllerLedFrameCompleteTask>(mListener, ledMessage, mTM2CorrelatedTimeStampShift, this);
                    mTaskHandler->addTask(ptr);
                    break;
                }

                case SLAM_SET_LOCALIZATION_DATA_STREAM:
                {
                    interrupt_message_set_localization_data_stream* setLocalizationDataStream = (interrupt_message_set_localization_data_stream*)header;
                    std::shared_ptr<CompleteTask> ptr = std::make_shared<LocalizationDataEventFrameCompleteTask>(mListener, setLocalizationDataStream, this);

                    DEVICELOGV("Got Set Localization Data frame complete: status = 0x%X", setLocalizationDataStream->wStatus);

                    MessageON_ASYNC_STOP setMsg;
                    mDispatcher->postMessage(&mFsm, setMsg);

                    mTaskHandler->addTask(ptr);
                    break;
                }

                case DEV_FIRMWARE_UPDATE:
                {
                    interrupt_message_fw_update_stream* fwUpdateStream = (interrupt_message_fw_update_stream*)header;
                    std::shared_ptr<CompleteTask> ptr = std::make_shared<FWUpdateCompleteTask>(mListener, fwUpdateStream, this);

                    uint32_t isCentralFWUpdate = 0;
                    for (int i = 0; i < MAC_ADDRESS_SIZE; i++)
                    {
                        isCentralFWUpdate += fwUpdateStream->bMacAddress[i];
                    }
                    if (isCentralFWUpdate == 0)
                    {
                        DEVICELOGD("Got Central FW update frame: status = 0x%X, progress = %u%%", fwUpdateStream->wStatus, fwUpdateStream->bProgress);
                        if (fwUpdateStream->wStatus != (uint16_t)MESSAGE_STATUS::SUCCESS)
                        {
                            DEVICELOGE("Failed on BLE FW update with status = 0x%X", fwUpdateStream->wStatus);
                            mDispatcher->postMessage(&mFsm, Message(ON_ERROR));
                        }
                    }
                    else
                    {
                        DEVICELOGD("Got Controller FW update frame: MAC address [%02X:%02X:%02X:%02X:%02X:%02X], status = 0x%X, progress = %u%%",
                            fwUpdateStream->bMacAddress[0], fwUpdateStream->bMacAddress[1], fwUpdateStream->bMacAddress[2], fwUpdateStream->bMacAddress[3], fwUpdateStream->bMacAddress[4], fwUpdateStream->bMacAddress[5],
                            fwUpdateStream->wStatus, fwUpdateStream->bProgress);
                    }

                    if ((fwUpdateStream->wStatus != (uint16_t)MESSAGE_STATUS::SUCCESS ||
                        fwUpdateStream->bProgress == 100) && mFsm.getCurrentState() == ASYNC_STATE)
                    {
                        mAsyncCV.notify_one();
                    }
                    mTaskHandler->addTask(ptr);
                    break;
                }

                default:
                    DEVICELOGE("Unsupported message (0x%X) on interrupt endpoint thread function", header->wMessageID);
                    break;
            }
        }

        mInterruptEndpointThreadActive = false;
        DEVICELOGD("Thread Stop - Interrupt thread (Accelerometer/Velocimeter/Gyro/6DOF/Controller/Localization events)");
        return;
    }

    Status Device::GetDeviceInfoInternal()
    {
        bulk_message_request_get_device_info request = {0};
        bulk_message_response_get_device_info response = {0};
        std::stringstream portChainString;

        request.header.wMessageID = DEV_GET_DEVICE_INFO;
        request.header.dwLength = sizeof(request);

        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);
        mFsm.fireEvent(msg);

        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        if (memcmp(&mDeviceInfo, &response.message, sizeof(mDeviceInfo)) == 0)
        {
            /* Device info didn't change */
            return Status::SUCCESS;
        }

        mDeviceInfo = response.message;

        if (mUsbDescriptor.portChainDepth > 1)
        {
            const char* seperator = "";
            portChainString << "(PC ";

            for (uint8_t i = 0; i < mUsbDescriptor.portChainDepth; i++)
            {
                portChainString << seperator << (unsigned int)mUsbDescriptor.portChain[i];
                seperator = "-->";
            }
            portChainString << " Camera)";
        }

        auto libusbVersion = libusb_get_version();

        DEVICELOGD("-----------------------------------------------------------------------------");
        DEVICELOGD("Device Info");
        DEVICELOGD("-----------------------------------------------------------------------------");
        DEVICELOGD("USB    | Vendor ID                   | 0x%04X", mUsbDescriptor.idVendor);
        DEVICELOGD("       | Product ID                  | 0x%04X", mUsbDescriptor.idProduct);
        DEVICELOGD("       | Speed                       | %s (0x%04X)", UsbPlugListener::usbSpeed(mUsbDescriptor.bcdUSB), mUsbDescriptor.bcdUSB);
        DEVICELOGD("       | Bus Number                  | %u", mUsbDescriptor.bus);
        DEVICELOGD("       | Port Number                 | %u %s", mUsbDescriptor.port, portChainString.str().c_str());
        DEVICELOGD("HW     | Type                        | 0x%X (%s)", mDeviceInfo.bDeviceType, (mDeviceInfo.bDeviceType == 0x1) ? "TM2" : "Error");
        DEVICELOGD("       | Status                      | 0x%X (%s)", (mDeviceInfo.bStatus & 0x7), ((mDeviceInfo.bStatus & 0x7) == 0) ? "Ok" : "Error");
        DEVICELOGD("       | Status Code                 | 0x%X", mDeviceInfo.dwStatusCode);
        DEVICELOGD("       | Extended Status             | 0x%X", mDeviceInfo.dwExtendedStatus);
        DEVICELOGD("       | HW Version                  | 0x%X (%s)", mDeviceInfo.bHwVersion, hwVersion(mDeviceInfo.bHwVersion));
        DEVICELOGD("       | ROM Version                 | 0x%08X", mDeviceInfo.dwRomVersion);
        DEVICELOGD("       | EEPROM Version              | %u.%u", mDeviceInfo.bEepromDataMajor, mDeviceInfo.bEepromDataMinor);
        DEVICELOGD("       | EEPROM Lock State           | %s (0x%X)", (mDeviceInfo.bEepromLocked == EEPROM_LOCK_STATE_PERMANENT_LOCKED) ? "Permanent locked" : ((mDeviceInfo.bEepromLocked == EEPROM_LOCK_STATE_LOCKED) ? "Locked" : (mDeviceInfo.bEepromLocked == EEPROM_LOCK_STATE_WRITEABLE) ? "Writeable" : "Unknown"), mDeviceInfo.bEepromLocked);
        DEVICELOGD("       | Serial Number               | %" PRIx64, bytesSwap(mDeviceInfo.llSerialNumber) >> 16);
        DEVICELOGD("       | SKU Info                    | %u", mDeviceInfo.bSKUInfo == SKU_INFO_TYPE::SKU_INFO_TYPE_WITHOUT_BLUETOOTH ? "No Bluetooth" : "Has Bluetooth");
        DEVICELOGD("FW     | FW Version                  | %u.%u.%u.%u", mDeviceInfo.bFWVersionMajor, mDeviceInfo.bFWVersionMinor, mDeviceInfo.bFWVersionPatch, mDeviceInfo.dwFWVersionBuild);
        DEVICELOGD("       | FW Interface Version        | %u.%u", mFWInterfaceVersion.dwMajor, mFWInterfaceVersion.dwMinor);
        DEVICELOGD("       | Central App Version         | %u.%u.%u.%u", mDeviceInfo.bCentralAppVersionMajor, mDeviceInfo.bCentralAppVersionMinor, mDeviceInfo.bCentralAppVersionPatch, mDeviceInfo.dwCentralAppVersionBuild);
        DEVICELOGD("       | Central Boot Loader Version | %u.%u.%u", mDeviceInfo.bCentralBootloaderVersionMajor, mDeviceInfo.bCentralBootloaderVersionMinor, mDeviceInfo.bCentralBootloaderVersionPatch);
        DEVICELOGD("       | Central Soft Device Version | %u", mDeviceInfo.bCentralSoftdeviceVersion);
        DEVICELOGD("       | Central Protocol Version    | %u", mDeviceInfo.bCentralProtocolVersion);
        DEVICELOGD("Host   | Host Version                | %u.%u.%u.%u", LIBTM_VERSION_MAJOR, LIBTM_VERSION_MINOR, LIBTM_VERSION_PATCH, LIBTM_VERSION_BUILD);
        DEVICELOGD("       | Host Interface Version      | %u.%u", LIBTM_API_VERSION_MAJOR, LIBTM_API_VERSION_MINOR);
        DEVICELOGD("       | Status                      | 0x%X (%s)", mDeviceStatus, (mDeviceStatus == Status::SUCCESS) ? "Ok" : "Error");
        DEVICELOGD("       | Device ID                   | 0x%p", this);
        DEVICELOGD("LibUsb | LibUsb Version              | %u.%u.%u.%u %s", libusbVersion->major, libusbVersion->minor, libusbVersion->micro, libusbVersion->nano, libusbVersion->rc);
        DEVICELOGD("-----------------------------------------------------------------------------");
        DEVICELOGD(" ");

        switch (mDeviceInfo.dwStatusCode)
        {
            case FW_STATUS_CODE_OK:
                return (Status)msg.Result;
                break;
            case FW_STATUS_CODE_FAIL:
                DEVICELOGE("FW Error: Device info returned init failed");
                return Status::ERROR_FW_INTERNAL;
                break;
            case FW_STATUS_CODE_NO_CALIBRATION_DATA:
                DEVICELOGW("FW Error: ******** Device is not calibrated! ********");
                return (Status)msg.Result;
                break;
            default:
                DEVICELOGE("FW Error: Device unknown error (0x%X)", mDeviceInfo.dwStatusCode);
                return Status::ERROR_FW_INTERNAL;
                break;
        }
    }

    Status Device::DeviceFlush()
    {
        bulk_message_request_flush request = {0};
        unsigned char msgBuffer[BUFFER_SIZE] = {0};
        bulk_message_response_flush* response = (bulk_message_response_flush*)&msgBuffer;
        bool stopFlush = false;
        int actual = 0;

        request.header.wMessageID = DEV_FLUSH;
        request.header.dwLength = sizeof(request);
        request.ddwToken = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

        DEVICELOGD("Flushing Command EndPoint - Start");

        int rc = libusb_bulk_transfer(mDevice, mEndpointBulkMessages | TO_DEVICE, (uint8_t*)&request, BUFFER_SIZE, &actual, USB_TRANSFER_FAST_TIMEOUT_MS);
        if (rc != 0 || actual != BUFFER_SIZE) // lets assume no message will be more than 2Gb size
        {
            DEVICELOGE("USB Error (0x%X)",rc);
            return Status::ERROR_USB_TRANSFER;
        }

        while (stopFlush == false)
        {
            DEVICELOGD("Flushing Command EndPoint...");
            rc = libusb_bulk_transfer(mDevice, mEndpointBulkMessages | TO_HOST, (unsigned char*)response, BUFFER_SIZE, &actual, USB_TRANSFER_FAST_TIMEOUT_MS);

            if (response->header.wStatus == toUnderlying(MESSAGE_STATUS::UNKNOWN_MESSAGE_ID))
            {
                DEVICELOGE("Command %s is not supported by FW", messageCodeToString(LIBUSB_TRANSFER_TYPE_BULK, response->header.wMessageID).c_str());
                //return Status::NOT_SUPPORTED_BY_FW;
                stopFlush = true;
            }

            switch (response->header.wMessageID)
            {
                case DEV_FLUSH:
                {
                    if (response->ddwToken == request.ddwToken)
                    {
                        stopFlush = true;
                    }
                    break;
                }
            }
        }

        DEVICELOGD("Flushing Command EndPoint - Finish");

        DEVICELOGD("Flushing Stream EndPoint - Start");
        stopFlush = false;

        while (stopFlush == false)
        {
            DEVICELOGD("Flushing Stream EndPoint...");
            auto rc = libusb_bulk_transfer(mDevice, mStreamEndpoint | TO_HOST, (unsigned char *)response, BUFFER_SIZE, &actual, USB_TRANSFER_FAST_TIMEOUT_MS);
            if (rc == LIBUSB_ERROR_TIMEOUT)
                continue;

            if (rc != 0 || actual == 0)
            {
                DEVICELOGE("Error while flushing stream endpoint (0x%X)", rc);
                mDispatcher->postMessage(&mFsm, Message(ON_ERROR));
                break;
            }

            if ((response->header.wMessageID == DEV_FLUSH) && (response->ddwToken == request.ddwToken))
            {
                stopFlush = true;
            }
        }

        DEVICELOGD("Flushing Stream EndPoint - Finish");

        DEVICELOGD("Flushing Event EndPoint - Start");
        stopFlush = false;

        while (stopFlush == false)
        {
            DEVICELOGD("Flushing Event EndPoint...");
            auto rc  = libusb_interrupt_transfer(mDevice, mEndpointInterrupt, (unsigned char *)response, BUFFER_SIZE, &actual, USB_TRANSFER_FAST_TIMEOUT_MS);
            if (rc == LIBUSB_ERROR_TIMEOUT)
            {
                continue;
            }

            if (rc != 0 || actual == 0)
            {
                DEVICELOGE("Error while flushing event endpoint (0x%X)", rc);
                mDispatcher->postMessage(&mFsm, Message(ON_ERROR));
                break;
            }

            if ((response->header.wMessageID == DEV_FLUSH) && (response->ddwToken == request.ddwToken))
            {
                stopFlush = true;
            }
        }
        DEVICELOGD("Flushing Event EndPoint - Finish");

        return Status::SUCCESS;
    }

    Status Device::SetDeviceStreamConfig(uint32_t maxSize)
    {
        bulk_message_request_stream_config request = {0};
        bulk_message_response_stream_config response = {0};

        request.header.wMessageID = DEV_STREAM_CONFIG;
        request.header.dwLength = sizeof(request);
        request.dwMaxSize = maxSize;

        DEVICELOGD("Set device stream config - MaxStreamEndpointSize = %d", request.dwMaxSize);

        Bulk_Message msg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST, USB_TRANSFER_FAST_TIMEOUT_MS);
        mFsm.fireEvent(msg);

        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("USB Error (0x%X)", msg.Result);
            return Status::ERROR_USB_TRANSFER;
        }

        return fwToHostStatus((MESSAGE_STATUS)response.header.wStatus);
    }

    Status Device::GetUsbConnectionDescriptor()
    {
        libusb_device_descriptor desc = { 0 };
        auto rc = libusb_get_device_descriptor(mLibusbDevice, &desc);
        if (rc != 0)
        {
            DEVICELOGE("Error: Failed to get device descriptor. LIBUSB_ERROR_CODE: 0x%X (%s)", rc, libusb_error_name(rc));
            return Status::COMMON_ERROR;
        }

        mUsbDescriptor.idProduct = desc.idProduct;
        mUsbDescriptor.idVendor = desc.idVendor;
        mUsbDescriptor.bcdUSB = desc.bcdUSB;
        mUsbDescriptor.bus = libusb_get_bus_number(mLibusbDevice);
        mUsbDescriptor.port = libusb_get_port_number(mLibusbDevice);
        mUsbDescriptor.portChainDepth = libusb_get_port_numbers(mLibusbDevice, &mUsbDescriptor.portChain[0], 64);

        return Status::SUCCESS;
    }

    Status Device::GetInterfaceVersionInternal()
    {
        control_message_request_get_interface_version request = {0};
        control_message_response_get_interface_version response = {0};

        request.header.bmRequestType = (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE);
        request.header.bRequest = CONTROL_GET_INTERFACE_VERSION;

        Control_Message msg((uint8_t*)&request, sizeof(request), (uint8_t*)&response, sizeof(response));

        /* Calling onControlMessage */
        mFsm.fireEvent(msg);
        if (msg.Result != toUnderlying(Status::SUCCESS))
        {
            DEVICELOGE("Error Transferring CONTROL_GET_INTERFACE_VERSION");
            return Status::ERROR_USB_TRANSFER;
        }

        mFWInterfaceVersion = response.message;
        DEVICELOGV("Interface Version =  %d.%d", mFWInterfaceVersion.dwMajor, mFWInterfaceVersion.dwMinor);

        return Status::SUCCESS;
    }

    Status Device::GetDeviceInfo(TrackingData::DeviceInfo& info)
    {
        Status status = Status::SUCCESS;
        status = GetDeviceInfoInternal();
        if (status != Status::SUCCESS)
        {
            DEVICELOGE("Error: Get Device Info error (0x%X)", status);
            return status;
        }

        if (mDeviceInfo.bDeviceType > 1)
        {
            DEVICELOGE("Error reading device info");
            return Status::COMMON_ERROR;
        }

        info.usbDescriptor = mUsbDescriptor;
        info.version.deviceInterface.set(mFWInterfaceVersion.dwMajor, mFWInterfaceVersion.dwMinor);
        info.version.eeprom.set(mDeviceInfo.bEepromDataMajor, mDeviceInfo.bEepromDataMinor);
        info.version.host.set(LIBTM_VERSION_MAJOR, LIBTM_VERSION_MINOR, LIBTM_VERSION_PATCH, LIBTM_VERSION_BUILD);
        info.version.fw.set(mDeviceInfo.bFWVersionMajor, mDeviceInfo.bFWVersionMinor, mDeviceInfo.bFWVersionPatch, mDeviceInfo.dwFWVersionBuild);
        info.version.rom.set(mDeviceInfo.dwRomVersion);
        info.version.centralApp.set(mDeviceInfo.bCentralAppVersionMajor, mDeviceInfo.bCentralAppVersionMinor, mDeviceInfo.bCentralAppVersionPatch, mDeviceInfo.dwCentralAppVersionBuild);
        info.version.centralBootLoader.set(mDeviceInfo.bCentralBootloaderVersionMajor, mDeviceInfo.bCentralBootloaderVersionMinor, mDeviceInfo.bCentralBootloaderVersionPatch);
        info.version.centralSoftDevice.set(mDeviceInfo.bCentralSoftdeviceVersion);
        info.version.centralProtocol.set(mDeviceInfo.bCentralProtocolVersion);
        info.version.hw.set(mDeviceInfo.bHwVersion);
        info.serialNumber = bytesSwap(mDeviceInfo.llSerialNumber);
        info.deviceType = mDeviceInfo.bDeviceType;
        info.skuInfo = mDeviceInfo.bSKUInfo;
        info.status.host = mDeviceStatus;

        switch (mDeviceInfo.dwStatusCode)
        {
            case FW_STATUS_CODE_OK:
                info.status.hw = Status::SUCCESS;
                break;
            case FW_STATUS_CODE_FAIL:
                info.status.hw = Status::INIT_FAILED;
                break;
            case FW_STATUS_CODE_NO_CALIBRATION_DATA:
                info.status.hw = Status::NO_CALIBRATION_DATA;
                break;
            default:
                info.status.hw = Status::COMMON_ERROR;
                break;
        }

        switch (mDeviceInfo.bEepromLocked)
        {
            case EEPROM_LOCK_STATE_WRITEABLE:
                info.eepromLockState = LockStateWriteable;
                break;
            case EEPROM_LOCK_STATE_LOCKED:
                info.eepromLockState = LockStateLocked;
                break;
            case EEPROM_LOCK_STATE_PERMANENT_LOCKED:
                info.eepromLockState = LockStatePermanentLocked;
                break;
            default:
                info.eepromLockState = LockStateMax;
                break;
        }

        info.numAccelerometerProfiles = (uint8_t)mAccelerometerProfiles.size();
        info.numGyroProfile = (uint8_t)mGyroProfiles.size();
        info.numVelocimeterProfile = (uint8_t)mVelocimeterProfiles.size();
        info.numVideoProfiles = (uint8_t)mVideoProfiles.size();

        return Status::SUCCESS;
    }

    Status Device::SetPlayback(bool on)
    {
        MessageON_PLAYBACK msg(on);

        mDispatcher->sendMessage(&mFsm, msg);
        return (Status)msg.Result;
    }

    // -[FSM]----------------------------------------------------------------------
    //
    // [FSM State::ANY]

    // [FSM State::IDLE]
    DEFINE_FSM_STATE_ENTRY(Device, IDLE_STATE)
    {
        // DEVICELOGD("Entry to IDLE_STATE");
    }

    DEFINE_FSM_ACTION(Device, IDLE_STATE, ON_INIT, msg)
    {
        msg.Result = toUnderlying(Status::SUCCESS);
    }
    DEFINE_FSM_GUARD(Device, IDLE_STATE, ON_START, msg)
    {
        /* Reset 6dof/IMU/Video device statistics from previous run to eliminate wrong drop/reorder errors */
        ResetStatistics();

        StartThreads(true, true);

        bulk_message_request_start req;
        req.header.dwLength = sizeof(bulk_message_request_start);
        req.header.wMessageID = DEV_START;
        bulk_message_response_start res = {0};

        DEVICELOGD("Set Start");
        Bulk_Message msg1((uint8_t*)&req, req.header.dwLength, (uint8_t*)&res, sizeof(res), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST, USB_TRANSFER_MEDIUM_TIMEOUT_MS);
        onBulkMessage(msg1);

        msg.Result = msg1.Result;

        if (msg1.Result == 0)
        {
            if (res.header.wStatus == 0)
            {
                return true;
            }

            msg.Result = toUnderlying(fwToHostStatus((MESSAGE_STATUS)res.header.wStatus));
        }

        DEVICELOGE("Error: Set Start Failed (0x%X)", msg.Result);

        StopThreads(true, true, true);

        return false;
    }

    DEFINE_FSM_GUARD(Device, IDLE_STATE, ON_ASYNC_START, msg)
    {
        StartThreads(true, true);
        return true;
    }

    DEFINE_FSM_ACTION(Device, IDLE_STATE, ON_START, msg)
    {
        // update listener
        MessageON_START m = dynamic_cast<const MessageON_START&>(msg);
        mListener = m.listener;
    }

    DEFINE_FSM_ACTION(Device, IDLE_STATE, ON_STOP, msg)
    {
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_GUARD(Device, ACTIVE_STATE, ON_STOP, msg)
    {
        bulk_message_request_stop req;
        req.header.dwLength = sizeof(bulk_message_request_stop);
        req.header.wMessageID = DEV_STOP;
        bulk_message_response_stop res = {0};

        DEVICELOGD("Set Stop");
        Bulk_Message msg1((uint8_t*)&req, req.header.dwLength, (uint8_t*)&res, sizeof(res), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST, USB_TRANSFER_MEDIUM_TIMEOUT_MS);
        onBulkMessage(msg1);

        msg.Result = msg1.Result;
        if (msg1.Result == 0)
        {
            if (res.header.wStatus == 0)
            {
                msg.Result = 0;
                return true;
            }
            
            msg.Result = toUnderlying(fwToHostStatus((MESSAGE_STATUS)res.header.wStatus));
            return false;
        }
        return false;

    }

    DEFINE_FSM_ACTION(Device, IDLE_STATE, ON_DETACH, msg)
    {
        msg.Result = toUnderlying(Status::COMMON_ERROR);
    }

    // [FSM State::ACTIVE]
    DEFINE_FSM_STATE_ENTRY(Device, ACTIVE_STATE)
    {
        //DEVICELOGD("Entry to ACTIVE_STATE");
    }

    DEFINE_FSM_STATE_EXIT(Device, ACTIVE_STATE)
    {
        StopThreads(true, true, true);

        if (mTaskHandler)
        {
            /* Creating onStatusEvent to indicate DEVICE_STOPPED status */
            std::shared_ptr<CompleteTask> ptr = std::make_shared<StatusEventFrameCompleteTask>(mListener, Status::DEVICE_STOPPED, this);
            mTaskHandler->addTask(ptr);

            /* Clean all events from our device completion queue */
            mTaskHandler->removeTasks(this, false);
        }

        mListener = NULL;
    }

    void Device::StartThreads(bool interruptThread, bool frameThread)
    {
        DEVICELOGV("Starting interruptThread = %s, frameThread = %s", (interruptThread ? "True" : "False"), (frameThread ? "True" : "False"));

        mStreamEndpointThreadStop = false;
        mInterruptEndpointThreadStop = false;

        if (interruptThread == true)
        {
            /* Start Interrupt thread */
            mInterruptEPThread = std::thread(&Device::interruptEndpointThread, this);

            /* Wait for interrupt thread to start */
            while (mInterruptEndpointThreadActive == false);
        }

        if (frameThread == true)
        {
            /* Start Bulk thread */
            mBulkEPThread = std::thread(&Device::streamEndpointThread, this);


            /* Wait for Bulk thread to start */
            while (mStreamEndpointThreadActive == false);
        }

        DEVICELOGV("All threads started");
    }

    void Device::StopThreads(bool force, bool interruptThread, bool frameThread)
    {
        DEVICELOGV("Stopping interruptThread = %s, frameThread = %s, force = %s", (interruptThread?"True":"False"), (frameThread ? "True" : "False"), (force ? "True" : "False"));

        if (force == true)
        {
            /* Time to wait before closing interrupt / event threads - give opportunity for the FW to send the DEVICE_STOPPED status to all endpoint threads */
            std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_FOR_STOP_STATUS_MSEC));

            mStreamEndpointThreadStop = true;
            mInterruptEndpointThreadStop = true;
        }

        if (interruptThread == true)
        {
            /* Stop Interrupt thread and wait for it to stop */
            if (mInterruptEPThread.joinable())
            {
                mInterruptEPThread.join();
            }
        }

        if (frameThread == true)
        {
            /* Stop Bulk thread and wait for it to stop */
            if (mBulkEPThread.joinable())
            {
                mBulkEPThread.join();
            }
        }

        DEVICELOGV("All threads stopped");
    }

    DEFINE_FSM_STATE_ENTRY(Device, ASYNC_STATE)
    {

    }

    DEFINE_FSM_STATE_EXIT(Device, ASYNC_STATE)
    {
        StopThreads(true, true, true);

        // clean all events from our device completion queue
        if (mTaskHandler)
        {
            mTaskHandler->removeTasks(this, true);
        }

        mListener = NULL;
    }

    DEFINE_FSM_ACTION(Device, ASYNC_STATE, ON_INIT, msg)
    {
        DEVICELOGE("State [ASYNC_STATE] got event [ON_INIT] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ASYNC_STATE, ON_DONE, msg)
    {
        DEVICELOGE("State [ASYNC_STATE] got event [ON_DONE] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ASYNC_STATE, ON_START, msg)
    {
        DEVICELOGE("State [ASYNC_STATE] got event [ON_START] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ASYNC_STATE, ON_STOP, msg)
    {
        DEVICELOGE("State [ASYNC_STATE] got event [ON_STOP] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ASYNC_STATE, ON_DETACH, msg)
    {
        DEVICELOGE("State [ASYNC_STATE] got event [ON_DETACH] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ASYNC_STATE, ON_ERROR, msg)
    {
        DEVICELOGE("State [ASYNC_STATE] got event [ON_ERROR] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ASYNC_STATE, ON_BULK_MESSAGE, msg)
    {
        onBulkMessage(msg);
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ASYNC_STATE, ON_LARGE_MESSAGE, msg)
    {
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ASYNC_STATE, ON_CONTROL_MESSAGE, msg)
    {
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ASYNC_STATE, ON_PLAYBACK_SET, msg)
    {
        DEVICELOGE("State [ASYNC_STATE] got event [ON_PLAYBACK_SET] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ASYNC_STATE, ON_SET_ENABLED_STREAMS, msg)
    {
        DEVICELOGE("State [ASYNC_STATE] got event [ON_SET_ENABLED_STREAMS] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ASYNC_STATE, ON_ASYNC_START, msg)
    {
        DEVICELOGE("State [ASYNC_STATE] got event [ON_ASYNC_START] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ASYNC_STATE, ON_ASYNC_STOP, msg)
    {
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    
    /**
    * @brief onBulkMessage - Bulk Endpoint Protocol
    *
    * The bulk IN/OUT endpoints are used for general communication from the host.
    * The protocol on the bulk endpoints is in a request/response convention,
    * i.e. a bulk OUT transfer (request) is always followed by a bulk IN transfer (response).
    */
    void Device::onBulkMessage(const Message& msg)
    {
        Bulk_Message usbMsg = dynamic_cast<const Bulk_Message&>(msg);
        int actual = 0;
        unsigned char buffer[BUFFER_SIZE] = { 0 };
        perc::copy(buffer, usbMsg.mSrc, usbMsg.srcSize);
        bulk_message_request_header* header = (bulk_message_request_header*)buffer;

        // WA for low power issue - FW may not hear this bulk message on low power, pre-sending control message to wake it
        if (header->wMessageID != DEV_GET_TIME)
        {
            WakeFW();
        }

        int rc = libusb_bulk_transfer(mDevice, usbMsg.mEndpointOut, buffer, BUFFER_SIZE, &actual, usbMsg.mTimeoutInMs);

        DEVICELOGV("Sent request - MessageID: 0x%X (%s), Len: %d, UsbLen: %d, Actual: %d, rc: %d (%s)", 
            header->wMessageID, messageCodeToString(LIBUSB_TRANSFER_TYPE_BULK, header->wMessageID).c_str(), header->dwLength, BUFFER_SIZE, actual, rc, libusb_error_name(rc));
        if (rc != 0 || actual != BUFFER_SIZE ) // lets assume no message will be more than 2Gb size
        {
            DEVICELOGE("ERROR: Bulk transfer message 0x%X (%s) request to device got %s. Bytes requested %d, bytes transferred %d",
                header->wMessageID, messageCodeToString(LIBUSB_TRANSFER_TYPE_BULK, header->wMessageID).c_str(), libusb_error_name(rc), usbMsg.srcSize, actual);
            msg.Result = toUnderlying(Status::ERROR_USB_TRANSFER);
            return;
        }

        rc = libusb_bulk_transfer(mDevice, usbMsg.mEndpointIn, usbMsg.mDst, usbMsg.dstSize, &actual, usbMsg.mTimeoutInMs);

        bulk_message_response_header* res = (bulk_message_response_header*)usbMsg.mDst;

        DEVICELOGV("Got response - MessageID: 0x%X (%s), Len: %d, Status: 0x%X, UsbLen: %d, Actual: %d, rc: %d (%s)", 
            res->wMessageID, messageCodeToString(LIBUSB_TRANSFER_TYPE_BULK, res->wMessageID).c_str(), res->dwLength, res->wStatus, usbMsg.dstSize, actual, rc, libusb_error_name(rc));

        if ((rc == 0) && (header->wMessageID != res->wMessageID))
        {
            DEVICELOGE("Command mismatch - Expected 0x%X (%s) length %d, Received  0x%X (%s) length %d",
                header->wMessageID, messageCodeToString(LIBUSB_TRANSFER_TYPE_BULK, header->wMessageID).c_str(), usbMsg.dstSize,
                res->wMessageID, messageCodeToString(LIBUSB_TRANSFER_TYPE_BULK, res->wMessageID).c_str(), res->dwLength);
        }

        if (rc != 0 || actual != usbMsg.dstSize) // lets assume no message will be more than 2Gb size
        {
            msg.Result = toUnderlying(Status::ERROR_USB_TRANSFER);
            if (actual == 0)
            {
                DEVICELOGE("ERROR: Bulk transfer message 0x%X (%s) response to host got %s. Host did not return answer", header->wMessageID, messageCodeToString(LIBUSB_TRANSFER_TYPE_BULK, header->wMessageID).c_str(),libusb_error_name(rc));
                msg.Result = toUnderlying(Status::ERROR_USB_TRANSFER);
            }
            else
            {
                if (res->wStatus == toUnderlying(MESSAGE_STATUS::UNKNOWN_MESSAGE_ID))
                {
                    DEVICELOGE("Command %s is not supported by FW", messageCodeToString(LIBUSB_TRANSFER_TYPE_BULK, res->wMessageID).c_str());
                    msg.Result = toUnderlying(Status::NOT_SUPPORTED_BY_FW);
                }
                else if(actual > usbMsg.dstSize)
                {
                    DEVICELOGD("WARNING: Bulk transfer message 0x%X (%s) response to host got %s. Bytes requested %d, bytes transferred %d", header->wMessageID, messageCodeToString(LIBUSB_TRANSFER_TYPE_BULK, header->wMessageID).c_str(), libusb_error_name(rc), usbMsg.dstSize, actual);
                    msg.Result = toUnderlying(Status::ERROR_USB_TRANSFER);
                }
                else /* actual < usbMsg.dstSize*/
                {
                    msg.Result = toUnderlying(Status::SUCCESS);
                }
            }
            
            return;
        }

        if (res->wStatus == toUnderlying(MESSAGE_STATUS::SUCCESS))
        {
            msg.Result = toUnderlying(Status::SUCCESS);
        }
        else if (res->wStatus == toUnderlying(MESSAGE_STATUS::UNSUPPORTED))
        {
            DEVICELOGE("MessageID 0x%X (%s) failed with status 0x%X", res->wMessageID, messageCodeToString(LIBUSB_TRANSFER_TYPE_BULK, header->wMessageID).c_str(), res->wStatus);
            msg.Result = toUnderlying(Status::FEATURE_UNSUPPORTED);
        }
        else
        {
            DEVICELOGE("MessageID 0x%X (%s) failed with status 0x%X", res->wMessageID, messageCodeToString(LIBUSB_TRANSFER_TYPE_BULK, header->wMessageID).c_str(), res->wStatus);
            msg.Result = toUnderlying(Status::COMMON_ERROR);
        }
    }

    void Device::onControlMessage(const Message& msg)
    {
        Control_Message usbMsg = dynamic_cast<const Control_Message&>(msg);
        control_message_request_header* header = (control_message_request_header*)usbMsg.mSrc;

        DEVICELOGV("Sending Control request - MessageID: 0x%X (%s)", header->bRequest, messageCodeToString(LIBUSB_TRANSFER_TYPE_CONTROL, header->bRequest).c_str());
        int result = libusb_control_transfer(mDevice, header->bmRequestType, header->bRequest, usbMsg.mValue, usbMsg.mIndex, usbMsg.mDst, usbMsg.dstSize, usbMsg.mTimeoutInMs);

        /* Control request Successed if result == expected dstSize or got PIPE error on reset message */
        if ((result == usbMsg.dstSize) || ((header->bRequest == CONTROL_USB_RESET) && (result == LIBUSB_ERROR_PIPE)))
        {
            msg.Result = toUnderlying(Status::SUCCESS);
            return;
        }

        DEVICELOGE("ERROR %s while control transfer of messageID: 0x%X (%s)", libusb_error_name(result), header->bRequest, messageCodeToString(LIBUSB_TRANSFER_TYPE_CONTROL, header->bRequest).c_str());
        msg.Result = toUnderlying(Status::ERROR_USB_TRANSFER);
    }

    void Device::WakeFW()
    {
        // WA for low power issue - FW may not hear this bulk message on low power, pre-sending control message to wake it
        GetInterfaceVersionInternal();
    }

    void Device::SendLargeMessage(const Message& msg)
    {
        Large_Message* pMsg = (Large_Message*)&msg;
        const uint32_t length = pMsg->mLength;
        const uint8_t* buffer = pMsg->mBuffer;
        mListener = pMsg->mListener;

        uint32_t maxChunkLength = MAX_BIG_DATA_MESSAGE_LENGTH - offsetof(bulk_message_large_stream, bPayload);
        uint32_t leftLength = length;
        uint32_t chunkLength = 0;
        uint16_t index = 0;
        int actual = 0;

        // WA for low power issue - FW may not hear this bulk message on low power, pre-sending control message to wake it
        WakeFW();

        DEVICELOGD("Set large message send - Total length %d", length);

        auto start = systemTime();

        bulk_message_large_stream* stream = (bulk_message_large_stream*)malloc(MAX_BIG_DATA_MESSAGE_LENGTH);
        if (stream == NULL)
        {
            LOGE("Error allocating %d buffer", MAX_BIG_DATA_MESSAGE_LENGTH);
            msg.Result = toUnderlying(Status::ALLOC_FAILED);
            return;
        }

        while (leftLength > 0)
        {
            if (leftLength > maxChunkLength)
            {
                chunkLength = maxChunkLength;
                stream->wStatus = (uint16_t)MESSAGE_STATUS::MORE_DATA_AVAILABLE;
            }
            else
            {
                chunkLength = leftLength;
                stream->wStatus = (uint16_t)MESSAGE_STATUS::SUCCESS;
            }

            DEVICELOGD("Set large message - Chunk %03d: [%09d - %09d], Length %d, Left %d, Status 0x%X", index, (length - leftLength), (length - leftLength + chunkLength), chunkLength, leftLength, stream->wStatus);

            stream->header.wMessageID = pMsg->mMessageId;
            stream->header.dwLength = chunkLength + offsetof(bulk_message_large_stream, bPayload);
            stream->wIndex = index++;
            perc::copy(&stream->bPayload, buffer + (length - leftLength), chunkLength);

            actual = 0;
            auto rc = libusb_bulk_transfer(mDevice, mStreamEndpoint | TO_DEVICE, (unsigned char*)stream, stream->header.dwLength, &actual, 5000);
            if (rc != 0 || actual == 0)
            {
                DEVICELOGE("Error while sending large message chunk %d (0x%X)", index, rc);
                msg.Result = toUnderlying(Status::ERROR_USB_TRANSFER);
                return;
            }
            leftLength -= chunkLength;
        }

        auto finish = systemTime();
        DEVICELOGD("Finished setting large message data - Total length %d with %d chunks in %d (msec)", length, index, ns2ms(finish - start));

        free(stream);
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, IDLE_STATE, ON_ASYNC_START, msg)
    {
        MessageON_ASYNC_START pMsg = dynamic_cast<const MessageON_ASYNC_START&>(msg);
        switch (pMsg.mMessageId)
        {
            case SLAM_GET_LOCALIZATION_DATA:
            {
                bulk_message_request_get_localization_data request = { 0 };
                bulk_message_response_get_localization_data response = { 0 };

                mListener = pMsg.mListener;
                request.header.wMessageID = pMsg.mMessageId;
                request.header.dwLength = sizeof(request);

                DEVICELOGD("Get Localization Data");

                Bulk_Message bulkMsg((uint8_t*)&request, request.header.dwLength, (uint8_t*)&response, sizeof(response), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);
                onBulkMessage(bulkMsg);

                if (bulkMsg.Result != toUnderlying(Status::SUCCESS))
                {
                    DEVICELOGE("USB Error (0x%X)", bulkMsg.Result);
                    msg.Result = toUnderlying(Status::ERROR_USB_TRANSFER);
                    return;
                }

                msg.Result = response.header.wStatus;
                break;
            }

            case SLAM_SET_LOCALIZATION_DATA_STREAM:
            {
                return SendLargeMessage(msg);
            }

            case DEV_FIRMWARE_UPDATE:
            {
                return SendLargeMessage(msg);
            }
        }

        return;
    }

    DEFINE_FSM_ACTION(Device, IDLE_STATE, ON_ASYNC_STOP, msg)
    {
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, IDLE_STATE, ON_SET_ENABLED_STREAMS, msg)
    {
        MessageON_SET_ENABLED_STREAMS pMsg = dynamic_cast<const MessageON_SET_ENABLED_STREAMS&>(msg);
        auto message = pMsg.mMessage;
        auto wNumEnabledStreams = pMsg.mNumEnabledStreams;

        for (int i = 0; i < wNumEnabledStreams; i++)
        {
            if (message[i].bPixelFormat != PixelFormat::ANY)
            {
                mFrameTempBufferSize = message[i].wHeight*message[i].wStride + sizeof(bulk_message_video_stream);

                AllocateBuffers();
                break;
            }
        }

        for (int i = 0; i < wNumEnabledStreams; i++)
        {
            if (GET_SENSOR_TYPE(message[i].bSensorID) <= SensorType::Fisheye)
            {
                mWidthsMap[message[i].bSensorID] = message[i].wWidth;
                mHeightsMap[message[i].bSensorID] = message[i].wHeight;
            }
        }

        uint8_t reqBuffer[BUFFER_SIZE];
        bulk_message_request_raw_streams_control* req = (bulk_message_request_raw_streams_control*)reqBuffer;

        req->header.wMessageID = mPlaybackIsOn ? DEV_RAW_STREAMS_PLAYBACK_CONTROL : DEV_RAW_STREAMS_CONTROL;
        req->wNumEnabledStreams = wNumEnabledStreams;
        req->header.dwLength = wNumEnabledStreams * sizeof(supported_raw_stream_libtm_message) + sizeof(req->header) + sizeof(req->wNumEnabledStreams);

        for (int i = 0; i < wNumEnabledStreams; i++)
        {
            req->stream[i] = message[i];
        }

        bulk_message_response_raw_streams_control res;

        DEVICELOGD("Set %d Supported RAW Streams %sControl", req->wNumEnabledStreams, (mPlaybackIsOn)?"Playback ":"");
        printSupportedRawStreams(message, wNumEnabledStreams);

        Bulk_Message msgUsb((uint8_t*)req, req->header.dwLength, (uint8_t*)&res, sizeof(res), mEndpointBulkMessages | TO_DEVICE, mEndpointBulkMessages | TO_HOST);
        onBulkMessage(msgUsb);

        msg.Result = msgUsb.Result;
    }

    DEFINE_FSM_ACTION(Device, IDLE_STATE, ON_BULK_MESSAGE, msg)
    {
        onBulkMessage(msg);
    }

    DEFINE_FSM_ACTION(Device, IDLE_STATE, ON_LARGE_MESSAGE, msg)
    {
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, IDLE_STATE, ON_PLAYBACK_SET, msg)
    {
        MessageON_PLAYBACK pMsg = dynamic_cast<const MessageON_PLAYBACK&>(msg);
        this->mPlaybackIsOn = pMsg.set;
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, IDLE_STATE, ON_CONTROL_MESSAGE, msg)
    {
        onControlMessage(msg);
    }

    DEFINE_FSM_ACTION(Device, IDLE_STATE, ON_ERROR, msg)
    {
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ERROR_STATE, ON_STOP, msg)
    {
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ERROR_STATE, ON_CONTROL_MESSAGE, msg)
    {
        onControlMessage(msg);
    }

    DEFINE_FSM_ACTION(Device, ERROR_STATE, ON_BULK_MESSAGE, msg)
    {
        //DEVICELOGE("State [ERROR_STATE] got event [ON_BULK_MESSAGE] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ERROR_STATE, ON_LARGE_MESSAGE, msg)
    {
        //DEVICELOGE("State [ERROR_STATE] got event [ON_LARGE_MESSAGE] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_STATE_ENTRY(Device, ERROR_STATE)
    {
        DEVICELOGE("Entered state [ERROR_STATE]");
        mSyncTimeEnabled = false;
    }

    DEFINE_FSM_ACTION(Device, ACTIVE_STATE, ON_BULK_MESSAGE, msg)
    {
        onBulkMessage(msg);
    }

    DEFINE_FSM_ACTION(Device, ACTIVE_STATE, ON_LARGE_MESSAGE, msg)
    {
        SendLargeMessage(msg);
    }

    DEFINE_FSM_ACTION(Device, ACTIVE_STATE, ON_CONTROL_MESSAGE, msg)
    {
        onControlMessage(msg);
    }

    DEFINE_FSM_ACTION(Device, ACTIVE_STATE, ON_STOP, msg)
    {

    }

    DEFINE_FSM_ACTION(Device, ACTIVE_STATE, ON_DETACH, msg)
    {
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ACTIVE_STATE, ON_ERROR, msg)
    {
        DEVICELOGE("State [ACTIVE_STATE] got event [ON_ERROR] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ACTIVE_STATE, ON_INIT, msg)
    {
        DEVICELOGE("State [ACTIVE_STATE] got event [ON_INIT] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ACTIVE_STATE, ON_DONE, msg)
    {
        DEVICELOGE("State [ACTIVE_STATE] got event [ON_DONE] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ACTIVE_STATE, ON_START, msg)
    {
        DEVICELOGE("State [ACTIVE_STATE] got event [ON_START] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ACTIVE_STATE, ON_PLAYBACK_SET, msg)
    {
        DEVICELOGE("State [ACTIVE_STATE] got event [ON_PLAYBACK_SET] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ACTIVE_STATE, ON_ASYNC_START, msg)
    {
        DEVICELOGE("State [ACTIVE_STATE] got event [ON_ASYNC_START] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    DEFINE_FSM_ACTION(Device, ACTIVE_STATE, ON_ASYNC_STOP, msg)
    {
        DEVICELOGE("State [ACTIVE_STATE] got event [ON_ASYNC_STOP] ==> [ERROR_STATE]");
        msg.Result = toUnderlying(Status::SUCCESS);
    }

    // [FSM State::ERROR]
    /*DEFINE_FSM_ACTION(Device, ERROR_STATE, ON_DONE, msg)
    {
    }*/

    // -[Definition]---------------------------------------------------------------
    #define FSM_ERROR_TIMEOUT               1000

    // [FSM State::IDLE]
    DEFINE_FSM_STATE_BEGIN(Device, IDLE_STATE)
    TRANSITION_INTERNAL(ON_INIT, 0, ACTION(Device, IDLE_STATE, ON_INIT))
    TRANSITION(ON_DETACH, 0, ACTION(Device, IDLE_STATE, ON_DETACH), ERROR_STATE)
    TRANSITION(ON_START, GUARD(Device, IDLE_STATE, ON_START), ACTION(Device, IDLE_STATE, ON_START), ACTIVE_STATE)
    TRANSITION(ON_STOP, 0, ACTION(Device, IDLE_STATE, ON_STOP), IDLE_STATE)
    TRANSITION(ON_ERROR, 0, ACTION(Device, IDLE_STATE, ON_ERROR), ERROR_STATE)
    TRANSITION(ON_ASYNC_STOP, 0, ACTION(Device, IDLE_STATE, ON_ERROR), ERROR_STATE)
    TRANSITION(ON_ASYNC_START, GUARD(Device, IDLE_STATE, ON_ASYNC_START), ACTION(Device, IDLE_STATE, ON_ASYNC_START), ASYNC_STATE)
    TRANSITION_INTERNAL(ON_BULK_MESSAGE, 0, ACTION(Device, IDLE_STATE, ON_BULK_MESSAGE))
    TRANSITION_INTERNAL(ON_LARGE_MESSAGE, 0, ACTION(Device, IDLE_STATE, ON_LARGE_MESSAGE))
    TRANSITION_INTERNAL(ON_CONTROL_MESSAGE, 0, ACTION(Device, IDLE_STATE, ON_CONTROL_MESSAGE))
    TRANSITION_INTERNAL(ON_PLAYBACK_SET, 0, ACTION(Device, IDLE_STATE, ON_PLAYBACK_SET))
    TRANSITION_INTERNAL(ON_SET_ENABLED_STREAMS, 0, ACTION(Device, IDLE_STATE, ON_SET_ENABLED_STREAMS))
    DEFINE_FSM_STATE_END(Device, IDLE_STATE, ENTRY(Device, IDLE_STATE), 0)

    // [FSM State::ASYNC]
    DEFINE_FSM_STATE_BEGIN(Device, ASYNC_STATE)
    TRANSITION(ON_INIT, 0, ACTION(Device, ASYNC_STATE, ON_INIT), ERROR_STATE)
    TRANSITION(ON_DONE, 0, ACTION(Device, ASYNC_STATE, ON_DONE), ERROR_STATE)
    TRANSITION(ON_START, 0, ACTION(Device, ASYNC_STATE, ON_START), ERROR_STATE)
    TRANSITION(ON_STOP, 0, ACTION(Device, ASYNC_STATE, ON_STOP), ERROR_STATE)
    TRANSITION(ON_DETACH, 0, ACTION(Device, ASYNC_STATE, ON_DETACH), ERROR_STATE)
    TRANSITION(ON_ERROR, 0, ACTION(Device, ASYNC_STATE, ON_ERROR), ERROR_STATE)
    TRANSITION(ON_PLAYBACK_SET, 0, ACTION(Device, ASYNC_STATE, ON_PLAYBACK_SET), ERROR_STATE)
    TRANSITION(ON_ASYNC_START, 0, ACTION(Device, ASYNC_STATE, ON_ASYNC_START), ERROR_STATE)
    TRANSITION(ON_ASYNC_STOP, 0, ACTION(Device, ASYNC_STATE, ON_ASYNC_STOP), IDLE_STATE)
    TRANSITION_INTERNAL(ON_BULK_MESSAGE, 0, ACTION(Device, ASYNC_STATE, ON_BULK_MESSAGE))
    TRANSITION_INTERNAL(ON_LARGE_MESSAGE, 0, ACTION(Device, ASYNC_STATE, ON_LARGE_MESSAGE))
    TRANSITION_INTERNAL(ON_CONTROL_MESSAGE, 0, ACTION(Device, ASYNC_STATE, ON_CONTROL_MESSAGE))
    TRANSITION_AFTER(0, ACTION(Device, ASYNC_STATE, ON_ERROR), ERROR_STATE, 60000) /* Protect ASYNC state - can't be in this state more than 60 sec, usually means the async operation never completed */
    DEFINE_FSM_STATE_END(Device, ASYNC_STATE, ENTRY(Device, ASYNC_STATE), EXIT(Device, ASYNC_STATE))

    // [FSM State::ACTIVE]
    DEFINE_FSM_STATE_BEGIN(Device, ACTIVE_STATE)
    TRANSITION(ON_STOP, GUARD(Device, ACTIVE_STATE, ON_STOP), ACTION(Device, ACTIVE_STATE, ON_STOP), IDLE_STATE)
    TRANSITION(ON_STOP, 0, ACTION(Device, ACTIVE_STATE, ON_STOP), ERROR_STATE)
    TRANSITION(ON_DETACH, 0, ACTION(Device, ACTIVE_STATE, ON_DETACH), ERROR_STATE)
    TRANSITION(ON_ERROR, 0, ACTION(Device, ACTIVE_STATE, ON_ERROR), ERROR_STATE)
    TRANSITION(ON_INIT, 0, ACTION(Device, ACTIVE_STATE, ON_INIT), ERROR_STATE)
    TRANSITION(ON_DONE, 0, ACTION(Device, ACTIVE_STATE, ON_DONE), ERROR_STATE)
    TRANSITION(ON_START, 0, ACTION(Device, ACTIVE_STATE, ON_START), ERROR_STATE)
    TRANSITION(ON_PLAYBACK_SET, 0, ACTION(Device, ACTIVE_STATE, ON_PLAYBACK_SET), ERROR_STATE)
    TRANSITION(ON_ASYNC_START, 0, ACTION(Device, ACTIVE_STATE, ON_ASYNC_START), ERROR_STATE)
    TRANSITION(ON_ASYNC_STOP, 0, ACTION(Device, ACTIVE_STATE, ON_ASYNC_STOP), IDLE_STATE)
    TRANSITION_INTERNAL(ON_BULK_MESSAGE, 0, ACTION(Device, ACTIVE_STATE, ON_BULK_MESSAGE))
    TRANSITION_INTERNAL(ON_LARGE_MESSAGE, 0, ACTION(Device, ACTIVE_STATE, ON_LARGE_MESSAGE))
    TRANSITION_INTERNAL(ON_CONTROL_MESSAGE, 0, ACTION(Device, ACTIVE_STATE, ON_CONTROL_MESSAGE))
    DEFINE_FSM_STATE_END(Device, ACTIVE_STATE, ENTRY(Device, ACTIVE_STATE), EXIT(Device, ACTIVE_STATE))

    // [FSM State::ERROR]
    DEFINE_FSM_STATE_BEGIN(Device, ERROR_STATE)
    //TRANSITION(ON_DONE, 0, ACTION(ERROR_STATE, ON_DONE), IDLE_STATE)
    TRANSITION_INTERNAL(ON_STOP, 0, ACTION(Device, ERROR_STATE, ON_STOP))
    TRANSITION_INTERNAL(ON_BULK_MESSAGE, 0, ACTION(Device, ERROR_STATE, ON_BULK_MESSAGE))
    TRANSITION_INTERNAL(ON_LARGE_MESSAGE, 0, ACTION(Device, ERROR_STATE, ON_LARGE_MESSAGE))
    TRANSITION_INTERNAL(ON_CONTROL_MESSAGE, 0, ACTION(Device, ERROR_STATE, ON_CONTROL_MESSAGE))
    DEFINE_FSM_STATE_END(Device, ERROR_STATE, ENTRY(Device, ERROR_STATE), 0)

    // -[FSM: Definition]----------------------------------------------------------
    DEFINE_FSM_BEGIN(Device, main)
    STATE(Device, IDLE_STATE)
    STATE(Device, ASYNC_STATE)
    STATE(Device, ACTIVE_STATE)
    STATE(Device, ERROR_STATE)
    DEFINE_FSM_END()

        // ----------------------------------------------------------------------------
} // namespace
