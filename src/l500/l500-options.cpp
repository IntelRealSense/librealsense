//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "l500-options.h"
#include "l500-private.h"
#include "l500-depth.h"

const std::string MIN_CONTROLS_FW_VERSION("1.3.9.0");
const std::string MIN_GET_DEFAULT_FW_VERSION( "1.5.4.0" );

namespace librealsense
{
    using namespace ivcam2;

    rs2_sensor_mode query_sensor_mode( option& resolution ) 
    {
        return ( rs2_sensor_mode )(int)resolution.query();
    }

    float l500_hw_options::query() const
    {
        return query_current( query_sensor_mode(*_resolution) );
    }

    void l500_hw_options::set(float value)
    {
        // Block activating alternate IR when IR reflectivity is on [RS5-8358]
        auto &ds = _l500_dev->get_depth_sensor();
        if( ( alternate_ir == _type ) && ( 1.0f == value ) )
        {
            if( ds.supports_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY )
                && ds.get_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY ).query() == 1.0f )
                throw librealsense::wrong_api_call_sequence_exception(
                    "Alternate IR cannot be enabled with IR Reflectivity" );
        }

        _hw_monitor->send(command{ AMCSET, _type, (int)value });
    }

    option_range l500_hw_options::get_range() const
    {
        return _range;
    }

    float l500_hw_options::query_default( bool & success ) const
    {
        success = true;
        if(_fw_version >= firmware_version( MIN_GET_DEFAULT_FW_VERSION ) )
        {
            return query_new_fw_default( success );
        }
        else
        {
            return query_old_fw_default();
        }
    }

    l500_hw_options::l500_hw_options( l500_device* l500_dev,
                                      hw_monitor * hw_monitor,
                                      l500_control type,
                                      option * resolution,
                                      const std::string & description,
                                      firmware_version fw_version,
                                      std::shared_ptr< digital_gain_option > digital_gain )
        : _l500_dev( l500_dev )
        , _hw_monitor( hw_monitor )
        , _type( type )
        , _resolution( resolution )
        , _description( description )
        , _fw_version( fw_version )
        , _digital_gain( digital_gain )
        , _is_read_only(false)
        , _was_set_manually( false )
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

            bool success;
            auto res = query_default( success );

            if( ! success )
            {
                _is_read_only = true;
                res = -1;
            }

            _range = option_range{ min_value,
                                   max_value,
                                   float( *( reinterpret_cast< int32_t * >( step.data() ) ) ),
                                   res };
        } );
    }

    void l500_hw_options::set_default( float def )
    {
        _range.def = def;
    }

    void l500_hw_options::set_read_only( bool read_only )
    {
        _is_read_only = read_only;
    }

    void l500_hw_options::set_manually( bool set ) 
    {
        _was_set_manually = set;
    }

    void l500_hw_options::enable_recording(std::function<void(const option&)> recording_action)
    {}

    // this method can throw an exception if un-expected error occurred
    // or return error by output parameter 'success' if default value not applicable
    // on this state return value is undefined
    float l500_hw_options::query_new_fw_default( bool & success ) const
    {
        success = true;
        hwmon_response response;
        auto res = _hw_monitor->send( command{ AMCGET,
                                               _type,
                                               l500_command::get_default,
                                               (int)query_sensor_mode( *_resolution ) },
                                      &response );

        // Some controls that are automatically set by the FW (e.g., APD when digital gain is AUTO) are read-only
        // and have no defaults: the FW will return hwm_IllegalHwState for these. Some can still be modified (e.g.,
        // laser power & min distance), and for these we expect hwm_Success.
        if( response == hwm_IllegalHwState )
        {
            success = false;
            return -1;
        }
        else if( response != hwm_Success )
        {
            std::stringstream s;
            s << "hw_monitor  AMCGET of " << _type << " return error " << response;
            throw std::runtime_error( s.str() );
        }
        else if (res.size() < sizeof(int32_t))
        {
            std::stringstream s;
            s << "Size of data returned from query(get_default) of " << _type << " is "
              << res.size() << " while min size = " << sizeof( int32_t );
            throw std::runtime_error( s.str() );
        }

        auto val = *( reinterpret_cast< uint32_t * >( res.data() ) );
        return float( val );
    }

    float l500_hw_options::query_old_fw_default( ) const
    {
        // For older FW without support for get_default, we have to:
        //     1. Save the current value
        //     2. Change to -1
        //     3. Wait for the next frame (if streaming)
        //     4. Read the current value
        //     5. Restore the current value
        auto current = query_current( query_sensor_mode( *_resolution ) );
        _hw_monitor->send( command{ AMCSET, _type, -1 } );

        // if the sensor is streaming the value of control will update only when the next frame
        // arrive
        if( _l500_dev->get_depth_sensor().is_streaming() )
            std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

        auto def = query_current( query_sensor_mode( *_resolution ) );

        if( current != def )
            _hw_monitor->send( command{ AMCSET, _type, (int)current } );

        return def;
    }

    float l500_hw_options::query_current( rs2_sensor_mode mode ) const 
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

    l500_options::l500_options(std::shared_ptr<context> ctx, const platform::backend_device_group & group) :
        device(ctx, group),
        l500_device(ctx, group)
    {
        auto& raw_depth_sensor = get_raw_depth_sensor();
        auto& depth_sensor = get_depth_sensor();

        // Keep the USB power on while triggering multiple HW monitor commands on it.
        ivcam2::group_multiple_fw_calls( depth_sensor, [&]() {
            if( _fw_version < firmware_version( MIN_CONTROLS_FW_VERSION ) )
            {
                depth_sensor.register_option(
                    RS2_OPTION_VISUAL_PRESET,
                    std::make_shared< uvc_xu_option< int > >(
                        raw_depth_sensor,
                        ivcam2::depth_xu,
                        ivcam2::L500_DIGITAL_GAIN,
                        "Change the depth digital gain to: 1 for high gain and 2 for low gain",
                        std::map< float, std::string >{
                            { float( RS2_DIGITAL_GAIN_HIGH ), "High Gain" },
                            { float( RS2_DIGITAL_GAIN_LOW ), "Low Gain" } } ) );
            }
            else
            {
                // On USB2 we have only QVGA sensor mode
                bool usb3mode
                    = ( _usb_mode >= platform::usb3_type || _usb_mode == platform::usb_undefined );

                auto default_sensor_mode
                    = static_cast< float >( usb3mode ? RS2_SENSOR_MODE_VGA : RS2_SENSOR_MODE_QVGA );

                auto resolution_option = std::make_shared< sensor_mode_option >(
                    this,
                    option_range{ RS2_SENSOR_MODE_VGA,
                                  RS2_SENSOR_MODE_COUNT - 1,
                                  1,
                                  default_sensor_mode },
                    "Notify the sensor about the intended streaming mode. Required for preset " );

                // changing the resolution affects the defaults
                resolution_option->add_observer( [&]( float ) {
                    update_defaults();

                    auto curr_preset = ( rs2_l500_visual_preset )(int)_preset->query();
                    if( curr_preset != RS2_L500_VISUAL_PRESET_AUTOMATIC
                        && curr_preset != RS2_L500_VISUAL_PRESET_CUSTOM )
                    {
                        change_preset( curr_preset );
                    }
                } );


                depth_sensor.register_option( RS2_OPTION_SENSOR_MODE, resolution_option );

                _digital_gain = std::make_shared< digital_gain_option >(
                    raw_depth_sensor,
                    ivcam2::depth_xu,
                    ivcam2::L500_DIGITAL_GAIN,
                    "Change the depth digital gain to: 1 for high gain and 2 for low gain",
                    std::map< float, std::string >{ /*{ RS2_DIGITAL_GAIN_AUTO, "Auto Gain" },*/
                                                { (float)(RS2_DIGITAL_GAIN_HIGH), "High Gain" },
                                                { (float)(RS2_DIGITAL_GAIN_LOW), "Low Gain" } },
                    _fw_version,
                    this );

                depth_sensor.register_option( RS2_OPTION_DIGITAL_GAIN, _digital_gain );

                if( _fw_version >= firmware_version( "1.5.2.0" ) )
                {
                    _alt_ir = std::make_shared< l500_hw_options >( this,
                                                                   _hw_monitor.get(),
                                                                   alternate_ir,
                                                                   resolution_option.get(),
                                                                   "Enable/Disable alternate IR",
                                                                   _fw_version,
                                                                   _digital_gain );

                    depth_sensor.register_option( RS2_OPTION_ALTERNATE_IR, _alt_ir );
                }

                _hw_options[RS2_OPTION_POST_PROCESSING_SHARPENING]
                    = register_option< l500_hw_options,
                                       l500_device *,
                                       hw_monitor *,
                                       l500_control,
                                       option *,
                                       std::string >(
                        RS2_OPTION_POST_PROCESSING_SHARPENING,
                        this,
                        _hw_monitor.get(),
                        post_processing_sharpness,
                        resolution_option.get(),
                        "Changes the amount of sharpening in the post-processed image",
                        _fw_version,
                        _digital_gain );

                _hw_options[RS2_OPTION_PRE_PROCESSING_SHARPENING]
                    = register_option< l500_hw_options,
                                       l500_device *,
                                       hw_monitor *,
                                       l500_control,
                                       option *,
                                       std::string >(
                        RS2_OPTION_PRE_PROCESSING_SHARPENING,
                        this,
                        _hw_monitor.get(),
                        pre_processing_sharpness,
                        resolution_option.get(),
                        "Changes the amount of sharpening in the pre-processed image",
                        _fw_version,
                        _digital_gain );

                _hw_options[RS2_OPTION_NOISE_FILTERING]
                    = register_option< l500_hw_options,
                                       l500_device *,
                                       hw_monitor *,
                                       l500_control,
                                       option *,
                                       std::string >( RS2_OPTION_NOISE_FILTERING,
                                                      this,
                                                      _hw_monitor.get(),
                                                      noise_filtering,
                                                      resolution_option.get(),
                                                      "Control edges and background noise",
                                                      _fw_version,
                                                      _digital_gain );

                _hw_options[RS2_OPTION_AVALANCHE_PHOTO_DIODE] = register_option< l500_hw_options,
                                                                                 l500_device *,
                                                                                 hw_monitor *,
                                                                                 l500_control,
                                                                                 option *,
                                                                                 std::string >(
                    RS2_OPTION_AVALANCHE_PHOTO_DIODE,
                    this,
                    _hw_monitor.get(),
                    apd,
                    resolution_option.get(),
                    "Changes the exposure time of Avalanche Photo Diode in the receiver",
                    _fw_version,
                    _digital_gain );

                _hw_options[RS2_OPTION_CONFIDENCE_THRESHOLD] = register_option< l500_hw_options,
                                                                                l500_device *,
                                                                                hw_monitor *,
                                                                                l500_control,
                                                                                option *,
                                                                                std::string >(
                    RS2_OPTION_CONFIDENCE_THRESHOLD,
                    this,
                    _hw_monitor.get(),
                    confidence,
                    resolution_option.get(),
                    "The confidence level threshold to use to mark a "
                    "pixel as valid by the depth algorithm",
                    _fw_version,
                    _digital_gain );

                _hw_options[RS2_OPTION_LASER_POWER] = register_option< l500_hw_options,
                                                                       l500_device *,
                                                                       hw_monitor *,
                                                                       l500_control,
                                                                       option *,
                                                                       std::string >(
                    RS2_OPTION_LASER_POWER,
                    this,
                    _hw_monitor.get(),
                    laser_gain,
                    resolution_option.get(),
                    "Power of the laser emitter, with 0 meaning projector off",
                    _fw_version,
                    _digital_gain );

                _hw_options[RS2_OPTION_MIN_DISTANCE]
                    = register_option< l500_hw_options,
                                       l500_device *,
                                       hw_monitor *,
                                       l500_control,
                                       option *,
                                       std::string >( RS2_OPTION_MIN_DISTANCE,
                                                      this,
                                                      _hw_monitor.get(),
                                                      min_distance,
                                                      resolution_option.get(),
                                                      "Minimal distance to the target (in mm)",
                                                      _fw_version,
                                                      _digital_gain );

                _hw_options[RS2_OPTION_INVALIDATION_BYPASS]
                    = register_option< l500_hw_options,
                                       l500_device *,
                                       hw_monitor *,
                                       l500_control,
                                       option *,
                                       std::string >( RS2_OPTION_INVALIDATION_BYPASS,
                                                      this,
                                                      _hw_monitor.get(),
                                                      invalidation_bypass,
                                                      resolution_option.get(),
                                                      "Enable/disable pixel invalidation",
                                                      _fw_version,
                                                      _digital_gain );

                auto preset = calc_preset_from_controls();

                _preset = std::make_shared< l500_preset_option >(
                    option_range{ RS2_L500_VISUAL_PRESET_CUSTOM,
                                  RS2_L500_VISUAL_PRESET_SHORT_RANGE,
                                  1,
                                  RS2_L500_VISUAL_PRESET_MAX_RANGE },
                    "Preset to calibrate the camera to environment ambient, no ambient or low "
                    "ambient.",
                    this );

                _preset->set_value( (float)preset );

                depth_sensor.register_option( RS2_OPTION_VISUAL_PRESET, _preset );

                _advanced_options = get_advanced_controls();
            }
        } );
    }

    std::vector<rs2_option> l500_options::get_advanced_controls()
    {
        std::vector<rs2_option> res;

        res.push_back(RS2_OPTION_DIGITAL_GAIN);
        for (auto&& o : _hw_options)
            res.push_back(o.first);

        return res;
    }

    rs2_l500_visual_preset l500_options::calc_preset_from_controls() const
    {
        try
        {
            // compare default values to current values except for laser power,
            // if we are on preset, we expect that all control's currents values will
            // be aqual to control's default values according to current digital gain value,
            // only laser power can have diffrant value according to preset
            // laser power value will be check later
            for( auto control : _hw_options )
            {
                if( control.first != RS2_OPTION_LASER_POWER && !control.second->is_read_only() )
                {
                    auto curr = control.second->query();
                    auto def = control.second->get_range().def;
                    if( def != curr )
                        return RS2_L500_VISUAL_PRESET_CUSTOM;
                }
            }

            // all the hw_options values are equal to their default values
            // now what is left to check if the gain and laser power correspond to one of the
            // presets
            
            auto laser = _hw_options.find(RS2_OPTION_LASER_POWER);
            if (laser == _hw_options.end())
            {
                LOG_ERROR("RS2_OPTION_LASER_POWER didnt found on hw_options list ");
                return RS2_L500_VISUAL_PRESET_CUSTOM;
            }
            auto max_laser = laser->second->get_range().max;
            auto def_laser = laser->second->get_range().def;

            std::map< std::pair< rs2_digital_gain, float >, rs2_l500_visual_preset >
                gain_and_laser_to_preset = {
                    { { RS2_DIGITAL_GAIN_HIGH, def_laser }, RS2_L500_VISUAL_PRESET_NO_AMBIENT },
                    { { RS2_DIGITAL_GAIN_LOW, max_laser }, RS2_L500_VISUAL_PRESET_LOW_AMBIENT },
                    { { RS2_DIGITAL_GAIN_HIGH, max_laser }, RS2_L500_VISUAL_PRESET_MAX_RANGE },
                    { { RS2_DIGITAL_GAIN_LOW, def_laser }, RS2_L500_VISUAL_PRESET_SHORT_RANGE },
                };

            auto gain = ( rs2_digital_gain )(int)_digital_gain->query();
            auto laser_val = laser->second->query();

            auto it = gain_and_laser_to_preset.find( { gain, laser_val } );

            if( it != gain_and_laser_to_preset.end() )
            {
                return it->second;
            }
            
            return RS2_L500_VISUAL_PRESET_CUSTOM;
        }
        catch( ... )
        {
            LOG_ERROR( "Exception caught in calc_preset_from_controls" );
        }
        return RS2_L500_VISUAL_PRESET_CUSTOM;
    }

    l500_preset_option::l500_preset_option(option_range range, std::string description, l500_options * owner)
        :float_option_with_description< rs2_l500_visual_preset >(range, description),
        _owner( owner )
    {}

    void l500_preset_option::set( float value )
    {
        if( static_cast< rs2_l500_visual_preset >( int( value ) )
            == RS2_L500_VISUAL_PRESET_DEFAULT )
            throw invalid_value_exception( to_string()
                                           << "RS2_L500_VISUAL_PRESET_DEFAULT was deprecated!" );

        verify_max_usable_range_restrictions( RS2_OPTION_VISUAL_PRESET, value );
        _owner->change_preset( static_cast< rs2_l500_visual_preset >( int( value ) ) );

        float_option_with_description< rs2_l500_visual_preset >::set( value );
    }

    void l500_preset_option::set_value( float value )
    {
        float_option_with_description< rs2_l500_visual_preset >::set( value );
    }

    void l500_preset_option::verify_max_usable_range_restrictions( rs2_option opt, float value ) 
    {
        // Block changing visual preset while Max Usable Range is on [RS5-8358]
        if( _owner->get_depth_sensor().supports_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE )
            && ( _owner->get_depth_sensor().get_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE ).query()
                 == 1.0f ) )
        {
            if( ( RS2_OPTION_VISUAL_PRESET == opt )
                && ( value == RS2_L500_VISUAL_PRESET_MAX_RANGE ) )
                return;

            throw wrong_api_call_sequence_exception(
                to_string()
                << "Visual Preset cannot be changed while Max Usable Range is enabled" );
        }
    }

    void l500_options::on_set_option( rs2_option opt, float value )
    {
        auto advanced_controls = get_advanced_controls();
        if( std::find( advanced_controls.begin(), advanced_controls.end(), opt )
            != advanced_controls.end() )
        {
            // when we moved to auto preset we set all controls to -1
            // so we have to set preset controls to defaults values now
            /*auto curr_preset = ( rs2_l500_visual_preset )(int)_preset->query();
            if( curr_preset == RS2_L500_VISUAL_PRESET_AUTOMATIC )
               set_preset_controls_to_defaults();*/

            move_to_custom();
            _hw_options[opt]->set_manually( true );
        }
        else
            throw wrong_api_call_sequence_exception(
                to_string() << "on_set_option support advanced controls only " << opt
                            << " injected" );
    }

    void l500_options::change_gain( rs2_l500_visual_preset preset )
    {
        switch( preset )
        {
        case RS2_L500_VISUAL_PRESET_NO_AMBIENT:
        case RS2_L500_VISUAL_PRESET_MAX_RANGE:
            _digital_gain->set_by_preset( RS2_DIGITAL_GAIN_HIGH );
            break;
        case RS2_L500_VISUAL_PRESET_LOW_AMBIENT:
        case RS2_L500_VISUAL_PRESET_SHORT_RANGE:
            _digital_gain->set_by_preset( RS2_DIGITAL_GAIN_LOW );
            break;
        case RS2_L500_VISUAL_PRESET_AUTOMATIC:
            _digital_gain->set_by_preset( RS2_DIGITAL_GAIN_AUTO );
            break;
        };
    }

    void l500_options::change_alt_ir( rs2_l500_visual_preset preset )
    {
        auto curr_preset = ( rs2_l500_visual_preset )(int)_preset->query();

        if( preset == RS2_L500_VISUAL_PRESET_AUTOMATIC )
            _alt_ir->set( 1 );
        else if( curr_preset == RS2_L500_VISUAL_PRESET_AUTOMATIC
                 && preset != RS2_L500_VISUAL_PRESET_CUSTOM )
            _alt_ir->set( 0 );
    }

    void l500_options::change_laser_power( rs2_l500_visual_preset preset )
    {
        if( preset == RS2_L500_VISUAL_PRESET_LOW_AMBIENT
            || preset == RS2_L500_VISUAL_PRESET_MAX_RANGE )
            set_max_laser();
    }

    void l500_options::change_preset( rs2_l500_visual_preset preset )
    {
        // Keep the USB power on while triggering multiple calls on it.
        ivcam2::group_multiple_fw_calls( get_depth_sensor(), [&]() {
            // we need to reset the controls before change gain because after moving to auto gain
            // APD is read only. This will tell the FW that the control values are defaults and
            // therefore can be overridden automatically according to gain
            if( preset == RS2_L500_VISUAL_PRESET_AUTOMATIC )
            {
                auto curr_preset = ( rs2_l500_visual_preset )(int)_preset->query();
                if( curr_preset == RS2_L500_VISUAL_PRESET_AUTOMATIC )
                    return;
                reset_hw_controls();
            }

            // if the user set preset custom we should not change the controls default or current
            // values the values should stay the same
            if( preset == RS2_L500_VISUAL_PRESET_CUSTOM )
            {
                move_to_custom();
                return;
            }

            // digital gain must be the first option that is set because it
            // impacts the default values of some of the hw controls
            change_gain( preset );
            change_alt_ir( preset );

            if( preset != RS2_L500_VISUAL_PRESET_AUTOMATIC )
                set_preset_controls_to_defaults();

            change_laser_power( preset );
        } );
    }

    void l500_options::set_preset_value( rs2_l500_visual_preset preset ) 
    {
        _preset->set_value( (float)preset );
    }

    void l500_options::set_preset_controls_to_defaults()
    {
        for( auto & o : _hw_options )
        {
            if( ! o.second->is_read_only() )
            {
                auto val = o.second->get_range().def;
                o.second->set_with_no_signal( val );
                o.second->set_manually( false );
            }
        }
    }

    void l500_options::move_to_custom ()
    {
        _preset->set_value( RS2_L500_VISUAL_PRESET_CUSTOM );
    }

    void l500_options::reset_hw_controls()
    {
        // Keep the USB power on while triggering multiple calls on it.
        ivcam2::group_multiple_fw_calls( get_depth_sensor(), [&]() {
            for (auto& o : _hw_options)
                if (!o.second->is_read_only())
                {
                    o.second->set_with_no_signal(-1);
                }
            });
    }

    void l500_options::set_max_laser()
    {
        auto range = _hw_options[RS2_OPTION_LASER_POWER]->get_range();
        _hw_options[RS2_OPTION_LASER_POWER]->set_with_no_signal(range.max);
    }

    // this method not uses l500_hw_options::query_default() because
    // we want to save the 50ms sleep on streaming and do it once to all the controlls
    void l500_options::update_defaults()
    {
        // Keep the USB power on while triggering multiple calls on it.
        ivcam2::group_multiple_fw_calls( get_depth_sensor(), [&]() {

            auto& resolution = get_depth_sensor().get_option(RS2_OPTION_SENSOR_MODE);

            std::map< rs2_option, float > defaults;
            if (_fw_version >= firmware_version(MIN_GET_DEFAULT_FW_VERSION))
            {
                for (auto opt : _hw_options)
                {
                    bool success;
                    defaults[opt.first] = opt.second->query_new_fw_default(success);

                    if (!success)
                    {
                        defaults[opt.first] = -1;
                        opt.second->set_read_only(true);
                    }
                    else
                        opt.second->set_read_only(false);
                }
            }
            else
            {
                // For older FW without support for get_default, we have to:
                //     1. Save the current value
                //     2. Wait for the next frame (if streaming)
                //     3. Change to -1
                //     4. Read the current value and store as default value
                //     5. Restore the current value
                std::map< rs2_option, float > currents;
                for (auto opt : _hw_options)
                    currents[opt.first] = opt.second->query_current(query_sensor_mode(resolution));

                // if the sensor is streaming the value of control will update only when the next frame
                // arrive
                if (get_depth_sensor().is_streaming())
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));

                for (auto opt : _hw_options)
                {
                    opt.second->set_with_no_signal(-1);
                    defaults[opt.first] = opt.second->query_current(query_sensor_mode(resolution));
                }

                for (auto opt : currents)
                {
                    _hw_options[opt.first]->set_with_no_signal(opt.second);
                }
            }

            for (auto opt : _hw_options)
            {
                opt.second->set_default(defaults[opt.first]);
            }
            });
    }

    void max_usable_range_option::set( float value )
    {
        // Restrictions for Max Usable Range option as required on [RS5-8358]
        auto& ds = _l500_depth_dev->get_depth_sensor();
        if (value == 1.0f)
        {
            auto &sensor_mode_option = ds.get_option(RS2_OPTION_SENSOR_MODE);
            auto sensor_mode = sensor_mode_option.query();
            bool sensor_mode_is_vga = (sensor_mode == rs2_sensor_mode::RS2_SENSOR_MODE_VGA);

            bool visual_preset_is_max_range = ds.is_max_range_preset();

            if (ds.is_streaming())
            {
                if (!sensor_mode_is_vga || !visual_preset_is_max_range)
                    throw wrong_api_call_sequence_exception("Please set 'VGA' resolution and 'Max Range' preset before enabling Max Usable Range");
            }
            else
            {
                if (!visual_preset_is_max_range)
                {
                    auto &visual_preset_option = ds.get_option(RS2_OPTION_VISUAL_PRESET);
                    visual_preset_option.set(rs2_l500_visual_preset::RS2_L500_VISUAL_PRESET_MAX_RANGE);
                    LOG_INFO("Visual Preset was changed to: " << visual_preset_option.get_value_description(rs2_l500_visual_preset::RS2_L500_VISUAL_PRESET_MAX_RANGE));
                }

                if (!sensor_mode_is_vga)
                {
                    sensor_mode_option.set(rs2_sensor_mode::RS2_SENSOR_MODE_VGA);
                    LOG_INFO("Sensor Mode was changed to: " << sensor_mode_option.get_value_description(rs2_sensor_mode::RS2_SENSOR_MODE_VGA));
                }
            }
        }
        else
        {
            if (ds.supports_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY) && ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY).query() == 1.0f)
            {
                ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY).set(0.0f);
                LOG_INFO("IR Reflectivity was on - turning it off");
            }
        }

        bool_option::set(value);
    }

    void sensor_mode_option::set(float value)
    {
        if (_value == value) return;

        // Restrictions for sensor mode option as required on [RS5-8358]
        auto &ds = _l500_depth_dev->get_depth_sensor();

        if( ds.supports_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY )
            && ds.get_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY ).query() == 1.0f
            && ( value != rs2_sensor_mode::RS2_SENSOR_MODE_VGA ) )
        {
            ds.get_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY ).set( 0.0f );
            LOG_INFO( "IR Reflectivity was on - turning it off" );
        }

        if( ds.supports_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE )
            && ( ds.get_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE ).query() == 1.0f )
            && ( value != rs2_sensor_mode::RS2_SENSOR_MODE_VGA ) )
        {
            ds.get_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE ).set( 0.0f );
            LOG_INFO( "Max Usable Range was on - turning it off" );
        }

        float_option_with_description::set(value);

        // Keep the USB power on while triggering multiple calls on it.
        ivcam2::group_multiple_fw_calls( ds, [&]() { notify( value ); } );
    }

    const char * max_usable_range_option::get_description() const
    {

        return "Max Usable Range calculates the maximum range of the camera given the amount of "
               "ambient light in the scene.\n"
               "For example, if Max Usable Range returns 5m, this means that the ambient light in "
               "the scene is reducing the maximum range from 9m down to 5m.\n"
               "Values are between 1.5 and 9 meters, in 1.5m increments. "
               "Max range refers to the center 10% of the frame.";
    }

    void ir_reflectivity_option::set(float value)
    {
        // Restrictions for IR Reflectivity option as required on [RS5-8358]
        auto &ds = _l500_depth_dev->get_depth_sensor();
        if (value == 1.0f)
        {
            // Verify option Alternate IR is off
            if( ds.supports_option( RS2_OPTION_ALTERNATE_IR )
                && ds.get_option( RS2_OPTION_ALTERNATE_IR ).query() == 1.0f )
            {
                throw wrong_api_call_sequence_exception("Cannot enable IR Reflectivity when Alternate IR option is on");
            }

            auto &sensor_mode_option = ds.get_option(RS2_OPTION_SENSOR_MODE);
            auto sensor_mode = sensor_mode_option.query();
            bool sensor_mode_is_vga = (sensor_mode == rs2_sensor_mode::RS2_SENSOR_MODE_VGA);

            bool visual_preset_is_max_range = ds.is_max_range_preset();

            if( ds.is_streaming() )
            {
                // While streaming we use active stream resolution and not sensor mode due as a
                // patch for the DQT sensor mode wrong handling. 
                // This should be changed for using sensor mode after DQT fix
                auto active_streams = ds.get_active_streams();
                auto dp = std::find_if( active_streams.begin(),
                                        active_streams.end(),
                                        []( std::shared_ptr< stream_profile_interface > sp ) {
                                            return sp->get_stream_type() == RS2_STREAM_DEPTH;
                                        } );

                bool vga_sensor_mode = false;
                if( dp != active_streams.end() )
                {
                    auto vs = dynamic_cast< video_stream_profile * >( ( *dp ).get() );
                    vga_sensor_mode
                        = ( get_resolution_from_width_height( vs->get_width(), vs->get_height() )
                            == RS2_SENSOR_MODE_VGA );
                }

                if( ( ! vga_sensor_mode ) || ! visual_preset_is_max_range )
                    throw wrong_api_call_sequence_exception(
                        "Please set 'VGA' resolution, 'Max Range' preset and 20% ROI before enabling IR Reflectivity" );
            }
            else
            {
                if (!visual_preset_is_max_range)
                {
                    auto &visual_preset_option = ds.get_option(RS2_OPTION_VISUAL_PRESET);
                    visual_preset_option.set(rs2_l500_visual_preset::RS2_L500_VISUAL_PRESET_MAX_RANGE);
                    LOG_INFO("Visual Preset was changed to: " << visual_preset_option.get_value_description(rs2_l500_visual_preset::RS2_L500_VISUAL_PRESET_MAX_RANGE));
                }

                if (!sensor_mode_is_vga)
                {
                    sensor_mode_option.set(rs2_sensor_mode::RS2_SENSOR_MODE_VGA);
                    LOG_INFO("Sensor Mode was changed to: " << sensor_mode_option.get_value_description(rs2_sensor_mode::RS2_SENSOR_MODE_VGA));
                }
            }

            auto &max_usable_range_option = ds.get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE);
            if (max_usable_range_option.query() != 1.0f)
            {
                max_usable_range_option.set(1.0f);
                _max_usable_range_forced_on = true;
                LOG_INFO("Max Usable Range was off - turning it on");
            }
        }
        else
        {
            if (_max_usable_range_forced_on)
            {
                _max_usable_range_forced_on = false;
                bool_option::set( value );  // We set the value here to prevent a loop between Max Usable Range &
                                            // Reflectivity options trying to turn off each other

                ds.get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE).set(0.0f);
                LOG_INFO("Max Usable Range was on - turning it off");
                return;
            }
        }

        bool_option::set(value);
    }

    const char * ir_reflectivity_option::get_description() const
    {

        return "IR Reflectivity Tool calculates the percentage of IR light reflected by the "
               "object and returns to the camera for processing.\nFor example, a value of 60% means "
               "that 60% of the IR light projected by the camera is reflected by the object and returns "
               "to the camera.\n\n"
               "Note: only active on 2D view, Visual Preset:Max Range, Resolution:VGA, ROI:20%";
    }

    void digital_gain_option::set( float value ) 
    {
        super::set( value );
        _owner->update_defaults();

        // when we moved to auto preset we set all controls to -1
        // so we have to set preset controls to defaults values now
        /*auto curr_preset = ( rs2_l500_visual_preset )(int)_preset->query();
        if( curr_preset == RS2_L500_VISUAL_PRESET_AUTOMATIC )
           set_preset_controls_to_defaults();*/

        _owner->move_to_custom();
    }

    // set gain as result of change preset
    void digital_gain_option::set_by_preset( float value )
    {
        work_around_for_old_fw();

        super::set( value );
        _owner->update_defaults();
    }

    // FW expose 'auto gain' with value 0 as one of the values of gain option in addition
    // to 'low gain' and 'high gain'
    // for now we don't want to expose it to user so the range starts from 1 
    // once we want to expose it we will remove this override method
    option_range digital_gain_option::get_range() const
    {
        auto range = uvc_xu_option< int >::get_range();
        range.min = 1;
        return range;
    }

    void digital_gain_option::work_around_for_old_fw() 
    {
        // On old FW versions the way to get the default values of the hw commands is to
        // reset hw commands current values -1 and than get the current values
        // on some of the old FW versions there is a bug that we must reset hw commands
        // before setting the digital gain, otherwise its not updates the current with default
        // values in digital_gain_option class we override the set_with_no_signal that called when
        // changing preset and reset hw commands before setting the digital gain as WA to this bug
        // we still have a limit on the scenario that user change digital gain manually (not from
        // preset) we won't get the correct default values
        if( _fw_version < firmware_version( MIN_GET_DEFAULT_FW_VERSION ) )
            _owner->reset_hw_controls();  // should be before gain changing as WA for old FW versions
    }
}  // namespace librealsense
