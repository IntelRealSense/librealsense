// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <rsutils/signal.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <map>


namespace librealsense
{
    class device_factory;
    class device_info;


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

        // Query any subset of available devices and return them as device-info objects from which actual devices can be
        // created as needed.
        //
        // Devices will match both the requested mask and the device-mask from the context settings. See
        // RS2_PRODUCT_LINE_... defines for possible values.
        //
        std::vector< std::shared_ptr< device_info > > query_devices( int mask ) const;

        using devices_changed_callback
            = std::function< void( std::vector< std::shared_ptr< device_info > > const & devices_removed,
                                   std::vector< std::shared_ptr< device_info > > const & devices_added ) >;

        // Subscribe to a notification to receive when the device-list changes.
        //
        rsutils::subscription on_device_changes( devices_changed_callback && );

        // Let the context maintain a list of custom devices. These can be anything, like playback devices or devices
        // maintained by the user.
        void add_device( std::shared_ptr< device_info > const & );
        void remove_device( std::shared_ptr< device_info > const & );

        const nlohmann::json & get_settings() const { return _settings; }

    private:
        void invoke_devices_changed_callbacks( std::vector< std::shared_ptr< device_info > > const & devices_removed,
                                               std::vector< std::shared_ptr< device_info > > const & devices_added );

        std::map< std::string /*address*/, std::weak_ptr< device_info > > _user_devices;

        rsutils::signal< std::vector< std::shared_ptr< device_info > > const & /*removed*/,
                         std::vector< std::shared_ptr< device_info > > const & /*added*/ >
            _devices_changed;

        nlohmann::json _settings; // Save operation settings
        unsigned const _device_mask;

        std::vector< std::shared_ptr< device_factory > > _factories;
    };

}
