// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include <vector>
#include "device.h"
#include "context.h"
#include "stream.h"
#include "l500-depth.h"
#include "l500-color.h"
#include "l500-private.h"
#include "proc/decimation-filter.h"
#include "proc/threshold.h" 
#include "proc/spatial-filter.h"
#include "proc/temporal-filter.h"
#include "proc/hole-filling-filter.h"
#include "proc/zero-order.h"
#include <cstddef>
#include "metadata-parser.h"
#include "l500-options.h"
#include "ac-trigger.h"
#include "algo/depth-to-rgb-calibration/debug.h"


#define MM_TO_METER 1/1000
#define MIN_ALGO_VERSION 115

namespace librealsense
{
    using namespace ivcam2;

    std::vector<uint8_t> l500_depth::get_raw_calibration_table() const
    {
        static const char* fw_ver = "1.2.11.0";

        if(_fw_version >= firmware_version(fw_ver))
            return _hw_monitor->send(command{ DPT_INTRINSICS_FULL_GET });
        else
        {
            //WA untill fw will fix DPT_INTRINSICS_GET command
            command cmd_fx(0x01, 0xa00e0804, 0xa00e0808);
            command cmd_fy(0x01, 0xa00e080c, 0xa00e0810);
            command cmd_cx(0x01, 0xa00e0814, 0xa00e0818);
            command cmd_cy(0x01, 0xa00e0818, 0xa00e081c);
            auto fx = _hw_monitor->send(cmd_fx); // CBUFspare_000
            auto fy = _hw_monitor->send(cmd_fy); // CBUFspare_002
            auto cx = _hw_monitor->send(cmd_cx); // CBUFspare_004
            auto cy = _hw_monitor->send(cmd_cy); // CBUFspare_005

            std::vector<uint8_t> vec;
            vec.insert(vec.end(), fx.begin(), fx.end());
            vec.insert(vec.end(), cx.begin(), cx.end());
            vec.insert(vec.end(), fy.begin(), fy.end());
            vec.insert(vec.end(), cy.begin(), cy.end());

            return vec;
        }
    }

    l500_depth::l500_depth(std::shared_ptr<context> ctx,
                             const platform::backend_device_group& group)
        :device(ctx, group),
        l500_device(ctx, group)
    {
        _calib_table_raw = [this]() { return get_raw_calibration_table(); };

        auto& depth_sensor = get_depth_sensor();
        auto& raw_depth_sensor = get_raw_depth_sensor();

        depth_sensor.register_option(RS2_OPTION_LLD_TEMPERATURE,
            std::make_shared <l500_temperature_options>(_hw_monitor.get(), RS2_OPTION_LLD_TEMPERATURE));

        depth_sensor.register_option(RS2_OPTION_MC_TEMPERATURE,
            std::make_shared <l500_temperature_options>(_hw_monitor.get(), RS2_OPTION_MC_TEMPERATURE));

        depth_sensor.register_option(RS2_OPTION_MA_TEMPERATURE,
            std::make_shared <l500_temperature_options>(_hw_monitor.get(), RS2_OPTION_MA_TEMPERATURE));

        depth_sensor.register_option(RS2_OPTION_APD_TEMPERATURE,
            std::make_shared <l500_temperature_options>(_hw_monitor.get(), RS2_OPTION_APD_TEMPERATURE));

        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_depth_stream, *_ir_stream);
        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_depth_stream, *_confidence_stream);

        register_stream_to_extrinsic_group(*_depth_stream, 0);
        register_stream_to_extrinsic_group(*_ir_stream, 0);

        auto error_control = std::unique_ptr<uvc_xu_option<int>>(new uvc_xu_option<int>(raw_depth_sensor, ivcam2::depth_xu, L500_ERROR_REPORTING, "Error reporting"));

        _polling_error_handler = std::unique_ptr<polling_error_handler>(
            new polling_error_handler(1000,
                std::move(error_control),
                raw_depth_sensor.get_notifications_processor(),
                std::unique_ptr<notification_decoder>(new l500_notification_decoder())));

        depth_sensor.register_option(RS2_OPTION_ERROR_POLLING_ENABLED, std::make_shared<polling_errors_disable>(_polling_error_handler.get()));

        // attributes of md_capture_timing
        auto md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_l500_depth, intel_capture_timing);

        depth_sensor.register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_uvc_header_parser(&platform::uvc_header::timestamp));
        depth_sensor.register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER, make_attribute_parser(&l500_md_capture_timing::frame_counter, md_capture_timing_attributes::frame_counter_attribute, md_prop_offset));
        depth_sensor.register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP, make_attribute_parser(&l500_md_capture_timing::sensor_timestamp, md_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset));
        depth_sensor.register_metadata(RS2_FRAME_METADATA_ACTUAL_FPS, make_attribute_parser(&l500_md_capture_timing::exposure_time, md_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset));

        // attributes of md_depth_control
        md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_l500_depth, intel_depth_control);

        depth_sensor.register_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER, make_attribute_parser(&md_l500_depth_control::laser_power, md_l500_depth_control_attributes::laser_power, md_prop_offset));
        depth_sensor.register_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE, make_attribute_parser(&md_l500_depth_control::laser_power_mode, md_l500_depth_control_attributes::laser_power_mode, md_prop_offset));
    }

    std::vector<tagged_profile> l500_depth::get_profiles_tags() const
    {
        std::vector<tagged_profile> tags;

        tags.push_back({ RS2_STREAM_DEPTH, -1, 640, 480, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
        tags.push_back({ RS2_STREAM_INFRARED, -1, 640, 480, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
        tags.push_back({ RS2_STREAM_CONFIDENCE, -1, 640, 480, RS2_FORMAT_RAW8, 30, profile_tag::PROFILE_TAG_SUPERSET });
        
        return tags;
    }

    std::shared_ptr<matcher> l500_depth::create_matcher(const frame_holder& frame) const
    {
        std::vector<std::shared_ptr<matcher>> depth_matchers;

        std::vector<stream_interface*> streams = { _depth_stream.get(), _ir_stream.get(), _confidence_stream.get() };

        // TODO
        for (auto& s : streams)
        {
            depth_matchers.push_back(std::make_shared<identity_matcher>(s->get_unique_id(), s->get_stream_type()));
        }
        std::vector<std::shared_ptr<matcher>> matchers;
        if (!frame.frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
        {
            matchers.push_back(std::make_shared<timestamp_composite_matcher>(depth_matchers));
        }
        else
        {
            matchers.push_back(std::make_shared<timestamp_composite_matcher>(depth_matchers));
        }

        return std::make_shared<timestamp_composite_matcher>(matchers);
    }

    l500_depth_sensor::l500_depth_sensor(
        l500_device* owner,
        std::shared_ptr<uvc_sensor> uvc_sensor,
        std::map<uint32_t, rs2_format> l500_depth_fourcc_to_rs2_format_map,
        std::map<uint32_t, rs2_stream> l500_depth_fourcc_to_rs2_stream_map
    )
        : synthetic_sensor( "L500 Depth Sensor",
            uvc_sensor,
            owner,
            l500_depth_fourcc_to_rs2_format_map,
            l500_depth_fourcc_to_rs2_stream_map )
        , _owner( owner )
    {
#ifdef ENABLE_L500_DEPTH_INVALIDATION
        _depth_invalidation_enabled = true;
#else
        _depth_invalidation_enabled = false;
#endif

        register_option( RS2_OPTION_DEPTH_UNITS, std::make_shared<const_value_option>( "Number of meters represented by a single depth unit",
            lazy<float>( [&]() {
                return read_znorm(); } ) ) );

        register_option( RS2_OPTION_DEPTH_OFFSET, std::make_shared<const_value_option>( "Offset from sensor to depth origin in millimetrers",
            lazy<float>( [&]() {
                return get_depth_offset(); } ) ) );

        _depth_invalidation_option = std::make_shared<depth_invalidation_option>(
            0,
            1,
            1,
            0,
            &_depth_invalidation_enabled,
            "depth invalidation enabled" );
        _depth_invalidation_option->on_set( [this]( float val )
            {
                if( !_depth_invalidation_option->is_valid( val ) )
                    throw invalid_value_exception( to_string()
                        << "Unsupported depth invalidation enabled " << val << " is out of range." );
            } );

        // The depth invalidation enable option is deprecated for now.
        //register_option(static_cast<rs2_option>(RS2_OPTION_DEPTH_INVALIDATION_ENABLE), _depth_invalidation_option);
    }

    int l500_depth_sensor::read_algo_version()
    {
        const int algo_version_address = 0xa0020bd8;
        command cmd(ivcam2::fw_cmd::MRD, algo_version_address, algo_version_address + 4);
        auto res = _owner->_hw_monitor->send(cmd);
        if (res.size() < 2)
        {
            throw std::runtime_error("Invalid result size!");
        }
        auto data = (uint8_t*)res.data();
        auto ver = data[0] + 100* data[1];
        return ver;
    }

    void l500_depth_sensor::override_intrinsics( rs2_intrinsics const & intr )
    {
        throw librealsense::not_implemented_exception( "depth sensor does not support intrinsics override" );
    }

    void l500_depth_sensor::override_extrinsics( rs2_extrinsics const & extr )
    {
        throw librealsense::not_implemented_exception( "depth sensor does not support extrinsics override" );
    }

    rs2_dsm_params l500_depth_sensor::get_dsm_params() const
    {
        ac_depth_results table = { { 0 } };
        read_fw_table( *_owner->_hw_monitor, table.table_id, &table, nullptr,
            [&]()
            {
                //time_t t;
                //time( &t );                                       // local time
                //table.params.timestamp = mktime( gmtime( &t ) );  // UTC time
                // Leave the timestamp & version at 0, so it's recognizable as "new"
                //table.params.version = table.this_version;
                table.params.model = RS2_DSM_CORRECTION_AOT;
                table.params.h_scale = table.params.v_scale = 1.;
            } );
        return table.params;
    }

    void l500_depth_sensor::override_dsm_params( rs2_dsm_params const & dsm_params )
    {
        /*  Considerable values for DSM correction:
            - h/vFactor: 0.98-1.02, representing up to 2% change in FOV.
            - h/vOffset:
                - Under AOT model: (-2)-2, representing up to 2deg FOV tilt
                - Under TOA model: (-125)-125, representing up to approximately
                  2deg FOV tilt
            These values are extreme. For more reasonable values take 0.99-1.01
            for h/vFactor and divide the suggested h/vOffset range by 10.
        */
        if( dsm_params.model != RS2_DSM_CORRECTION_AOT )
            throw invalid_value_exception( "DSM non-AoT (1) mode is currently unsupported" );

        ac_depth_results table( dsm_params );
        // table.params.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        time_t t;
        time( &t );                                       // local time
        table.params.timestamp = mktime( gmtime( &t ) );  // UTC time
        table.params.version = ac_depth_results::this_version;

        // The temperature may depend on streaming?
        auto res = _owner->_hw_monitor->send( command{TEMPERATURES_GET} );
        if( res.size() < sizeof( temperatures ) )  // New temperatures may get added by FW...
        {
            AC_LOG( ERROR, "Failed to get temperatures; result size= " << res.size() << "; expecting at least " << sizeof( temperatures ) );
        }
        else
        {
            auto const & ts = *( reinterpret_cast<temperatures *>( res.data() ) );
            table.params.temp_x2 = byte( ts.LDD_temperature * 2 );
        }

        AC_LOG( INFO, "Overriding DSM : " << table.params );
        ivcam2::write_fw_table( *_owner->_hw_monitor, ac_depth_results::table_id, table );
    }

    void l500_depth_sensor::reset_calibration()
    {
        command cmd( ivcam2::fw_cmd::DELETE_TABLE, ac_depth_results::table_id );
        _owner->_hw_monitor->send( cmd );
        AC_LOG( INFO, "Depth sensor calibration has been reset" );
    }


    float l500_depth_sensor::read_baseline() const
    {
        const int baseline_address = 0xa00e0868;
        command cmd(ivcam2::fw_cmd::MRD, baseline_address, baseline_address + 4);
        auto res = _owner->_hw_monitor->send(cmd);
        if (res.size() < 1)
        {
            throw std::runtime_error("Invalid result size!");
        }
        auto data = (float*)res.data();
        return *data;
    }

    float l500_depth_sensor::read_znorm()
    {
        auto intrin = get_intrinsic();
        if (intrin.resolution.num_of_resolutions < 1)
        {
            throw std::runtime_error("Invalid intrinsics!");
        }
        auto znorm = intrin.resolution.intrinsic_resolution[0].world.znorm;
        return 1/znorm* MM_TO_METER;
    }

    float l500_depth_sensor::get_depth_offset() const
    {
        using namespace ivcam2;
        auto intrinsic = check_calib<intrinsic_depth>(*_owner->_calib_table_raw);
        return intrinsic->orient.depth_offset;
    }

    rs2_time_t l500_timestamp_reader_from_metadata::get_frame_timestamp(const std::shared_ptr<frame_interface>& frame)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        auto f = std::dynamic_pointer_cast<librealsense::frame>(frame);
        if (has_metadata_ts(frame))
        {
            auto md = (librealsense::metadata_raw*)(f->additional_data.metadata_blob.data());
            return (double)(md->header.timestamp)*TIMESTAMP_USEC_TO_MSEC;
        }
        else
        {
            if (!one_time_note)
            {
                LOG_WARNING("UVC metadata payloads are not available for stream "
                    //<< std::hex << mode.pf->fourcc << std::dec << (mode.profile.format)
                    << ". Please refer to installation chapter for details.");
                one_time_note = true;
            }
            return _backup_timestamp_reader->get_frame_timestamp(frame);
        }
    }

    unsigned long long l500_timestamp_reader_from_metadata::get_frame_counter(const std::shared_ptr<frame_interface>& frame) const
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        auto f = std::dynamic_pointer_cast<librealsense::frame>(frame);
        if (has_metadata_fc(frame))
        {
            auto md = (byte*)(f->additional_data.metadata_blob.data());
            // WA until we will have the struct of metadata
            return ((int*)md)[7];
        }

        return _backup_timestamp_reader->get_frame_counter(frame);
    }

    void l500_timestamp_reader_from_metadata::reset()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        one_time_note = false;
        _backup_timestamp_reader->reset();
        ts_wrap.reset();
    }

    rs2_timestamp_domain l500_timestamp_reader_from_metadata::get_frame_timestamp_domain(const std::shared_ptr<frame_interface>& frame) const
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        return (has_metadata_ts(frame)) ? RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK : _backup_timestamp_reader->get_frame_timestamp_domain(frame);
    }

    processing_blocks l500_depth_sensor::get_l500_recommended_proccesing_blocks()
    {
        processing_blocks res;
        res.push_back(std::make_shared<temporal_filter>());
        return res;
    }

    void l500_depth_sensor::start(frame_callback_ptr callback)
    {
        _action_delayer.do_after_delay( [&]() {
            if( _depth_invalidation_enabled )
                synthetic_sensor::start(
                    std::make_shared< frame_validator >( shared_from_this(),
                                                         callback,
                                                         _user_requests,
                                                         _validator_requests ) );
            else
                synthetic_sensor::start( callback );
            if( _owner->_autocal )
                _owner->_autocal->start();
        } );
    }

    void l500_depth_sensor::stop()
    {
        _action_delayer.do_after_delay([&]() {
            synthetic_sensor::stop();
            _depth_invalidation_option->set_streaming(false);
        });
        if( _owner->_autocal )
            _owner->_autocal->stop();
    }

    rs2_sensor_mode get_resolution_from_width_height(int width, int height)
    {
        if ((width == 240 && height == 320) || (width == 320 && height == 240))
            return RS2_SENSOR_MODE_QVGA;
        else if ((width == 640 && height == 480) || (width == 480  && height == 640))
            return RS2_SENSOR_MODE_VGA;
        else if ((width == 1024 && height == 768) || (width == 768 && height == 1024))
            return RS2_SENSOR_MODE_XGA;
        else
            throw std::runtime_error(to_string() << "Invalid resolution " << width << "x" << height);
    }

    bool stream_profiles_correspond(stream_profile_interface* l, stream_profile_interface* r)
    {
        auto vl = dynamic_cast<video_stream_profile_interface*>(l);
        auto vr = dynamic_cast<video_stream_profile_interface*>(r);

        if (!vl || !vr)
            return false;

        return  l->get_framerate() == r->get_framerate() &&
            vl->get_width() == vr->get_width() &&
            vl->get_height() == vr->get_height();
    }


    std::shared_ptr< stream_profile_interface > l500_depth_sensor::is_color_sensor_needed() const
    {
        // If AC is off, we don't need the color stream on
        if( !_owner->_autocal )
            return {};

        auto is_rgb_requested
            = std::find_if( _user_requests.begin(),
                            _user_requests.end(),
                            []( std::shared_ptr< stream_profile_interface > const & sp )
                            { return sp->get_stream_type() == RS2_STREAM_COLOR; } )
           != _user_requests.end();
        if( is_rgb_requested )
            return {};

        // Find a profile that's acceptable for RGB:
        //     1. has the same framerate
        //     2. format is RGB8
        //     2. has the right resolution (1280x720 should be good enough)
        auto user_request
            = std::find_if( _user_requests.begin(),
                            _user_requests.end(),
                            []( std::shared_ptr< stream_profile_interface > const & sp ) {
                                return sp->get_stream_type() == RS2_STREAM_DEPTH;
                            } );
        if( user_request == _user_requests.end() )
        {
            AC_LOG( ERROR, "Depth input stream profiles do not contain depth!" );
            return {};
        }
        auto requested_depth_profile
            = dynamic_cast< video_stream_profile * >( user_request->get() );

        auto & color_sensor = *_owner->get_color_sensor();
        auto color_profiles = color_sensor.get_stream_profiles();
        auto rgb_profile = std::find_if(
            color_profiles.begin(),
            color_profiles.end(),
            [&]( std::shared_ptr< stream_profile_interface > const & sp )
            {
                auto vsp = dynamic_cast< video_stream_profile * >( sp.get() );
                return vsp->get_stream_type() == RS2_STREAM_COLOR
                    && vsp->get_framerate() == requested_depth_profile->get_framerate()
                    && vsp->get_format() == RS2_FORMAT_RGB8
                    && vsp->get_width() == 1280  // flipped
                    && vsp->get_height() == 720;
            } );
        if( rgb_profile == color_profiles.end() )
        {
            AC_LOG( ERROR,
                "Can't find color stream corresponding to depth; AC will not work" );
            return {};
        }
        return *rgb_profile;
    }

    void l500_depth_sensor::open(const stream_profiles& requests)
    {
        try
        {
            _user_requests = requests;
            _depth_invalidation_option->set_streaming(true);

            if (_depth_invalidation_enabled)
            {
                auto is_ir_requested
                    = std::find_if( requests.begin(),
                                    requests.end(),
                                    []( std::shared_ptr< stream_profile_interface > const & sp ) {
                                        return sp->get_stream_type() == RS2_STREAM_INFRARED;
                                    } )
                   != requests.end();

                _validator_requests = requests;

                //enable ir if user didn't asked ir in order to validate the ir frame
                if (!is_ir_requested)
                {
                    auto user_request = std::find_if(requests.begin(), requests.end(), [](std::shared_ptr<stream_profile_interface> sp)
                    {return sp->get_stream_type() != RS2_STREAM_INFRARED;});

                    if (user_request == requests.end())
                        throw std::runtime_error(to_string() << "input stream_profiles is invalid");

                    auto user_request_profile = dynamic_cast<video_stream_profile*>(user_request->get());

                    auto sp = synthetic_sensor::get_stream_profiles();

                    auto corresponding_ir = std::find_if(sp.begin(), sp.end(), [&](std::shared_ptr<stream_profile_interface> sp)
                    {
                        auto vs = dynamic_cast<video_stream_profile*>(sp.get());
                        return sp->get_stream_type() == RS2_STREAM_INFRARED && stream_profiles_correspond(sp.get(), user_request_profile);
                    });

                    if (corresponding_ir == sp.end())
                        throw std::runtime_error(to_string() << "can't find ir stream corresponding to user request");

                    _validator_requests.push_back(*corresponding_ir);
                }
            }
            else
            {
                _validator_requests = requests;
            }

            auto dp = std::find_if( requests.begin(),
                                    requests.end(),
                                    []( std::shared_ptr< stream_profile_interface > sp ) {
                                        return sp->get_stream_type() == RS2_STREAM_DEPTH;
                                    } );
            if( dp != requests.end() && supports_option( RS2_OPTION_SENSOR_MODE ) )
            {
                auto&& sensor_mode_option = get_option(RS2_OPTION_SENSOR_MODE);
                auto vs = dynamic_cast<video_stream_profile*>((*dp).get());
                if (supports_option(RS2_OPTION_VISUAL_PRESET))
                {
                    auto&& preset_option = get_option(RS2_OPTION_VISUAL_PRESET);
                    if (preset_option.query() == RS2_L500_VISUAL_PRESET_CUSTOM)
                    {
                        if(sensor_mode_option.query() != get_resolution_from_width_height(vs->get_width(), vs->get_height()))
                            throw  std::runtime_error(to_string() << "sensor mode option ("<< sensor_mode_option.query()<<") is incompatible with requested resolution ("
                                << get_resolution_from_width_height(vs->get_width(), vs->get_height())<<")");
                    }
                }
                
                sensor_mode_option.set(float(get_resolution_from_width_height(vs->get_width(), vs->get_height())));
            }

            synthetic_sensor::open(_validator_requests);
        }
        catch( ... )
        {
            LOG_ERROR( "Exception caught in l500_depth_sensor::open" );
            _depth_invalidation_option->set_streaming(false);
            throw;
        }
    }

}  // namespace librealsense
