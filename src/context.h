// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend-device-factory.h"
#include "device-info.h"
#include "types.h"

#include <rsutils/lazy.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <map>


namespace librealsense
{
    class context;
    class device_info;
    class stream_profile_interface;
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
    class stream_interface;

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

        ~context();

        // The 'device-mask' is specified in the context settings, and governs which devices will be matched by us
        //
        unsigned get_device_mask() const { return _device_mask; }

        // Given the requested RS2_PRODUCT_LINE mask, returns the final mask when combined with the device-mask field in
        // settings.
        // 
        // E.g., if the device-mask specifies only D400 devices, and the user requests only SW devices, the result
        // should be only SW D400 devices.
        //
        static unsigned combine_device_masks( unsigned requested_mask, unsigned mask_in_settings );

        std::vector<std::shared_ptr<device_info>> query_devices(int mask) const;
        const platform::backend& get_backend() const { return *_backend_device_factory.get_backend(); }

        uint64_t register_internal_device_callback(devices_changed_callback_ptr callback);
        void unregister_internal_device_callback(uint64_t cb_id);
        void set_devices_changed_callback(devices_changed_callback_ptr callback);

        void query_software_devices( std::vector< std::shared_ptr< device_info > > & list, unsigned requested_mask ) const;

        std::shared_ptr<playback_device_info> add_device(const std::string& file);
        void remove_device(const std::string& file);

        void add_software_device(std::shared_ptr<device_info> software_device);
        
        const nlohmann::json & get_settings() const { return _settings; }

    private:
        void invoke_devices_changed_callbacks( std::vector<rs2_device_info> & rs2_devices_info_removed,
                                               std::vector<rs2_device_info> & rs2_devices_info_added );
        void raise_devices_changed(const std::vector<rs2_device_info>& removed, const std::vector<rs2_device_info>& added);

        std::map<std::string, std::weak_ptr<device_info>> _playback_devices;
        std::map<uint64_t, devices_changed_callback_ptr> _devices_changed_callbacks;
#ifdef BUILD_WITH_DDS
        std::shared_ptr< realdds::dds_participant > _dds_participant;
        std::shared_ptr< realdds::dds_device_watcher > _dds_watcher;

        void start_dds_device_watcher();
#endif

        nlohmann::json _settings; // Save operation settings
        unsigned const _device_mask;
        backend_device_factory _backend_device_factory;

        devices_changed_callback_ptr _devices_changed_callback;
        std::map<int, std::weak_ptr<const stream_interface>> _streams;
        std::map< int, std::map< int, std::weak_ptr< rsutils::lazy< rs2_extrinsics > > > > _extrinsics;
        std::mutex _streams_mutex, _devices_changed_callbacks_mtx;
    };

}
