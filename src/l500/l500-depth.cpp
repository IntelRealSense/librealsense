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
#include "algo/depth-to-rgb-calibration/utils.h"  // validate_dsm_params
#include "algo/max-usable-range/l500/max-usable-range.h" 


#define MM_TO_METER 1/1000
#define MIN_ALGO_VERSION 115

namespace librealsense
{
    using namespace ivcam2;

    ivcam2::intrinsic_depth l500_depth::read_intrinsics_table() const
    {
        // Get RAW data from firmware
        AC_LOG(DEBUG, "DPT_INTRINSICS_FULL_GET");
        std::vector< uint8_t > response_vec = _hw_monitor->send( command{ DPT_INTRINSICS_FULL_GET } );

        if (response_vec.empty())
            throw invalid_value_exception("Calibration data invalid,buffer size is zero");

        auto resolutions_depth_table_ptr = reinterpret_cast<const ivcam2::intrinsic_depth *>(response_vec.data());

        auto num_of_resolutions = resolutions_depth_table_ptr->resolution.num_of_resolutions;
        // Get full maximum size of the resolution array and deduct the unused resolutions size from it
        size_t expected_size = sizeof( ivcam2::intrinsic_depth ) - 
                                     ( ( MAX_NUM_OF_DEPTH_RESOLUTIONS
                                     - num_of_resolutions)
                                     * sizeof( intrinsic_per_resolution ) );

        // Validate table size
        // FW API guarantee only as many bytes as required for the given number of resolutions, 
        // The FW keeps the right to send more bytes.
        if (expected_size > response_vec.size() || 
            num_of_resolutions > MAX_NUM_OF_DEPTH_RESOLUTIONS)
        {
            throw invalid_value_exception(
                to_string() << "Calibration data invalid, number of resolutions is: " << num_of_resolutions <<
                ", expected size: " << expected_size << " , actual size: " << response_vec.size() );
        }

        // Set a new memory allocated intrinsics struct (Full size 5 resolutions)
        // Copy the relevant data from the dynamic resolution received from the FW
        ivcam2::intrinsic_depth  resolutions_depth_table_output;
        librealsense::copy(&resolutions_depth_table_output, resolutions_depth_table_ptr, expected_size);
      
        return resolutions_depth_table_output;
    }

    l500_depth::l500_depth(std::shared_ptr<context> ctx,
                             const platform::backend_device_group& group)
        :device(ctx, group),
        l500_device(ctx, group)
    {
        _calib_table = [this]() { return read_intrinsics_table(); };

        auto& depth_sensor = get_depth_sensor();
        auto& raw_depth_sensor = get_raw_depth_sensor();

        depth_sensor.register_option(
            RS2_OPTION_LLD_TEMPERATURE,
            std::make_shared< l500_temperature_options >( this,
                                                          RS2_OPTION_LLD_TEMPERATURE,
                                                          "Laser Driver temperature" ) );

        depth_sensor.register_option(
            RS2_OPTION_MC_TEMPERATURE,
            std::make_shared< l500_temperature_options >( this,
                                                          RS2_OPTION_MC_TEMPERATURE,
                                                          "Mems Controller temperature" ) );

        depth_sensor.register_option(
            RS2_OPTION_MA_TEMPERATURE,
            std::make_shared< l500_temperature_options >( this,
                                                          RS2_OPTION_MA_TEMPERATURE,
                                                          "DSP controller temperature" ) );

        depth_sensor.register_option(
            RS2_OPTION_APD_TEMPERATURE,
            std::make_shared< l500_temperature_options >( this,
                                                          RS2_OPTION_APD_TEMPERATURE,
                                                          "Avalanche Photo Diode temperature" ) );

        depth_sensor.register_option(
            RS2_OPTION_HUMIDITY_TEMPERATURE,
            std::make_shared< l500_temperature_options >( this,
                                                          RS2_OPTION_HUMIDITY_TEMPERATURE,
                                                          "Humidity temperature" ) );

        depth_sensor.register_option(
            RS2_OPTION_NOISE_ESTIMATION,
            std::make_shared< nest_option >( this, "Noise estimation" ) );

        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_depth_stream, *_ir_stream);
        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_depth_stream, *_confidence_stream);

        register_stream_to_extrinsic_group(*_depth_stream, 0);
        register_stream_to_extrinsic_group(*_ir_stream, 0);
        register_stream_to_extrinsic_group(*_confidence_stream, 0);

        auto error_control = std::make_shared<uvc_xu_option<int>>(raw_depth_sensor, ivcam2::depth_xu, L500_ERROR_REPORTING, "Error reporting");

        _polling_error_handler = std::make_shared<polling_error_handler>(1000,
            error_control,
            raw_depth_sensor.get_notifications_processor(),
            std::make_shared<l500_notification_decoder>());

        depth_sensor.register_option(RS2_OPTION_ERROR_POLLING_ENABLED, std::make_shared<polling_errors_disable>(_polling_error_handler));

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

            depth_sensor.register_option(RS2_OPTION_HOST_PERFORMANCE, std::make_shared<librealsense::float_option_with_description<rs2_host_perf_mode>>(option_range{ RS2_HOST_PERF_DEFAULT, RS2_HOST_PERF_COUNT - 1, 1, (float) launch_perf_mode }, "Optimize based on host performance, low power low performance host or high power high performance host"));

        }

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

        bool usb3mode = (_usb_mode >= platform::usb3_type || _usb_mode == platform::usb_undefined);

        int width = usb3mode ? 640 : 320;
        int height = usb3mode ? 480 : 240;

        tags.push_back({ RS2_STREAM_DEPTH, -1, width, height, RS2_FORMAT_Z16, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
        tags.push_back({ RS2_STREAM_INFRARED, -1, width, height, RS2_FORMAT_Y8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
        tags.push_back({ RS2_STREAM_CONFIDENCE, -1, width, height, RS2_FORMAT_RAW8, 30, profile_tag::PROFILE_TAG_SUPERSET });
        tags.push_back({ RS2_STREAM_DEPTH, -1, -1, -1, RS2_FORMAT_FG, -1, profile_tag::PROFILE_TAG_DEBUG } );
        return tags;
    }

    std::shared_ptr<matcher> l500_depth::create_matcher(const frame_holder& frame) const
    {
        std::vector<std::shared_ptr<matcher>> depth_matchers;

        std::vector<stream_interface*> streams = { _depth_stream.get(), _ir_stream.get(), _confidence_stream.get() };

        for (auto& s : streams)
        {
            depth_matchers.push_back(std::make_shared<identity_matcher>(s->get_unique_id(), s->get_stream_type()));
        }
        std::vector<std::shared_ptr<matcher>> matchers;
        matchers.push_back(std::make_shared<timestamp_composite_matcher>(depth_matchers));

        return std::make_shared<timestamp_composite_matcher>(matchers);
    }

    // If the user did not ask for IR, The open function will add it anyway. 
    // This class filters the IR frames is this case so they will not get to the user callback function
    // Note: This is a workaround , theoretically we did not need to add manually the IR profile to the "open" function,
    // The processing block should take care of it, this caused a syncer bug to pop-up so we avoid this solution for now.
    class frame_filter : public rs2_frame_callback
    {
    public:
        explicit frame_filter(frame_callback_ptr user_callback, const stream_profiles &user_requests) :
            _user_callback(user_callback),
            _user_requests(user_requests) {}

        void on_frame( rs2_frame * f ) override
        {
            if( is_user_requested_frame( (frame_interface *)f ) )
            {
                _user_callback->on_frame( f );
            }
            else
            {
                // No RAII - explicit release is required
                ( (frame_interface *)f )->release();
            }
        }

        void release() override {}

    private:
        bool is_user_requested_frame(frame_interface* frame)
        {
            return std::find_if(_user_requests.begin(), _user_requests.end(), [&](std::shared_ptr<stream_profile_interface> sp)
            {
                return stream_profiles_equal(frame->get_stream().get(), sp.get());
            }) != _user_requests.end();
        }

        bool stream_profiles_equal(stream_profile_interface* l, stream_profile_interface* r)
        {
            auto vl = dynamic_cast<video_stream_profile_interface*>(l);
            auto vr = dynamic_cast<video_stream_profile_interface*>(r);

            if (!vl || !vr)
                return false;

            return  l->get_framerate() == r->get_framerate() &&
                vl->get_width() == vr->get_width() &&
                vl->get_height() == vr->get_height() &&
                vl->get_stream_type() == vr->get_stream_type();
        }

        frame_callback_ptr _user_callback;
        stream_profiles _user_requests;
    };

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

        register_option( RS2_OPTION_DEPTH_UNITS, std::make_shared<const_value_option>( "Number of meters represented by a single depth unit",
            lazy<float>( [&]() {
                return read_znorm(); } ) ) );

        register_option( RS2_OPTION_DEPTH_OFFSET, std::make_shared<const_value_option>( "Offset from sensor to depth origin in millimetrers",
            lazy<float>( [&]() {
                return get_depth_offset(); } ) ) );

    }

    l500_depth_sensor::~l500_depth_sensor()
    {
        _owner->stop_temperatures_reader();
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
        try
        {
            algo::depth_to_rgb_calibration::validate_dsm_params( dsm_params );  // throws!
        }
        catch( invalid_value_exception const & e )
        {
            if( ! getenv( "RS2_AC_IGNORE_LIMITERS" ))
                throw;
            AC_LOG( ERROR, "Ignoring (RS2_AC_IGNORE_LIMITERS) " << e.what() );
        }

        ac_depth_results table( dsm_params );
        // table.params.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        time_t t;
        time( &t );                                       // local time
        table.params.timestamp = mktime( gmtime( &t ) );  // UTC time
        table.params.version = ac_depth_results::this_version;

        AC_LOG( INFO, "Overriding DSM : " << table.params );
        ivcam2::write_fw_table( *_owner->_hw_monitor, ac_depth_results::table_id, table );
    }

    void l500_depth_sensor::reset_calibration()
    {
        command cmd( ivcam2::fw_cmd::DELETE_TABLE, ac_depth_results::table_id );
        _owner->_hw_monitor->send( cmd );
        AC_LOG( INFO, "Depth sensor calibration has been reset" );
    }

    float l500_depth_sensor::get_max_usable_depth_range() const
    {
        using namespace algo::max_usable_range;

        if( !supports_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE) )
            throw librealsense::wrong_api_call_sequence_exception( "max usable range option is not supported" );

        if( get_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE).query() != 1.0f )
            throw librealsense::wrong_api_call_sequence_exception( "max usable range option is not on" );

        if( ! is_streaming() )
        {
            throw librealsense::wrong_api_call_sequence_exception("depth sensor is not streaming!");
        }

       float noise_estimation = static_cast<float>(_owner->get_temperatures().nest_avg);

       return l500::max_usable_range(noise_estimation);
    }

    stream_profiles l500_depth_sensor::get_debug_stream_profiles() const
    {
       return get_stream_profiles( PROFILE_TAG_DEBUG );
    }

    // We want to disable max-usable-range when not in a particular preset:
    bool l500_depth_sensor::is_max_range_preset() const
    {
        auto res = _owner->_hw_monitor->send(command(ivcam2::IRB, 0x6C, 0x2, 0x1));

        if (res.size() < sizeof(uint8_t))
        {
            throw invalid_value_exception(
                to_string() << "Gain trim FW command failed: size expected: " << sizeof(uint8_t)
                << " , size received: " << res.size());
        }

        int gtr = static_cast<int>(res[0]);
        int apd = static_cast<int>(get_option(RS2_OPTION_AVALANCHE_PHOTO_DIODE).query());
        int laser_power = static_cast<int>(get_option(RS2_OPTION_LASER_POWER).query());
        int max_laser_power = static_cast<int>(get_option(RS2_OPTION_LASER_POWER).get_range().max);

        return ((apd == 9) && (gtr == 0) && (laser_power == max_laser_power)); // indicates max_range preset
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
        auto & intrinsic = *_owner->_calib_table;
        return intrinsic.orient.depth_offset;
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
                int ep2_usb_trb = 7;           // 7 KB
                int ep3_usb_trb = 3;           // 3 KB
                int ep4_usb_trb = 3;           // 3 KB

                if (host_perf == RS2_HOST_PERF_LOW)
                {
                    ep2_usb_trb = 16;          // 16 KB
                    ep3_usb_trb = 12;          // 12 KB
                    ep4_usb_trb = 6;           //  6 KB
                }

                try {
                    // Keep the USB power on while triggering multiple calls on it.
                    ivcam2::group_multiple_fw_calls(*this, [&]() {
                        // endpoint 2 (depth)
                        command cmdTprocGranEp2(ivcam2::TPROC_USB_GRAN_SET, 2, ep2_usb_trb);
                        _owner->_hw_monitor->send(cmdTprocGranEp2);

                        command cmdTprocThresholdEp2(ivcam2::TPROC_TRB_THRSLD_SET, 2, 1);
                        _owner->_hw_monitor->send(cmdTprocThresholdEp2);

                        // endpoint 3 (IR)
                        command cmdTprocGranEp3(ivcam2::TPROC_USB_GRAN_SET, 3, ep3_usb_trb);
                        _owner->_hw_monitor->send(cmdTprocGranEp3);

                        command cmdTprocThresholdEp3(ivcam2::TPROC_TRB_THRSLD_SET, 3, 1);
                        _owner->_hw_monitor->send(cmdTprocThresholdEp3);

                        // endpoint 4 (confidence)
                        command cmdTprocGranEp4(ivcam2::TPROC_USB_GRAN_SET, 4, ep4_usb_trb);
                        _owner->_hw_monitor->send(cmdTprocGranEp4);

                        command cmdTprocThresholdEp4(ivcam2::TPROC_TRB_THRSLD_SET, 4, 1);
                        _owner->_hw_monitor->send(cmdTprocThresholdEp4);
                        });
                    LOG_DEBUG("Depth and IR usb tproc granularity and TRB threshold updated.");
                } catch (...)
                {
                    LOG_WARNING("FAILED TO UPDATE depth usb tproc granularity and TRB threshold. performance and stability maybe impacted on certain platforms.");
                }
            }
            else if (host_perf == RS2_HOST_PERF_DEFAULT)
            {
                LOG_DEBUG("Default host performance mode, Depth/IR/Confidence usb tproc granularity and TRB threshold not changed");
            }
        }

        // The delay is here as a work around to a firmware bug [RS5-5453]
        _action_delayer.do_after_delay( [&]() {
            synthetic_sensor::start( std::make_shared< frame_filter >( callback, _user_requests ) );

            _owner->start_temperatures_reader();

            if( _owner->_autocal )
                _owner->_autocal->start();
        } );
    }

    void l500_depth_sensor::stop()
    {
    // The delay is here as a work around to a firmware bug [RS5-5453]
        _action_delayer.do_after_delay([&]() { synthetic_sensor::stop(); });

        if( _owner->_autocal )
            _owner->_autocal->stop();

        _owner->stop_temperatures_reader();

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

            auto is_ir_requested
                = std::find_if( requests.begin(),
                                requests.end(),
                                []( std::shared_ptr< stream_profile_interface > const & sp ) {
                                    return sp->get_stream_type() == RS2_STREAM_INFRARED;
                                } )
               != requests.end();

            auto is_ir_needed
                = std::find_if( requests.begin(),
                                requests.end(),
                                []( std::shared_ptr< stream_profile_interface > const & sp ) {
                                    return sp->get_format() != RS2_FORMAT_FG;
                                } )
               != requests.end();

            _validator_requests = requests;

            // Enable IR stream if user didn't asked for IR
            // IR stream improves depth frames
            if( ! is_ir_requested && is_ir_needed )
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


            auto dp = std::find_if( requests.begin(),
                                    requests.end(),
                                    []( std::shared_ptr< stream_profile_interface > sp ) {
                                        return sp->get_stream_type() == RS2_STREAM_DEPTH;
                                    } );
            if( dp != requests.end() && supports_option( RS2_OPTION_SENSOR_MODE ) )
            {
                auto&& sensor_mode_option = get_option(RS2_OPTION_SENSOR_MODE);
                auto vs = dynamic_cast<video_stream_profile*>((*dp).get());
                if( vs->get_format() == RS2_FORMAT_Z16 )
                    sensor_mode_option.set(float(get_resolution_from_width_height(vs->get_width(), vs->get_height())));
            }

            synthetic_sensor::open(_validator_requests);
        }
        catch( ... )
        {
            LOG_ERROR( "Exception caught in l500_depth_sensor::open" );
            throw;
        }
    }

}  // namespace librealsense
