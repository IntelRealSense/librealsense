// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "types.h"
#include <map>

namespace rsimpl {
    namespace ds {
        const uint16_t RS400P_PID = 0x0ad1; // PSR
        const uint16_t RS410A_PID = 0x0ad2; // ASR
        const uint16_t RS420R_PID = 0x0ad3; // ASRC
        const uint16_t RS430C_PID = 0x0ad4; // AWG
        const uint16_t RS450T_PID = 0x0ad5; // AWGT

        // DS5 depth XU identifiers
        const uint8_t DS5_HWMONITOR             = 1;
        const uint8_t DS5_DEPTH_EMITTER_ENABLED = 2;
        const uint8_t DS5_EXPOSURE              = 3;
        const uint8_t DS5_LASER_POWER           = 4;

        // DS5 fisheye XU identifiers
        const uint8_t FISHEYE_EXPOSURE = 1;

        const int gvd_fw_version_offset = 12;

        const uvc::extension_unit depth_xu = { 0, 3, 2,
        { 0xC9606CCB, 0x594C, 0x4D25,{ 0xaf, 0x47, 0xcc, 0xc4, 0x96, 0x43, 0x59, 0x95 } } };

        const uvc::extension_unit fisheye_xu = { 3, 3, 2,
        { 0xf6c3c3d1, 0x5cde, 0x4477,{ 0xad, 0xf0, 0x41, 0x33, 0xf5, 0x8d, 0xa6, 0xf4 } } };

        enum fw_cmd : uint8_t
        {
            GVD = 0x10,
            GETINTCAL = 0x15,     // Read calibration table
        };

        struct table_header
        {
            big_endian<uint16_t>    version;        // major.minor. Big-endian
            uint16_t                table_type;     // ctCalibration
            uint32_t                table_size;     // full size including: TOC header + TOC + actual tables
            uint32_t                param;          // This field content is defined ny table type
            uint32_t                crc32;          // crc of all the actual table data excluding header/CRC
        };

        enum ds5_rect_resolutions : unsigned short
        {
            res_1920_1080,
            res_1280_720,
            res_640_480,
            res_848_480,
            res_640_360,
            res_424_240,
            res_320_240,
            res_480_270,
            reserved_1,
            reserved_2,
            reserved_3,
            reserved_4,
            max_ds5_rect_resoluitons
        };

        struct coefficients_table
        {
            table_header        header;
            float3x3            intrinsic_left;             //  left camera intrinsic data, normilized
            float3x3            intrinsic_right;            //  right camera intrinsic data, normilized
            float3x3            world2left_rot;             //  the inverse rotation of the left camera
            float3x3            world2right_rot;            //  the inverse rotation of the right camera
            float               baseline;                   //  the baseline between the cameras
            uint8_t             reserved1[88];
            uint32_t            brown_model;                // 0 - using DS distorion model, 1 - using Brown model
            float4              rect_params[max_ds5_rect_resoluitons];
            uint8_t             reserved2[64];
        };

        enum calibration_table_id
        {
            coefficients_table_id = 25,
            depth_calibration_id = 31,
            rgb_calibration_id = 32,
            fisheye_calibration_id = 33,
            imu_calibration_id = 34,
            lens_shading_id = 35,
            projector_id = 36
        };

        struct ds5_calibration
        {
            uint16_t        version;                        // major.minor
            rs_intrinsics   left_imager_intrinsic;
            rs_intrinsics   right_imager_intrinsic;
            rs_intrinsics   depth_intrinsic[max_ds5_rect_resoluitons];
            rs_extrinsics   left_imager_extrinsic;
            rs_extrinsics   right_imager_extrinsic;
            rs_extrinsics   depth_extrinsic;
            std::map<calibration_table_id, bool> data_present;

            ds5_calibration() : version(0), left_imager_intrinsic({}), right_imager_intrinsic({}),
                left_imager_extrinsic({}), right_imager_extrinsic({}), depth_extrinsic({})
            {
                for (auto i = 0; i < max_ds5_rect_resoluitons; i++)
                    depth_intrinsic[i] = {};
                data_present.emplace(coefficients_table_id, false);
                data_present.emplace(depth_calibration_id, false);
                data_present.emplace(rgb_calibration_id, false);
                data_present.emplace(fisheye_calibration_id, false);
                data_present.emplace(imu_calibration_id, false);
                data_present.emplace(lens_shading_id, false);
                data_present.emplace(projector_id, false);
            };
        };

        static std::map< ds5_rect_resolutions, int2> resolutions_list = {
            { res_320_240,{ 320, 240 } },
            { res_424_240,{ 424, 240 } },
            { res_480_270,{ 480, 270 } },
            { res_640_360,{ 640, 360 } },
            { res_640_480,{ 640, 480 } },
            { res_848_480,{ 848, 480 } },
            { res_1280_720,{ 1280, 720 } },
            { res_1920_1080,{ 1920, 1080 } },
        };

        inline ds5_rect_resolutions width_height_to_ds5_rect_resolutions(uint32_t width, uint32_t height)
        {
            for (auto& elem : resolutions_list)
            {
                if (elem.second.x == width && elem.second.y == height)
                    return elem.first;
            }
            throw std::runtime_error("resolution not found.");
        }

        rs_intrinsics get_intrinsic_by_resolution(const std::vector<unsigned char> & raw_data, calibration_table_id table_id, uint32_t width, uint32_t height);

        //static std::vector<uvc::uvc_device_info> filter_by_product(const std::vector<uvc::uvc_device_info>& devices, uint32_t pid)
        //{
        //    std::vector<uvc::uvc_device_info> result;
        //    for (auto&& info : devices)
        //    {
        //        if (info.pid == pid) result.push_back(info);
        //    }
        //    return result;
        //}

        //static std::vector<std::vector<uvc::uvc_device_info>> group_by_unique_id(const std::vector<uvc::uvc_device_info>& devices)
        //{
        //    std::map<std::string, std::vector<uvc::uvc_device_info>> map;
        //    for (auto&& info : devices)
        //    {
        //        map[info.unique_id].push_back(info);
        //    }
        //    std::vector<std::vector<uvc::uvc_device_info>> result;
        //    for (auto&& kvp : map)
        //    {
        //        result.push_back(kvp.second);
        //    }
        //    return result;
        //}

        //static void trim_device_list(std::vector<uvc::uvc_device_info>& devices, const std::vector<uvc::uvc_device_info>& chosen)
        //{
        //    if (chosen.empty())
        //        return;

        //    auto was_chosen = [&chosen](const uvc::uvc_device_info& info)
        //    {
        //        return find(chosen.begin(), chosen.end(), info) == chosen.end();
        //    };
        //    devices.erase(std::remove_if(devices.begin(), devices.end(), was_chosen), devices.end());
        //}

        //static bool mi_present(const std::vector<uvc::uvc_device_info>& devices, uint32_t mi)
        //{
        //    for (auto&& info : devices)
        //    {
        //        if (info.mi == mi) return true;
        //    }
        //    return false;
        //}

        //static uvc::uvc_device_info get_mi(const std::vector<uvc::uvc_device_info>& devices, uint32_t mi)
        //{
        //    for (auto&& info : devices)
        //    {
        //        if (info.mi == mi) return info;
        //    }
        //    throw std::runtime_error("Interface not found!");
        //}

        static bool try_fetch_usb_device(std::vector<uvc::usb_device_info>& devices,
            const uvc::uvc_device_info& info, uvc::usb_device_info& result)
        {
            for (auto it = devices.begin(); it != devices.end(); ++it)
            {
                if (it->unique_id == info.unique_id)
                {
                    result = *it;
                    switch (info.pid)
                    {
                    case RS400P_PID:
                    case RS430C_PID:
                    case RS410A_PID:
                        result.mi = 3;
                        break;
                    case RS420R_PID:
                        throw std::runtime_error("not implemented.");
                        break;
                    case RS450T_PID:
                        result.mi = 6;
                        break;
                    default:
                        throw std::runtime_error("not implemented.");
                        break;
                    }

                    devices.erase(it);
                    return true;
                }
            }
            return false;
        }

    } // rsimpl::ds
} // namespace rsimpl
