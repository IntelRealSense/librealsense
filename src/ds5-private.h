// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"

namespace rsimpl {
    namespace ds {
        // DS5 depth XU identifiers
        const uint8_t DS5_HWMONITOR = 1;
        const uint8_t DS5_DEPTH_LASER_POWER = 2;
        const uint8_t DS5_EXPOSURE = 3;

        const uvc::extension_unit depth_xu = { 0, 3, 2,
        { 0xC9606CCB, 0x594C, 0x4D25,{ 0xaf, 0x47, 0xcc, 0xc4, 0x96, 0x43, 0x59, 0x95 } } };

        enum fw_cmd : uint8_t
        {
            GVD = 0x10,
        };

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
                    result.mi = 3;
                    devices.erase(it);
                    return true;
                }
            }
            return false;
        }

    } // rsimpl::ivcam
} // namespace rsimpl
