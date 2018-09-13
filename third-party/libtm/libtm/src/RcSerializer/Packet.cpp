/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/

#define LOG_TAG "Packet"

#include <chrono>
#include "Packet.h"
#include "Utils.h"

using namespace perc;
const uint32_t TS_MAX_GAP_US = 200;

inline uint64_t ns2us(int64_t ts)
{
    std::chrono::duration<int64_t, std::nano> ns(ts);
    return std::chrono::duration_cast<std::chrono::duration<uint64_t, std::micro>>(ns).count();
}

ImagePacket::ImagePacket(TrackingData::VideoFrame& frame) : mExposurePacket(frame.exposuretime, static_cast<float>(frame.gain), frame.sensorIndex, ns2us(frame.timestamp))
{
    uint32_t bufferSize = frame.profile.stride  * frame.profile.height;
    mPacket = std::shared_ptr<packet_image_raw_t>((packet_image_raw_t*) ::operator new (sizeof(packet_image_raw_t) + bufferSize));
    mPacket->height = frame.profile.height;
    mPacket->width = frame.profile.width;
    mPacket->stride = frame.profile.stride;
    mPacket->format = 0;
    mPacket->exposure_time_us = static_cast<uint64_t>(frame.exposuretime);
    perc::copy(mPacket->data, frame.data, bufferSize);

    mPacket->header.sensor_id = frame.sensorIndex;
    mPacket->header.type = packet_image_raw;
    mPacket->header.bytes = sizeof(packet_image_raw_t) + bufferSize;
    mPacket->header.time = ns2us(frame.timestamp);

}
const uint8_t* ImagePacket::getBytes() 
{ 
    return (uint8_t*)mPacket.get(); 
}
size_t ImagePacket::getSize()
{ 
    return sizeof(packet_image_raw_t) + (mPacket->height * mPacket->stride);
};
packet_type ImagePacket::getType() 
{
    return packet_image_raw;
}
uint16_t ImagePacket::getSensorId()
{
    return mPacket->header.sensor_id;
}
std::shared_ptr<packet_image_raw_t> ImagePacket::getRCPacket()
{
    return mPacket;
}
ExposurePacket ImagePacket::getExposurePacket()
{
    return mExposurePacket;
}

const uint8_t* StereoPacket::getBytes()
{
    if (isComplete() == false)
    {
        return nullptr;
    }
    mPacket.header.sensor_id = mImagePackets[0]->header.sensor_id / 2;
    mPacket.header.type = packet_stereo_raw;
    mPacket.header.time = mImagePackets[0]->header.time;
    mPacket.height = mImagePackets[0]->height;
    mPacket.width = mImagePackets[0]->width;
    mPacket.format = mImagePackets[0]->format;
    mPacket.exposure_time_us = mImagePackets[0]->exposure_time_us;
    mPacket.stride1 = mImagePackets[0]->stride;
    mPacket.stride2 = mImagePackets[1]->stride;
    mPacket.header.bytes = sizeof(packet_stereo_raw_t) + mPacket.height * (mPacket.stride1 + mPacket.stride2);
    
    return (uint8_t*)&mPacket;
}
size_t StereoPacket::getSize()
{
    if (isComplete() == false)
    {
        return 0;
    }
    return sizeof(packet_stereo_raw_t);
}
packet_type StereoPacket::getType()
{
    return packet_stereo_raw;
}
bool StereoPacket::addPacket(ImagePacket* packet)
{
    std::shared_ptr<packet_image_raw_t> imagePacket = packet->getRCPacket();
    uint16_t sensorIndex = imagePacket->header.sensor_id % 2;
    if (mImagePackets.find(sensorIndex) != mImagePackets.end())
    {
        LOGE("Image packet is out of order");
        return false;
    }
    uint16_t coupeledSensorIndex = (sensorIndex + 1) % 2;
    if (mImagePackets.find(coupeledSensorIndex) != mImagePackets.end() &&
        !areCoupeled(mImagePackets[coupeledSensorIndex]->header.time, imagePacket->header.time))
    {
        LOGE("Image packet is out of order");
        return false;
    }
    mImagePackets[sensorIndex] = imagePacket;
    packet_exposure_t imageExposurePacket = packet->getExposurePacket().getRCPacket();
    mExposurePacket = ExposurePacket(imageExposurePacket.exposure_time_us, imageExposurePacket.gain, imagePacket->header.sensor_id / 2, imageExposurePacket.header.time);
    return true;
}
bool StereoPacket::areCoupeled(uint64_t time_us1, uint64_t time_us2)
{
    return std::abs(static_cast<int>(time_us1) - static_cast<int>(time_us2)) < TS_MAX_GAP_US;
}
bool StereoPacket::isComplete()
{
    return mImagePackets.size() == 2;
}
const uint8_t* StereoPacket::getImageBytes(uint16_t index)
{
    if (mImagePackets[index])
    {
        return (uint8_t*)mImagePackets[index].get()->data;
    }
    LOGE("Invalid read of stereo packet");
    return nullptr;
}
size_t StereoPacket::getImageSize(uint16_t index)
{
    if (mImagePackets[index])
    {
        return  mImagePackets[index]->height * mImagePackets[index]->stride;
    }
    LOGE("Invalid read of stereo packet size");
    return 0;
}
void StereoPacket::clear()
{
    mImagePackets.clear();
}
ExposurePacket StereoPacket::getExposurePacket()
{
    return mExposurePacket;
}

GyroPacket::GyroPacket(TrackingData::GyroFrame& frame) : mThermometerPacket(frame.temperature, frame.sensorIndex, frame.timestamp)
{
    mPacket.w[0] = frame.angularVelocity.x;
    mPacket.w[1] = frame.angularVelocity.y;
    mPacket.w[2] = frame.angularVelocity.z;

    mPacket.header.sensor_id = frame.sensorIndex;
    mPacket.header.type = packet_gyroscope;
    mPacket.header.bytes = sizeof(packet_gyroscope_t);
    mPacket.header.time = ns2us(frame.timestamp);
}
const uint8_t* GyroPacket::getBytes() 
{ 
    return (uint8_t*)&mPacket; 
}
size_t GyroPacket::getSize() 
{ 
    return sizeof(mPacket);
}
packet_type GyroPacket::getType()
{
    return packet_gyroscope;
}
ThermometerPacket GyroPacket::getThermometerPacket()
{
    return mThermometerPacket;
}

VelocimeterPacket::VelocimeterPacket(TrackingData::VelocimeterFrame& frame) : mThermometerPacket(frame.temperature, frame.sensorIndex, frame.timestamp)
{
    mPacket.v[0] = frame.angularVelocity.x;
    mPacket.v[1] = frame.angularVelocity.y;
    mPacket.v[2] = frame.angularVelocity.z;

    mPacket.header.sensor_id = frame.sensorIndex;
    mPacket.header.type = packet_velocimeter;
    mPacket.header.bytes = sizeof(packet_velocimeter_t);
    mPacket.header.time = ns2us(frame.timestamp);
}
const uint8_t* VelocimeterPacket::getBytes()
{
    return (uint8_t*)&mPacket;
}
size_t VelocimeterPacket::getSize()
{
    return sizeof(mPacket);
}
packet_type VelocimeterPacket::getType()
{
    return packet_velocimeter;
}
ThermometerPacket VelocimeterPacket::getThermometerPacket()
{
    return mThermometerPacket;
}

AcclPacket::AcclPacket(TrackingData::AccelerometerFrame& frame) : mThermometerPacket(frame.temperature, frame.sensorIndex, frame.timestamp)
{
    mPacket.a[0] = frame.acceleration.x;
    mPacket.a[1] = frame.acceleration.y;
    mPacket.a[2] = frame.acceleration.z;

    mPacket.header.sensor_id = frame.sensorIndex;
    mPacket.header.type = packet_accelerometer;
    mPacket.header.bytes = sizeof(packet_accelerometer_t);
    mPacket.header.time = ns2us(frame.timestamp);
}
const uint8_t* AcclPacket::getBytes() 
{ 
    return (uint8_t*)&mPacket; 
}
size_t AcclPacket::getSize() 
{ 
    return sizeof(mPacket); 
}
packet_type AcclPacket::getType()
{
    return packet_accelerometer;
}
ThermometerPacket AcclPacket::getThermometerPacket()
{
    return mThermometerPacket;
}

ThermometerPacket::ThermometerPacket(float temperature, uint16_t sensor_index, uint64_t time)
{
    mPacket.temperature_C = temperature;
           
    mPacket.header.sensor_id = sensor_index;
    mPacket.header.type = packet_thermometer;
    mPacket.header.bytes = sizeof(packet_thermometer_t);
    mPacket.header.time = ns2us(time);
}
const uint8_t* ThermometerPacket::getBytes() 
{ 
    return (uint8_t*)&mPacket; 
}
size_t ThermometerPacket::getSize() 
{ 
    return sizeof(mPacket);
}
packet_type ThermometerPacket::getType()
{
    return packet_thermometer;
}

ArrivalTimePacket::ArrivalTimePacket(uint64_t arrivalTime)
{
    mPacket.header.sensor_id = 0;
    mPacket.header.type = packet_arrival_time;
    mPacket.header.bytes = sizeof(packet_arrival_time_t);
    mPacket.header.time = ns2us(arrivalTime);
}
const uint8_t* ArrivalTimePacket::getBytes() 
{ 
    return (uint8_t*)&mPacket; 
}
size_t ArrivalTimePacket::getSize() 
{
    return sizeof(mPacket); 
}
packet_type ArrivalTimePacket::getType()
{
    return packet_arrival_time;
}

CalibrationPacket::CalibrationPacket(const uint8_t* buffer, uint32_t size)
{
    mPacket = std::shared_ptr<packet_calibration_bin_t>((packet_calibration_bin_t*) ::operator new (sizeof(packet_calibration_bin_t) + size));
    mPacket->header.sensor_id = 0;
    mPacket->header.type = packet_calibration_bin;
    mPacket->header.bytes = sizeof(packet_calibration_bin_t) + size;
    mPacket->header.time = 0;
    perc::copy(mPacket->data, buffer, size);
}
const uint8_t* CalibrationPacket::getBytes()
{
    return (uint8_t*)mPacket.get();
}
size_t CalibrationPacket::getSize()
{
    return mPacket->header.bytes;
}
packet_type CalibrationPacket::getType()
{
    return packet_calibration_bin;
}

ExposurePacket::ExposurePacket(uint64_t exposure_time, float gain, uint16_t sensor_index, uint64_t time)
{
    mPacket.exposure_time_us = exposure_time;
    mPacket.gain = gain;

    mPacket.header.sensor_id = sensor_index;
    mPacket.header.type = packet_exposure;
    mPacket.header.bytes = sizeof(packet_exposure_t);
    mPacket.header.time = time;
}
const uint8_t* ExposurePacket::getBytes()
{
    return (uint8_t*)&mPacket;
}
size_t ExposurePacket::getSize()
{
    return sizeof(mPacket);
}
packet_type ExposurePacket::getType()
{
    return packet_exposure;
}
packet_exposure_t ExposurePacket::getRCPacket()
{
    return mPacket;
}

PhysicalInfoPacket::PhysicalInfoPacket(uint16_t sensor_index, const uint8_t* buffer, uint32_t size)
{
    uint32_t packetSize = sizeof(packet_controller_physical_info_t) + size;
    mPacket = std::shared_ptr<packet_controller_physical_info_t>((packet_controller_physical_info_t*) ::operator new (packetSize));
    mPacket->header.sensor_id = sensor_index;
    mPacket->header.type = packet_controller_physical_info;
    mPacket->header.bytes = packetSize;
    mPacket->header.time = 0;
    perc::copy(mPacket->data, buffer, size);
}
const uint8_t* PhysicalInfoPacket::getBytes()
{
    return (uint8_t*)mPacket.get();
}
size_t PhysicalInfoPacket::getSize()
{
    return mPacket->header.bytes;
}
packet_type PhysicalInfoPacket::getType()
{
    return packet_controller_physical_info;
}

LedPacket::LedPacket(TrackingData::ControllerLedEventFrame& frame)
{
    uint32_t packetSize = sizeof(packet_led_intensity_t);
    mPacket = std::shared_ptr<packet_led_intensity_t>((packet_led_intensity_t*) ::operator new (packetSize));
    mPacket->header.sensor_id = frame.controllerId;
    mPacket->header.type = packet_led_intensity;
    mPacket->header.bytes = packetSize;
    mPacket->header.time = ns2us(frame.timestamp);
    mPacket->intensity = frame.intensity;
    mPacket->led_id = frame.ledId;
}
const uint8_t* LedPacket::getBytes()
{
    return (uint8_t*)mPacket.get();
}
size_t LedPacket::getSize()
{
    return mPacket->header.bytes;
}
packet_type LedPacket::getType()
{
    return packet_led_intensity;
}



