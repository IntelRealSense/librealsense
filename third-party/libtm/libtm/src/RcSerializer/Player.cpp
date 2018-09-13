/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/
#define LOG_TAG "Player"
#include <chrono>
#include "Player.h"
#include "rc_packet.h"
#include "Utils.h"

using namespace perc;
const int LEFT_FISHEYE_INDEX = 0;
const int RIGHT_FISHEYE_INDEX = 1;

TrackingPlayer* TrackingPlayer::CreateInstance(TrackingDevice::Listener* listener, const char* file)
{
    if (listener == nullptr || file == nullptr)
    {
        LOGE("TrackingRecorder::CreateInstance : Invalid parameters");
        return nullptr;
    }
    try
    {
        return new PlayerImpl(listener, file);
    }
    catch (std::exception& e)
    {
        std::string error = std::string("Failed to create Player : ") + e.what();
        LOGE(error.c_str());
        return nullptr;
    }
}

PlayerImpl::PlayerImpl(TrackingDevice::Listener* listener, const char* file) : 
    mListener(listener), mThreadIsAlive(true), mIsStreaming(false), mfilePath(file), mDevice(nullptr), mDeviceListener(nullptr), mProfile(nullptr)
{
    LOGD("By default the device will start streaming after reading the calibration packet");
}

bool PlayerImpl::device_configure(TrackingDevice* device, TrackingDevice::Listener* listener, TrackingData::Profile* profile)
{
    if (!device || !listener)
    {
        LOGE("One of the parameters is invalid");
        return false;
    }
    mDeviceListener = listener;
    mDevice = device;
    mProfile = profile;
    return true;
}

inline int64_t us2ns(uint64_t ts)
{
    std::chrono::duration<uint64_t, std::micro> ns(ts);
    return std::chrono::duration_cast<std::chrono::duration<int64_t, std::nano>>(ns).count();
}

void PlayerImpl::sendStereoFrame(std::shared_ptr<packet_t> packet, float gain, uint64_t exposureTime, uint64_t arrivalTime, uint32_t index)
{
    packet_stereo_raw_t *_packet = (packet_stereo_raw_t *)packet.get();
    TrackingData::VideoFrame frame;
    frame.profile.height = _packet->height;
    frame.profile.width = _packet->width;
    frame.exposuretime = static_cast<uint32_t>(exposureTime);
    frame.profile.pixelFormat = PixelFormat::Y8;
    frame.timestamp = us2ns(_packet->header.time);
    frame.arrivalTimeStamp = us2ns(arrivalTime);
    frame.systemTimestamp = 0;
    frame.frameLength = frame.profile.height * frame.profile.width;
    frame.gain = gain;
    frame.frameId = 0;
    if (index == LEFT_FISHEYE_INDEX)
    {
        frame.sensorIndex = (_packet->header.sensor_id) * 2;
        frame.profile.stride = _packet->stride1;
        frame.data = reinterpret_cast<const uint8_t*>(_packet->data);
    }
    else if (index == RIGHT_FISHEYE_INDEX)
    {
        frame.sensorIndex = (_packet->header.sensor_id) * 2 + 1;
        frame.profile.stride = _packet->stride2;
        frame.data = reinterpret_cast<const uint8_t*>(_packet->data + (_packet->stride1 * _packet->height));
    }
    else
    {
        LOGE("Invalid stereo packet");
    }
    mListener->onVideoFrame(frame);
}

void PlayerImpl::sendImageFrame(std::shared_ptr<packet_t> packet, float gain, uint64_t exposureTime, uint64_t arrivalTime)
{
    packet_image_raw_t *_packet = (packet_image_raw_t *)packet.get();
    TrackingData::VideoFrame frame;
    frame.profile.height = _packet->height;
    frame.profile.width = _packet->width;
    frame.exposuretime = static_cast<uint32_t>(exposureTime);
    frame.profile.pixelFormat = PixelFormat::Y8;
    frame.timestamp = us2ns(_packet->header.time);
    frame.arrivalTimeStamp = us2ns(arrivalTime);
    frame.systemTimestamp = 0;
    frame.frameLength = frame.profile.height * frame.profile.width;
    frame.gain = gain;
    frame.frameId = 0;
    frame.sensorIndex = static_cast<uint8_t>(_packet->header.sensor_id);
    frame.profile.stride = _packet->stride;
    frame.data = reinterpret_cast<const uint8_t*>(_packet->data);
    mListener->onVideoFrame(frame);
}

void PlayerImpl::sendGyroFrame(std::shared_ptr<packet_t> packet, uint64_t arrivalTime, float temperature)
{
    packet_gyroscope_t *_packet = (packet_gyroscope_t *)packet.get();
    TrackingData::GyroFrame frame;
    frame.sensorIndex = static_cast<uint8_t>(_packet->header.sensor_id);
    frame.angularVelocity.set(_packet->w[0], _packet->w[1], _packet->w[2]);
    frame.timestamp = us2ns(_packet->header.time);
    frame.temperature = temperature;
    frame.arrivalTimeStamp = us2ns(arrivalTime);
    frame.systemTimestamp = 0;
    frame.frameId = 0;
    mListener->onGyroFrame(frame);
}

void PlayerImpl::sendAccelFrame(std::shared_ptr<packet_t> packet, uint64_t arrivalTime, float temperature)
{
    packet_accelerometer_t *_packet = (packet_accelerometer_t *)packet.get();
    TrackingData::AccelerometerFrame frame;
    frame.sensorIndex = static_cast<uint8_t>(_packet->header.sensor_id);
    frame.acceleration.set(_packet->a[0], _packet->a[1], _packet->a[2]);
    frame.timestamp = us2ns(_packet->header.time);
    frame.temperature = temperature;
    frame.arrivalTimeStamp = us2ns(arrivalTime);
    frame.systemTimestamp = 0;
    frame.frameId = 0;
    mListener->onAccelerometerFrame(frame);
}

bool PlayerImpl::startDevice()
{
    if (!mDevice) return false;

    mProfile->playbackEnabled = true;
    auto status = mDevice->Start(mDeviceListener, mProfile);
    if (status != Status::SUCCESS)
    {
        LOGE("Failed to start device : (0x%X)", status);
        return false;
    }
    return true;
}

void PlayerImpl::start(bool start_after_calibration)
{
    LOGD("Set playing of file %s", mfilePath.c_str());
    mReadingThread = std::thread([&]() -> void {

        if (!start_after_calibration && mDevice)
        {
            if (!startDevice()) return;
        }
        mIsStreaming = true;

        std::ifstream ifs(mfilePath.c_str(), std::fstream::in | std::fstream::binary);
        if (!setProcessPriorityToRealtime())
        {
            throw std::runtime_error("Failed to set process priority");
        }
        packet_header_t header;
        static const uint64_t INITIAL_ARRIVAL_TIME = 0;
        static const uint64_t INITIAL_EXPOSURE_TIME = 0;
        static const float INITIAL_TEMPERATURE = -100;
        static const float INITIAL_GAIN = -100;

        uint64_t arrivalTime = INITIAL_ARRIVAL_TIME, exposureTime = INITIAL_EXPOSURE_TIME;
        float temperature = INITIAL_TEMPERATURE, gain = INITIAL_GAIN;
        while (ifs.read((char*)&header, sizeof(packet_header_t)) && mThreadIsAlive)
        {
            if (header.bytes == 0)
            {
                LOGE("Failed to read a packet - the header is empty");
                mIsStreaming = false;
                return;
            }

            std::shared_ptr<packet_t> packet = std::shared_ptr<packet_t>((packet_t *) ::operator new (header.bytes));
            packet->header = header;
            if (packet->header.bytes - sizeof(packet_header_t) > 0)
            {
                if (!ifs.read((char*)&packet->data, packet->header.bytes - sizeof(packet_header_t)))
                {
                    LOGE("Failed to read the packet");
                    mIsStreaming = false;
                    return;
                }
            }

            switch (packet->header.type)
            {
            case packet_controller_physical_info:
            {
                if (!mDevice) break;
                size_t size = packet->header.bytes - sizeof(packet_controller_physical_info_t);
                writePhysicalInfo(packet->header.sensor_id, packet->data, size);
                break;
            }
            case packet_calibration_bin:
            {
                if (!mDevice) break;
                size_t size = packet->header.bytes - sizeof(packet_calibration_bin_t);
                writeCalibration(packet->data, size);
                if (start_after_calibration)
                {
                    if (!startDevice())
                    {
                        mIsStreaming = false;
                        return;
                    }
                }
                break;
            }
            case packet_arrival_time:
            {
                if (arrivalTime != INITIAL_ARRIVAL_TIME)
                {
                    LOGW("packet_arrival_time: arrivalTime packet was not consumed\n");
                }
                else
                {
                    arrivalTime = packet->header.time;
                }
                break;
            }
            case packet_thermometer:
            {
                if (arrivalTime == INITIAL_ARRIVAL_TIME)
                {
                    LOGW("packet_thermometer: arrivalTime packet was not consumed\n");
                }
                if (temperature != INITIAL_TEMPERATURE)
                {
                    LOGW("packet_thermometer: thermometer packet was not consumed\n");
                }
                else
                {
                    packet_thermometer_t *thermometer = (packet_thermometer_t *)packet.get();
                    temperature = thermometer->temperature_C;
                    arrivalTime = INITIAL_ARRIVAL_TIME;
                }
                break;
            }
            case packet_exposure:
            {
                if (arrivalTime == INITIAL_ARRIVAL_TIME)
                {
                    LOGW("packet_exposure: arrivalTime packet was not consumed\n");
                }
                if (exposureTime != INITIAL_EXPOSURE_TIME && gain != INITIAL_GAIN)
                {
                    LOGW("packet_exposure: exposure packet was not consumed\n");
                }
                packet_exposure_t *exposure = (packet_exposure_t *)packet.get();
                gain = exposure->gain;
                exposureTime = exposure->exposure_time_us;
                arrivalTime = INITIAL_ARRIVAL_TIME;
                break;
            }
            case packet_stereo_raw:
            {
                if (arrivalTime == INITIAL_ARRIVAL_TIME)
                {
                    LOGW("packet_stereo_raw: arrivalTime packet was not consumed\n");
                }
                packet_stereo_raw_t *stereo = (packet_stereo_raw_t *)packet.get();
                int leftIndex = stereo->header.sensor_id * 2 + LEFT_FISHEYE_INDEX;
                int rightIndex = stereo->header.sensor_id * 2 + RIGHT_FISHEYE_INDEX;

                sendStereoFrame(packet, gain, exposureTime, arrivalTime, LEFT_FISHEYE_INDEX);
                sendStereoFrame(packet, gain, exposureTime, arrivalTime, RIGHT_FISHEYE_INDEX);
                arrivalTime = INITIAL_ARRIVAL_TIME;
                exposureTime = INITIAL_EXPOSURE_TIME;
                gain = INITIAL_GAIN;
                break;
            }
            case packet_image_raw:
            {
                if (exposureTime == INITIAL_EXPOSURE_TIME)
                {
                    LOGW("packet_image_raw: exposure packet was not consumed\n");
                }
                if (arrivalTime == INITIAL_ARRIVAL_TIME)
                {
                    LOGW("packet_image_raw: arrivalTime packet was not consumed\n");
                }
                packet_image_raw_t *image = (packet_image_raw_t *)packet.get();
                sendImageFrame(packet, gain, exposureTime, arrivalTime);
                arrivalTime = INITIAL_ARRIVAL_TIME;
                exposureTime = INITIAL_EXPOSURE_TIME;
                gain = INITIAL_GAIN;
                break;

            }
            case packet_accelerometer:
            {
                if (arrivalTime == INITIAL_ARRIVAL_TIME)
                {
                    LOGW("packet_accelerometer: arrivalTime packet was not consumed\n");
                }
                if (temperature == INITIAL_TEMPERATURE)
                {
                    LOGW("packet_accelerometer: thermometer packet was not consumed\n");
                }
                sendAccelFrame(packet, arrivalTime, temperature);
                temperature = INITIAL_TEMPERATURE;
                arrivalTime = INITIAL_ARRIVAL_TIME;
                break;
            }
            case packet_gyroscope:
            {
                if (arrivalTime == INITIAL_ARRIVAL_TIME)
                {
                    LOGW("packet_gyroscope: arrivalTime packet was not consumed\n");
                }
                if (temperature == INITIAL_TEMPERATURE)
                {
                    LOGW("packet_gyroscope: thermometer packet was not consumed\n");
                }
                sendGyroFrame(packet, arrivalTime, temperature);
                temperature = INITIAL_TEMPERATURE;
                arrivalTime = INITIAL_ARRIVAL_TIME;
                break;
            }
            default:
            {
                LOGW("one of the packets has unknown type : %d", packet->header.type);
            }
            }
        }
        mIsStreaming = false;
    });
}

void PlayerImpl::stop()
{
    mIsStreaming = false;
    mThreadIsAlive = false;
    if (mReadingThread.joinable())
    {
        mReadingThread.join();
    }
}

bool PlayerImpl::isStreaming()
{
    return mIsStreaming;
}

void PlayerImpl::writeCalibration(uint8_t* buffer, size_t size)
{
    if (mDevice == nullptr) return;
    const static int CALIBRATION_TABLE_TYPE = 0x0;
    auto status = mDevice->WriteConfiguration(CALIBRATION_TABLE_TYPE, static_cast<uint16_t>(size), buffer);
    if (status != perc::Status::SUCCESS)
    {
        LOGE("Failed to write write calibration table with status : (0x%X)", status);
    }
}

void PlayerImpl::writePhysicalInfo(uint16_t sensorIndex, uint8_t* buffer, size_t size)
{
    if (mDevice == nullptr) return;
    if(sensorIndex != 1 && sensorIndex != 2)
    {
        LOGE("Invalid sensor id on physical info message %d", sensorIndex);
        return;
    }

    int tableType = 0x100 + sensorIndex;
    auto status = mDevice->WriteConfiguration(tableType, static_cast<uint16_t>(size), buffer);
    if (status != perc::Status::SUCCESS)
    {
        LOGE("Failed to write write calibration table with status : (0x%X)", status);
    }
}

PlayerImpl::~PlayerImpl()
{
    stop();
}
