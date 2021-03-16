// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "l500-color.h"

#include <cstddef>
#include <mutex>



#include "l500-private.h"
#include "proc/color-formats-converter.h"
#include "ac-trigger.h"
#include "algo/depth-to-rgb-calibration/debug.h"
#include "algo/thermal-loop/l500-thermal-loop.h"


namespace librealsense
{
    using namespace ivcam2;

    std::map<uint32_t, rs2_format> l500_color_fourcc_to_rs2_format = {
        {rs_fourcc('Y','U','Y','2'), RS2_FORMAT_YUYV},
        {rs_fourcc('Y','U','Y','V'), RS2_FORMAT_YUYV},
        {rs_fourcc('U','Y','V','Y'), RS2_FORMAT_UYVY}
    };

    std::map<uint32_t, rs2_stream> l500_color_fourcc_to_rs2_stream = {
        {rs_fourcc('Y','U','Y','2'), RS2_STREAM_COLOR},
        {rs_fourcc('Y','U','Y','V'), RS2_STREAM_COLOR},
        {rs_fourcc('U','Y','V','Y'), RS2_STREAM_COLOR}
    };

    std::shared_ptr<synthetic_sensor> l500_color::create_color_device(std::shared_ptr<context> ctx, const std::vector<platform::uvc_device_info>& color_devices_info)
    {
        auto&& backend = ctx->get_backend();

        std::unique_ptr<frame_timestamp_reader> timestamp_reader_metadata(new ivcam2::l500_timestamp_reader_from_metadata(backend.create_time_service()));
        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());
        auto raw_color_ep = std::make_shared<uvc_sensor>("RGB Camera", ctx->get_backend().create_uvc_device(color_devices_info.front()),
            std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(timestamp_reader_metadata), _tf_keeper, enable_global_time_option)),
            this);
        auto color_ep = std::make_shared<l500_color_sensor>(this, raw_color_ep, ctx, l500_color_fourcc_to_rs2_format, l500_color_fourcc_to_rs2_stream);

        color_ep->register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, color_devices_info.front().device_path);

        // processing blocks
        if( _autocal )
        {
            color_ep->register_processing_block(
                processing_block_factory::create_pbf_vector< yuy2_converter >(
                    RS2_FORMAT_YUYV,                                                      // from
                    map_supported_color_formats( RS2_FORMAT_YUYV ), RS2_STREAM_COLOR,     // to
                    [=]( std::shared_ptr< generic_processing_block > pb )
                    {
                        auto cpb = std::make_shared< composite_processing_block >();
                        cpb->add(std::make_shared< ac_trigger::color_processing_block >(_autocal));
                        cpb->add( pb );
                        return cpb;
                    } ) );
        }
        else
        {
            color_ep->register_processing_block(
                processing_block_factory::create_pbf_vector< yuy2_converter >(
                    RS2_FORMAT_YUYV,                                                      // from
                    map_supported_color_formats( RS2_FORMAT_YUYV ), RS2_STREAM_COLOR ) ); // to
        }

        // options
        color_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);
        color_ep->get_option(RS2_OPTION_GLOBAL_TIME_ENABLED).set(0);
        color_ep->register_pu(RS2_OPTION_BACKLIGHT_COMPENSATION);
        color_ep->register_pu(RS2_OPTION_BRIGHTNESS);
        color_ep->register_pu(RS2_OPTION_CONTRAST);
        color_ep->register_pu(RS2_OPTION_GAIN);
        color_ep->register_pu(RS2_OPTION_HUE);
        color_ep->register_pu(RS2_OPTION_SATURATION);
        color_ep->register_pu(RS2_OPTION_SHARPNESS);
        color_ep->register_pu(RS2_OPTION_AUTO_EXPOSURE_PRIORITY);

        color_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);

        auto white_balance_option = std::make_shared<uvc_pu_option>(*raw_color_ep, RS2_OPTION_WHITE_BALANCE);
        auto auto_white_balance_option = std::make_shared<uvc_pu_option>(*raw_color_ep, RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);
        color_ep->register_option(RS2_OPTION_WHITE_BALANCE, white_balance_option);
        color_ep->register_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, auto_white_balance_option);
        color_ep->register_option(RS2_OPTION_WHITE_BALANCE,
            std::make_shared<auto_disabling_control>(
                white_balance_option,
                auto_white_balance_option));

        auto exposure_option = std::make_shared<uvc_pu_option>(*raw_color_ep, RS2_OPTION_EXPOSURE);
        auto auto_exposure_option = std::make_shared<uvc_pu_option>(*raw_color_ep, RS2_OPTION_ENABLE_AUTO_EXPOSURE);
        color_ep->register_option(RS2_OPTION_EXPOSURE, exposure_option);
        color_ep->register_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, auto_exposure_option);
        color_ep->register_option(RS2_OPTION_EXPOSURE,
            std::make_shared<auto_disabling_control>(
                exposure_option,
                auto_exposure_option));

        color_ep->register_option(RS2_OPTION_POWER_LINE_FREQUENCY,
            std::make_shared<uvc_pu_option>(*raw_color_ep, RS2_OPTION_POWER_LINE_FREQUENCY,
                std::map<float, std::string>{ { 0.f, "Disabled"},
                { 1.f, "50Hz" },
                { 2.f, "60Hz" },
                { 3.f, "Auto" }, }));


        // option to enable workaround for help weak usb hosts to support L515 devices
        // with improved performance and stability
        if(_fw_version >= firmware_version( "1.5.1.0" ) )
        {
            rs2_host_perf_mode launch_perf_mode = RS2_HOST_PERF_DEFAULT;

#ifdef __ANDROID__
            // android devices most likely low power low performance host
            launch_perf_mode = RS2_HOST_PERF_LOW;
#else
            // other hosts use default settings from firmware, user still have the option to change later after launch
            launch_perf_mode = RS2_HOST_PERF_DEFAULT;
#endif

            color_ep->register_option(RS2_OPTION_HOST_PERFORMANCE, std::make_shared<librealsense::float_option_with_description<rs2_host_perf_mode>>(option_range{ RS2_HOST_PERF_DEFAULT, RS2_HOST_PERF_COUNT - 1, 1, (float) launch_perf_mode }, "Optimize based on host performance, low power low performance host or high power high performance host"));
        }

        // metadata
        // attributes of md_capture_timing
        auto md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_rgb_normal_mode, intel_capture_timing);

        color_ep->register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER, make_attribute_parser(&l500_md_capture_timing::frame_counter, md_capture_timing_attributes::frame_counter_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP, make_attribute_parser(&l500_md_capture_timing::sensor_timestamp, md_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_ACTUAL_FPS, make_attribute_parser(&l500_md_capture_timing::exposure_time, md_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset));

        // attributes of md_capture_stats
        md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_rgb_normal_mode, intel_capture_stats);

        color_ep->register_metadata(RS2_FRAME_METADATA_WHITE_BALANCE, make_attribute_parser(&md_capture_stats::white_balance, md_capture_stat_attributes::white_balance_attribute, md_prop_offset));

        // attributes of md_rgb_control
        md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_rgb_normal_mode, intel_rgb_control);

        color_ep->register_metadata(RS2_FRAME_METADATA_GAIN_LEVEL, make_attribute_parser(&md_rgb_control::gain, md_rgb_control_attributes::gain_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE, make_attribute_parser(&md_rgb_control::manual_exp, md_rgb_control_attributes::manual_exp_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_AUTO_EXPOSURE, make_attribute_parser(&md_rgb_control::ae_mode, md_rgb_control_attributes::ae_mode_attribute, md_prop_offset,
            [](rs2_metadata_type param) { return (param != 1); }));
        color_ep->register_metadata(RS2_FRAME_METADATA_BRIGHTNESS, make_attribute_parser(&md_rgb_control::brightness, md_rgb_control_attributes::brightness_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_CONTRAST, make_attribute_parser(&md_rgb_control::contrast, md_rgb_control_attributes::contrast_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_SATURATION, make_attribute_parser(&md_rgb_control::saturation, md_rgb_control_attributes::saturation_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_SHARPNESS, make_attribute_parser(&md_rgb_control::sharpness, md_rgb_control_attributes::sharpness_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_AUTO_WHITE_BALANCE_TEMPERATURE, make_attribute_parser(&md_rgb_control::awb_temp, md_rgb_control_attributes::awb_temp_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_BACKLIGHT_COMPENSATION, make_attribute_parser(&md_rgb_control::backlight_comp, md_rgb_control_attributes::backlight_comp_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_GAMMA, make_attribute_parser(&md_rgb_control::gamma, md_rgb_control_attributes::gamma_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_HUE, make_attribute_parser(&md_rgb_control::hue, md_rgb_control_attributes::hue_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_MANUAL_WHITE_BALANCE, make_attribute_parser(&md_rgb_control::manual_wb, md_rgb_control_attributes::manual_wb_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_POWER_LINE_FREQUENCY, make_attribute_parser(&md_rgb_control::power_line_frequency, md_rgb_control_attributes::power_line_frequency_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_LOW_LIGHT_COMPENSATION, make_attribute_parser(&md_rgb_control::low_light_comp, md_rgb_control_attributes::low_light_comp_attribute, md_prop_offset));
        color_ep->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_uvc_header_parser(&platform::uvc_header::timestamp));

        // We manipulate several controls when the CAH process turn on/off the color stream
        // This should be done after registration of the device options
        if (_autocal)
        {
            color_ep->register_calibration_controls();
        }

        return color_ep;
    }

    l500_color::l500_color(std::shared_ptr<context> ctx, const platform::backend_device_group & group)
        :device(ctx, group),
        l500_device(ctx, group),
         _color_stream(new stream(RS2_STREAM_COLOR))
    {
        auto color_devs_info = filter_by_mi(group.uvc_devices, 4);
        if (color_devs_info.size() != 1)
            throw invalid_value_exception(to_string() << "L500 with RGB models are expected to include a single color device! - "
                << color_devs_info.size() << " found");

        _color_intrinsics_table = [this]() { return read_intrinsics_table(); };
        _color_extrinsics_table_raw = [this]() { return get_raw_extrinsics_table(); };

        // This lazy instance will get shared between all the extrinsics edges. If you ever need to override
        // it, be careful not to overwrite the shared-ptr itself (register_extrinsics) or the sharing
        // will get ruined. Instead, overwriting the lazy<> function should do it:
        //      *_color_extrinsic = [=]() { return extr; };
        _color_extrinsic = std::make_shared<lazy<rs2_extrinsics>>(
            [this]()
            {
                return get_color_stream_extrinsic(*_color_extrinsics_table_raw);
            } );
        environment::get_instance().get_extrinsics_graph().register_extrinsics(*_depth_stream, *_color_stream, _color_extrinsic);
        register_stream_to_extrinsic_group(*_color_stream, 0);

        _thermal_table = [this]() {

            hwmon_response response;
            auto data = read_fw_table_raw( *_hw_monitor,
                                           algo::thermal_loop::l500::thermal_calibration_table::id,
                                           response );
            if( response != hwm_Success )
            {
                AC_LOG( WARNING,
                        "Failed to read FW table 0x"
                            << std::hex
                            << algo::thermal_loop::l500::thermal_calibration_table::id );
                throw invalid_value_exception(
                    to_string() << "Failed to read FW table 0x" << std::hex
                                << algo::thermal_loop::l500::thermal_calibration_table::id );
            }

            if( data.size() > sizeof( table_header ))
                data.erase( data.begin(), data.begin() + sizeof( table_header ) );
            return algo::thermal_loop::l500::thermal_calibration_table{ data };
        };

        _color_device_idx = add_sensor(create_color_device(ctx, color_devs_info));
    }

    l500_color_sensor * l500_color::get_color_sensor()
    {
        return &dynamic_cast< l500_color_sensor & >( get_sensor( _color_device_idx ));
    }


    rs2_intrinsics l500_color_sensor::get_raw_intrinsics( uint32_t const width,
                                                          uint32_t const height ) const
    {
        using namespace ivcam2;

        auto & intrinsic = *_owner->_color_intrinsics_table;

        auto num_of_res = intrinsic.resolution.num_of_resolutions;

        for( auto i = 0; i < num_of_res; i++ )
        {
            auto model = intrinsic.resolution.intrinsic_resolution[i];
            if( model.height == height && model.width == width )
            {
                rs2_intrinsics intrinsics;
                intrinsics.width = model.width;
                intrinsics.height = model.height;
                intrinsics.fx = model.ipm.focal_length.x;
                intrinsics.fy = model.ipm.focal_length.y;
                intrinsics.ppx = model.ipm.principal_point.x;
                intrinsics.ppy = model.ipm.principal_point.y;

                if( model.distort.radial_k1 || model.distort.radial_k2
                    || model.distort.tangential_p1 || model.distort.tangential_p2
                    || model.distort.radial_k3 )
                {
                    intrinsics.coeffs[0] = model.distort.radial_k1;
                    intrinsics.coeffs[1] = model.distort.radial_k2;
                    intrinsics.coeffs[2] = model.distort.tangential_p1;
                    intrinsics.coeffs[3] = model.distort.tangential_p2;
                    intrinsics.coeffs[4] = model.distort.radial_k3;

                    intrinsics.model = RS2_DISTORTION_INVERSE_BROWN_CONRADY;
                }

                return intrinsics;
            }
        }
        throw std::runtime_error( to_string() << "intrinsics for resolution " << width << ","
                                              << height << " don't exist" );
    }

    rs2_intrinsics normalize( const rs2_intrinsics & intr )
    {
        auto res = intr;
        res.fx = 2 * intr.fx / intr.width;
        res.fy = 2 * intr.fy / intr.height;
        res.ppx = 2 * intr.ppx / intr.width - 1;
        res.ppy = 2 * intr.ppy / intr.height - 1;

        return res;
    }

    rs2_intrinsics
    denormalize( const rs2_intrinsics & intr, const uint32_t & width, const uint32_t & height )
    {
        auto res = intr;

        res.fx = intr.fx * width / 2;
        res.fy = intr.fy * height / 2;
        res.ppx = ( intr.ppx + 1 ) * width / 2;
        res.ppy = ( intr.ppy + 1 ) * height / 2;

        res.width = width;
        res.height = height;

        return res;
    }


    rs2_intrinsics l500_color_sensor::get_intrinsics( const stream_profile & profile ) const
    {
        if( ! _k_thermal_intrinsics)
        {
            // Until we've calculated temp-based intrinsics, simply use the camera-specified
            // intrinsics
            return get_raw_intrinsics( profile.width, profile.height );
        }

        return denormalize( *_k_thermal_intrinsics, profile.width, profile.height );
    }


    rs2_intrinsics ivcam2::rgb_calibration_table::get_intrinsics() const
    {
        // TODO: we currently use the wrong distortion model, but all the code is
        // written to expect the INVERSE brown. The table assumes REGULAR brown.
        return { width, height,
                 intr.px, intr.py,  // NOTE: this is normalized!
                 intr.fx, intr.fy,  // NOTE: this is normalized!
                 RS2_DISTORTION_INVERSE_BROWN_CONRADY,  // see comment above
                 { intr.d[0], intr.d[1], intr.d[2], intr.d[3], intr.d[4] } };
    }

    void ivcam2::rgb_calibration_table::set_intrinsics( rs2_intrinsics const & i )
    {
        // The table in FW is resolution-agnostic; it can apply to ALL resolutions. To
        // do this, the focal length and principal point are normalized:
        auto norm = normalize( i );

        width = i.width;
        height = i.height;
        intr.fx = norm.fx;
        intr.fy = norm.fy;
        intr.px = norm.ppx;
        intr.py = norm.ppy;
        intr.d[0] = i.coeffs[0];
        intr.d[1] = i.coeffs[1];
        intr.d[2] = i.coeffs[2];
        intr.d[3] = i.coeffs[3];
        intr.d[4] = i.coeffs[4];
    }


    void l500_color_sensor::override_intrinsics( rs2_intrinsics const& intr )
    {
        // The distortion model is not part of the table. The FW assumes it is brown,
        // but in LRS we (mistakenly) use INVERSE brown. We therefore make sure the user
        // has not tried to change anything from the intrinsics reported:
        if( intr.model != RS2_DISTORTION_INVERSE_BROWN_CONRADY )
            throw invalid_value_exception( "invalid intrinsics distortion model" );

        rgb_calibration_table table;
        AC_LOG( DEBUG, "Reading RGB calibration table 0x" << std::hex << table.table_id );
        ivcam2::read_fw_table( *_owner->_hw_monitor, table.table_id, &table );
        AC_LOG( DEBUG, "    version:     " << table.version );
        AC_LOG( DEBUG, "    timestamp:   " << table.timestamp << "; incrementing" );
        AC_LOG( DEBUG, "    type:        " << table.type << "; setting to 0x10" );
        AC_LOG( DEBUG, "    intrinsics:  " << table.get_intrinsics() );
        table.set_intrinsics( intr );
        AC_LOG( INFO, "Overriding intr: " << intr );
        AC_LOG( DEBUG, "    normalized:  " << table.get_intrinsics() );
        table.update_write_fields();
        write_fw_table( *_owner->_hw_monitor, table.table_id, table );
        AC_LOG( DEBUG, "    done" );

        // Intrinsics are resolution-specific, so all the rest of the profile info is not
        // important
        _owner->_color_intrinsics_table.reset();
        reset_k_thermal_intrinsics();
    }

    void l500_color_sensor::override_extrinsics( rs2_extrinsics const& extr )
    {
        rgb_calibration_table table;
        AC_LOG( DEBUG, "Reading RGB calibration table 0x" << std::hex << table.table_id );
        ivcam2::read_fw_table( *_owner->_hw_monitor, table.table_id, &table );
        AC_LOG( DEBUG, "    version:     " << table.version );
        AC_LOG( DEBUG, "    timestamp:   " << table.timestamp << "; incrementing" );
        AC_LOG( DEBUG, "    type:        " << table.type << "; setting to 0x10" );
        AC_LOG( DEBUG, "    raw extr:    " << table.get_extrinsics() );
        table.extr = to_raw_extrinsics(extr);
        AC_LOG( INFO , "Overriding extr: " << extr );
        table.update_write_fields();
        AC_LOG( DEBUG, "    as raw:      " << table.get_extrinsics());
        ivcam2::write_fw_table( *_owner->_hw_monitor, table.table_id, table );
        AC_LOG( DEBUG, "    done" );


        environment::get_instance().get_extrinsics_graph().override_extrinsics( *_owner->_depth_stream, *_owner->_color_stream, extr );
    }

    rs2_dsm_params l500_color_sensor::get_dsm_params() const
    {
        throw std::logic_error( "color sensor does not support DSM parameters" );
    }

    void l500_color_sensor::override_dsm_params( rs2_dsm_params const & dsm )
    {
        throw std::logic_error( "color sensor does not support DSM parameters" );
    }

    void l500_color_sensor::set_k_thermal_intrinsics( rs2_intrinsics const & intr )
    {

        _k_thermal_intrinsics = std::make_shared< rs2_intrinsics >( normalize(intr) );
    }

    void l500_color_sensor::reset_k_thermal_intrinsics() 
    {
        _k_thermal_intrinsics.reset();
    }

    algo::thermal_loop::l500::thermal_calibration_table l500_color_sensor::get_thermal_table() const
    {
        return *_owner->_thermal_table;
    }

    void ivcam2::rgb_calibration_table::update_write_fields()
    {
        // We don't touch the version...
        //version = ;

        // This signifies AC results:
        type = 0x10;

        // The time-stamp is simply a sequential number that we increment
        ++timestamp;
    }

    void l500_color_sensor::reset_calibration()
    {
        // Read from EEPROM (factory defaults), write to FLASH (current)
        // Note that factory defaults may be different than the trinsics at the time of
        // our initialization!
        rgb_calibration_table table;
        AC_LOG( DEBUG, "Reading factory calibration from table 0x" << std::hex << table.eeprom_table_id );
        ivcam2::read_fw_table( *_owner->_hw_monitor, table.eeprom_table_id, &table );
        AC_LOG( DEBUG, "    version:     " << table.version );
        AC_LOG( DEBUG, "    timestamp:   " << table.timestamp << "; incrementing" );
        AC_LOG( DEBUG, "    type:        " << table.type << "; setting to 0x10" );
        AC_LOG( DEBUG, "Normalized:" );
        AC_LOG( DEBUG, "    intrinsics:  " << table.get_intrinsics() );
        AC_LOG( DEBUG, "    extrinsics:  " << table.get_extrinsics() );
        AC_LOG( DEBUG, "Writing RGB calibration table 0x" << std::hex << table.table_id );
        ivcam2::write_fw_table( *_owner->_hw_monitor, table.table_id, table );
        AC_LOG( DEBUG, "    done" );

        _owner->_color_intrinsics_table.reset();
        reset_k_thermal_intrinsics();

         environment::get_instance().get_extrinsics_graph().override_extrinsics(
            *_owner->_depth_stream,
            *_owner->_color_stream,
            from_raw_extrinsics( table.get_extrinsics() ) );
        AC_LOG( INFO, "Color sensor calibration has been reset" );
    }

    void l500_color_sensor::start(frame_callback_ptr callback)
    {
        std::lock_guard<std::mutex> lock(_state_mutex);

        if (_state != sensor_state::OWNED_BY_USER)
            throw wrong_api_call_sequence_exception("tried to start an unopened sensor");

        if (supports_option(RS2_OPTION_HOST_PERFORMANCE))
        {
            // option to improve performance and stability on weak android hosts
            // please refer to following bug report for details
            // RS5-8011 [Android-L500 Hard failure] Device detach and disappear from system during stream

            auto host_perf = get_option(RS2_OPTION_HOST_PERFORMANCE).query();

            if (host_perf == RS2_HOST_PERF_LOW || host_perf == RS2_HOST_PERF_HIGH)
            {
                // TPROC USB Granularity and TRB threshold settings for improved performance and stability
                // on hosts with weak cpu and system performance
                // settings values are validated through many experiments, do not change

                // on low power low performance host, usb trb set to larger granularity to reduce number of transactions
                // and improve performance
                int usb_trb = 7;       // 7 KB

                if (host_perf == RS2_HOST_PERF_LOW)
                {
                    usb_trb = 32;      // 32 KB
                }

                try {
                    // Keep the USB power on while triggering multiple calls on it.
                    ivcam2::group_multiple_fw_calls(*this, [&]() {
                        // endpoint 5 - 32KB
                        command cmdTprocGranEp5(ivcam2::TPROC_USB_GRAN_SET, 5, usb_trb);
                        _owner->_hw_monitor->send(cmdTprocGranEp5);

                        command cmdTprocThresholdEp5(ivcam2::TPROC_TRB_THRSLD_SET, 5, 1);
                        _owner->_hw_monitor->send(cmdTprocThresholdEp5);
                        });
                    LOG_DEBUG("Color usb tproc granularity and TRB threshold updated.");
                } catch (...)
                {
                    LOG_WARNING("Failed to update color usb tproc granularity and TRB threshold. performance and stability maybe impacted on certain platforms.");
                }
            }
            else if (host_perf == RS2_HOST_PERF_DEFAULT)
            {
                    LOG_DEBUG("Default host performance mode, color usb tproc granularity and TRB threshold not changed");
            }
        }

        delayed_start(callback);
    }

    
    void l500_color_sensor::stop()
    {
        std::lock_guard<std::mutex> lock(_state_mutex);
        
        // Protect not stopping the calibration color stream due to wrong API sequence
        if (_state != sensor_state::OWNED_BY_USER)
            throw wrong_api_call_sequence_exception("tried to stop sensor without starting it");

        delayed_stop();
    }


    void l500_color_sensor::open( const stream_profiles & requests )
    {
        std::lock_guard< std::mutex > lock( _state_mutex );

        if( sensor_state::OWNED_BY_AUTO_CAL == _state )
        {
            if( is_streaming() )
            {
                delayed_stop();
            }
            if( is_opened() )
            {
                AC_LOG( DEBUG, "Calibration color stream was on, Closing color sensor..." );
                synthetic_sensor::close();
            }

            restore_pre_calibration_controls();

            set_sensor_state( sensor_state::CLOSED );
        }

        synthetic_sensor::open( requests );
        set_sensor_state( sensor_state::OWNED_BY_USER );
    }

    
    void l500_color_sensor::close()
    {
        std::lock_guard< std::mutex > lock( _state_mutex );

        if( _state != sensor_state::OWNED_BY_USER )
            throw wrong_api_call_sequence_exception( "tried to close sensor without opening it" );

        synthetic_sensor::close();
        set_sensor_state(sensor_state::CLOSED);
    }

    // Helper function for start stream callback
    template<class T>
    frame_callback_ptr make_frame_callback(T callback)
    {
        return {
            new internal_frame_callback<T>(callback),
            [](rs2_frame_callback* p) { p->release(); }
        };
    }

    bool l500_color_sensor::start_stream_for_calibration(const stream_profiles& requests)
    {
        std::lock_guard< std::mutex > lock( _state_mutex );

        // Allow calibration process to open the color stream only if it is not started by the user.
        if( _state == sensor_state::CLOSED )
        {
            set_calibration_controls_to_defaults();

            synthetic_sensor::open(requests);
            set_sensor_state(sensor_state::OWNED_BY_AUTO_CAL);
            AC_LOG( DEBUG, "Starting color sensor stream -- for calibration" );
            delayed_start( make_frame_callback( [&]( frame_holder fref ) {} ) );
            return true;
        }
        if( ! is_streaming() )
        {
            // This is a corner case that is not covered at the moment: The user opened the sensor
            // but did not start it.
            AC_LOG( WARNING,
                    "The color sensor was opened but never started by the user; streaming may not work" );
        }
        else
            AC_LOG( DEBUG, "Color sensor is already streaming (" << state_to_string(_state) << ")" );
        return false;
    }

    void l500_color_sensor::stop_stream_for_calibration()
    {
        std::lock_guard< std::mutex > lock( _state_mutex );
        
        if( _state == sensor_state::OWNED_BY_AUTO_CAL )
        {
            AC_LOG(DEBUG, "Closing color sensor stream from calibration");

            if( is_streaming() )
            {
                delayed_stop();

            }
            if (is_opened())
            {
                synthetic_sensor::close();
            }

            restore_pre_calibration_controls();

            // If we got here with no exception it means the start has succeeded.
            set_sensor_state( sensor_state::CLOSED );
        }
        else
        {
            AC_LOG( DEBUG, "Color sensor was not opened by us; no need to close" );
        }
    }

    std::string l500_color_sensor::state_to_string(sensor_state state)
    {
        switch (state)
        {
        case sensor_state::CLOSED:
            return "CLOSED";
        case sensor_state::OWNED_BY_AUTO_CAL:
            return "OWNED_BY_AUTO_CAL";
        case sensor_state::OWNED_BY_USER:
            return "OWNED_BY_USER";
        default:
            LOG_DEBUG("Invalid color sensor state: " << static_cast<int>(state));
            return "Unknown state";
        }
    }


    void l500_color_sensor::set_calibration_controls_to_defaults()
    {
        for (auto && calib_control : _calib_controls)
        {
            auto && control = get_option(calib_control.option);

            auto curr_val = control.query();
            if (curr_val != calib_control.default_value)
            {
                AC_LOG(DEBUG, "Calibration - changed option: " << rs2_option_to_string( calib_control.option ) << " value,"
                                              << " from: " << curr_val
                                              << " to: " << calib_control.default_value );
                calib_control.need_to_restore = true;
                calib_control.previous_value = curr_val;
                control.set(calib_control.default_value);
            }
            else
            {
                AC_LOG(DEBUG, "Calibration - no need to changed option: " << rs2_option_to_string(calib_control.option) << " value,"
                    << " current value is: " << curr_val
                    << " which is the default value");
            }
        }
    }

    void l500_color_sensor::restore_pre_calibration_controls()
    {
        for (auto && calib_control : _calib_controls)
        {
            auto && control = get_option(calib_control.option);

            auto curr_val = control.query();
            if( calib_control.need_to_restore &&
                ( curr_val == calib_control.default_value ) )
            {
                AC_LOG(DEBUG, "Calibration - restored option: " << rs2_option_to_string(calib_control.option) << " value,"
                    << " from: " << curr_val
                    << " to: " << calib_control.previous_value);
                control.set(calib_control.previous_value);
            }
            else
            {
                std::stringstream ss;
                ss << "Calibration - no need to restore option : "
                   << rs2_option_to_string( calib_control.option ) << " value, "
                   << " current value is: " << curr_val;

                if( curr_val == calib_control.default_value ) ss << " which is the default value";
                else  ss << " which is the user value";

                AC_LOG(DEBUG, ss.str());
            }
            calib_control.need_to_restore = false;
        }
    }

    void l500_color_sensor::register_calibration_controls()
    {
        for( auto && option : { RS2_OPTION_ENABLE_AUTO_EXPOSURE,
                                RS2_OPTION_BACKLIGHT_COMPENSATION,
                                RS2_OPTION_BRIGHTNESS,
                                RS2_OPTION_CONTRAST } )
        {
            auto && control = get_option(option);
            _calib_controls.push_back({ option, control.get_range().def });
        }
    }

    std::vector<tagged_profile> l500_color::get_profiles_tags() const
    {
        std::vector<tagged_profile> tags;

        bool usb3mode = (_usb_mode >= platform::usb3_type || _usb_mode == platform::usb_undefined);

        int width = usb3mode ? 1280 : 960;
        int height = usb3mode ? 720 : 540;

        tags.push_back({ RS2_STREAM_COLOR, -1, width, height, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });

        return tags;
    }

    ivcam2::intrinsic_rgb l500_color::read_intrinsics_table() const
    {
        // Get RAW data from firmware
        AC_LOG(DEBUG, "RGB_INTRINSIC_GET");
        std::vector< uint8_t > response_vec = _hw_monitor->send(command{ RGB_INTRINSIC_GET });

        if (response_vec.empty())
            throw invalid_value_exception("Calibration data invalid,buffer size is zero");

        auto resolutions_rgb_table_ptr = reinterpret_cast<const ivcam2::intrinsic_rgb *>(response_vec.data());
        auto num_of_resolutions = resolutions_rgb_table_ptr->resolution.num_of_resolutions;

        // Get full maximum size of the resolution array and deduct the unused resolutions size from it
        size_t expected_size = sizeof( ivcam2::intrinsic_rgb )
                                - ( ( MAX_NUM_OF_RGB_RESOLUTIONS
                                - num_of_resolutions)
                                * sizeof( pinhole_camera_model ) );

        // Validate table size
        // FW API guarantee only as many bytes as required for the given number of resolutions, 
        // The FW keeps the right to send more bytes.
        if (expected_size > response_vec.size() ||
            num_of_resolutions > MAX_NUM_OF_RGB_RESOLUTIONS)
        {
            throw invalid_value_exception(
                to_string() << "Calibration data invalid, number of resolutions is: " << num_of_resolutions <<
                ", expected size: " << expected_size << " , actual size: " << response_vec.size());
        }

        // Set a new memory allocated intrinsics struct (Full size 5 resolutions)
        // Copy the relevant data from the dynamic resolution received from the FW
        ivcam2::intrinsic_rgb  resolutions_rgb_table_output;
        librealsense::copy(&resolutions_rgb_table_output, resolutions_rgb_table_ptr, expected_size);

        return resolutions_rgb_table_output;
    }
    std::vector<uint8_t> l500_color::get_raw_extrinsics_table() const
    {
        AC_LOG( DEBUG, "RGB_EXTRINSIC_GET" );
        return _hw_monitor->send(command{ RGB_EXTRINSIC_GET });
    }
}
