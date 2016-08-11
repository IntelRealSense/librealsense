// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef HW_MONITOR_PROTOCOL_H
#define HW_MONITOR_PROTOCOL_H

#include "uvc.h"

#include <cstring>

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
const uint8_t   PARAMETERS_BUFFER_SIZE          = 50;
const uint8_t   SIZE_OF_CALIB_HEADER_BYTES      = 4;
const uint8_t   NUM_OF_CALIBRATION_COEFFS       = 64;

const uint16_t  MAX_SIZE_OF_CALIB_PARAM_BYTES   = 800;
const uint16_t  SIZE_OF_CALIB_PARAM_BYTES       = 512;
const uint16_t  IVCAM_MONITOR_MAGIC_NUMBER      = 0xcdab;
const uint16_t  IVCAM_MONITOR_MAX_BUFFER_SIZE   = 1024;
const uint16_t  IVCAM_MONITOR_MUTEX_TIMEOUT     = 3000;
const uint16_t  HW_MONITOR_COMMAND_SIZE         = 1000;
const uint16_t  HW_MONITOR_BUFFER_SIZE          = 1000;

// IVCAM depth XU identifiers
const uint8_t IVCAM_DEPTH_LASER_POWER           = 1;
const uint8_t IVCAM_DEPTH_ACCURACY              = 2;
const uint8_t IVCAM_DEPTH_MOTION_RANGE          = 3;
const uint8_t IVCAM_DEPTH_ERROR                 = 4;
const uint8_t IVCAM_DEPTH_FILTER_OPTION         = 5;
const uint8_t IVCAM_DEPTH_CONFIDENCE_THRESH     = 6;
const uint8_t IVCAM_DEPTH_DYNAMIC_FPS           = 7; // Only available on IVCAM 1.0 / F200

// IVCAM color XU identifiers
const uint8_t IVCAM_COLOR_EXPOSURE_PRIORITY     = 1;
const uint8_t IVCAM_COLOR_AUTO_FLICKER          = 2;
const uint8_t IVCAM_COLOR_ERROR                 = 3;
const uint8_t IVCAM_COLOR_EXPOSURE_GRANULAR     = 4;

namespace rsimpl
{
    namespace hw_monitor
    {


        struct hwmon_cmd
        {
            uint8_t     cmd;
            int         Param1;
            int         Param2;
            int         Param3;
            int         Param4;
            uint8_t     data[HW_MONITOR_BUFFER_SIZE];
            int         sizeOfSendCommandData;
            long        TimeOut;
            bool        oneDirection;
            uint8_t     receivedCommandData[HW_MONITOR_BUFFER_SIZE];
            size_t      receivedCommandDataLength;
            uint8_t     receivedOpcode[4];

            hwmon_cmd(uint8_t cmd_id) : cmd(cmd_id), Param1(0), Param2(0), Param3(0), Param4(0), sizeOfSendCommandData(0), TimeOut(5000), oneDirection(false){}
        };

        struct hwmon_cmd_details
        {
            bool        oneDirection;
            uint8_t     sendCommandData[HW_MONITOR_COMMAND_SIZE];
            int         sizeOfSendCommandData;
            long        TimeOut;
            uint8_t     receivedOpcode[4];
            uint8_t     receivedCommandData[HW_MONITOR_BUFFER_SIZE];
            size_t      receivedCommandDataLength;
        };
        
        void fill_usb_buffer(int opCodeNumber, int p1, int p2, int p3, int p4, uint8_t * data, int dataLength, uint8_t * bufferToSend, int & length);

        void execute_usb_command(uvc::device & device, std::timed_mutex & mutex, uint8_t *out, size_t outSize, uint32_t & op, uint8_t * in, size_t & inSize);        

        void send_hw_monitor_command(uvc::device & device, std::timed_mutex & mutex, hwmon_cmd_details & details);

        void perform_and_send_monitor_command(uvc::device & device, std::timed_mutex & mutex, hwmon_cmd & newCommand);
        void perform_and_send_monitor_command(uvc::device & device, std::timed_mutex & mutex, hwmon_cmd & newCommand);

        void i2c_write_reg(int command, uvc::device & device, uint16_t slave_address, uint16_t reg, uint32_t value);
        void i2c_read_reg(int command, uvc::device & device, uint16_t slave_address, uint16_t reg, uint32_t size, byte* data);

        void read_from_eeprom(int IRB_opcode, int IWB_opcode, uvc::device & device, unsigned int offset, int size, byte* data);

        void get_raw_data(uint8_t opcode, uvc::device & device, std::timed_mutex & mutex, uint8_t * data, size_t & bytesReturned);
    }
}

#endif // HW_MONITOR_PROTOCOL_H
