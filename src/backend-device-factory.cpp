// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "backend-device-factory.h"
#include "context.h"
#include "backend.h"
#include "platform/platform-device-info.h"
#include "platform/device-watcher.h"

#include "ds/d400/d400-info.h"
#include "ds/d500/d500-info.h"
#include "fw-update/fw-update-factory.h"
#include "platform-camera.h"

#include <rsutils/shared-ptr-singleton.h>
#include <rsutils/signal.h>
#include <rsutils/json.h>


namespace {


// Returns true if the left group is completely accounted for in the right group
//
bool group_contained_in( librealsense::platform::backend_device_group const & first_data,
                         librealsense::platform::backend_device_group const & second_data )
{
    for( auto && uvc : first_data.uvc_devices )
    {
        if( std::find( second_data.uvc_devices.begin(), second_data.uvc_devices.end(), uvc )
            == second_data.uvc_devices.end() )
            return false;
    }
    for( auto && usb : first_data.usb_devices )
    {
        if( std::find( second_data.usb_devices.begin(), second_data.usb_devices.end(), usb )
            == second_data.usb_devices.end() )
            return false;
    }
    for( auto && hid : first_data.hid_devices )
    {
        if( std::find( second_data.hid_devices.begin(), second_data.hid_devices.end(), hid )
            == second_data.hid_devices.end() )
            return false;
    }
    return true;
}


std::vector< std::shared_ptr< librealsense::platform::platform_device_info > >
subtract_sets( const std::vector< std::shared_ptr< librealsense::platform::platform_device_info > > & first,
               const std::vector< std::shared_ptr< librealsense::platform::platform_device_info > > & second )
{
    std::vector< std::shared_ptr< librealsense::platform::platform_device_info > > results;
    std::for_each(
        first.begin(),
        first.end(),
        [&]( std::shared_ptr< librealsense::platform::platform_device_info > const & data )
        {
            if( std::find_if(
                    second.begin(),
                    second.end(),
                    [&]( std::shared_ptr< librealsense::platform::platform_device_info > const & new_dev )
                    {
                        return group_contained_in( data->get_group(), new_dev->get_group() );
                    } )
                == second.end() )
            {
                results.push_back( data );
            }
        } );
    return results;
}


}  // namespace


namespace librealsense {


// This singleton creates the actual backend; as long as someone holds it, the backend will stay alive.
// The instance is held below by the device-watcher. I.e., the device-watcher triggers creation of the
// backend!
//
class backend_singleton
{
    std::shared_ptr< platform::backend > const _backend;

public:
    backend_singleton()
        : _backend( platform::create_backend() )
    {
    }

    std::shared_ptr< platform::backend > get() const { return _backend; }
};


static rsutils::shared_ptr_singleton< backend_singleton > the_backend;


// The device-watcher is also a singleton: we don't need multiple agents of notifications. It is held alive by the
// device-factory below, which is held per context. I.e., as long as the context is alive, we'll stay alive and the
// backend-singleton will stay alive.
// 
// We are responsible for exposing the single notification from the platform-device-watcher to several subscribers:
// one device-watcher, but many contexts, each with further subscriptions.
//
class device_watcher_singleton
{
    // The device-watcher keeps a direct pointer to the backend instance, so we have to make sure it stays alive!
    std::shared_ptr< backend_singleton > const _backend;
    std::shared_ptr< platform::device_watcher > const _device_watcher;
    rsutils::signal< platform::backend_device_group const &, platform::backend_device_group const & > _callbacks;

public:
    device_watcher_singleton()
        : _backend( the_backend.instance() )
        , _device_watcher( _backend->get()->create_device_watcher() )
    {
        assert( _device_watcher->is_stopped() );
        _device_watcher->start(
            [this]( platform::backend_device_group const & old, platform::backend_device_group const & curr )
            { _callbacks.raise( old, curr ); } );
    }

    rsutils::subscription subscribe( platform::device_changed_callback && cb )
    {
        return _callbacks.subscribe( std::move( cb ) );
    }

    std::shared_ptr< platform::backend > const get_backend() const { return _backend->get(); }
};


static rsutils::shared_ptr_singleton< device_watcher_singleton > backend_device_watcher;


backend_device_factory::backend_device_factory( context & ctx, callback && cb )
    : _context( ctx )
    , _device_watcher( backend_device_watcher.instance() )
    , _dtor( _device_watcher->subscribe(
          [this, cb = std::move( cb )]( platform::backend_device_group const & old,
                                        platform::backend_device_group const & curr )
          {
              auto old_list = create_devices_from_group( old, RS2_PRODUCT_LINE_ANY );
              auto new_list = create_devices_from_group( curr, RS2_PRODUCT_LINE_ANY );

              std::vector< rs2_device_info > devices_removed;
              for( auto & device_removed : subtract_sets( old_list, new_list ) )
              {
                  devices_removed.push_back( { _context.shared_from_this(), device_removed } );
                  LOG_DEBUG( "Device disconnected: " << device_removed->get_address() );
              }

              std::vector< rs2_device_info > devices_added;
              for( auto & device_added : subtract_sets( new_list, old_list ) )
              {
                  devices_added.push_back( { _context.shared_from_this(), device_added } );
                  LOG_DEBUG( "Device connected: " << device_added->get_address() );
              }

              if( devices_removed.size() + devices_added.size() )
              {
                  cb( devices_removed, devices_added );
              }
          } ) )
{
}


backend_device_factory::~backend_device_factory()
{
}


std::vector< std::shared_ptr< device_info > > backend_device_factory::query_devices( unsigned requested_mask ) const
{
    if( ( requested_mask & RS2_PRODUCT_LINE_SW_ONLY ) || ( _context.get_device_mask() & RS2_PRODUCT_LINE_SW_ONLY ) )
        return {};  // We don't carry any software devices

    auto backend = _device_watcher->get_backend();
    platform::backend_device_group group( backend->query_uvc_devices(),
                                          backend->query_usb_devices(),
                                          backend->query_hid_devices() );
    auto devices = create_devices_from_group( group, requested_mask );
    return { devices.begin(), devices.end() };
}


std::vector< std::shared_ptr< platform::platform_device_info > >
backend_device_factory::create_devices_from_group( platform::backend_device_group devices, int requested_mask ) const
{
    std::vector< std::shared_ptr< platform::platform_device_info > > list;
    unsigned const mask = context::combine_device_masks( requested_mask, _context.get_device_mask() );
    if( ! ( mask & RS2_PRODUCT_LINE_SW_ONLY ) )
    {
        if( mask & RS2_PRODUCT_LINE_D400 )
        {
            auto d400_devices = d400_info::pick_d400_devices( _context.shared_from_this(), devices );
            std::copy( std::begin( d400_devices ), end( d400_devices ), std::back_inserter( list ) );
        }

        if( mask & RS2_PRODUCT_LINE_D500 )
        {
            auto d500_devices = d500_info::pick_d500_devices( _context.shared_from_this(), devices );
            std::copy( begin( d500_devices ), end( d500_devices ), std::back_inserter( list ) );
        }

        // Supported recovery devices
        {
            auto recovery_devices
                = fw_update_info::pick_recovery_devices( _context.shared_from_this(), devices.usb_devices, mask );
            std::copy( begin( recovery_devices ), end( recovery_devices ), std::back_inserter( list ) );
        }

        if( mask & RS2_PRODUCT_LINE_NON_INTEL )
        {
            auto uvc_devices
                = platform_camera_info::pick_uvc_devices( _context.shared_from_this(), devices.uvc_devices );
            std::copy( begin( uvc_devices ), end( uvc_devices ), std::back_inserter( list ) );
        }
    }
    return list;
}


}  // namespace librealsense