/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/

#pragma once

#include <stdint.h>
#include <memory>
#include <map>
#include "TrackingData.h"
#include "rc_packet.h"

namespace perc
{
    class Packet
    {
    public:
        virtual const uint8_t* getBytes() = 0;
        virtual size_t getSize() = 0;
        virtual packet_type getType() = 0;
    };

    class ThermometerPacket : public Packet
    {
    public:
        ThermometerPacket(float temperature, uint16_t sensor_index, uint64_t time);
        virtual ~ThermometerPacket() = default;
        virtual const uint8_t* getBytes() override;
        virtual size_t getSize() override;
        virtual packet_type getType() override;

    private:
        packet_thermometer_t mPacket;
    };

    class ExposurePacket : public Packet
    {
    public:
        ExposurePacket() : mPacket({ 0 }) {}
        ExposurePacket(uint64_t exposure_time_us, float gain, uint16_t sensor_index, uint64_t time);
        virtual ~ExposurePacket() = default;
        virtual const uint8_t* getBytes() override;
        virtual size_t getSize() override;
        virtual packet_type getType() override;
        packet_exposure_t getRCPacket();

    private:
        packet_exposure_t mPacket;
    };

    class ImagePacket : public Packet
    {
    public:
        ImagePacket(TrackingData::VideoFrame& frame);
        virtual const uint8_t* getBytes() override;
        virtual size_t getSize() override;
        virtual packet_type getType() override;
        uint16_t getSensorId();
        std::shared_ptr<packet_image_raw_t> getRCPacket();
        ExposurePacket getExposurePacket();
    private:
        std::shared_ptr<packet_image_raw_t> mPacket;
        ExposurePacket mExposurePacket;
    };

    class StereoPacket : public Packet
    {
    public:
        virtual ~StereoPacket() = default;
        virtual const uint8_t* getBytes() override;
        virtual size_t getSize() override;
        virtual packet_type getType() override;
        bool addPacket(ImagePacket* packet);
        bool isComplete();
        const uint8_t* getImageBytes(uint16_t index);
        size_t getImageSize(uint16_t index);
        void clear();
        ExposurePacket getExposurePacket();
    private:
        static bool areCoupeled(uint64_t time_us1, uint64_t time_us2);
    private:
        std::map<uint16_t, std::shared_ptr<packet_image_raw_t>> mImagePackets;
        ExposurePacket mExposurePacket;
        packet_stereo_raw_t mPacket;
    };

    class GyroPacket : public Packet
    {
    public:
        GyroPacket(TrackingData::GyroFrame& frame);
        ~GyroPacket() = default;
        virtual const uint8_t* getBytes() override;
        virtual size_t getSize() override;
        virtual packet_type getType() override;
        ThermometerPacket getThermometerPacket();

    private:
        packet_gyroscope_t mPacket;
        ThermometerPacket mThermometerPacket;
    };

    class VelocimeterPacket : public Packet
    {
    public:
        VelocimeterPacket(TrackingData::VelocimeterFrame& frame);
        ~VelocimeterPacket() = default;
        virtual const uint8_t* getBytes() override;
        virtual size_t getSize() override;
        virtual packet_type getType() override;
        ThermometerPacket getThermometerPacket();

    private:
        packet_velocimeter_t mPacket;
        ThermometerPacket mThermometerPacket;
    };

    class AcclPacket : public Packet
    {
    public:
        AcclPacket(TrackingData::AccelerometerFrame& frame);
        ~AcclPacket() = default;
        virtual const uint8_t* getBytes() override;
        virtual size_t getSize() override;
        virtual packet_type getType() override;
        ThermometerPacket getThermometerPacket();

    private:
        packet_accelerometer_t mPacket;
        ThermometerPacket mThermometerPacket;
    };

    class ArrivalTimePacket : public Packet
    {
    public:
        ArrivalTimePacket(uint64_t arrivalTime);
        ~ArrivalTimePacket() = default;
        virtual const uint8_t* getBytes() override;
        virtual size_t getSize() override;
        virtual packet_type getType() override;

    private:
        packet_arrival_time_t mPacket;
    };

    class CalibrationPacket : public Packet
    {
    public:
        CalibrationPacket(const uint8_t* buffer, uint32_t size);
        ~CalibrationPacket() = default;
        virtual const uint8_t* getBytes() override;
        virtual size_t getSize() override;
        virtual packet_type getType() override;

    private:
        std::shared_ptr<packet_calibration_bin_t> mPacket;
    };

    class PhysicalInfoPacket : public Packet
    {
    public:
        PhysicalInfoPacket(uint16_t sensor_index, const uint8_t* buffer, uint32_t size);
        ~PhysicalInfoPacket() = default;
        virtual const uint8_t* getBytes() override;
        virtual size_t getSize() override;
        virtual packet_type getType() override;

    private:
        std::shared_ptr<packet_controller_physical_info_t> mPacket;
    };

    class LedPacket : public Packet
    {
    public:
        LedPacket(TrackingData::ControllerLedEventFrame& frame);
        ~LedPacket() = default;
        virtual const uint8_t* getBytes() override;
        virtual size_t getSize() override;
        virtual packet_type getType() override;

    private:
        std::shared_ptr<packet_led_intensity_t> mPacket;

    };
}
