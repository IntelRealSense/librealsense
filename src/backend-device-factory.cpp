// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "backend-device-factory.h"
#include "context.h"
#include "backend.h"
#include "platform/platform-device-info.h"

#include "ds/d400/d400-info.h"
#include "ds/d500/d500-info.h"
#include "fw-update/fw-update-factory.h"
#include "platform-camera.h"

#include <rsutils/json.h>


namespace {


template< class T >
bool contains( const T & first, const T & second )
{
    return first == second;
}


template<>
bool contains( librealsense::platform::backend_device_group const & first_data,
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


std::vector< std::shared_ptr< librealsense::device_info > >
subtract_sets( const std::vector< std::shared_ptr< librealsense::device_info > > & first,
               const std::vector< std::shared_ptr< librealsense::device_info > > & second )
{
    std::vector< std::shared_ptr< librealsense::device_info > > results;
    std::for_each(
        first.begin(),
        first.end(),
        [&]( std::shared_ptr< librealsense::device_info > const & data )
        {
            if( std::find_if(
                    second.begin(),
                    second.end(),
                    [&]( std::shared_ptr< librealsense::device_info > const & new_dev )
                    {
                        if( auto pnew
                            = std::dynamic_pointer_cast< librealsense::platform::platform_device_info >( new_dev ) )
                            if( auto pold
                                = std::dynamic_pointer_cast< librealsense::platform::platform_device_info >( data ) )
                                return contains( pold->get_group(), pnew->get_group() );
                        return data->is_same_as( new_dev );
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


backend_device_factory::backend_device_factory( context & ctx, callback cb )
    : _device_watcher( ctx.get_backend().create_device_watcher() )
    , _device_mask( rsutils::json::get< unsigned >( ctx.get_settings(), "device-mask", RS2_PRODUCT_LINE_ANY ) )
    , _context( ctx )
    , _callback( cb )
{
    assert( _device_watcher->is_stopped() );
    _device_watcher->start(
        [this]( platform::backend_device_group old, platform::backend_device_group curr )
        {
            auto old_list = create_devices_from_group( old, RS2_PRODUCT_LINE_ANY );
            auto new_list = create_devices_from_group( curr, RS2_PRODUCT_LINE_ANY );

            if( librealsense::list_changed< std::shared_ptr< device_info > >(
                    old_list,
                    new_list,
                    []( std::shared_ptr< device_info > first, std::shared_ptr< device_info > second )
                    { return first->is_same_as( second ); } ) )
            {
                std::vector< rs2_device_info > rs2_devices_info_removed;

                auto devices_info_removed = subtract_sets( old_list, new_list );
                for( size_t i = 0; i < devices_info_removed.size(); i++ )
                {
                    rs2_devices_info_removed.push_back( { _context.shared_from_this(), devices_info_removed[i] } );
                    LOG_DEBUG( "Device disconnected: " << devices_info_removed[i]->get_address() );
                }

                std::vector< rs2_device_info > rs2_devices_info_added;
                auto devices_info_added = subtract_sets( new_list, old_list );
                for( size_t i = 0; i < devices_info_added.size(); i++ )
                {
                    rs2_devices_info_added.push_back( { _context.shared_from_this(), devices_info_added[i] } );
                    LOG_DEBUG( "Device connected: " << devices_info_added[i]->get_address() );
                }

                _callback( rs2_devices_info_removed, rs2_devices_info_added );
            }
        } );
}


backend_device_factory::~backend_device_factory()
{
    if( _device_watcher )
        _device_watcher->stop();
}


unsigned backend_device_factory::calc_mask( unsigned requested_mask ) const
{
    unsigned mask = requested_mask;
    // The normal bits enable, so enable only those that are on
    mask &= _device_mask & ~RS2_PRODUCT_LINE_SW_ONLY;
    // But the above turned off the SW-only bits, so turn them back on again
    if( ( _device_mask & RS2_PRODUCT_LINE_SW_ONLY ) || ( requested_mask & RS2_PRODUCT_LINE_SW_ONLY ) )
        mask |= RS2_PRODUCT_LINE_SW_ONLY;
    return mask;
}


std::vector< std::shared_ptr< device_info > > backend_device_factory::query_devices( unsigned requested_mask ) const
{
    platform::backend_device_group devices;
    if( ! ( requested_mask & RS2_PRODUCT_LINE_SW_ONLY ) && ! ( _device_mask & RS2_PRODUCT_LINE_SW_ONLY ) )
    {
        auto & backend = _context.get_backend();
        devices.uvc_devices = backend.query_uvc_devices();
        devices.usb_devices = backend.query_usb_devices();
        devices.hid_devices = backend.query_hid_devices();
    }
    return create_devices_from_group( devices, requested_mask );
}


std::vector< std::shared_ptr< device_info > >
backend_device_factory::create_devices_from_group( platform::backend_device_group devices, int requested_mask ) const
{
    std::vector< std::shared_ptr< device_info > > list;
    unsigned const mask = calc_mask( requested_mask );

    if( mask & RS2_PRODUCT_LINE_D400 && ! ( mask & RS2_PRODUCT_LINE_SW_ONLY ) )
    {
        auto d400_devices = d400_info::pick_d400_devices( _context.shared_from_this(), devices );
        std::copy( std::begin( d400_devices ), end( d400_devices ), std::back_inserter( list ) );
    }

    if( mask & RS2_PRODUCT_LINE_D500 && ! ( mask & RS2_PRODUCT_LINE_SW_ONLY ) )
    {
        auto d500_devices = d500_info::pick_d500_devices( _context.shared_from_this(), devices );
        std::copy( begin( d500_devices ), end( d500_devices ), std::back_inserter( list ) );
    }

    // Supported recovery devices
    if( ! ( mask & RS2_PRODUCT_LINE_SW_ONLY ) )
    {
        auto recovery_devices
            = fw_update_info::pick_recovery_devices( _context.shared_from_this(), devices.usb_devices, mask );
        std::copy( begin( recovery_devices ), end( recovery_devices ), std::back_inserter( list ) );
    }

    if( mask & RS2_PRODUCT_LINE_NON_INTEL && ! ( mask & RS2_PRODUCT_LINE_SW_ONLY ) )
    {
        auto uvc_devices = platform_camera_info::pick_uvc_devices( _context.shared_from_this(), devices.uvc_devices );
        std::copy( begin( uvc_devices ), end( uvc_devices ), std::back_inserter( list ) );
    }

    return list;
}


}  // namespace librealsense
