//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "l535-options.h"
#include "l500-private.h"
#include "l500-depth.h"

const std::string MIN_CONTROLS_FW_VERSION("1.3.9.0");
const std::string MIN_GET_DEFAULT_FW_VERSION( "1.5.4.0" );

namespace librealsense
{
    using namespace ivcam2;

    float l535_hw_options::query() const
    {
        return query_current(RS2_SENSOR_MODE_VGA); //todo: remove sensor mode
    }

    void l535_hw_options::set(float value)
    {      
        _hw_monitor->send(command{ AMCSET, _type, (int)value });
    }

    option_range l535_hw_options::get_range() const
    {
        return _range;
    }

    float l535_hw_options::query_default() const
    {
        auto res = _hw_monitor->send(command{ AMCGET,
            _type,
            l500_command::get_default,
            RS2_SENSOR_MODE_VGA } //todo: remove sensor mode
            );
        
        auto val = *(reinterpret_cast< uint32_t * >(res.data()));
        return float(val);
    }

    l535_hw_options::l535_hw_options( l500_device* l500_dev,
                                      hw_monitor * hw_monitor,
                                      l535_control type,
                                      const std::string & description)
        : _l500_dev( l500_dev )
        , _hw_monitor( hw_monitor )
        , _type( type )
        , _description( description )
    {
        // Keep the USB power on while triggering multiple calls on it.
        ivcam2::group_multiple_fw_calls( _l500_dev->get_depth_sensor(), [&]() {
            auto min = _hw_monitor->send( command{ AMCGET, _type, get_min } );
            auto max = _hw_monitor->send( command{ AMCGET, _type, get_max } );
            auto step = _hw_monitor->send( command{ AMCGET, _type, get_step } );

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

    void l535_hw_options::enable_recording(std::function<void(const option&)> recording_action)
    {}

    float l535_hw_options::query_current( rs2_sensor_mode mode ) const //todo: remove sensor mode
    {
        auto res = _hw_monitor->send( command{ AMCGET, _type, get_current, mode } );

        if( res.size() < sizeof( int32_t ) )
        {
            std::stringstream s;
            s << "Size of data returned from query(get_current) of " << _type << " is " << res.size()
              << " while min size = " << sizeof( int32_t );
            throw std::runtime_error( s.str() );
        }
        auto val = *( reinterpret_cast< uint32_t * >( res.data() ) );
        return float( val );
    }

    l535_options::l535_options( std::shared_ptr< context > ctx,
                                const platform::backend_device_group & group )
        : device( ctx, group )
        , l500_device( ctx, group )
    {
        auto & raw_depth_sensor = get_raw_depth_sensor();
        auto & depth_sensor = get_depth_sensor();

        // Keep the USB power on while triggering multiple HW monitor commands on it.
        ivcam2::group_multiple_fw_calls( depth_sensor, [&]() {
            auto default_sensor_mode = RS2_SENSOR_MODE_VGA;

            std::map< rs2_option, std::pair< l535_control, std::string > > options = {
                { RS2_OPTION_POST_PROCESSING_SHARPENING,
                  { l535_post_processing_sharpness, "Changes the amount of sharpening in the post-processed image" } },
                { RS2_OPTION_PRE_PROCESSING_SHARPENING,
                  { l535_pre_processing_sharpness, "Changes the amount of sharpening in the pre-processed image" } },
                { RS2_OPTION_NOISE_FILTERING,
                  { l535_noise_filtering, "Control edges and background noise" } },
                { RS2_OPTION_AVALANCHE_PHOTO_DIODE,
                  { l535_apd, "Changes the exposure time of Avalanche Photo Diode in the receiver" } },
                { RS2_OPTION_CONFIDENCE_THRESHOLD,
                  { l535_confidence, "The confidence level threshold to use to mark a pixel as valid by the depth algorithm" } },
                { RS2_OPTION_LASER_POWER,
                  { l535_laser_gain, "Power of the laser emitter, with 0 meaning projector off" } },
                { RS2_OPTION_MIN_DISTANCE,
                  { l535_min_distance, "Minimal distance to the target (in mm)" } },
                { RS2_OPTION_INVALIDATION_BYPASS,
                  { l535_invalidation_bypass, "Enable/disable pixel invalidation" } },
                { RS2_OPTION_INVALIDATION_BYPASS,
                  { l535_invalidation_bypass, "Enable/disable pixel invalidation" } },
                {RS2_OPTION_RX_SENSITIVITY,{ l535_rx_sensitivity, "auto gain"}} //TODO: replace the description
            };

            for( auto i : options )
            {
                auto opt = std::make_shared< l535_hw_options >( this,
                                                                _hw_monitor.get(),
                                                                i.second.first,
                                                                i.second.second );

                depth_sensor.register_option( i.first, opt );
            }

            auto preset = std::make_shared< l535_preset_option >(
                option_range{ RS2_L500_VISUAL_PRESET_CUSTOM,
                              RS2_L500_VISUAL_PRESET_CUSTOM,
                              1,
                              RS2_L500_VISUAL_PRESET_CUSTOM },
                "Preset to calibrate the camera to environment ambient, no ambient or low "
                "ambient." );

            depth_sensor.register_option( RS2_OPTION_VISUAL_PRESET, preset );

        } );
    }

    l535_preset_option::l535_preset_option(option_range range, std::string description)
        :float_option_with_description< rs2_l500_visual_preset >(range, description)
    {}

    void l535_preset_option::set( float value )
    {
        if( static_cast< rs2_l500_visual_preset >( int( value ) )
            == RS2_L500_VISUAL_PRESET_DEFAULT )
            throw invalid_value_exception( to_string()
                                           << "RS2_L500_VISUAL_PRESET_DEFAULT was deprecated!" );

        float_option_with_description< rs2_l500_visual_preset >::set( value );
    }

    void l535_preset_option::set_value( float value )
    {
        float_option_with_description< rs2_l500_visual_preset >::set( value );
    }

}  // namespace librealsense
