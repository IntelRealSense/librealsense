// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <algorithm>

#include "hw-monitor.h"
#include "ds-private.h"
#include "ds5-private.h"

using namespace rsimpl::hw_monitor;
using namespace rsimpl::ds5;

namespace rsimpl {
namespace ds5 {

    const uvc::extension_unit depth_xu = { 0, 2, 1,{ 0xC9606CCB, 0x594C, 0x4D25,{ 0xaf, 0x47, 0xcc, 0xc4, 0x96, 0x43, 0x59, 0x95 } } };
    const uvc::guid DS5_WIN_USB_DEVICE_GUID = { 0x08090549, 0xCE78, 0x41DC,{ 0xA0, 0xFB, 0x1B, 0xD6, 0x66, 0x94, 0xBB, 0x0C } };

    enum class fw_cmd : uint8_t
    {
        GVD = 0x10
    };

    const uint8_t DS5_MONITOR_INTERFACE                 = 0x3;
    const uint8_t DS5_MOTION_MODULE_INTERRUPT_INTERFACE = 0x4;


    void claim_ds5_monitor_interface(uvc::device & device)
    {
        claim_interface(device, DS5_WIN_USB_DEVICE_GUID, DS5_MONITOR_INTERFACE);
    }

    void claim_ds5_motion_module_interface(uvc::device & device)
    {
        const uvc::guid MOTION_MODULE_USB_DEVICE_GUID = {};
        claim_aux_interface(device, MOTION_MODULE_USB_DEVICE_GUID, DS5_MOTION_MODULE_INTERRUPT_INTERFACE);
    }

    // "Get Version and Date"
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
        memcpy(fws, gvd.data() + gvd_fields::fw_version_offset, sizeof(uint32_t)); // Four-bytes at address 0x20C
        version = std::string(std::to_string(fws[3]) + "." + std::to_string(fws[2]) + "." + std::to_string(fws[1]) + "." + std::to_string(fws[0]));
    }

    void get_module_serial_string(uvc::device & device, std::timed_mutex & mutex, std::string & serial, unsigned int offset)
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

    void get_laser_power(const uvc::device & device, uint8_t & laser_power)
    {
        ds::xu_read(device, depth_xu, ds::control::ds5_laser_power, &laser_power, sizeof(uint8_t));
    }

    void set_laser_power(uvc::device & device, uint8_t laser_power)
    {
        ds::xu_write(device, depth_xu, ds::control::ds5_laser_power, &laser_power, sizeof(uint8_t));
    }

} // namespace rsimpl::ds5
} // namespace rsimpl
