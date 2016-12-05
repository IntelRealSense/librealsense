// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "backend.h"
#include <vector>

namespace rsimpl
{
    class device;

    class device_info
    {
    public:
        virtual std::shared_ptr<device> create(const uvc::backend& backend) const = 0;
        virtual std::shared_ptr<device_info> clone() const = 0;

        virtual ~device_info() = default;
    };

    enum class backend_type
    {
        standard,
        record,
        playback
    };

    class context
    {
    public:
        explicit context(backend_type type, 
               const char* filename = nullptr, 
               const char* section = nullptr, 
               rs_recording_mode mode = RS_RECORDING_MODE_COUNT);

        std::vector<std::shared_ptr<device_info>> query_devices() const;
        const uvc::backend& get_backend() const { return *_backend; }
    private:
        std::shared_ptr<uvc::backend> _backend;
    };

    static std::vector<uvc::uvc_device_info> filter_by_product(const std::vector<uvc::uvc_device_info>& devices, const std::vector<uint16_t>& pid_list)
    {
        std::vector<uvc::uvc_device_info> result;
        for (auto&& info : devices)
        {
            if (pid_list.end() != std::find(pid_list.begin(), pid_list.end(), info.pid))
                result.push_back(info);
        }
        return result;
    }

    static std::vector<std::pair<std::vector<uvc::uvc_device_info>, std::vector<uvc::hid_device_info>>> group_by_busnum_and_port_id(const std::vector<std::vector<uvc::uvc_device_info>>& devices,
                                                                                                                         const std::vector<uvc::hid_device_info>& hids)
    {
        std::vector<std::pair<std::vector<uvc::uvc_device_info>, std::vector<uvc::hid_device_info>>> results;
        for (auto&& dev : devices)
        {
            std::vector<uvc::hid_device_info> hid_group;
            auto port_id = dev.front().port_id;
            auto busnum = dev.front().busnum;
            for (auto&& hid : hids)
            {
                if (hid.port_id == port_id && hid.busnum == busnum)
                    hid_group.push_back(hid);
            }
            results.push_back(std::make_pair(dev, hid_group));
        }
        return results;
    }

    static std::vector<std::vector<uvc::uvc_device_info>> group_by_unique_id(const std::vector<uvc::uvc_device_info>& devices)
    {
        std::map<std::string, std::vector<uvc::uvc_device_info>> map;
        for (auto&& info : devices)
        {
            map[info.unique_id].push_back(info);
        }
        std::vector<std::vector<uvc::uvc_device_info>> result;
        for (auto&& kvp : map)
        {
            result.push_back(kvp.second);
        }
        return result;
    }

    static void trim_device_list(std::vector<uvc::uvc_device_info>& devices, const std::vector<uvc::uvc_device_info>& chosen)
    {
        if (chosen.empty())
            return;

        auto was_chosen = [&chosen](const uvc::uvc_device_info& info)
        {
            return find(chosen.begin(), chosen.end(), info) == chosen.end();
        };
        devices.erase(std::remove_if(devices.begin(), devices.end(), was_chosen), devices.end());
    }

    static bool mi_present(const std::vector<uvc::uvc_device_info>& devices, uint32_t mi)
    {
        for (auto&& info : devices)
        {
            if (info.mi == mi) return true;
        }
        return false;
    }

    static uvc::uvc_device_info get_mi(const std::vector<uvc::uvc_device_info>& devices, uint32_t mi)
    {
        for (auto&& info : devices)
        {
            if (info.mi == mi) return info;
        }
        throw std::runtime_error("Interface not found!");
    }

    static std::vector<uvc::uvc_device_info> filter_by_mi(const std::vector<uvc::uvc_device_info>& devices, uint32_t mi)
    {
        std::vector<uvc::uvc_device_info> results;
        for (auto&& info : devices)
        {
            if (info.mi == mi) results.push_back(info);
        }
        return results;
    }
}
