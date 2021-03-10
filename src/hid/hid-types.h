// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <stdint.h>
#include <sstream>
#include <string>
#include <map>

#define HID_REPORT_TYPE_INPUT       1
#define HID_REPORT_TYPE_FEATURE     3

#define DEVICE_POWER_D0             2
#define DEVICE_POWER_D4             6

#define REPORT_ID_ACCELEROMETER_3D  1
#define REPORT_ID_GYROMETER_3D      2
#define REPORT_ID_CUSTOM            3

const size_t SIZE_OF_HID_IMU_FRAME = 32;

static std::string gyro = "gyro_3d";
static std::string accel = "accel_3d";
static std::string custom = "custom";

namespace librealsense
{
    namespace platform
    {
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
      unsigned short unknown;
    };

    struct REALSENSE_HID_REPORT {
      unsigned char reportId;
      unsigned char unknown;
      unsigned long long timeStamp;
      short x;
      short y;
      short z;
      unsigned int customValue1;
      unsigned int customValue2;
      unsigned short customValue3;
      unsigned short customValue4;
      unsigned short customValue5;
      unsigned char customValue6;
      unsigned char customValue7;
    };

#pragma pack(pop)
    static_assert(sizeof(REALSENSE_HID_REPORT) == SIZE_OF_HID_IMU_FRAME, "HID IMU Frame struct expected size is 32 bytes");

    struct hid_device_info
    {
        std::string id;
        std::string vid;
        std::string pid;
        std::string unique_id;
        std::string device_path;
        std::string serial_number;

        operator std::string()
        {
            std::stringstream s;
            s << "id- " << id <<
                "\nvid- " << std::hex << vid <<
                "\npid- " << std::hex << pid <<
                "\nunique_id- " << unique_id <<
                "\npath- " << device_path;

            return s.str();
        }
    };

    inline bool operator==(const hid_device_info& a,
        const hid_device_info& b)
    {
        return  (a.id == b.id) &&
            (a.vid == b.vid) &&
            (a.pid == b.pid) &&
            (a.unique_id == b.unique_id) &&
            (a.device_path == b.device_path);
    }

#pragma pack(push, 1)
    struct FEATURE_REPORT
    {
      unsigned char reportId;
      unsigned char connectionType;
      unsigned char sensorState;
      unsigned char power;
      unsigned char minReport;
      unsigned short report;
      unsigned short unknown;
    };
#pragma pack(pop)
    }
}
