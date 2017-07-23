// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "backend.h"
#include "mock/recorder.h"
#include "core/streaming.h"

#include <vector>
#include <media/playback/playback_device.h>

namespace librealsense
{
    class context;
    class device_info;
}

struct rs2_device_info
{
    std::shared_ptr<librealsense::context> ctx;
    std::shared_ptr<librealsense::device_info> info;
};


struct rs2_device_list
{
    std::shared_ptr<librealsense::context> ctx;
    std::vector<rs2_device_info> list;
};

namespace librealsense
{
    class device;
    class context;
    class device_info;



    class device_info
    {
    public:
        virtual std::shared_ptr<device_interface> create_device() const
        {
            return create(*_backend);
        }

        virtual ~device_info() = default;


        virtual platform::backend_device_group get_device_data()const = 0;

        virtual bool operator==(const device_info& other) const
        {
            return other.get_device_data() == get_device_data();
        }

    protected:
        explicit device_info(std::shared_ptr<platform::backend> backend)
            : _backend(std::move(backend))
        {}

        virtual std::shared_ptr<device_interface> create(const platform::backend& backend) const = 0;

        std::shared_ptr<platform::backend> _backend;
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
        std::shared_ptr<device_interface> create(const platform::backend& /*backend*/) const override
        {
            throw unrecoverable_exception(RECOVERY_MESSAGE,
                RS2_EXCEPTION_TYPE_DEVICE_IN_RECOVERY_MODE);
        }

        static bool is_recovery_pid(uint16_t pid)
        {
            return pid == 0x0ADB || pid == 0x0AB3;
        }

        static std::vector<std::shared_ptr<device_info>> pick_recovery_devices(
            const std::shared_ptr<platform::backend>& backend,
            const std::vector<platform::usb_device_info>& usb_devices)
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

        explicit recovery_info(std::shared_ptr<platform::backend> backend, platform::usb_device_info dfu)
            : device_info(backend), _dfu(std::move(dfu)) {}

        platform::backend_device_group get_device_data()const override
        {
            return platform::backend_device_group({ _dfu });
        }

    private:
        platform::usb_device_info _dfu;
        const char* RECOVERY_MESSAGE = "Selected RealSense device is in recovery mode!\nEither perform a firmware update or reconnect the camera to fall-back to last working firmware if available!";
    };

    class platform_camera_info : public device_info
    {
    public:
        std::shared_ptr<device_interface> create(const platform::backend& /*backend*/) const override;

        static std::vector<std::shared_ptr<device_info>> pick_uvc_devices(
            const std::shared_ptr<platform::backend>& backend,
            const std::vector<platform::uvc_device_info>& uvc_devices)
        {
            std::vector<std::shared_ptr<device_info>> list;
            for (auto&& uvc : uvc_devices)
            {
                list.push_back(std::make_shared<platform_camera_info>(backend, uvc));
            }
            return list;
        }

        explicit platform_camera_info(std::shared_ptr<platform::backend> backend,
                                      platform::uvc_device_info uvc)
            : device_info(backend), _uvc(std::move(uvc)) {}

        platform::backend_device_group get_device_data() const override
        {
            return platform::backend_device_group();
        }

    private:
        platform::uvc_device_info _uvc;
    };


    class playback_device_info : public device_info
    {
        std::shared_ptr<playback_device> _dev;
    public:
        explicit playback_device_info(std::shared_ptr<playback_device> dev)
            : device_info(nullptr), _dev(dev)
        {

        }

        std::shared_ptr<device_interface> create_device() const override
        {
            return _dev;
        }
        platform::backend_device_group get_device_data() const override
        {
            return {}; //TODO: WTD?
        }

        std::shared_ptr<device_interface> create(const platform::backend& backend) const override
        {
            return _dev;
        }
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
        const platform::backend& get_backend() const { return *_backend; }
        double get_time();

        std::shared_ptr<platform::time_service> get_time_service() { return _ts; }

        void set_devices_changed_callback(devices_changed_callback_ptr callback);

        std::vector<std::shared_ptr<device_info>> create_devices(platform::backend_device_group devices, const std::map<std::string, std::shared_ptr<device_info>>& playback_devices) const;

        std::shared_ptr<device_interface> add_device(const std::string& file);
        void remove_device(const std::string& file);
    private:
        void on_device_changed(platform::backend_device_group old, platform::backend_device_group curr, const std::map<std::string, std::shared_ptr<device_info>>& old_playback_devices, const std::map<std::string, std::shared_ptr<device_info>>& new_playback_devices);

        std::shared_ptr<platform::backend> _backend;
        std::shared_ptr<platform::time_service> _ts;
        std::shared_ptr<platform::device_watcher> _device_watcher;
        std::map<std::string, std::shared_ptr<device_info>> _playback_devices;
        devices_changed_callback_ptr _devices_changed_callback;
    };

    static std::vector<platform::uvc_device_info> filter_by_product(const std::vector<platform::uvc_device_info>& devices, const std::set<uint16_t>& pid_list)
    {
        std::vector<platform::uvc_device_info> result;
        for (auto&& info : devices)
        {
            if (pid_list.count(info.pid))
                result.push_back(info);
        }
        return result;
    }

    static std::vector<std::pair<std::vector<platform::uvc_device_info>, std::vector<platform::hid_device_info>>> group_devices_and_hids_by_unique_id(const std::vector<std::vector<platform::uvc_device_info>>& devices,
        const std::vector<platform::hid_device_info>& hids)
    {
        std::vector<std::pair<std::vector<platform::uvc_device_info>, std::vector<platform::hid_device_info>>> results;
        for (auto&& dev : devices)
        {
            std::vector<platform::hid_device_info> hid_group;
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

    static std::vector<std::vector<platform::uvc_device_info>> group_devices_by_unique_id(const std::vector<platform::uvc_device_info>& devices)
    {
        std::map<std::string, std::vector<platform::uvc_device_info>> map;
        for (auto&& info : devices)
        {
            map[info.unique_id].push_back(info);
        }
        std::vector<std::vector<platform::uvc_device_info>> result;
        for (auto&& kvp : map)
        {
            result.push_back(kvp.second);
        }
        return result;
    }

    static void trim_device_list(std::vector<platform::uvc_device_info>& devices, const std::vector<platform::uvc_device_info>& chosen)
    {
        if (chosen.empty())
            return;

        auto was_chosen = [&chosen](const platform::uvc_device_info& info)
        {
            return find(chosen.begin(), chosen.end(), info) != chosen.end();
        };
        devices.erase(std::remove_if(devices.begin(), devices.end(), was_chosen), devices.end());
    }

    static bool mi_present(const std::vector<platform::uvc_device_info>& devices, uint32_t mi)
    {
        for (auto&& info : devices)
        {
            if (info.mi == mi)
                return true;
        }
        return false;
    }

    static platform::uvc_device_info get_mi(const std::vector<platform::uvc_device_info>& devices, uint32_t mi)
    {
        for (auto&& info : devices)
        {
            if (info.mi == mi)
                return info;
        }
        throw invalid_value_exception("Interface not found!");
    }

    static std::vector<platform::uvc_device_info> filter_by_mi(const std::vector<platform::uvc_device_info>& devices, uint32_t mi)
    {
        std::vector<platform::uvc_device_info> results;
        for (auto&& info : devices)
        {
            if (info.mi == mi)
                results.push_back(info);
        }
        return results;
    }

}
