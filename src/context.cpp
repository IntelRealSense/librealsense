// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "context.h"
#include "media/playback/playback-device-info.h"

#include <rsutils/string/from.h>
#include <rsutils/json.h>
using json = nlohmann::json;


namespace librealsense
{
    context::context( json const & settings )
        : _settings( settings )
        , _device_mask( rsutils::json::get< unsigned >( settings, "device-mask", RS2_PRODUCT_LINE_ANY ) )
        , _devices_changed_callback( nullptr, []( rs2_devices_changed_callback * ) {} )
        , _backend_device_factory(
              *this,
              [this]( std::vector< rs2_device_info > & removed, std::vector< rs2_device_info > & added )
              { invoke_devices_changed_callbacks( removed, added ); } )
#ifdef BUILD_WITH_DDS
        , _dds_device_factory( *this,
                               [this]( std::vector< rs2_device_info > & removed, std::vector< rs2_device_info > & added )
                               { invoke_devices_changed_callbacks( removed, added ); } )
#endif
    {
        static bool version_logged = false;
        if( ! version_logged )
        {
            version_logged = true;
            LOG_DEBUG( "Librealsense VERSION: " << RS2_API_VERSION_STR );
        }
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


    std::vector<std::shared_ptr<device_info>> context::query_devices( int requested_mask ) const
    {
        auto list = _backend_device_factory.query_devices( requested_mask );
        query_software_devices( list, requested_mask );
        LOG_INFO( "Found " << list.size() << " RealSense devices (0x" << std::hex << requested_mask << " requested & 0x"
                           << get_device_mask() << " from device-mask in settings)" << std::dec );
        for( auto & item : list )
            LOG_INFO( "... " << item->get_address() );
        return list;
    }


    void context::query_software_devices( std::vector< std::shared_ptr< device_info > > & list, unsigned requested_mask ) const
    {
        unsigned mask = combine_device_masks( requested_mask, get_device_mask() );

        auto t = const_cast<context *>(this); // While generally a bad idea, we need to provide mutable reference to the devices
        // to allow them to modify context later on
        auto ctx = t->shared_from_this();

#ifdef BUILD_WITH_DDS
        for( auto & info : _dds_device_factory.query_devices( requested_mask ) )
            list.push_back( info );
#endif //BUILD_WITH_DDS

        for( auto && item : _playback_devices )
        {
            if( auto dev = item.second.lock() )
                list.push_back( dev );
        }
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
            catch( ... )
            {
                LOG_ERROR( "Exception thrown from user callback handler" );
            }
        }

        raise_devices_changed( rs2_devices_info_removed, rs2_devices_info_added );
    }

    void context::raise_devices_changed(const std::vector<rs2_device_info>& removed, const std::vector<rs2_device_info>& added)
    {
        if (_devices_changed_callback)
        {
            try
            {
                _devices_changed_callback->on_devices_changed(new rs2_device_list({ shared_from_this(), removed }),
                    new rs2_device_list({ shared_from_this(), added }));
            }
            catch( std::exception const & e )
            {
                LOG_ERROR( "Exception thrown from user callback handler: " << e.what() );
            }
            catch (...)
            {
                LOG_ERROR("Exception thrown from user callback handler");
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
        _devices_changed_callback = std::move(callback);
    }

    std::shared_ptr<playback_device_info> context::add_device(const std::string& file)
    {
        auto it = _playback_devices.find(file);
        if (it != _playback_devices.end() && it->second.lock())
        {
            //Already exists
            throw librealsense::invalid_value_exception( rsutils::string::from()
                                                         << "File \"" << file << "\" already loaded to context" );
        }
        auto dinfo = std::make_shared< playback_device_info >( shared_from_this(), file );
        _playback_devices[file] = dinfo;

        std::vector< rs2_device_info > rs2_device_info_added{ { shared_from_this(), dinfo } };
        std::vector< rs2_device_info > rs2_device_info_removed;
        invoke_devices_changed_callbacks( rs2_device_info_removed, rs2_device_info_added );

        return dinfo;
    }

    void context::add_software_device(std::shared_ptr<device_info> dev)
    {
        auto address = dev->get_address();

        auto it = _playback_devices.find(address);
        if (it != _playback_devices.end() && it->second.lock())
            throw librealsense::invalid_value_exception( "File \"" + address + "\" already loaded to context" );
        _playback_devices[address] = dev;

        std::vector< rs2_device_info > rs2_device_info_added{ { shared_from_this(), dev } };
        std::vector< rs2_device_info > rs2_device_info_removed;
        invoke_devices_changed_callbacks( rs2_device_info_removed, rs2_device_info_added );
    }

    void context::remove_device(const std::string& file)
    {
        auto it = _playback_devices.find(file);
        if(it == _playback_devices.end() )
            return;
        auto dev_info = it->second.lock();
        _playback_devices.erase(it);

        if( dev_info )
        {
            std::vector< rs2_device_info > rs2_device_info_added;
            std::vector< rs2_device_info > rs2_device_info_removed{ { shared_from_this(), dev_info } };
            invoke_devices_changed_callbacks( rs2_device_info_removed, rs2_device_info_added );
        }
    }

}
