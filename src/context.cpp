// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "context.h"

#include "media/playback/playback-device-info.h"
#include "environment.h"
#include <src/backend.h>


#ifdef BUILD_WITH_DDS
#include "dds/rs-dds-device-info.h"

#include <realdds/dds-device-watcher.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-device.h>
#include <realdds/topics/device-info-msg.h>
#include <rsutils/shared-ptr-singleton.h>
#include <rsutils/os/executable-name.h>
#include <rsutils/string/slice.h>

// We manage one participant and device-watcher per domain:
// Two contexts with the same domain-id will share the same participant and watcher, while a third context on a
// different domain will have its own.
//
struct dds_domain_context
{
    rsutils::shared_ptr_singleton< realdds::dds_participant > participant;
    rsutils::shared_ptr_singleton< realdds::dds_device_watcher > device_watcher;
};
//
// Domains are mapped by ID:
// Two contexts with the same participant name on different domain-ids are using two different participants!
//
static std::map< realdds::dds_domain_id, dds_domain_context > dds_domain_context_by_id;

#endif // BUILD_WITH_DDS

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
    {
        static bool version_logged = false;
        if( ! version_logged )
        {
            version_logged = true;
            LOG_DEBUG( "Librealsense VERSION: " << RS2_API_VERSION_STR );
        }

#ifdef BUILD_WITH_DDS
        nlohmann::json dds_settings
            = rsutils::json::get< nlohmann::json >( settings, std::string( "dds", 3 ), nlohmann::json::object() );
        if( dds_settings.is_object() )
        {
            realdds::dds_domain_id domain_id
                = rsutils::json::get< int >( dds_settings, std::string( "domain", 6 ), 0 );
            std::string participant_name = rsutils::json::get< std::string >( dds_settings,
                                                                              std::string( "participant", 11 ),
                                                                              rsutils::os::executable_name() );

            auto & domain = dds_domain_context_by_id[domain_id];
            _dds_participant = domain.participant.instance();
            if( ! _dds_participant->is_valid() )
            {
                _dds_participant->init( domain_id, participant_name, std::move( dds_settings ) );
            }
            else if( rsutils::json::has_value( dds_settings, std::string( "participant", 11 ) )
                     && participant_name != _dds_participant->name() )
            {
                throw std::runtime_error( rsutils::string::from() << "A DDS participant '" << _dds_participant->name()
                                                                  << "' already exists in domain " << domain_id
                                                                  << "; cannot create '" << participant_name << "'" );
            }
            _dds_watcher = domain.device_watcher.instance( _dds_participant );

            // The DDS device watcher should always be on
            if( _dds_watcher && _dds_watcher->is_stopped() )
            {
                start_dds_device_watcher();
            }
        }
#endif //BUILD_WITH_DDS
    }


    context::context( char const * json_settings )
        : context( json_settings ? json::parse( json_settings ) : json() )
    {
    }


    context::~context()
    {
#ifdef BUILD_WITH_DDS
        if( _dds_watcher )
            _dds_watcher->stop();
#endif //BUILD_WITH_DDS
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
        if( _dds_watcher )
            _dds_watcher->foreach_device(
                [&]( std::shared_ptr< realdds::dds_device > const & dev ) -> bool
                {
                    if( !dev->is_ready() )
                    {
                        LOG_DEBUG( "device '" << dev->device_info().debug_name() << "' is not yet ready" );
                        return true;
                    }
                    if( dev->device_info().product_line == "D400" )
                    {
                        if( !(mask & RS2_PRODUCT_LINE_D400) )
                            return true;
                    }
                    else if( dev->device_info().product_line == "D500" )
                    {
                        if( !(mask & RS2_PRODUCT_LINE_D500) )
                            return true;
                    }
                    else if( !(mask & RS2_PRODUCT_LINE_NON_INTEL) )
                    {
                        return true;
                    }

                    std::shared_ptr< device_info > info = std::make_shared< dds_device_info >( ctx, dev );
                    list.push_back( info );
                    return true;
                } );
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


#ifdef BUILD_WITH_DDS
    void context::start_dds_device_watcher()
    {
        _dds_watcher->on_device_added( [this]( std::shared_ptr< realdds::dds_device > const & dev ) {
            dev->wait_until_ready();  // make sure handshake is complete

            std::vector<rs2_device_info> rs2_device_info_added;
            std::vector<rs2_device_info> rs2_device_info_removed;
            auto info = std::make_shared< dds_device_info >( shared_from_this(), dev );
            rs2_device_info_added.push_back( { shared_from_this(), info } );
            invoke_devices_changed_callbacks( rs2_device_info_removed, rs2_device_info_added );
        } );
        _dds_watcher->on_device_removed( [this]( std::shared_ptr< realdds::dds_device > const & dev ) {
            std::vector<rs2_device_info> rs2_device_info_added;
            std::vector<rs2_device_info> rs2_device_info_removed;
            auto info = std::make_shared< dds_device_info >( shared_from_this(), dev );
            rs2_device_info_removed.push_back( { shared_from_this(), info } );
            invoke_devices_changed_callbacks( rs2_device_info_removed, rs2_device_info_added );
        } );
        _dds_watcher->start();
    }
#endif //BUILD_WITH_DDS

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
