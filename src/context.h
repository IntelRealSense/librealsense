// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/streaming.h"

#include "device-info.h"
#include "platform/device-watcher.h"

#include <nlohmann/json.hpp>
#include <vector>
#include <set>


#include <rsutils/lazy.h>


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


#ifdef BUILD_WITH_DDS
namespace realdds {
    class dds_device_watcher;
    class dds_participant;
}  // namespace realdds
#endif


namespace librealsense
{
    class device;
    class context;
    class playback_device_info;

    namespace platform {
        class backend;
        class device_watcher;
    }

    class context : public std::enable_shared_from_this<context>
    {
        context();
    public:
        explicit context( nlohmann::json const & );
        explicit context( char const * json_settings );

        void stop() { _device_watcher->stop(); }
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
        
        const nlohmann::json & get_settings() const { return _settings; }

    private:
        void invoke_devices_changed_callbacks( std::vector<rs2_device_info> & rs2_devices_info_removed,
                                               std::vector<rs2_device_info> & rs2_devices_info_added );
        void raise_devices_changed(const std::vector<rs2_device_info>& removed, const std::vector<rs2_device_info>& added);
        void start_device_watcher();
        std::shared_ptr<platform::backend> _backend;
        std::shared_ptr<platform::device_watcher> _device_watcher;

        std::map<std::string, std::weak_ptr<device_info>> _playback_devices;
        std::map<uint64_t, devices_changed_callback_ptr> _devices_changed_callbacks;
#ifdef BUILD_WITH_DDS
        std::shared_ptr< realdds::dds_participant > _dds_participant;
        std::shared_ptr< realdds::dds_device_watcher > _dds_watcher;

        void start_dds_device_watcher();
#endif

        nlohmann::json _settings; // Save operation settings

        devices_changed_callback_ptr _devices_changed_callback;
        std::map<int, std::weak_ptr<const stream_interface>> _streams;
        std::map< int, std::map< int, std::weak_ptr< rsutils::lazy< rs2_extrinsics > > > > _extrinsics;
        std::mutex _streams_mutex, _devices_changed_callbacks_mtx;
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
