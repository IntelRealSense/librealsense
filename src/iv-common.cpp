// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <cstring>
#include <algorithm>
#include <thread>
#include <cmath>

#include "hw-monitor.h"
#include "iv-common.h"

using namespace rsimpl::hw_monitor;

namespace rsimpl {
namespace iv {

    enum Property
    {
        IVCAM_PROPERTY_COLOR_EXPOSURE               = 1,
        IVCAM_PROPERTY_COLOR_BRIGHTNESS             = 2,
        IVCAM_PROPERTY_COLOR_CONTRAST               = 3,
        IVCAM_PROPERTY_COLOR_SATURATION             = 4,
        IVCAM_PROPERTY_COLOR_HUE                    = 5,
        IVCAM_PROPERTY_COLOR_GAMMA                  = 6,
        IVCAM_PROPERTY_COLOR_WHITE_BALANCE          = 7,
        IVCAM_PROPERTY_COLOR_SHARPNESS              = 8,
        IVCAM_PROPERTY_COLOR_BACK_LIGHT_COMPENSATION = 9,
        IVCAM_PROPERTY_COLOR_GAIN                   = 10,
        IVCAM_PROPERTY_COLOR_POWER_LINE_FREQUENCY   = 11,
        IVCAM_PROPERTY_AUDIO_MIX_LEVEL              = 12,
        IVCAM_PROPERTY_APERTURE                     = 13,
        IVCAM_PROPERTY_DISTORTION_CORRECTION_I      = 202,
        IVCAM_PROPERTY_DISTORTION_CORRECTION_DPTH   = 203,
        IVCAM_PROPERTY_DEPTH_MIRROR                 = 204,    //0 - not mirrored, 1 - mirrored
        IVCAM_PROPERTY_COLOR_MIRROR                 = 205,
        IVCAM_PROPERTY_COLOR_FIELD_OF_VIEW          = 207,
        IVCAM_PROPERTY_COLOR_SENSOR_RANGE           = 209,
        IVCAM_PROPERTY_COLOR_FOCAL_LENGTH           = 211,
        IVCAM_PROPERTY_COLOR_PRINCIPAL_POINT        = 213,
        IVCAM_PROPERTY_DEPTH_FIELD_OF_VIEW          = 215,
        IVCAM_PROPERTY_DEPTH_UNDISTORTED_FIELD_OF_VIEW = 223,
        IVCAM_PROPERTY_DEPTH_SENSOR_RANGE           = 217,
        IVCAM_PROPERTY_DEPTH_FOCAL_LENGTH           = 219,
        IVCAM_PROPERTY_DEPTH_UNDISTORTED_FOCAL_LENGTH = 225,
        IVCAM_PROPERTY_DEPTH_PRINCIPAL_POINT        = 221,
        IVCAM_PROPERTY_MF_DEPTH_LOW_CONFIDENCE_VALUE = 5000,
        IVCAM_PROPERTY_MF_DEPTH_UNIT                = 5001,   // in micron
        IVCAM_PROPERTY_MF_CALIBRATION_DATA          = 5003,
        IVCAM_PROPERTY_MF_LASER_POWER               = 5004,
        IVCAM_PROPERTY_MF_ACCURACY                  = 5005,
        IVCAM_PROPERTY_MF_INTENSITY_IMAGE_TYPE      = 5006,   //0 - (I0 - laser off), 1 - (I1 - Laser on), 2 - (I1-I0), default is I1.
        IVCAM_PROPERTY_MF_MOTION_VS_RANGE_TRADE     = 5007,
        IVCAM_PROPERTY_MF_POWER_GEAR                = 5008,
        IVCAM_PROPERTY_MF_FILTER_OPTION             = 5009,
        IVCAM_PROPERTY_MF_VERSION                   = 5010,
        IVCAM_PROPERTY_MF_DEPTH_CONFIDENCE_THRESHOLD = 5013,
        IVCAM_PROPERTY_ACCELEROMETER_READING        = 3000,   // three values
        IVCAM_PROPERTY_PROJECTION_SERIALIZABLE      = 3003,
        IVCAM_PROPERTY_CUSTOMIZED                   = 0x04000000,
    };

    //////////////////
    // XU functions //
    //////////////////

    // N.B. f200 xu_read and xu_write hard code the xu interface to the depth suvdevice. There is only a
    // single *potentially* useful XU on the color device, so let's ignore it for now.
    void xu_read(const uvc::device & device, uint8_t xu_ctrl, void * buffer, uint32_t length)
    {
        uvc::get_control_with_retry(device, iv::depth_xu, static_cast<int>(xu_ctrl), buffer, length);
    }

    void xu_write(uvc::device & device, uint8_t xu_ctrl, void * buffer, uint32_t length)
    {
        uvc::set_control_with_retry(device, iv::depth_xu, static_cast<int>(xu_ctrl), buffer, length);
    }

    void get_laser_power(const uvc::device & device, uint8_t & laser_power)
    {
        xu_read(device, IVCAM_DEPTH_LASER_POWER, &laser_power, sizeof(laser_power));
    }

    void set_laser_power(uvc::device & device, uint8_t laser_power)
    {
        xu_write(device, IVCAM_DEPTH_LASER_POWER, &laser_power, sizeof(laser_power));
    }

    void get_accuracy(const uvc::device & device, uint8_t & accuracy)
    {
        xu_read(device, IVCAM_DEPTH_ACCURACY, &accuracy, sizeof(accuracy));
    }

    void set_accuracy(uvc::device & device, uint8_t accuracy)
    {
        xu_write(device, IVCAM_DEPTH_ACCURACY, &accuracy, sizeof(accuracy));
    }

    void get_motion_range(const uvc::device & device, uint8_t & motion_range)
    {
        xu_read(device, IVCAM_DEPTH_MOTION_RANGE, &motion_range, sizeof(motion_range));
    }

    void set_motion_range(uvc::device & device, uint8_t motion_range)
    {
        xu_write(device, IVCAM_DEPTH_MOTION_RANGE, &motion_range, sizeof(motion_range));
    }

    void get_filter_option(const uvc::device & device, uint8_t & filter_option)
    {
        xu_read(device, IVCAM_DEPTH_FILTER_OPTION, &filter_option, sizeof(filter_option));
    }

    void set_filter_option(uvc::device & device, uint8_t filter_option)
    {
        xu_write(device, IVCAM_DEPTH_FILTER_OPTION, &filter_option, sizeof(filter_option));
    }

    void get_confidence_threshold(const uvc::device & device, uint8_t & conf_thresh)
    {
        xu_read(device, IVCAM_DEPTH_CONFIDENCE_THRESH, &conf_thresh, sizeof(conf_thresh));
    }

    void set_confidence_threshold(uvc::device & device, uint8_t conf_thresh)
    {
        xu_write(device, IVCAM_DEPTH_CONFIDENCE_THRESH, &conf_thresh, sizeof(conf_thresh));
    }

    void get_dynamic_fps(const uvc::device & device, uint8_t & dynamic_fps)
    {
        return xu_read(device, IVCAM_DEPTH_DYNAMIC_FPS, &dynamic_fps, sizeof(dynamic_fps));
    }

    void set_dynamic_fps(uvc::device & device, uint8_t dynamic_fps)
    {
        return xu_write(device, IVCAM_DEPTH_DYNAMIC_FPS, &dynamic_fps, sizeof(dynamic_fps));
    }

    ///////////////////
    // USB functions //
    ///////////////////

    void claim_ivcam_interface(uvc::device & device)
    {
        const uvc::guid IVCAM_WIN_USB_DEVICE_GUID = { 0x175695CD, 0x30D9, 0x4F87, {0x8B, 0xE3, 0x5A, 0x82, 0x70, 0xF4, 0x9A, 0x31} };
        claim_interface(device, IVCAM_WIN_USB_DEVICE_GUID, IVCAM_MONITOR_INTERFACE);
    }

    size_t prepare_usb_command(uint8_t * request, size_t & requestSize, uint32_t op, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4, uint8_t * data, size_t dataLength)
    {

        if (requestSize < IVCAM_MONITOR_HEADER_SIZE)
            return 0;

        size_t index = sizeof(uint16_t);
        *(uint16_t *)(request + index) = IVCAM_MONITOR_MAGIC_NUMBER;
        index += sizeof(uint16_t);
        *(uint32_t *)(request + index) = op;
        index += sizeof(uint32_t);
        *(uint32_t *)(request + index) = p1;
        index += sizeof(uint32_t);
        *(uint32_t *)(request + index) = p2;
        index += sizeof(uint32_t);
        *(uint32_t *)(request + index) = p3;
        index += sizeof(uint32_t);
        *(uint32_t *)(request + index) = p4;
        index += sizeof(uint32_t);

        if (dataLength)
        {
            memcpy(request + index, data, dataLength);
            index += dataLength;
        }

        // Length doesn't include header size (sizeof(uint32_t))
        *(uint16_t *)request = (uint16_t)(index - sizeof(uint32_t));
        requestSize = index;
        return index;
    }

    // "Get Version and Date"
    // Reference: Commands.xml in IVCAM_DLL
    void get_gvd(uvc::device & device, std::timed_mutex & mutex, size_t sz, char * gvd)
    {
        hwmon_cmd cmd((uint8_t)fw_cmd::GVD);
        perform_and_send_monitor_command(device, mutex, cmd);
        auto minSize = std::min(sz, cmd.receivedCommandDataLength);
        memcpy(gvd, cmd.receivedCommandData, minSize);
    }

    void get_firmware_version_string(uvc::device & device, std::timed_mutex & mutex, std::string & version)
    {
        std::vector<char> gvd(1024);
        get_gvd(device, mutex, 1024, gvd.data());
        char fws[8];
        memcpy(fws, gvd.data(), 8); // offset 0
        version = std::string(std::to_string(fws[3]) + "." + std::to_string(fws[2]) + "." + std::to_string(fws[1]) + "." + std::to_string(fws[0]));
    }

    void get_module_serial_string(uvc::device & device, std::timed_mutex & mutex, std::string & serial, int offset)
    {
        std::vector<char> gvd(1024);
        get_gvd(device, mutex, 1024, gvd.data());
        unsigned char ss[8];
        memcpy(ss, gvd.data() + offset, 8);
        char formattedBuffer[64];
        if (offset == 96)
        {
            sprintf(formattedBuffer, "%02X%02X%02X%02X%02X%02X", ss[0], ss[1], ss[2], ss[3], ss[4], ss[5]);
            serial = std::string(formattedBuffer);
        }
        else if (offset == 132)
        {
            sprintf(formattedBuffer, "%02X%02X%02X%02X%02X%-2X", ss[0], ss[1], ss[2], ss[3], ss[4], ss[5]);
            serial = std::string(formattedBuffer);
        }
    }

    void force_hardware_reset(uvc::device & device, std::timed_mutex & mutex)
    {
        hwmon_cmd cmd((uint8_t)fw_cmd::HWReset);
        cmd.oneDirection = true;
        perform_and_send_monitor_command(device, mutex, cmd);
    }

    void enable_timestamp(uvc::device & device, std::timed_mutex & mutex, bool colorEnable, bool depthEnable)
    {
        hwmon_cmd cmd((uint8_t)fw_cmd::TimeStampEnable);
        cmd.Param1 = depthEnable ? 1 : 0;
        cmd.Param2 = colorEnable ? 1 : 0;
        perform_and_send_monitor_command(device, mutex, cmd);
    }

    void set_auto_range(uvc::device & device, std::timed_mutex & mutex, int enableMvR, int16_t minMvR, int16_t maxMvR, int16_t startMvR, int enableLaser, int16_t minLaser, int16_t maxLaser, int16_t startLaser, int16_t ARUpperTH, int16_t ARLowerTH)
    {
        hwmon_cmd CommandParameters((uint8_t)fw_cmd::SetAutoRange);
        CommandParameters.Param1 = enableMvR;
        CommandParameters.Param2 = enableLaser;

        auto data = reinterpret_cast<int16_t *>(CommandParameters.data);
        data[0] = minMvR;
        data[1] = maxMvR;
        data[2] = startMvR;
        data[3] = minLaser;
        data[4] = maxLaser;
        data[5] = startLaser;
        auto size = 12;

        if (ARUpperTH != -1)
        {
            data[6] = ARUpperTH;
            size += 2;
        }

        if (ARLowerTH != -1)
        {
            data[7] = ARLowerTH;
            size += 2;
        }

        CommandParameters.sizeOfSendCommandData = size;
        perform_and_send_monitor_command(device, mutex, CommandParameters);
    }

    FirmwareError get_fw_last_error(uvc::device & device, std::timed_mutex & mutex)
    {
        hwmon_cmd cmd((uint8_t)fw_cmd::GetFWLastError);
        memset(cmd.data, 0, 4);

        perform_and_send_monitor_command(device, mutex, cmd);
        return *reinterpret_cast<FirmwareError *>(cmd.receivedCommandData);
    }

} // namespace iv_common
} // namespace rsimpl
