// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <algorithm>
#include <iostream>
#include <fstream>      // std::ifstream, std::ofstream

#include "types.h"
#include "hw-monitor.h"
#include "ds-private.h"
#include "ds5-private.h"

#define stringify( name ) # name

using namespace rsimpl::hw_monitor;
using namespace rsimpl::ds5;

namespace rsimpl {
namespace ds5 {

    const uvc::extension_unit depth_xu = { 0, 2, 1,{ 0xC9606CCB, 0x594C, 0x4D25,{ 0xaf, 0x47, 0xcc, 0xc4, 0x96, 0x43, 0x59, 0x95 } } };
    const uvc::guid DS5_WIN_USB_DEVICE_GUID = { 0x08090549, 0xCE78, 0x41DC,{ 0xA0, 0xFB, 0x1B, 0xD6, 0x66, 0x94, 0xBB, 0x0C } };

    enum fw_cmd : uint8_t
    {
        GVD         = 0x10,
        GETINTCAL   = 0x15,     // Read calibration table
    };

    enum calibration_table_id
    {
        depth_coefficients_id   =   25, // TBD
        depth_calibration_id    =   31,
        rgb_calibration_id      =   32,
        fisheye_calibration_id  =   33,
        imu_calibration_id      =   34,
        lens_shading_id         =   35,
        projector_id            =   36
    };

#pragma pack(push, 1)
    struct table_header
    {
        uint16_t        version;        // major.minor
        uint16_t        table_type;     // ctCalibration
        uint32_t        table_size;     // full size including: TOC header + TOC + actual tables
        uint32_t        param;          // This field is determined uniquely by each table
        uint32_t        crc32;          // crc of all the actual table data excluding header/CRC
    };


    struct table_link
    {
        uint16_t        table_id;       // enumerated table id
        uint16_t        param;          // This field is determined uniquely by each table
        uint32_t        offset;         // table actual location offset (address) in the flash memory
    };

    struct eprom_table
    {
        table_header    header;
        table_link      optical_module_info;   // enumerated table id
        table_link      calib_table_info;      // index of the depth camera in case of multiple instances
    };

    /* Used for iterating through internal TOC stored on the flash*/
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
        table_link          toc[max_ds5_intrinsic_table];   // list of all data tables stored on flash
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

    struct coefficients_table
    {
        table_header        header;
        float3x3            intrinsic_left;             //  left camera intrinsic data, normilized
        float3x3            intrinsic_right;            //  right camera intrinsic data, normilized
        float3x3            world2left_rot;             //  the inverse rotation of the left camera
        float3x3            world2right_rot;            //  the inverse rotation of the right camera
        float               baseline;                   //  the baseline between the cameras
        float4              rect_params[max_resoluitons];
        uint8_t             reserved[156];
    };

    struct depth_calibration_table
    {
        table_header        header;
        float               r_max;                      //  the maximum depth value in mm corresponding to 65535
        float3x3            k_left;                     //  Left intrinsic matrix, normalize by [-1 1]
        float               distortion_left[5];         //  Left forward distortion parameters, brown model
        float               r_left[3];                  //  Left rotation angles (Rodrigues)
        float               t_left[3];                  //  Left translation vector, mm
        float3x3            k_right;                    //  Right intrinsic matrix, normalize by [-1 1]
        float               distortion_right[5];        //  Right forward distortion parameters, brown model
        float               r_right[3];                 //  Right rotation angles (Rodrigues)
        float               t_right[3];                 //  Right translation vector, mm
        float3x3            k_depth;                    //  the Depth camera intrinsic matrix
        float               r_depth[3];                 //  Depth rotation angles (Rodrigues)
        float               t_depth[3];                 //  Depth translation vector, mm
        unsigned char       reserved[16];
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

    void get_calibration_table_entry(uvc::device & device, std::timed_mutex & mutex,calibration_table_id table_id, std::vector<unsigned char> & raw_data)
    {
        hwmon_cmd cmd((uint8_t)fw_cmd::GETINTCAL);
        cmd.Param1 = table_id;
        perform_and_send_monitor_command(device, mutex, cmd);
        raw_data.clear();
        raw_data.resize(cmd.receivedCommandDataLength);
        memcpy(raw_data.data(), cmd.receivedCommandData, cmd.receivedCommandDataLength);
    }

    template<typename T, int sz>
    int arr_size(T(&)[sz])
    {
        return sz;
    }

    template<typename T>
    const std::string array2str(T& data)
    {
        std::stringstream ss;
        for (int i = 0; i < arr_size(data); i++)
            ss << " [" << i << "] = " << data[i];
        return std::string(ss.str().c_str());
    }

    typedef float float_9[9];
    typedef float float_4[4];

    void parse_calibration_table(ds5_calibration& calib, calibration_table_id table_id, std::vector<unsigned char> & raw_data)
    {
        switch (table_id)
        {
        case depth_coefficients_id:
        {
            assert(raw_data.size() >= sizeof(coefficients_table));
            coefficients_table table = *reinterpret_cast<coefficients_table *>(raw_data.data());
            LOG_DEBUG("Table header: table version major.minor: " << std::hex  << table.header.version     << std::dec
                << ",table type " << table.header.table_type          << ", size "    << table.header.table_size
                << ", calibration version [mjr.mnr.ptch.rev]: " << std::hex     << table.header.version
                << ", CRC: " << table.header.crc32);
            LOG_DEBUG(stringify(intrinsic_left) << array2str((float_9&)table.intrinsic_left) << std::endl
                << stringify(intrinsic_right) << array2str((float_9&)table.intrinsic_right) << std::endl
                << stringify(world2left_rot) << array2str((float_9&)table.world2left_rot) << std::endl
                << stringify(world2right_rot) << array2str((float_9&)table.world2right_rot) << std::endl
                << stringify(world2left_rot) << array2str((float_9&)table.world2left_rot) << std::endl
                << "baseline = " << table.baseline << std::endl
                << stringify(res_1920_1080) << array2str((float_4&)table.rect_params[res_1920_1080]) << std::endl
                << stringify(res_1280_720) << array2str((float_4&)table.rect_params[res_1280_720]) << std::endl
                << stringify(res_640_480) << array2str((float_4&)table.rect_params[res_640_480]) << std::endl
                << stringify(res_854_480) << array2str((float_4&)table.rect_params[res_854_480]) << std::endl
                << stringify(res_640_360) << array2str((float_4&)table.rect_params[res_640_360]) << std::endl
                << stringify(res_432_240) << array2str((float_4&)table.rect_params[res_432_240]) << std::endl
                << stringify(res_320_240) << array2str((float_4&)table.rect_params[res_320_240]) << std::endl
                << stringify(res_480_270) << array2str((float_4&)table.rect_params[res_480_270]));

            // Fill in actual data
            for (auto i = 0; i < max_resoluitons; i++)
            {
                auto it = std::find_if(resolutions_list.begin(), resolutions_list.end(), [i](resolution_def res) { return i == res.name; });
                if (it != resolutions_list.end())
                {
                    calib.leftImagerIntrinsic[i].width  = calib.rightImagerIntrinsic[i].width = (*it).dims.x;
                    calib.leftImagerIntrinsic[i].height = calib.rightImagerIntrinsic[i].height = (*it).dims.y;

                    calib.leftImagerIntrinsic[i].fx         = table.intrinsic_left(0, 0);   // focal length of the image plane, as a multiple of pixel width
                    calib.leftImagerIntrinsic[i].fy         = table.intrinsic_left(0, 1);   // focal length of the image plane, as a multiple of pixel height
                    calib.leftImagerIntrinsic[i].ppx        = table.intrinsic_left(0, 2);   // horizontal coordinate of the principal point of the image, as a pixel offset from the left edge
                    calib.leftImagerIntrinsic[i].ppy        = table.intrinsic_left(1, 0);   // horizontal coordinate of the principal point of the image, as a pixel offset from the top edge
                    calib.leftImagerIntrinsic[i].coeffs[0]  = table.intrinsic_left(1, 1);   // Distortion coeeficients 1-5
                    calib.leftImagerIntrinsic[i].coeffs[1]  = table.intrinsic_left(1, 2);
                    calib.leftImagerIntrinsic[i].coeffs[2]  = table.intrinsic_left(2, 0);
                    calib.leftImagerIntrinsic[i].coeffs[3]  = table.intrinsic_left(2, 1);
                    calib.leftImagerIntrinsic[i].coeffs[4]  = table.intrinsic_left(2, 2);
                    calib.leftImagerIntrinsic[i].model = rs_distortion::RS_DISTORTION_COUNT;    // TBD Evgeni

                    calib.rightImagerIntrinsic[i].fx        = table.intrinsic_left(0, 0);   // 1st element
                    calib.rightImagerIntrinsic[i].fy        = table.intrinsic_left(0, 1);   // 2nd
                    calib.rightImagerIntrinsic[i].ppx       = table.intrinsic_left(0, 2);   // 3rd...
                    calib.rightImagerIntrinsic[i].ppy       = table.intrinsic_left(1, 0);
                    calib.rightImagerIntrinsic[i].coeffs[0] = table.intrinsic_left(1, 1);
                    calib.rightImagerIntrinsic[i].coeffs[1] = table.intrinsic_left(1, 2);
                    calib.rightImagerIntrinsic[i].coeffs[2] = table.intrinsic_left(2, 0);
                    calib.rightImagerIntrinsic[i].coeffs[3] = table.intrinsic_left(2, 1);
                    calib.rightImagerIntrinsic[i].coeffs[4] = table.intrinsic_left(2, 2);
                    calib.rightImagerIntrinsic[i].model = rs_distortion::RS_DISTORTION_COUNT;   // TBD Evgeni

                    // TODO where to appy rPY, rFy values ??? table  Evgeni
                    ///table.rect_params[i].   .......
                }
            }
        }
        break;
        case depth_calibration_id:
        {
            assert(raw_data.size() >= sizeof(depth_calibration_table));
            depth_calibration_table *table = reinterpret_cast<depth_calibration_table *>(raw_data.data());
            LOG_DEBUG("Table header: version " << table->header.version
                << ",type " << table->header.table_type << ", size " << table->header.table_size << ", max depth value [mm] " << table->r_max << std::endl
                << stringify(distortion_left) << (float_9&)table->distortion_left
                << stringify(distortion_right) << (float_9&)table->distortion_right
                << stringify(k_depth) << (float_9&)table->k_depth
                << stringify(r_depth) << table->r_depth
                << stringify(t_depth) << table->t_depth);

            // Fill in actual data. Note that only the Focal and Principal points data varies between different resolutions
            for (auto i = 0; i < max_resoluitons; i++)
            {
                auto it = std::find_if(resolutions_list.begin(), resolutions_list.end(), [i](resolution_def res) { return i == res.name; });
                if (it != resolutions_list.end())
                {
                    calib.leftImagerIntrinsic[i].width  = (*it).dims.x;
                    calib.leftImagerIntrinsic[i].height = (*it).dims.y;

                    calib.depthIntrinsic[i].fx          = table->k_depth(0, 0); // TBD - Where are FP and PP parameters per resolution for Depth ?
                    calib.depthIntrinsic[i].fy          = table->k_depth(0, 1); // TBD
                    calib.depthIntrinsic[i].ppx         = table->k_depth(0, 2); // TBD
                    calib.depthIntrinsic[i].ppy         = table->k_depth(1, 0); // TBD
                    calib.depthIntrinsic[i].coeffs[0]   = table->k_depth(1, 1);
                    calib.depthIntrinsic[i].coeffs[1]   = table->k_depth(1, 2);
                    calib.depthIntrinsic[i].coeffs[2]   = table->k_depth(2, 0);
                    calib.depthIntrinsic[i].coeffs[3]   = table->k_depth(2, 1);
                    calib.depthIntrinsic[i].coeffs[4]   = table->k_depth(2, 2);
                    calib.depthIntrinsic[i].model       = rs_distortion::RS_DISTORTION_COUNT;   // TBD - Model definition is missing
                }
            }
        }
        break;
        default:
            LOG_WARNING("Parsing Calibration table type " << table_id << " is not supported yet");
        }
    }

    ds5_calibration read_calibration(uvc::device & dev, std::timed_mutex & mutex)
    {
        bool mock = true; // Use offline data for verification. TODO - remove before release. Evgeni
        ds5_calibration calib;
        memset(&calib, 0, sizeof(ds5_calibration));

        std::vector<unsigned char> table_raw_data;
        const std::vector<calibration_table_id> actual_list = { depth_calibration_id, depth_coefficients_id, rgb_calibration_id, fisheye_calibration_id, imu_calibration_id, lens_shading_id, projector_id };  // Will be extended as FW matures

        for (auto id : actual_list)     // Fetch and parse calibration data
        {
            std::stringstream ss;
            ss << "Table_" << id;
            table_raw_data.clear();
            if (mock)
            {
                std::ifstream infile(ss.str().c_str(), std::ifstream::binary);
                if (!infile.good())
                    throw std::runtime_error(to_string() << "Mock calibration file is missing in runtime directory : " << ss.str().c_str());
                // get size of file
                infile.seekg(0, infile.end);
                std::streampos size = infile.tellg();
                if ((int)size==0)
                    throw std::runtime_error(to_string() << "Zero-size calibration file : " << ss.str().c_str() << " execution is aborted");
                infile.seekg(0);
                // read content of infile
                table_raw_data.resize(size);
                // todo - verify the content of the file using the CRC function applied by FW
                infile.read((char*)table_raw_data.data(), size);
                infile.close();
            }
            else
                get_calibration_table_entry(dev, mutex, id, table_raw_data);

            parse_calibration_table( calib, id, table_raw_data);
        }
        return calib;
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
