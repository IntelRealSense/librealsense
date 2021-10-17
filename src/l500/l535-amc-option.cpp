//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "l535-amc-option.h"
#include "l500-private.h"
#include "l500-depth.h"


using librealsense::ivcam2::l535::amc_option;

amc_option::amc_option( librealsense::l500_device * l500_dev,
                        librealsense::hw_monitor * hw_monitor,
                        librealsense::ivcam2::l535::amc_control type,
                        const std::string & description )
    : _device( l500_dev )
    , _hw_monitor( hw_monitor )
    , _control( type )
    , _description( description )
{
    // Keep the USB power on while triggering multiple calls on it.
    group_multiple_fw_calls( _device->get_depth_sensor(), [&]() {
        auto min = _hw_monitor->send( command{ AMCGET, _control, get_min } );
        auto max = _hw_monitor->send( command{ AMCGET, _control, get_max } );
        auto step = _hw_monitor->send( command{ AMCGET, _control, get_step } );

        if( min.size() < sizeof( int32_t ) || max.size() < sizeof( int32_t )
            || step.size() < sizeof( int32_t ) )
        {
            std::stringstream s;
            s << "Size of data returned is not valid min size = " << min.size()
              << ", max size = " << max.size() << ", step size = " << step.size();
            throw std::runtime_error( s.str() );
        }

        auto max_value = float( *( reinterpret_cast< int32_t * >( max.data() ) ) );
        auto min_value = float( *( reinterpret_cast< int32_t * >( min.data() ) ) );

        auto res = query_default();

        _range = option_range{ min_value,
                               max_value,
                               float( *( reinterpret_cast< int32_t * >( step.data() ) ) ),
                               res };
    } );
}

float amc_option::query() const
{
    auto res = _hw_monitor->send( command{ AMCGET, _control, get_current } );

    if( res.size() < sizeof( int32_t ) )
    {
        std::stringstream s;
        s << "Size of data returned from query(get_current) of " << _control << " is " << res.size()
          << " while min size = " << sizeof( int32_t );
        throw std::runtime_error( s.str() );
    }
    auto val = *( reinterpret_cast< uint32_t * >( res.data() ) );
    return float( val );
}

void amc_option::set( float value )
{
    _hw_monitor->send( command{ AMCSET, _control, (int)value } );
}

librealsense::option_range amc_option::get_range() const
{
    return _range;
}

float amc_option::query_default() const
{
    auto res = _hw_monitor->send( command{ AMCGET, _control, l500_command::get_default } );

    auto val = *( reinterpret_cast< uint32_t * >( res.data() ) );
    return float( val );
}

void amc_option::enable_recording( std::function< void( const option & ) > recording_action ) {}
            
