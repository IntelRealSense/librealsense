// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "backend.h"
#include "mock/recorder.h"
#include "core/streaming.h"

#include <vector>

namespace rsimpl2
{
    class context;
    class device_info;
}

struct rs2_device_info
{
    std::shared_ptr<rsimpl2::context> ctx;
    std::shared_ptr<rsimpl2::device_info> info;
};


struct rs2_device_list
{
    std::shared_ptr<rsimpl2::context> ctx;
    std::vector<rs2_device_info> list;
};

struct rs2_sensor_list
{
    std::shared_ptr<rsimpl2::device_interface> dev;
};

namespace rsimpl2
{
    class device;
    class context;
    class device_info;


    class device_info
    {
    public:
        std::shared_ptr<device_interface> create_device() const
        {
            return create(*_backend);
        }

        virtual ~device_info() = default;


        virtual uvc::devices_data get_device_data()const = 0;

        bool operator==(const device_info& other) const
        {
            return other.get_device_data() == get_device_data();
        }

    protected:
        explicit device_info(std::shared_ptr<uvc::backend> backend)
            : _backend(std::move(backend))
        {}

        virtual std::shared_ptr<device_interface> create(const uvc::backend& backend) const = 0;

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
        std::shared_ptr<device_interface> create(const uvc::backend& /*backend*/) const override
        {
            throw unrecoverable_exception(RECOVERY_MESSAGE,
                RS2_EXCEPTION_TYPE_DEVICE_IN_RECOVERY_MODE);
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

        uvc::devices_data get_device_data()const override
        {
            return uvc::devices_data({ _dfu });
        }

    private:
        uvc::usb_device_info _dfu;
        const char* RECOVERY_MESSAGE = "Selected RealSense device is in recovery mode!\nEither perform a firmware update or reconnect the camera to fall-back to last working firmware if available!";
    };

    class platform_camera_info : public device_info
    {
    public:
        std::shared_ptr<device_interface> create(const uvc::backend& /*backend*/) const override;

        static std::vector<std::shared_ptr<device_info>> pick_uvc_devices(
            const std::shared_ptr<uvc::backend>& backend,
            const std::vector<uvc::uvc_device_info>& uvc_devices)
        {
            std::vector<std::shared_ptr<device_info>> list;
            for (auto&& uvc : uvc_devices)
            {
                list.push_back(std::make_shared<platform_camera_info>(backend, uvc));
            }
            return list;
        }

        explicit platform_camera_info(std::shared_ptr<uvc::backend> backend,
            uvc::uvc_device_info uvc)
            : device_info(backend), _uvc(std::move(uvc)) {}

        uvc::devices_data get_device_data() const override
        {
            return uvc::devices_data();
        }

    private:
        uvc::uvc_device_info _uvc;
    };

    typedef std::vector<std::shared_ptr<device_info>> devices_info;

    class context : public std::enable_shared_from_this<context>
    {
    public:
        explicit context(backend_type type,
            const char* filename = nullptr,
            const char* section = nullptr,
            rs2_recording_mode mode = RS2_RECORDING_MODE_COUNT);

        ~context();
        std::vector<std::shared_ptr<device_info>> query_devices() const;
        const uvc::backend& get_backend() const { return *_backend; }
        double get_time();
        void set_devices_changed_callback(devices_changed_callback_ptr callback);

        std::vector<std::shared_ptr<device_info>> create_devices(uvc::devices_data devices) const;

    private:
        void on_device_changed(uvc::devices_data old, uvc::devices_data curr);

        std::shared_ptr<uvc::backend> _backend;
        std::shared_ptr<uvc::time_service> _ts;
        std::shared_ptr<uvc::device_watcher> _device_watcher;
        devices_changed_callback_ptr _devices_changed_callback;
    };

    static std::vector<uvc::uvc_device_info> filter_by_product(const std::vector<uvc::uvc_device_info>& devices, const std::set<uint16_t>& pid_list)
    {
        std::vector<uvc::uvc_device_info> result;
        for (auto&& info : devices)
        {
            if (pid_list.count(info.pid))
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
