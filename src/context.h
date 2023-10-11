// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"  // devices_changed_callback_ptr

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


namespace librealsense
{
    class device_factory;

    class context : public std::enable_shared_from_this<context>
    {
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

        uint64_t register_internal_device_callback(devices_changed_callback_ptr callback);
        void unregister_internal_device_callback(uint64_t cb_id);
        void set_devices_changed_callback(devices_changed_callback_ptr callback);

        // Let the context maintain a list of custom devices. These can be anything, like playback devices or devices
        // maintained by the user.
        void add_device( std::shared_ptr< device_info > const & );
        void remove_device( std::shared_ptr< device_info > const & );

        const nlohmann::json & get_settings() const { return _settings; }

    private:
        void invoke_devices_changed_callbacks( std::vector<rs2_device_info> & rs2_devices_info_removed,
                                               std::vector<rs2_device_info> & rs2_devices_info_added );
        void raise_devices_changed(const std::vector<rs2_device_info>& removed, const std::vector<rs2_device_info>& added);

        std::map< std::string, std::weak_ptr< device_info > > _user_devices;
        std::map<uint64_t, devices_changed_callback_ptr> _devices_changed_callbacks;

        nlohmann::json _settings; // Save operation settings
        unsigned const _device_mask;

        std::vector< std::shared_ptr< device_factory > > _factories;

        devices_changed_callback_ptr _devices_changed_callback;
        std::mutex _devices_changed_callbacks_mtx;
    };

}
