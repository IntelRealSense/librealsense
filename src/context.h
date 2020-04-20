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

struct rs2_stream_profile
{
    librealsense::stream_profile_interface* profile;
    std::shared_ptr<librealsense::stream_profile_interface> clone;
};

namespace librealsense
{
    class device;
    class context;

    class device_info
    {
    public:
        virtual std::shared_ptr<device_interface> create_device(bool register_device_notifications = false) const
        {
            return create(_ctx, register_device_notifications);
        }

        virtual ~device_info() = default;

        virtual platform::backend_device_group get_device_data()const = 0;

        virtual bool operator==(const device_info& other) const
        {
            return other.get_device_data() == get_device_data();
        }

    protected:
        explicit device_info(std::shared_ptr<context> backend)
            : _ctx(move(backend))
        {}

        virtual std::shared_ptr<device_interface> create(std::shared_ptr<context> ctx,
                                                         bool register_device_notifications) const = 0;

        std::shared_ptr<context> _ctx;
    };

    enum class backend_type
    {
        standard,
        record,
        playback
    };


    class playback_device_info : public device_info
    {
        std::shared_ptr<playback_device> _dev;
    public:
        explicit playback_device_info(std::shared_ptr<playback_device> dev)
            : device_info(nullptr), _dev(dev)
        {

        }

        std::shared_ptr<device_interface> create_device(bool) const override
        {
            return _dev;
        }
        platform::backend_device_group get_device_data() const override
        {
            return platform::backend_device_group({ platform::playback_device_info{ _dev->get_file_name() } });
        }

        std::shared_ptr<device_interface> create(std::shared_ptr<context>, bool) const override
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
            rs2_recording_mode mode = RS2_RECORDING_MODE_COUNT,
            std::string min_api_version = "0.0.0");

        void stop(){ if (!_devices_changed_callbacks.size()) _device_watcher->stop();}
        ~context();
        std::vector<std::shared_ptr<device_info>> query_devices(int mask) const;
        const platform::backend& get_backend() const { return *_backend; }

        uint64_t register_internal_device_callback(devices_changed_callback_ptr callback);
        void unregister_internal_device_callback(uint64_t cb_id);
        void set_devices_changed_callback(devices_changed_callback_ptr callback);

        std::vector<std::shared_ptr<device_info>> create_devices(platform::backend_device_group devices,
            const std::map<std::string, std::weak_ptr<device_info>>& playback_devices, int mask) const;

        std::shared_ptr<playback_device_info> add_device(const std::string& file);
        void remove_device(const std::string& file);

        void add_software_device(std::shared_ptr<device_info> software_device);

#if WITH_TRACKING
        void unload_tracking_module();
#endif

    private:
        void on_device_changed(platform::backend_device_group old,
                               platform::backend_device_group curr,
                               const std::map<std::string, std::weak_ptr<device_info>>& old_playback_devices,
                               const std::map<std::string, std::weak_ptr<device_info>>& new_playback_devices);
        void raise_devices_changed(const std::vector<rs2_device_info>& removed, const std::vector<rs2_device_info>& added);
        int find_stream_profile(const stream_interface& p);
        std::shared_ptr<lazy<rs2_extrinsics>> fetch_edge(int from, int to);

        std::shared_ptr<platform::backend> _backend;
        std::shared_ptr<platform::device_watcher> _device_watcher;
        std::map<std::string, std::weak_ptr<device_info>> _playback_devices;
        std::map<uint64_t, devices_changed_callback_ptr> _devices_changed_callbacks;

        devices_changed_callback_ptr _devices_changed_callback;
        std::map<int, std::weak_ptr<const stream_interface>> _streams;
        std::map<int, std::map<int, std::weak_ptr<lazy<rs2_extrinsics>>>> _extrinsics;
        std::mutex _streams_mutex, _devices_changed_callbacks_mtx;
    };

    class readonly_device_info : public device_info
    {
    public:
        readonly_device_info(std::shared_ptr<device_interface> dev) : device_info(dev->get_context()), _dev(dev) {}
        std::shared_ptr<device_interface> create(std::shared_ptr<context> ctx, bool register_device_notifications) const override
        {
            return _dev;
        }

        platform::backend_device_group get_device_data() const override
        {
            return _dev->get_device_data();
        }
    private:
        std::shared_ptr<device_interface> _dev;
    };

    // Helper functions for device list manipulation:
    std::vector<platform::uvc_device_info> filter_by_product(const std::vector<platform::uvc_device_info>& devices, const std::set<uint16_t>& pid_list);
    std::vector<std::pair<std::vector<platform::uvc_device_info>, std::vector<platform::hid_device_info>>> group_devices_and_hids_by_unique_id(
        const std::vector<std::vector<platform::uvc_device_info>>& devices,
        const std::vector<platform::hid_device_info>& hids);
    std::vector<std::vector<platform::uvc_device_info>> group_devices_by_unique_id(const std::vector<platform::uvc_device_info>& devices);
    void trim_device_list(std::vector<platform::uvc_device_info>& devices, const std::vector<platform::uvc_device_info>& chosen);
    bool mi_present(const std::vector<platform::uvc_device_info>& devices, uint32_t mi);
    platform::uvc_device_info get_mi(const std::vector<platform::uvc_device_info>& devices, uint32_t mi);
    std::vector<platform::uvc_device_info> filter_by_mi(const std::vector<platform::uvc_device_info>& devices, uint32_t mi);

    std::vector<platform::usb_device_info> filter_by_product(const std::vector<platform::usb_device_info>& devices, const std::set<uint16_t>& pid_list);
    void trim_device_list(std::vector<platform::usb_device_info>& devices, const std::vector<platform::usb_device_info>& chosen);
}
