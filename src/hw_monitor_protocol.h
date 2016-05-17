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
const uint16_t  IVCAM_VID                       = 0x8086;
const uint16_t  IVCAM_PID                       = 0x0A66;
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
    namespace hw_mon
    {
        //enum class CX3_GrossTete_MonitorCommand : uint32_t
        //{
        //  IRB         = 0x01,     // Read from i2c ( 8x8 )
        //  IWB         = 0x02,     // Write to i2c ( 8x8 )
        //  GVD         = 0x03,     // Get Version and Date
        //  IAP_IRB     = 0x04,     // Read from IAP i2c ( 8x8 )
        //  IAP_IWB     = 0x05,     // Write to IAP i2c ( 8x8 )
        //  FRCNT       = 0x06,     // Read frame counter
        //  GLD         = 0x07,     // Get logger data
        //  GPW         = 0x08,     // Write to GPIO
        //  GPR         = 0x09,     // Read from GPIO
        //  MMPWR       = 0x0A,     // Motion module power up/down
        //  DSPWR       = 0x0B,     // DS4 power up/down
        //  EXT_TRIG    = 0x0C,     // external trigger mode
        //  CX3FWUPD    = 0x0D      // FW update
        //};

        struct HWMonitorCommand
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

            HWMonitorCommand(uint8_t cmd_id) : cmd(cmd_id), Param1(0), Param2(0), Param3(0), Param4(0), sizeOfSendCommandData(0), TimeOut(5000), oneDirection(false){}
        };

        struct HWMonCommandDetails
        {
            bool        oneDirection;
            uint8_t     sendCommandData[HW_MONITOR_COMMAND_SIZE];
            int         sizeOfSendCommandData;
            long        TimeOut;
            uint8_t     receivedOpcode[4];
            uint8_t     receivedCommandData[HW_MONITOR_BUFFER_SIZE];
            size_t      receivedCommandDataLength;
        };
        
        inline void fill_usb_buffer(int opCodeNumber, int p1, int p2, int p3, int p4, uint8_t * data, int dataLength, uint8_t * bufferToSend, int & length)
        {
            uint16_t preHeaderData = IVCAM_MONITOR_MAGIC_NUMBER;

            uint8_t * writePtr = bufferToSend;
            int header_size = 4;

            
            int cur_index = 2;
            *(uint16_t *)(writePtr + cur_index) = preHeaderData;
            cur_index += sizeof(uint16_t);
            *(int *)(writePtr + cur_index) = opCodeNumber;
            cur_index += sizeof(uint32_t);
            *(int *)(writePtr + cur_index) = p1;
            cur_index += sizeof(uint32_t);
            *(int *)(writePtr + cur_index) = p2;
            cur_index += sizeof(uint32_t);
            *(int *)(writePtr + cur_index) = p3;
            cur_index += sizeof(uint32_t);
            *(int *)(writePtr + cur_index) = p4;
            cur_index += sizeof(uint32_t);

            if (dataLength)
            {
                memcpy(writePtr + cur_index, data, dataLength);
                cur_index += dataLength;
            }

            length = cur_index;
            *(uint16_t *)bufferToSend = (uint16_t)(length - header_size); // Length doesn't include header
        }

        inline void execute_usb_command(uvc::device & device, std::timed_mutex & mutex, uint8_t *out, size_t outSize, uint32_t & op, uint8_t * in, size_t & inSize)
        {
            // write
            errno = 0;

            int outXfer;

            if (!mutex.try_lock_for(std::chrono::milliseconds(IVCAM_MONITOR_MUTEX_TIMEOUT))) throw std::runtime_error("timed_mutex::try_lock_for(...) timed out");
            std::lock_guard<std::timed_mutex> guard(mutex, std::adopt_lock);

            bulk_transfer(device, IVCAM_MONITOR_ENDPOINT_OUT, out, (int)outSize, &outXfer, 1000); // timeout in ms

            // read
            if (in && inSize)
            {
                uint8_t buf[IVCAM_MONITOR_MAX_BUFFER_SIZE];

                errno = 0;

                bulk_transfer(device, IVCAM_MONITOR_ENDPOINT_IN, buf, sizeof(buf), &outXfer, 1000);
                if (outXfer < (int)sizeof(uint32_t)) throw std::runtime_error("incomplete bulk usb transfer");

                op = *(uint32_t *)buf;
                if (outXfer > (int)inSize) throw std::runtime_error("bulk transfer failed - user buffer too small");
                inSize = outXfer;
                memcpy(in, buf, inSize);
            }
        }

        inline void send_hw_monitor_command(uvc::device & device, std::timed_mutex & mutex, HWMonCommandDetails & details)
        {
            unsigned char outputBuffer[HW_MONITOR_BUFFER_SIZE];

            uint32_t op;
            size_t receivedCmdLen = HW_MONITOR_BUFFER_SIZE;

            execute_usb_command(device, mutex, (uint8_t*)details.sendCommandData, (size_t)details.sizeOfSendCommandData, op, outputBuffer, receivedCmdLen);
            details.receivedCommandDataLength = receivedCmdLen;

            if (details.oneDirection) return;

            if (details.receivedCommandDataLength < 4) throw std::runtime_error("received incomplete response to usb command");

            details.receivedCommandDataLength -= 4;
            memcpy(details.receivedOpcode, outputBuffer, 4);

            if (details.receivedCommandDataLength > 0)
                memcpy(details.receivedCommandData, outputBuffer + 4, details.receivedCommandDataLength);
        }

        inline void perform_and_send_monitor_command(uvc::device & device, std::timed_mutex & mutex, HWMonitorCommand & newCommand)
        {
            uint32_t opCodeXmit = (uint32_t)newCommand.cmd;

            HWMonCommandDetails details;
            details.oneDirection = newCommand.oneDirection;
            details.TimeOut = newCommand.TimeOut;

            fill_usb_buffer(opCodeXmit,
                newCommand.Param1,
                newCommand.Param2,
                newCommand.Param3,
                newCommand.Param4,
                newCommand.data,
                newCommand.sizeOfSendCommandData,
                details.sendCommandData,
                details.sizeOfSendCommandData);

            send_hw_monitor_command(device, mutex, details);

            // Error/exit conditions
            if (newCommand.oneDirection)
                return;

            memcpy(newCommand.receivedOpcode, details.receivedOpcode, 4);
            memcpy(newCommand.receivedCommandData, details.receivedCommandData, details.receivedCommandDataLength);
            newCommand.receivedCommandDataLength = details.receivedCommandDataLength;

            // endian?
            uint32_t opCodeAsUint32 = pack(details.receivedOpcode[3], details.receivedOpcode[2], details.receivedOpcode[1], details.receivedOpcode[0]);
            if (opCodeAsUint32 != opCodeXmit)
            {
                throw std::runtime_error("opcodes do not match");
            }
        }
    }
}

#endif // HW_MONITOR_PROTOCOL_H
