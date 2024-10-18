// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <stdint.h>
#include <sstream>
#include <string>
#include <map>


static std::string gyro = "gyro_3d";
static std::string accel = "accel_3d";
static std::string custom = "custom";

namespace librealsense
{
    namespace platform
    {
        constexpr int HID_REPORT_TYPE_INPUT = 1;
        constexpr int HID_REPORT_TYPE_FEATURE = 3;
        constexpr int DEVICE_POWER_D0 = 2;
        constexpr int DEVICE_POWER_D4 = 6;

    enum REPORT_ID
    {
        REPORT_ID_ACCELEROMETER_3D = 1,
        REPORT_ID_GYROMETER_3D = 2,
        REPORT_ID_CUSTOM = 3
    };

    enum USB_REQUEST_CODE {
      USB_REQUEST_CODE_GET = 0xa1,
      USB_REQUEST_CODE_SET = 0x21
    };

    enum HID_REQUEST_CODE {
      HID_REQUEST_GET_REPORT = 0x1,
      HID_REQUEST_GET_IDLE = 0x2,
      HID_REQUEST_GET_PROTOCOL = 0x3,
      HID_REQUEST_SET_REPORT = 0x9,
      HID_REQUEST_SET_IDLE   = 0xa,
      HID_REQUEST_SET_PROTOCOL = 0xb
    };

#pragma pack(push, 1)
    struct REALSENSE_FEATURE_REPORT {
      unsigned char reportId;
      unsigned char connectionType;
      unsigned char sensorState;
      unsigned char power;
      unsigned char minReport;
      unsigned short report;
      unsigned short sensitivity;
    };

    struct REALSENSE_HID_REPORT {
      unsigned char reportId;
      unsigned char unknown;
      unsigned long long timeStamp;
      int32_t x;
      int32_t y;
      int32_t z;
      unsigned int customValue1;
      unsigned int customValue2;
      unsigned short customValue3;
      unsigned short customValue4;
      unsigned short customValue5;
      unsigned char customValue6;
      unsigned char customValue7;
    };
#pragma pack(pop)


#pragma pack(push, 1)
    struct FEATURE_REPORT
    {
      unsigned char reportId;
      unsigned char connectionType;
      unsigned char sensorState;
      unsigned char power;
      unsigned char minReport;
      unsigned short report;
      unsigned short sensitivity;
    };
#pragma pack(pop)
    }
}
