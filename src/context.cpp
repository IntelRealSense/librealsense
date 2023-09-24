// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "context.h"
#include "device-info.h"

#include "backend-device-factory.h"
#ifdef BUILD_WITH_DDS
#include "dds/rsdds-device-factory.h"
#endif

#include <rsutils/string/from.h>
#include <rsutils/json.h>
using json = nlohmann::json;


namespace librealsense
{
    context::context( json const & settings )
        : _settings( settings )
        , _device_mask( rsutils::json::get< unsigned >( settings, "device-mask", RS2_PRODUCT_LINE_ANY ) )
    {
        static bool version_logged = false;
        if( ! version_logged )
        {
            version_logged = true;
            LOG_DEBUG( "Librealsense VERSION: " << RS2_API_VERSION_STR );
        }

        _factories.push_back( std::make_shared< backend_device_factory >(
            *this,
            [this]( std::vector< rs2_device_info > & removed, std::vector< rs2_device_info > & added )
            { invoke_devices_changed_callbacks( removed, added ); } ) );

#ifdef BUILD_WITH_DDS
        _factories.push_back( std::make_shared< rsdds_device_factory >(
            *this,
            [this]( std::vector< rs2_device_info > & removed, std::vector< rs2_device_info > & added )
            { invoke_devices_changed_callbacks( removed, added ); } ) );
#endif
    }


    context::context( char const * json_settings )
        : context( json_settings ? json::parse( json_settings ) : json() )
    {
    }


    context::~context()
    {
    }


    /*static*/ unsigned context::combine_device_masks( unsigned const requested_mask, unsigned const mask_in_settings )
    {
        unsigned mask = requested_mask;
        // The normal bits enable, so enable only those that are on
        mask &= mask_in_settings & ~RS2_PRODUCT_LINE_SW_ONLY;
        // But the above turned off the SW-only bits, so turn them back on again
        if( ( mask_in_settings & RS2_PRODUCT_LINE_SW_ONLY ) || ( requested_mask & RS2_PRODUCT_LINE_SW_ONLY ) )
            mask |= RS2_PRODUCT_LINE_SW_ONLY;
        return mask;
    }


    std::vector< std::shared_ptr< device_info > > context::query_devices( int requested_mask ) const
    {
        std::vector< std::shared_ptr< device_info > > list;
        for( auto & factory : _factories )
        {
            for( auto & dev_info : factory->query_devices( requested_mask ) )
            {
                LOG_INFO( "... " << dev_info->get_address() );
                list.push_back( dev_info );
            }
        }
        for( auto & item : _user_devices )
        {
            if( auto dev_info = item.second.lock() )
            {
                LOG_INFO( "... " << dev_info->get_address() );
                list.push_back( dev_info );
            }
        }
        LOG_INFO( "Found " << list.size() << " RealSense devices (0x" << std::hex << requested_mask << " requested & 0x"
                           << get_device_mask() << " from device-mask in settings)" << std::dec );
        return list;
    }


    void context::invoke_devices_changed_callbacks( std::vector<rs2_device_info> & rs2_devices_info_removed,
                                                    std::vector<rs2_device_info> & rs2_devices_info_added )
    {
        std::map<uint64_t, devices_changed_callback_ptr> devices_changed_callbacks;
        {
            std::lock_guard<std::mutex> lock( _devices_changed_callbacks_mtx );
            devices_changed_callbacks = _devices_changed_callbacks;
        }

        for( auto & kvp : devices_changed_callbacks )
        {
            try
            {
                kvp.second->on_devices_changed( new rs2_device_list( { shared_from_this(), rs2_devices_info_removed } ),
                                                new rs2_device_list( { shared_from_this(), rs2_devices_info_added } ) );
            }
            catch( std::exception const & e )
            {
                LOG_ERROR( "Exception thrown from user callback handler: " << e.what() );
            }
            catch( ... )
            {
                LOG_ERROR( "Exception thrown from user callback handler" );
            }
        }
    }


    uint64_t context::register_internal_device_callback(devices_changed_callback_ptr callback)
    {
        std::lock_guard<std::mutex> lock(_devices_changed_callbacks_mtx);
        auto callback_id = unique_id::generate_id();
        _devices_changed_callbacks.insert(std::make_pair(callback_id, std::move(callback)));
        return callback_id;
    }

    void context::unregister_internal_device_callback(uint64_t cb_id)
    {
        std::lock_guard<std::mutex> lock(_devices_changed_callbacks_mtx);
        _devices_changed_callbacks.erase(cb_id);
    }

    void context::set_devices_changed_callback(devices_changed_callback_ptr callback)
    {
        std::lock_guard<std::mutex> lock(_devices_changed_callbacks_mtx);
        // unique_id::generate_id() will never be 0; so we use 0 for the "public" callback so it'll get overriden on
        // subsequent calls
        _devices_changed_callbacks[0] = std::move( callback );
    }


    void context::add_device( std::shared_ptr< device_info > const & dev )
    {
        auto address = dev->get_address();

        auto it = _user_devices.find( address );
        if( it != _user_devices.end() && it->second.lock() )
            throw librealsense::invalid_value_exception( "device already in context: " + address );
        _user_devices[address] = dev;

        std::vector< rs2_device_info > rs2_device_info_added{ { shared_from_this(), dev } };
        std::vector< rs2_device_info > rs2_device_info_removed;
        invoke_devices_changed_callbacks( rs2_device_info_removed, rs2_device_info_added );
    }


    void context::remove_device( std::shared_ptr< device_info > const & dev )
    {
        auto address = dev->get_address();

        auto it = _user_devices.find( address );
        if(it == _user_devices.end() )
            return;  // Why not throw?!
        auto dev_info = it->second.lock();
        _user_devices.erase(it);

        if( dev_info )
        {
            std::vector< rs2_device_info > rs2_device_info_added;
            std::vector< rs2_device_info > rs2_device_info_removed{ { shared_from_this(), dev_info } };
            invoke_devices_changed_callbacks( rs2_device_info_removed, rs2_device_info_added );
        }
    }

}
