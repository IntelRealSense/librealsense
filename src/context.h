// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "backend.h"
#include "recorder.h"
#include <vector>

namespace rsimpl2
{
    class device;

    class device_info
    {
    public:
        std::shared_ptr<device> get_device() const
        {
            return *_device;
        }

        virtual uint8_t get_subdevice_count() const = 0;

        virtual ~device_info() = default;

    protected:
        device_info(std::shared_ptr<uvc::backend> backend)
            : _backend(std::move(backend)),
              _device([this]() { return create(*_backend); })
        {}

        virtual std::shared_ptr<device> create(const uvc::backend& backend) const = 0;

        lazy<std::shared_ptr<device>> _device;
        std::shared_ptr<uvc::backend> _backend;
    };

    enum class backend_type
    {
        standard,
        record,
        playback
    };


    class recovery_info : public device_info
    {
    public:
        std::shared_ptr<device> create(const uvc::backend& /*backend*/) const override
        {
            throw unrecoverable_exception(RECOVERY_MESSAGE,
                RS2_EXCEPTION_TYPE_DEVICE_IN_RECOVERY_MODE);
        }

        uint8_t get_subdevice_count() const override
        {
            return 1;
        }

        static bool is_recovery_pid(uint16_t pid)
        {
            return pid == 0x0ADB || pid == 0x0AB3;
        }

        static std::vector<std::shared_ptr<device_info>> pick_recovery_devices(
            const std::shared_ptr<uvc::backend>& backend,
            const std::vector<uvc::usb_device_info>& usb_devices)
        {
            std::vector<std::shared_ptr<device_info>> list;
            for (auto&& usb : usb_devices)
            {
                if (is_recovery_pid(usb.pid))
                {
                    list.push_back(std::make_shared<recovery_info>(backend, usb));
                }
            }
            return list;
        }

        explicit recovery_info(std::shared_ptr<uvc::backend> backend,
                               uvc::usb_device_info dfu)
            : device_info(backend), _dfu(std::move(dfu)) {}

    private:
        uvc::usb_device_info _dfu;
        const char* RECOVERY_MESSAGE = "Selected RealSense device is in recovery mode!\nEither perform a firmware update or reconnect the camera to fall-back to last working firmware if available!";
    };

    class context
    {
    public:
        explicit context(backend_type type,
               const char* filename = nullptr,
               const char* section = nullptr,
               rs2_recording_mode mode = RS2_RECORDING_MODE_COUNT);

        std::vector<std::shared_ptr<device_info>> query_devices() const;
        const uvc::backend& get_backend() const { return *_backend; }
        double get_time();

    private:
        std::shared_ptr<uvc::backend> _backend;
        std::shared_ptr<uvc::time_service> _ts;
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

    static std::vector<std::pair<std::vector<uvc::uvc_device_info>, std::vector<uvc::hid_device_info>>> group_devices_and_hids_by_unique_id(const std::vector<std::vector<uvc::uvc_device_info>>& devices,
                                                                                                                         const std::vector<uvc::hid_device_info>& hids)
    {
        std::vector<std::pair<std::vector<uvc::uvc_device_info>, std::vector<uvc::hid_device_info>>> results;
        for (auto&& dev : devices)
        {
            std::vector<uvc::hid_device_info> hid_group;
            auto unique_id = dev.front().unique_id;
            for (auto&& hid : hids)
            {
                if (hid.unique_id == unique_id || hid.unique_id == "*")
                    hid_group.push_back(hid);
            }
            results.push_back(std::make_pair(dev, hid_group));
        }
        return results;
    }

    static std::vector<std::vector<uvc::uvc_device_info>> group_devices_by_unique_id(const std::vector<uvc::uvc_device_info>& devices)
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
            return find(chosen.begin(), chosen.end(), info) != chosen.end();
        };
        devices.erase(std::remove_if(devices.begin(), devices.end(), was_chosen), devices.end());
    }

    static bool mi_present(const std::vector<uvc::uvc_device_info>& devices, uint32_t mi)
    {
        for (auto&& info : devices)
        {
            if (info.mi == mi)
                return true;
        }
        return false;
    }

    static uvc::uvc_device_info get_mi(const std::vector<uvc::uvc_device_info>& devices, uint32_t mi)
    {
        for (auto&& info : devices)
        {
            if (info.mi == mi)
                return info;
        }
        throw invalid_value_exception("Interface not found!");
    }

    static std::vector<uvc::uvc_device_info> filter_by_mi(const std::vector<uvc::uvc_device_info>& devices, uint32_t mi)
    {
        std::vector<uvc::uvc_device_info> results;
        for (auto&& info : devices)
        {
            if (info.mi == mi)
                results.push_back(info);
        }
        return results;
    }
}
