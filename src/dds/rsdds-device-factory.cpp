// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "rsdds-device-factory.h"
#include "context.h"
#include "rs-dds-device-info.h"

#include <librealsense2/h/rs_context.h>  // RS2_PRODUCT_LINE_...

#include <realdds/dds-device-watcher.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-device.h>
#include <realdds/topics/device-info-msg.h>

#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/shared-ptr-singleton.h>
#include <rsutils/os/executable-name.h>
#include <rsutils/string/slice.h>
#include <rsutils/string/from.h>
#include <rsutils/signal.h>
#include <rsutils/json.h>

#include <mutex>


namespace librealsense {


// The device-watcher is a singleton per domain. It is held alive by the device-factory below, which is held per
// context. I.e., as long as a context is alive, it stays alive alongside the participant.
//
// We are responsible for exposing the single notification from the dds-device-watcher to several subscribers:
// one device-watcher to many contexts, each with further subscriptions.
//
class rsdds_watcher_singleton
{
    std::shared_ptr< realdds::dds_device_watcher > const _device_watcher;
    using signal = rsutils::signal< std::shared_ptr< realdds::dds_device > const &, bool /*added*/ >;
    signal _callbacks;

public:
    rsdds_watcher_singleton( std::shared_ptr< realdds::dds_participant > const & participant )
        : _device_watcher( std::make_shared< realdds::dds_device_watcher >( participant ) )
    {
        assert( _device_watcher->is_stopped() );

        _device_watcher->on_device_added(
            [this]( std::shared_ptr< realdds::dds_device > const & dev )
            {
                dev->wait_until_ready();  // make sure handshake is complete
                _callbacks.raise( dev, true );
            } );

        _device_watcher->on_device_removed( [this]( std::shared_ptr< realdds::dds_device > const & dev )
                                            { _callbacks.raise( dev, false ); } );

        _device_watcher->start();
    }

    rsutils::subscription subscribe( signal::callback && cb ) { return _callbacks.subscribe( std::move( cb ) ); }

    std::shared_ptr< realdds::dds_device_watcher > const get_device_watcher() const { return _device_watcher; }
};


// We manage one participant and device-watcher per domain:
// Two contexts with the same domain-id will share the same participant and watcher, while a third context on a
// different domain will have its own.
//
struct domain_context
{
    rsutils::shared_ptr_singleton< realdds::dds_participant > participant;
    rsutils::shared_ptr_singleton< rsdds_watcher_singleton > device_watcher;
};
//
// Domains are mapped by ID:
// Two contexts with the same participant name on different domain-ids are using two different participants!
//
static std::map< realdds::dds_domain_id, domain_context > domain_context_by_id;
static std::mutex domain_context_by_id_mutex;


rsdds_device_factory::rsdds_device_factory( std::shared_ptr< context > const & ctx, callback && cb )
    : super( ctx )
{
    auto dds_settings = ctx->get_settings().nested( std::string( "dds", 3 ) );
    if( ! dds_settings.exists()
        || dds_settings.is_object() && dds_settings.nested( std::string( "enabled", 7 ) ).default_value( true ) )
    {
        auto domain_id = dds_settings.nested( std::string( "domain", 6 ) ).default_value< realdds::dds_domain_id >( 0 );
        auto participant_name_j = dds_settings.nested( std::string( "participant", 11 ) );
        auto participant_name = participant_name_j.default_value( rsutils::os::executable_name() );

        std::lock_guard< std::mutex > lock( domain_context_by_id_mutex );
        auto & domain = domain_context_by_id[domain_id];
        _participant = domain.participant.instance();
        if( ! _participant->is_valid() )
        {
            _participant->init( domain_id, participant_name, dds_settings.default_object() );
        }
        else if( participant_name_j.exists() && participant_name != _participant->name() )
        {
            throw std::runtime_error( rsutils::string::from()
                                      << "A DDS participant '" << _participant->name() << "' already exists in domain "
                                      << domain_id << "; cannot create '" << participant_name << "'" );
        }
        _watcher_singleton = domain.device_watcher.instance( _participant );
        _subscription = _watcher_singleton->subscribe(
            [liveliness = std::weak_ptr< context >( ctx ),
             cb = std::move( cb )]( std::shared_ptr< realdds::dds_device > const & dev, bool added )
            {
                // the factory should be alive as long as the context is alive
                auto ctx = liveliness.lock();
                if( ! ctx )
                    return;
                std::vector< std::shared_ptr< device_info > > infos_added;
                std::vector< std::shared_ptr< device_info > > infos_removed;
                auto dev_info = std::make_shared< dds_device_info >( ctx, dev );
                if( added )
                    infos_added.push_back( dev_info );
                else
                    infos_removed.push_back( dev_info );
                cb( infos_removed, infos_added );
            } );
    }
}


rsdds_device_factory::~rsdds_device_factory() {}


std::vector< std::shared_ptr< device_info > > rsdds_device_factory::query_devices( unsigned requested_mask ) const
{
    std::vector< std::shared_ptr< device_info > > list;
    if( _watcher_singleton )
    {
        unsigned const mask = context::combine_device_masks( requested_mask, get_context()->get_device_mask() );

        _watcher_singleton->get_device_watcher()->foreach_device(
            [&]( std::shared_ptr< realdds::dds_device > const & dev ) -> bool
            {
                if( ! dev->is_ready() )
                {
                    LOG_DEBUG( "device '" << dev->device_info().debug_name() << "' is not ready" );
                    return true;
                }
                std::string product_line;
                if( dev->device_info().to_json().nested( "product-line" ).get_ex( product_line ) )
                {
                    if( product_line == "D400" )
                    {
                        if( ! ( mask & RS2_PRODUCT_LINE_D400 ) )
                            return true;
                    }
                    else if( product_line == "D500" )
                    {
                        if( ! ( mask & RS2_PRODUCT_LINE_D500 ) )
                            return true;
                    }
                    else if( ! ( mask & RS2_PRODUCT_LINE_NON_INTEL ) )
                    {
                        return true;
                    }
                }
                else if( ! ( mask & RS2_PRODUCT_LINE_NON_INTEL ) )
                {
                    return true;
                }

                if( auto ctx = get_context() )
                {
                    std::shared_ptr< device_info > info = std::make_shared< dds_device_info >( ctx, dev );
                    list.push_back( info );
                }
                return true;  // continue iteration
            } );
    }
    return list;
}


}  // namespace librealsense
