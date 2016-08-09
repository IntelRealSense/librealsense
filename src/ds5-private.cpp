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
        FRB = 0x09,     // Read from Flash
        GVD = 0x10,
    };


#pragma pack(push, 1)
    struct table_header
    {
        uint16_t    version;        // major.minor
        uint16_t    table_type;     // ctCalibration
        uint32_t    table_size;     // full size including: TOC header + TOC + actual tables
        uint32_t    optional;
        uint32_t    crc32;          // crc of all the actual table data excluding header/CRC
    };

    struct toc_entry
    {
        uint16_t    table_id;       // enumerated table id
        uint16_t    cam_index;      // index of the depth camera in case of multiple instances
        uint32_t    offset;         // table actual location offset (address) in the flash memory
    };

    enum data_tables
    {
        depth_calibration_a,
        depth_coefficients_a,
        rgb_calibration,
        fisheye_calibration,
        imu_calibration,
        lens_shading,
        projector,
        max_ds5_intrinsic_table
    };

    struct calibrations_header
    {
        table_header        toc_header;
        toc_entry           toc[max_ds5_intrinsic_table];   // list of all data tables stored on flash
    };

    enum m3_3_field
    {
        f_1_1           = 0,
        f_1_2           = 1,
        f_1_3           = 2,
        f_2_1           = 3,
        f_2_2           = 4,
        f_2_3           = 5,
        f_3_1           = 6,
        f_3_2           = 7,
        f_3_3           = 8,
        f_x             = 0,
        f_y             = 1,
        p_x             = 2,
        p_y             = 3,
        distortion_0    = 4,
        distortion_1    = 5,
        distortion_2    = 6,
        distortion_3    = 7,
        distortion_4    = 8,
        max_m3_3_field  = 9
    };

    typedef float   rect_entry[4];
    typedef float   matrix_entry[max_m3_3_field] ;

    enum resolutions
    {
        res_1920_1080,
        res_1280_720,
        res_640_480,
        res_854_480,
        res_640_360,
        res_432_240,
        res_320_240,
        res_480_270,
        reserved_1,
        reserved_2,
        reserved_3,
        reserved_4,
        max_resoluitons
    };

    struct coefficients_table
    {
        table_header        header;
        matrix_entry        intrinsic_left;             //  left camera intrinsic data, normilized
        matrix_entry        intrinsic_right;            //  right camera intrinsic data, normilized
        matrix_entry        world2left_rot;             //  the inverse rotation of the left camera
        matrix_entry        world2right_rot;            //  the inverse rotation of the right camera
        matrix_entry        baseline;                   //  the baseline between the cameras
        rect_entry          rect_list[max_resoluitons];
        float               reserved[156];
    };

    struct depth_calibration_table
    {
        table_header        header;
        float               r_max;                      //  the maximum depth value in mm corresponding to 65535
        matrix_entry        k_left;                     //  Left intrinsic matrix, normalize by [-1 1]
        float               distortion_left[5];         //  Left forward distortion parameters, brown model
        float               r_left[3];                  //  Left rotation angles (Rodrigues)
        float               t_left[3];                  //  Left translation vector, mm
        matrix_entry        k_right;                    //  Right intrinsic matrix, normalize by [-1 1]
        float               distortion_right[5];        //  Right forward distortion parameters, brown model
        float               r_right[3];                 //  Right rotation angles (Rodrigues)
        float               t_right[3];                 //  Right translation vector, mm
        matrix_entry        k_depth;                    //  the Depth camera intrinsic matrix
        float               r_depth[3];                 //  Depth rotation angles (Rodrigues)
        float               t_depth[3];                 //  Depth translation vector, mm
        float               reserved[16];
    };


#pragma pack(pop)

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
