#include "MatlabParamParser.h"
#include "Factory.h"
#include "librealsense2/rs.hpp"

#pragma comment(lib, "libmx.lib")
#pragma comment(lib, "libmex.lib")
#pragma comment(lib, "libmat.lib")
#pragma comment(lib, "realsense2.lib")

Factory *factory;

void make_factory(){
    factory = new Factory();

    // rs_frame.hpp
    {
        ClassFactory stream_profile_factory("rs2::stream_profile");
        stream_profile_factory.record("new", 1, 0, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            outv[0] = MatlabParamParser::wrap(rs2::stream_profile());
        });
        stream_profile_factory.record("delete", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::stream_profile>(inv[0]);
        });
        stream_profile_factory.record("stream_index", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.stream_index());
        });
        stream_profile_factory.record("stream_type", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.stream_type());
        });
        stream_profile_factory.record("format", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.format());
        });
        stream_profile_factory.record("fps", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.fps());
        });
        stream_profile_factory.record("unique_id", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.unique_id());
        });
        stream_profile_factory.record("clone", 1, 4, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            auto type = MatlabParamParser::parse<rs2_stream>(inv[1]);
            auto index = MatlabParamParser::parse<int>(inv[2]);
            auto format = MatlabParamParser::parse<rs2_format>(inv[3]);
            outv[0] = MatlabParamParser::wrap(thiz.clone(type, index, format));
        });
        // rs2::stream_profile::operator==                              [TODO]
        // rs2::stream_profile::is                                      [TODO] [T = {stream_profile, video_stream_profile, motion_stream_profile}]
        // rs2::stream_profile::as                                      [TODO] [T = {stream_profile, video_stream_profile, motion_stream_profile}]
        stream_profile_factory.record("stream_name", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.stream_name());
        });
        stream_profile_factory.record("is_default", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.is_default());
        });
        // rs2::stream_profile::operator bool                           [?]
        // rs2::stream_profile::get_extrinsics_to                       [TODO]
        // rs2::stream_profile::register_extrinsics_to                  [?]
        stream_profile_factory.record("is_cloned", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.is_cloned());
        });
        factory->record(stream_profile_factory);
    }
    {
        ClassFactory video_stream_profile_factory("rs2::video_stream_profile");
        // rs2::video_stream_profile::constructor(rs2::stream_profile)  [?]
        // rs2::video_stream_profile::width                             [TODO]
        // rs2::video_stream_profile::height                            [TODO]
        // rs2::video_stream_profile::get_intrinsics                    [TODO]
        factory->record(video_stream_profile_factory);
    }
    {
        ClassFactory motion_stream_profile_factory("rs2::motion_stream_profile");
        // rs2::motion_stream_profile::constructor(rs2::stream_profile) [?]
        // rs2::motion_stream_profile::get_motion_intrinsics            [TODO]
        factory->record(motion_stream_profile_factory);
    }
    {
        ClassFactory frame_factory("rs2::frame");
        // rs2::frame::constructor()                                    [?]
        frame_factory.record("delete", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::frame>(inv[0]);
        });
        // rs2::frame::operator=                                        [?/HOW]
        // rs2::frame::copy constructor                                 [?/HOW]
        // rs2::frame::swap                                             [?/HOW]
        // rs2::frame::keep                                             [TODO]
        // rs2::frame::operator bool                                    [?]
        frame_factory.record("get_timestamp", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_timestamp());
        });
        frame_factory.record("get_frame_timestamp_domain", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_frame_timestamp_domain());
        });
        frame_factory.record("get_frame_metadata", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            auto frame_metadata = MatlabParamParser::parse<rs2_frame_metadata_value>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.get_frame_metadata(frame_metadata));
        });
        frame_factory.record("supports_frame_metadata", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            auto frame_metadata = MatlabParamParser::parse<rs2_frame_metadata_value>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.supports_frame_metadata(frame_metadata));
        });
        frame_factory.record("get_frame_number", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_frame_number());
        });
        frame_factory.record("get_data", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO - What's going on here? / support more formats
            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            if (auto vf = thiz.as<rs2::video_frame>()) {
                /*switch (vf.get_profile().format()) {
                case RS2_FORMAT_RGB8: case RS2_FORMAT_BRG2:
                outv[0] = MatlabParamParser::wrap_array<uint8_t>(vf.get_data(), )
                }*/
                size_t dims[] = { static_cast<size_t>(vf.get_height() * vf.get_stride_in_bytes()) };
                outv[0] = MatlabParamParser::wrap_array<uint8_t>(reinterpret_cast<const uint8_t*>(vf.get_data()), /*1,*/ dims);
            }
            else {
                uint8_t byte = *reinterpret_cast<const uint8_t*>(thiz.get_data());
                outv[0] = MatlabParamParser::wrap(std::move(byte));
                mexWarnMsgTxt("Can't detect frame dims, sending only first byte");
            }
        });
        frame_factory.record("get_profile", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_frame_number());
        });
        // rs2::frame::is                                               [TODO] [T = {frame, video_frame, points, depth_frame, disparity_frame, motion_frame, pose_frame, frameset}]
        // rs2::frame::as                                               [TODO] [T = {frame, video_frame, points, depth_frame, disparity_frame, motion_frame, pose_frame, frameset}]
        factory->record(frame_factory);
    }
    {
        ClassFactory video_frame_factory("rs2::video_frame");
        // rs2::video_frame::constructor()                              [?]
        video_frame_factory.record("delete", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::video_frame>(inv[0]);
        });
        video_frame_factory.record("get_width", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::video_frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_width());
        });
        video_frame_factory.record("get_height", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::video_frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_height());
        });
        video_frame_factory.record("get_stride_in_bytes", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::video_frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_stride_in_bytes());
        });
        video_frame_factory.record("get_bits_per_pixel", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::video_frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_bits_per_pixel());
        });
        video_frame_factory.record("get_bytes_per_pixel", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::video_frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_bytes_per_pixel());
        });
        factory->record(video_frame_factory);
    }
    {
        ClassFactory points_factory("rs2::points");
        // rs2::points::constructor()                                   [TODO]
        // rs2::points::constrcutor(rs2::frame)                         [?]
        // rs2::points::get_vertices                                    [TODO]
        // rs2::points::export_to_ply                                   [TODO]
        // rs2::points::get_texture_coordinates                         [TODO]
        // rs2::points::size                                            [TODO]
        factory->record(points_factory);
    }
    {
        ClassFactory depth_frame_factory("rs2::depth_frame");
        // rs2::depth_frame::constructor()                              [?]
        depth_frame_factory.record("delete", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::depth_frame>(inv[0]);
        });
        depth_frame_factory.record("get_distance", 1, 3, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::depth_frame>(inv[0]);
            auto x = MatlabParamParser::parse<int>(inv[1]);
            auto y = MatlabParamParser::parse<int>(inv[2]);
            outv[0] = MatlabParamParser::wrap(thiz.get_distance(x, y));
        });
        factory->record(depth_frame_factory);
    }
    {
        ClassFactory disparity_frame_factory("rs2::disparity_frame");
        // rs2::disparity_frame::constructor(rs2::frame)                [?]
        // rs2::disparity_frame::get_baseline                           [TODO]
        factory->record(disparity_frame_factory);
    }
    {
        ClassFactory motion_frame_factory("rs2::motion_frame");
        // rs2::motion_frame::constructor(rs2::frame)                   [?]
        // rs2::motion_frame::get_motion_data                           [TODO]
        factory->record(motion_frame_factory);
    }
    {
        ClassFactory pose_frame_factory("rs2::pose_frame");
        // rs2::pose_frame::constructor(rs2::frame)                     [?]
        // rs2::pose_frame::get_pose_data                               [TODO]
        factory->record(pose_frame_factory);
    }
    {
        ClassFactory frameset_factory("rs2::frameset");
        // TODO:
        // rs2::frameset::constructor()                                 [?]
        // rs2::frameset::constructor(frame)                            [?]
        frameset_factory.record("delete", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::frameset>(inv[0]);
        });
        frameset_factory.record("first_or_default", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frameset>(inv[0]);
            auto s = MatlabParamParser::parse<rs2_stream>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.first_or_default(s));
        });
        frameset_factory.record("first", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frameset>(inv[0]);
            auto s = MatlabParamParser::parse<rs2_stream>(inv[1]);
            // try/catch moved to outer framework
            outv[0] = MatlabParamParser::wrap(thiz.first(s));
        });
        frameset_factory.record("get_depth_frame", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frameset>(inv[0]);
            // try/catch moved to outer framework
            outv[0] = MatlabParamParser::wrap(thiz.get_depth_frame());
        });
        frameset_factory.record("get_color_frame", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frameset>(inv[0]);
            // try/catch moved to outer framework
            outv[0] = MatlabParamParser::wrap(thiz.get_color_frame());
        });
        // rs2::frameset::get_infrared_frame                            [TODO]
        frameset_factory.record("size", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frameset>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.size());
        });
        // rs2::frameset::foreach                                       [?/Callbacks]
        // rs2::frameset::operator[]                                    [?/HOW]
        // rs2::frameset::iterator+begin+end                            [Pure Matlab?]
        factory->record(frameset_factory);
    }

    // rs_sensor.hpp
    // rs2::notification                                                [?]
    {
        ClassFactory options_factory("rs2::options");
        options_factory.record("supports#rs2_option", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::options>(inv[0]);
            auto option = MatlabParamParser::parse<rs2_option>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.supports(option));
        });
        options_factory.record("get_option_description", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::options>(inv[0]);
            auto option = MatlabParamParser::parse<rs2_option>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.get_option_description(option));
        });
        // rs2::options::get_option_value_description                   [TODO]
        // rs2::options::get_option                                     [TODO]
        // rs2::options::get_option_range                               [TODO]
        // rs2::options::set_option                                     [TODO]
        // rs2::options::is_option_read_only                            [TODO]
        // rs2::options::operator=                                      [?/HOW]
        // rs2::options::destructor                                     [?]
        factory->record(options_factory);
    }
    {
        ClassFactory sensor_factory("rs2::sensor");
        // rs2::sensor::constructor()                                   [?]
        sensor_factory.record("delete", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::sensor>(inv[0]);
        });
        // rs2::sensor::open(stream_profile)                            [TODO]
        // rs2::sensor::supports(rs2_camera_info)                       [TODO]
        // rs2::sensor::get_info                                        [TODO]
        // rs2::sensor::open(vector<stream_profile>)                    [TODO]
        // rs2::sensor::close                                           [TODO]
        // rs2::sensor::start(callback)                                 [?/Callbacks]
        // rs2::sensor::start(frame_queue)                              [TODO/Others?]
        // rs2::sensor::stop                                            [TODO]
        // rs2::sensor::get_stream_profiles                             [TODO]
        // rs2::sensor::operator=                                       [?]
        // rs2::sensor::operator bool                                   [?]
        // rs2::sensor::is                                              [TODO] [T = {sensor, roi_sensor, depth_sensor, depth_stereo_sensor}]
        // rs2::sensor::as                                              [TODO] [T = {sensor, roi_sensor, depth_sensor, depth_stereo_sensor}]
        // rs2::sensor::operator==                                      [TODO]
        factory->record(sensor_factory);
    }
    {
        ClassFactory roi_sensor_factory("rs2::roi_sensor");
        // rs2::roi_sensor::constructor(rs2::sensor)                    [?]
        // rs2::roi_sensor::set_region_of_interest                      [TODO]
        // rs2::roi_sensor::get_region_of_interest                      [TODO]
        // rs2::roi_sensor::operator bool                               [?]
        factory->record(roi_sensor_factory);
    }
    {
        ClassFactory depth_sensor_factory("rs2::depth_sensor");
        // rs2::depth_sensor::constructor(rs2::sensor)                  [?]
        // rs2::depth_sensor::get_depth_scale                           [TODO]
        // rs2::depth_sensor::operator bool                             [?]
        factory->record(depth_sensor_factory);
    }
    {
        ClassFactory depth_stereo_sensor_factory("rs2::depth_stereo_sensor");
        // rs2::depth_stereo_sensor::constructor(rs2::sensor)           [?]
        // rs2::depth_stereo_sensor::get_stereo_baseline                [TODO]
        factory->record(depth_stereo_sensor_factory);
    }

    // rs_device.hpp
    {
        ClassFactory device_factory("rs2::device");
        // rs2::device::constructor()                                   [?]
        // extra helper function for constructing device from device_list
        device_factory.record("init", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto list = MatlabParamParser::parse<rs2::device_list>(inv[0]);
            auto idx = MatlabParamParser::parse<uint64_t>(inv[1]);
            outv[0] = MatlabParamParser::wrap(list[idx]);
            MatlabParamParser::destroy<rs2::device_list>(inv[0]);
        });
        // deconstructor in case device was never initialized
        device_factory.record("delete#uninit", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::device_list>(inv[0]);
        });
        // deconstructor in case device was initialized
        device_factory.record("delete#init", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::device>(inv[0]);
        });
        device_factory.record("query_sensors", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.query_sensors());
        });
        device_factory.record("first", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: better, more maintainable implementation?
            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            auto type = MatlabParamParser::parse<std::string>(inv[1]);
            try {
                if (type == "sensor")
                    outv[0] = MatlabParamParser::wrap(thiz.first<rs2::sensor>());
                else if (type == "roi_sensor")
                    outv[0] = MatlabParamParser::wrap(thiz.first<rs2::roi_sensor>());
                else if (type == "depth_sensor")
                    outv[0] = MatlabParamParser::wrap(thiz.first<rs2::depth_sensor>());
                else if (type == "depth_stereo_sensor")
                    outv[0] = MatlabParamParser::wrap(thiz.first<rs2::depth_stereo_sensor>());
                else mexErrMsgTxt("rs2::device::first: Could not find requested sensor type!");
            }
            catch (rs2::error) {
                mexErrMsgTxt("rs2::device::first: Could not find requested sensor type!");
            }
        });
        device_factory.record("supports", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            auto info = MatlabParamParser::parse<rs2_camera_info>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.supports(info));
        });
        device_factory.record("get_info", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            auto info = MatlabParamParser::parse<rs2_camera_info>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.get_info(info));
        });
        device_factory.record("hardware_reset", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            thiz.hardware_reset();
        });
        // rs2::device::operator=                                       [?]
        // rs2::device::operator bool                                   [?]
        device_factory.record("is", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: something more maintainable?
            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            auto type = MatlabParamParser::parse<std::string>(inv[1]);
            try {
                if (type == "device")
                    outv[0] = MatlabParamParser::wrap(thiz.is<rs2::device>());
                else if (type == "debug_protocol") {
                    mexWarnMsgTxt("rs2::device::is: Debug Protocol not supported in MATLAB");
                    outv[0] = MatlabParamParser::wrap(thiz.is<rs2::debug_protocol>());
                }
                else if (type == "advanced_mode") {
                    mexWarnMsgTxt("rs2::device::is: Advanced Mode not supported in MATLAB");
                    outv[0] = MatlabParamParser::wrap(false);
                }
                else if (type == "recorder")
                    outv[0] = MatlabParamParser::wrap(thiz.is<rs2::recorder>());
                else if (type == "playback")
                    outv[0] = MatlabParamParser::wrap(thiz.is<rs2::playback>());
                else {
                    // TODO: need warn/error message? which? if so, fill in
                    mexWarnMsgTxt("rs2::device::is: ...");
                    outv[0] = MatlabParamParser::wrap(false);
                }
            }
            catch (rs2::error) {
                // TODO: need error message? if so, fill in
                mexErrMsgTxt("rs2::device::is: ...");
            }
        });
        // rs2::device::as                                              [Pure Matlab]
        factory->record(device_factory);
    }
    // rs2::debug_protocol                                              [?]
    // rs2::device_list                                                 [Pure Matlab]

    // rs2_record_playback.hpp
    {
        ClassFactory playback_factory("rs2::playback");
        // rs2::playback::constructor(rs2::device)                      [?]
        // rs2::playback::pause()                                       [TODO]
        // rs2::playback::resume()                                      [TODO]
        // rs2::playback::file_name()                                   [TODO]
        // rs2::playback::get_position()                                [TODO]
        // rs2::playback::get_duration()                                [TODO]
        // rs2::playback::seek()                                        [TODO]
        // rs2::playback::is_real_time()                                [TODO]
        // rs2::playback::set_real_time()                               [TODO]
        // rs2::playback::set_playback_speed()                          [TODO]
        // rs2::playback::set_status_changed_callback()                 [?/Callbacks]
        // rs2::playback::current_status()                              [TODO]
        // rs2::playback::stop()                                        [TODO]
        factory->record(playback_factory);
    }
    {
        ClassFactory recorder_factory("rs2::recorder");
        // rs2::recorder::constructor(rs2::device)                      [TODO]
        // rs2::recorder::constructor(string, rs2::device)              [TODO]
        // rs2::recorder::pause()                                       [TODO]
        // rs2::recorder::resume()                                      [TODO]
        // rs2::recorder::filename()                                    [TODO]
        factory->record(recorder_factory);
    }

    // rs2_processing.hpp
    // rs2::processing_block                                            [?]
    {
        ClassFactory frame_queue_factory("rs2::frame_queue");
        // rs2::frame_queue::constructor(uint)                          [TODO]
        // rs2::frame_queue::constructor()                              [TODO]
        // rs2::frame_queue::enqueue(frame)                             [TODO]
        // rs2::frame_queue::wait_for_frame(uint=DEF)                   [TODO]
        // rs2::frame_queue::poll_for_frame(T*)                         [TODO] [T = {frame, video_frame, points, depth_frame, disparity_frame, motion_frame, pose_frame, frameset}]
        // rs2::frame_queue::capacity()                                 [TODO]
        factory->record(frame_queue_factory);
    }
    {
        ClassFactory pointcloud_factory("rs2::pointcloud");
        // rs2::pointcloud::constructor()                               [TODO]
        // rs2::pointcloud::calculate(frame)                            [TODO]
        // rs2::pointcloud::map_to(frame)                               [TODO]
        factory->record(pointcloud_factory);
    }
    {
        ClassFactory syncer_factory("rs2::syncer");
        // rs2::syncer::constructor(int=DEF)                            [TODO]
        // rs2::syncer::wait_for_frames(uint=DEF)                       [TODO]
        // rs2::syncer::poll_for_frames(frameset*)                      [TODO]
        factory->record(syncer_factory);
    }
    {
        ClassFactory align_factory("rs2::align");
        // rs2::align:constructor(rs2_stream)                           [TODO]
        // rs2::align:process(frameset)                                 [TODO]
        factory->record(align_factory);
    }
    {
        ClassFactory colorizer_factory("rs2::colorizer");
        // rs2::colorizer::constructor()                                [TODO]
        // rs2::colorizer::colorize(frame)                              [TODO]
        factory->record(colorizer_factory);
    }
    {
        ClassFactory process_interface_factory("rs2::process_interface");
        // rs2::process_interface::process()                            [TODO]
        // rs2::process_interface::destructor()                         [?]
        factory->record(process_interface_factory);
    }
    {
        ClassFactory decimation_filter_factory("rs2::decimation_filter");
        // rs2::decimation_filter::constructor()                        [TODO]
        factory->record(decimation_filter_factory);
    }
    {
        ClassFactory temporal_filter_factory("rs2::temporal_filter");
        // rs2::temporal_filter::constructor()                          [TODO]
        factory->record(temporal_filter_factory);
    }
    {
        ClassFactory spatial_filter_factory("rs2::spatial_filter");
        // rs2::spatial_filter::constructor()                           [TODO]
        factory->record(spatial_filter_factory);
    }
    {
        ClassFactory disparity_transform_factory("rs2::disparity_transform");
        // rs2::disparity_transform::constructor()                      [TODO]
        factory->record(disparity_transform_factory);
    }
    {
        ClassFactory hole_filling_filter_factory("rs2::hole_filling_filter");
        // rs2::hole_filling_filter::constructor()                      [TODO]
        factory->record(hole_filling_filter_factory);
    }

    // rs_context.hpp
    // rs2::event_information                                           [?]
    {
        ClassFactory context_factory("rs2::context");
        // This lambda feels like it should be possible to generate automatically with templates
        context_factory.record("new", 1, 0, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            outv[0] = MatlabParamParser::wrap(rs2::context());
        });
        context_factory.record("delete", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::context>(inv[0]);
        });
        context_factory.record("query_devices", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::context>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.query_devices());
        });
        context_factory.record("query_all_sensors", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::context>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.query_all_sensors());
        });
        context_factory.record("get_sensor_parent", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::context>(inv[0]);
            auto sensor = MatlabParamParser::parse<rs2::sensor>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.get_sensor_parent(sensor));
        });
        context_factory.record("load_device", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::context>(inv[0]);
            auto file = MatlabParamParser::parse<std::string>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.load_device(file));
        });
        context_factory.record("unload_device", 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::context>(inv[0]);
            auto file = MatlabParamParser::parse<std::string>(inv[1]);
            thiz.unload_device(file);
        });
        factory->record(context_factory);
    }
    {
        ClassFactory device_hub_factory("rs2::device_hub");
        device_hub_factory.record("new", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto ctx = MatlabParamParser::parse<rs2::context>(inv[0]);
            outv[0] = MatlabParamParser::wrap(rs2::device_hub(ctx));
        });
        device_hub_factory.record("delete", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::device_hub>(inv[0]);
        });
        device_hub_factory.record("wait_for_device", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::device_hub>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.wait_for_device());
        });
        device_hub_factory.record("is_connected", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::device_hub>(inv[0]);
            auto dev = MatlabParamParser::parse<rs2::device>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.is_connected(dev));
        });
        factory->record(device_hub_factory);
    }

    // rs_pipeline.hpp
    {
        ClassFactory pipeline_profile_factory("rs2::pipeline_profile");
        // rs2::pipeline_profile::constructor()                         [?]
        // rs2::pipeline_profile::get_streams()                         [TODO]
        // rs2::pipeline_profile::get_stream(rs2_stream, int=DEF)       [TODO]
        // rs2::pipeline_profile::get_device()                          [TODO]
        // rs2::pipeline_profile::bool()                                [?]
        factory->record(pipeline_profile_factory);
    }
    {
        ClassFactory config_factory("rs2::config");
        // rs2::config::constructor()                                   [TODO]
        // rs2::config::enable_stream(rs2_stream, int, int, int, rs2_format=DEF, int=DEF)   [TODO]
        // rs2::config::enable_stream(rs2_stream, int=DEF)              [TODO]
        // rs2::config::enable_stream(rs2_stream, int, int, rs2_format=DEF, int=DEF)        [TODO]
        // rs2::config::enable_stream(rs2_stream, rs2_format, int=DEF)  [TODO]
        // rs2::config::enable_stream(rs2_stream, int, rs2_format, int=DEF)                 [TODO]
        // rs2::config::enable_all_streams()                            [TODO]
        // rs2::config::enable_device(string)                           [TODO]
        // rs2::config::enable_device_from_file(string)                 [TODO]
        // rs2::config::enable_record_to_file(string)                   [TODO]
        // rs2::config::disable_stream(rs2_stream, int=DEF)             [TODO]
        // rs2::config::disable_all_streams()                           [TODO]
        factory->record(config_factory);
    }
    {
        ClassFactory pipeline_factory("rs2::pipeline");
        pipeline_factory.record("new", 1, 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (inc == 0) {
                outv[0] = MatlabParamParser::wrap(rs2::pipeline());
            } else if (inc == 1) {
                auto ctx = MatlabParamParser::parse<rs2::context>(inv[0]);
                outv[0] = MatlabParamParser::wrap(rs2::pipeline(ctx));
            }
        });
        pipeline_factory.record("delete", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::pipeline>(inv[0]);
        });
        pipeline_factory.record("start#void", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::pipeline>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.start());
        });
        pipeline_factory.record("start#config", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::pipeline>(inv[0]);
            auto config = MatlabParamParser::parse<rs2::config>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.start(config));
        });
        pipeline_factory.record("stop", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: add output parameter after binding pipeline_profile
            auto thiz = MatlabParamParser::parse<rs2::pipeline>(inv[0]);
            thiz.stop();
        });
        pipeline_factory.record("wait_for_frames", 1, 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::pipeline>(inv[0]);
            if (inc == 1) {
                outv[0] = MatlabParamParser::wrap(thiz.wait_for_frames());
            } else if (inc == 2) {
                auto timeout_ms = MatlabParamParser::parse<unsigned int>(inv[1]);
                outv[0] = MatlabParamParser::wrap(thiz.wait_for_frames(timeout_ms));
            }
        });
        // rs2::pipeline::poll_for_frames                               [TODO/HOW] [multi-output?]
        // rs2::pipeline::get_active_profile                            [TODO]
        factory->record(pipeline_factory);
    }

    // rs.hpp
    // rs2::log_to_console                                              [TODO]
    // rs2::log_to_file                                                 [TODO]
    // rs2::log                                                         [TODO]

    mexAtExit([]() { delete factory; });
}

void mexFunction(int nOutParams, mxArray *outParams[], int nInParams, const mxArray *inParams[])
{
    // does this need to be made threadsafe? also maybe better idea than global object?
    if (!factory) make_factory();

    if (nInParams < 2) {
        mexErrMsgTxt("At least class and command name are needed.");
        return;
    }
    
    auto cname = MatlabParamParser::parse<std::string>(inParams[0]);
    auto fname = MatlabParamParser::parse<std::string>(inParams[1]);

    auto f_data = factory->get(cname, fname);
    if (!f_data.f) {
        mexErrMsgTxt("Unknown Command received.");
        return;
    }

    if (f_data.out != nOutParams) {
        std::string errmsg = cname + "::" + fname.substr(0, fname.find("#", 0)) + ": Wrong number of outputs";
        mexErrMsgTxt(errmsg.c_str());
    }

    if (f_data.in_min > nInParams - 2 || f_data.in_max < nInParams - 2) {
        std::string errmsg = cname + "::" + fname.substr(0, fname.find("#", 0)) + ": Wrong number of inputs";
        mexErrMsgTxt(errmsg.c_str());
    }
    try {
        f_data.f(nOutParams, outParams, nInParams - 2, inParams + 2); // "eat" the two function specifiers
    } catch (std::exception &e) {
        mexErrMsgTxt(e.what());
    }
}