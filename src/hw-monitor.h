// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "sensor.h"
#include <mutex>

const uint8_t   IV_COMMAND_FIRMWARE_UPDATE_MODE = 0x01;
const uint8_t   IV_COMMAND_GET_CALIBRATION_DATA = 0x02;
const uint8_t   IV_COMMAND_LASER_POWER          = 0x03;
const uint8_t   IV_COMMAND_DEPTH_ACCURACY       = 0x04;
const uint8_t   IV_COMMAND_ZUNIT                = 0x05;
const uint8_t   IV_COMMAND_LOW_CONFIDENCE_LEVEL = 0x06;
const uint8_t   IV_COMMAND_INTENSITY_IMAGE_TYPE = 0x07;
const uint8_t   IV_COMMAND_MOTION_VS_RANGE_TRADE= 0x08;
const uint8_t   IV_COMMAND_POWER_GEAR           = 0x09;
const uint8_t   IV_COMMAND_FILTER_OPTION        = 0x0A;
const uint8_t   IV_COMMAND_VERSION              = 0x0B;
const uint8_t   IV_COMMAND_CONFIDENCE_THRESHHOLD= 0x0C;

const uint8_t   IVCAM_MONITOR_INTERFACE         = 0x4;
const uint8_t   IVCAM_MONITOR_ENDPOINT_OUT      = 0x1;
const uint8_t   IVCAM_MONITOR_ENDPOINT_IN       = 0x81;
const uint8_t   IVCAM_MIN_SUPPORTED_VERSION     = 13;
const uint8_t   IVCAM_MONITOR_HEADER_SIZE       = (sizeof(uint32_t) * 6);
const uint8_t   NUM_OF_CALIBRATION_PARAMS       = 100;
const uint8_t   PARAMETERS2_BUFFER_SIZE          = 50;
const uint8_t   SIZE_OF_CALIB_HEADER_BYTES      = 4;
const uint8_t   NUM_OF_CALIBRATION_COEFFS       = 64;

const uint16_t  MAX_SIZE_OF_CALIB_PARAM_BYTES   = 800;
const uint16_t  SIZE_OF_CALIB_PARAM_BYTES       = 512;
const uint16_t  IVCAM_MONITOR_MAGIC_NUMBER      = 0xcdab;
const uint16_t  IVCAM_MONITOR_MAX_BUFFER_SIZE   = 1024;
const uint16_t  IVCAM_MONITOR_MUTEX_TIMEOUT     = 3000;
const uint16_t  HW_MONITOR_COMMAND_SIZE         = 1000;
const uint16_t  HW_MONITOR_BUFFER_SIZE          = 1024;
const uint16_t  HW_MONITOR_DATA_SIZE_OFFSET     = 1020;
const uint16_t  SIZE_OF_HW_MONITOR_HEADER       = 4;



namespace librealsense
{
    class uvc_sensor;

    class locked_transfer
    {
    public:
        locked_transfer(std::shared_ptr<platform::command_transfer> command_transfer, uvc_sensor& uvc_ep)
            :_command_transfer(command_transfer),
             _uvc_sensor_base(uvc_ep)
        {}

        std::vector<uint8_t> send_receive(
            const std::vector<uint8_t>& data,
            int timeout_ms = 5000,
            bool require_response = true)
        {
            std::lock_guard<std::recursive_mutex> lock(_local_mtx);
            return _uvc_sensor_base.invoke_powered([&]
                (platform::uvc_device& dev)
                {
                    std::lock_guard<platform::uvc_device> lock(dev);
                    return _command_transfer->send_receive(data, timeout_ms, require_response);
                });
        }

    private:
        std::shared_ptr<platform::command_transfer> _command_transfer;
        uvc_sensor& _uvc_sensor_base;
        std::recursive_mutex _local_mtx;
    };

    struct command
    {
        uint8_t cmd;
        int     param1;
        int     param2;
        int     param3;
        int     param4;
        std::vector<uint8_t> data;
        int     timeout_ms = 5000;
        bool    require_response = true;

        explicit command(uint8_t cmd, int param1 = 0, int param2 = 0,
                int param3 = 0, int param4 = 0, int timeout_ms = 5000,
                bool require_response = true)
            : cmd(cmd), param1(param1),
              param2(param2),
              param3(param3), param4(param4),
              timeout_ms(timeout_ms), require_response(require_response)
        {
        }
    };

    class hw_monitor
    {
        struct hwmon_cmd
        {
            uint8_t     cmd;
            int         param1;
            int         param2;
            int         param3;
            int         param4;
            uint8_t     data[HW_MONITOR_BUFFER_SIZE];
            int         sizeOfSendCommandData;
            long        timeOut;
            bool        oneDirection;
            uint8_t     receivedCommandData[HW_MONITOR_BUFFER_SIZE];
            size_t      receivedCommandDataLength;
            uint8_t     receivedOpcode[4];

            explicit hwmon_cmd(uint8_t cmd_id)
                : cmd(cmd_id),
                  param1(0),
                  param2(0),
                  param3(0),
                  param4(0),
                  sizeOfSendCommandData(0),
                  timeOut(5000),
                  oneDirection(false),
                  receivedCommandDataLength(0)
            {}


            explicit hwmon_cmd(const command& cmd)
                : cmd(cmd.cmd),
                  param1(cmd.param1),
                  param2(cmd.param2),
                  param3(cmd.param3),
                  param4(cmd.param4),
                  sizeOfSendCommandData(std::min((uint16_t)cmd.data.size(), HW_MONITOR_BUFFER_SIZE)),
                  timeOut(cmd.timeout_ms),
                  oneDirection(!cmd.require_response),
                  receivedCommandDataLength(0)
            {
                librealsense::copy(data, cmd.data.data(), sizeOfSendCommandData);
            }
        };

        struct hwmon_cmd_details
        {
            bool                                         oneDirection;
            std::array<uint8_t, HW_MONITOR_COMMAND_SIZE> sendCommandData;
            int                                          sizeOfSendCommandData;
            long                                         timeOut;
            std::array<uint8_t, 4>                       receivedOpcode;
            std::array<uint8_t, HW_MONITOR_BUFFER_SIZE>  receivedCommandData;
            size_t                                       receivedCommandDataLength;
        };

        static void fill_usb_buffer(int opCodeNumber, int p1, int p2, int p3, int p4, uint8_t* data, int dataLength, uint8_t* bufferToSend, int& length);
        void execute_usb_command(uint8_t *out, size_t outSize, uint32_t& op, uint8_t* in, size_t& inSize) const;
        static void update_cmd_details(hwmon_cmd_details& details, size_t receivedCmdLen, unsigned char* outputBuffer);
        void send_hw_monitor_command(hwmon_cmd_details& details) const;

        std::shared_ptr<locked_transfer> _locked_transfer;
    public:
        explicit hw_monitor(std::shared_ptr<locked_transfer> locked_transfer)
            : _locked_transfer(std::move(locked_transfer))
        {}

        std::vector<uint8_t> send(std::vector<uint8_t> data) const;
        std::vector<uint8_t> send(command cmd) const;
        void get_gvd(size_t sz, unsigned char* gvd, uint8_t gvd_cmd) const;
        std::string get_firmware_version_string(int gvd_cmd, uint32_t offset) const;
        std::string get_module_serial_string(uint8_t gvd_cmd, uint32_t offset, int size = 6) const;
        bool is_camera_locked(uint8_t gvd_cmd, uint32_t offset) const;
    };
}
