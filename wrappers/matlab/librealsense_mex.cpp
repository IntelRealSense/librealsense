#include "MatlabParamParser.h"
#include "Factory.h"
#include "librealsense2/rs.hpp"
#include "librealsense2/rs_advanced_mode.hpp"
#include "librealsense2/hpp/rs_export.hpp"

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
        stream_profile_factory.record("operator==", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto rhs = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            auto lhs = MatlabParamParser::parse<rs2::stream_profile>(inv[1]);
            MatlabParamParser::wrap(rhs == lhs);
        });
        stream_profile_factory.record("is", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: something more maintainable?
            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            auto type = MatlabParamParser::parse<std::string>(inv[1]);
            if (type == "stream_profile")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::stream_profile>());
            else if (type == "video_stream_profile")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::video_stream_profile>());
            else if (type == "motion_stream_profile")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::motion_stream_profile>());
            else {
                mexWarnMsgTxt("rs2::stream_profile::is: invalid type parameter");
                outv[0] = MatlabParamParser::wrap(false);
            }
        });
        stream_profile_factory.record("as", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: something more maintainable?
            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            auto type = MatlabParamParser::parse<std::string>(inv[1]);
            if (type == "stream_profile")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::stream_profile>());
            else if (type == "video_stream_profile")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::video_stream_profile>());
            else if (type == "motion_stream_profile")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::motion_stream_profile>());
            else {
                mexWarnMsgTxt("rs2::stream_profile::as: invalid type parameter");
                outv[0] = MatlabParamParser::wrap(uint64_t(0));
            }
        });
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
        stream_profile_factory.record("operator bool", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(bool(thiz));
        });
        stream_profile_factory.record("get_extrinsics_to", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            auto to = MatlabParamParser::parse<rs2::stream_profile>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.get_extrinsics_to(to));
        });
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
        video_stream_profile_factory.record("width", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::video_stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.width());
        });
        video_stream_profile_factory.record("height", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::video_stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.height());
        });
        video_stream_profile_factory.record("get_intrinsics", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::video_stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_intrinsics());
        });
        factory->record(video_stream_profile_factory);
    }
    {
        ClassFactory motion_stream_profile_factory("rs2::motion_stream_profile");
        // rs2::motion_stream_profile::constructor(rs2::stream_profile) [?]
        motion_stream_profile_factory.record("get_motion_intrinsics", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::motion_stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_motion_intrinsics());
        });
        factory->record(motion_stream_profile_factory);
    }
    //{
    //    ClassFactory pose_stream_profile_factory("rs2::pose_stream_profile");
    //    // rs2::pose_stream_profile::constructor(rs2::stream_profile) [?]
    //    factory->record(pose_stream_profile_factory);
    //}
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
        // rs2::frame::keep                                             [TODO/HOW]
        frame_factory.record("operator bool", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(bool(thiz));
        });
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
            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            size_t n_bytes = 0;
            if (auto vf = thiz.as<rs2::video_frame>()) {
                n_bytes = vf.get_height() * vf.get_stride_in_bytes();
            } // TODO: support more frame types?

            switch (thiz.get_profile().format()) {
            case RS2_FORMAT_RAW10:
                // TODO: Do the bit hackery ourselves?
                mexWarnMsgTxt("Raw10 data provided as unsigned byte array.");
            case RS2_FORMAT_RGB8: case RS2_FORMAT_RGBA8:
            case RS2_FORMAT_BGR8: case RS2_FORMAT_BGRA8:
            case RS2_FORMAT_Y8: case RS2_FORMAT_RAW8:
                if (n_bytes == 0) {
                    n_bytes = 1;
                    mexWarnMsgTxt("Can't detect frame dims, sending only first pixel");
                }
                outv[0] = MatlabParamParser::wrap_array(reinterpret_cast<const uint8_t*>(thiz.get_data()), n_bytes);
                break;
            case RS2_FORMAT_Z16: case RS2_FORMAT_DISPARITY16:
            case RS2_FORMAT_Y16: case RS2_FORMAT_RAW16:
                if (n_bytes == 0) {
                    n_bytes = 2;
                    mexWarnMsgTxt("Can't detect frame dims, sending only first pixel");
                }
                outv[0] = MatlabParamParser::wrap_array(reinterpret_cast<const uint16_t*>(thiz.get_data()), n_bytes / 2);
                break;
            case RS2_FORMAT_XYZ32F: case RS2_FORMAT_DISPARITY32:
            case RS2_FORMAT_MOTION_XYZ32F:
                if (n_bytes == 0) {
                    n_bytes = 4;
                    mexWarnMsgTxt("Can't detect frame dims, sending only first pixel");
                }
                outv[0] = MatlabParamParser::wrap_array(reinterpret_cast<const float*>(thiz.get_data()), n_bytes / 4);
                break;
            case RS2_FORMAT_UYVY: case RS2_FORMAT_YUYV:
                if (n_bytes == 0) {
                    n_bytes = 4;
                    mexWarnMsgTxt("Can't detect frame dims, sending only first pixel");
                }
                outv[0] = MatlabParamParser::wrap_array(reinterpret_cast<const uint32_t*>(thiz.get_data()), n_bytes / 4);
                break;
            default:
                mexWarnMsgTxt("This format isn't supported yet. Sending unsigned byte stream");
                if (n_bytes == 0) {
                    n_bytes = 1;
                    mexWarnMsgTxt("Can't detect frame dims, sending only first pixel");
                }
                outv[0] = MatlabParamParser::wrap_array(reinterpret_cast<const uint8_t*>(thiz.get_data()), n_bytes);
            }
        });
        frame_factory.record("get_profile", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_profile());
        });
        frame_factory.record("is", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: something more maintainable?
            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            auto type = MatlabParamParser::parse<std::string>(inv[1]);
            if (type == "frame")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::frame>());
            else if (type == "video_frame")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::video_frame>());
            else if (type == "points")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::points>());
            else if (type == "depth_frame")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::depth_frame>());
            else if (type == "disparity_frame")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::disparity_frame>());
            else if (type == "motion_frame")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::motion_frame>());
            else if (type == "pose_frame")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::pose_frame>());
            else if (type == "frameset")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::frameset>());
            else {
                mexWarnMsgTxt("rs2::frame::is: invalid type parameter");
                outv[0] = MatlabParamParser::wrap(false);
            }
        });
        frame_factory.record("as", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: something more maintainable?
            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            auto type = MatlabParamParser::parse<std::string>(inv[1]);
            if (type == "frame")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::frame>());
            else if (type == "video_frame")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::video_frame>());
            else if (type == "points")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::points>());
            else if (type == "depth_frame")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::depth_frame>());
            else if (type == "disparity_frame")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::disparity_frame>());
            else if (type == "motion_frame")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::motion_frame>());
            else if (type == "pose_frame")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::pose_frame>());
            else if (type == "frameset")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::frameset>());
            else {
                mexWarnMsgTxt("rs2::frame::as: invalid type parameter");
                outv[0] = MatlabParamParser::wrap(uint64_t(0));
            }
        });
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
        // rs2::points::constructor()                                   [?]
        // rs2::points::constrcutor(rs2::frame)                         [?]
        points_factory.record("get_vertices", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::points>(inv[0]);
            // TODO: turn into matrix instead of column?
            outv[0] = MatlabParamParser::wrap_array(thiz.get_vertices(), thiz.size());
        });
        points_factory.record("export_to_ply", 0, 3, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::points>(inv[0]);
            auto fname = MatlabParamParser::parse<std::string>(inv[1]);
            auto texture = MatlabParamParser::parse<rs2::video_frame>(inv[2]);
            thiz.export_to_ply(fname, texture);
        });
        points_factory.record("get_texture_coordinates", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::points>(inv[0]);
            // TODO: turn into matrix instead of column?
            outv[0] = MatlabParamParser::wrap_array(thiz.get_texture_coordinates(), thiz.size());
        });
        points_factory.record("size", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::points>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.size());
        });
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
        disparity_frame_factory.record("get_baseline", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::disparity_frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_baseline());
        });
        factory->record(disparity_frame_factory);
    }
    {
        ClassFactory motion_frame_factory("rs2::motion_frame");
        // rs2::motion_frame::constructor(rs2::frame)                   [?]
        motion_frame_factory.record("get_motion_data", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::motion_frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_motion_data());
        });
        factory->record(motion_frame_factory);
    }
    {
        ClassFactory pose_frame_factory("rs2::pose_frame");
        // rs2::pose_frame::constructor(rs2::frame)                     [?]
        pose_frame_factory.record("get_pose_data", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::pose_frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_pose_data());
        });
        factory->record(pose_frame_factory);
    }
    {
        ClassFactory frameset_factory("rs2::frameset");
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
        frameset_factory.record("get_infrared_frame", 1, 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frameset>(inv[0]);
            // try/catch moved to outer framework
            if (inc == 1)
                outv[0] = MatlabParamParser::wrap(thiz.get_infrared_frame());
            else {
                auto index = MatlabParamParser::parse<size_t>(inv[1]);
                outv[0] = MatlabParamParser::wrap(thiz.get_infrared_frame(index));
            }
        });
        frameset_factory.record("get_fisheye_frame", 1, 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frameset>(inv[0]);
            // try/catch moved to outer framework
            if (inc == 1)
                outv[0] = MatlabParamParser::wrap(thiz.get_fisheye_frame());
            else {
                auto index = MatlabParamParser::parse<size_t>(inv[1]);
                outv[0] = MatlabParamParser::wrap(thiz.get_fisheye_frame(index));
            }
        });
        frameset_factory.record("get_pose_frame", 1, 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frameset>(inv[0]);
            // try/catch moved to outer framework
            if (inc == 1)
                outv[0] = MatlabParamParser::wrap(thiz.get_pose_frame());
            else {
                auto index = MatlabParamParser::parse<size_t>(inv[1]);
                outv[0] = MatlabParamParser::wrap(thiz.get_pose_frame(index));
            }
        });
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
        options_factory.record("get_option_value_description", 1, 3, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::options>(inv[0]);
            auto option = MatlabParamParser::parse<rs2_option>(inv[1]);
            auto val = MatlabParamParser::parse<float>(inv[2]);
            outv[0] = MatlabParamParser::wrap(thiz.get_option_value_description(option, val));
        });
        options_factory.record("get_option", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::options>(inv[0]);
            auto option = MatlabParamParser::parse<rs2_option>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.get_option(option));
        });
        options_factory.record("get_option_range", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::options>(inv[0]);
            auto option = MatlabParamParser::parse<rs2_option>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.get_option_range(option));
        });
        options_factory.record("set_option", 0, 3, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::options>(inv[0]);
            auto option = MatlabParamParser::parse<rs2_option>(inv[1]);
            auto val = MatlabParamParser::parse<float>(inv[2]);
            thiz.set_option(option, val);
        });
        options_factory.record("is_option_read_only", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::options>(inv[0]);
            auto option = MatlabParamParser::parse<rs2_option>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.is_option_read_only(option));
        });
        // rs2::options::operator=                                      [?/HOW]
        options_factory.record("delete", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::options>(inv[0]);
        });
        factory->record(options_factory);
    }
    {
        ClassFactory sensor_factory("rs2::sensor");
        // rs2::sensor::constructor()                                   [?]
        sensor_factory.record("delete", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::sensor>(inv[0]);
        });
        sensor_factory.record("open#stream_profile", 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::sensor>(inv[0]);
            auto profile = MatlabParamParser::parse<rs2::stream_profile>(inv[1]);
            thiz.open(profile);
        });
        sensor_factory.record("supports#rs2_camera_info", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::sensor>(inv[0]);
            auto info = MatlabParamParser::parse<rs2_camera_info>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.supports(info));
        });
        sensor_factory.record("get_info", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::sensor>(inv[0]);
            auto info = MatlabParamParser::parse<rs2_camera_info>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.get_info(info));
        });
        sensor_factory.record("open#vec_stream_profile", 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::sensor>(inv[0]);
            auto profiles = MatlabParamParser::parse<std::vector<rs2::stream_profile>>(inv[1]);
            thiz.open(profiles);
        });
        sensor_factory.record("close", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::sensor>(inv[0]);
            thiz.close();
        });
        // rs2::sensor::start(*)                                 [?/Which/Callbacks]
        sensor_factory.record("start#frame_queue", 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::sensor>(inv[0]);
            auto queue = MatlabParamParser::parse<rs2::frame_queue>(inv[1]);
            thiz.start(queue);
        });
        sensor_factory.record("stop", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::sensor>(inv[0]);
            thiz.stop();
        });
        sensor_factory.record("get_stream_profiles", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::sensor>(inv[0]);
            MatlabParamParser::wrap(thiz.get_stream_profiles());
        });
        // rs2::sensor::operator=                                       [?]
        sensor_factory.record("operator bool", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::sensor>(inv[0]);
            outv[0] = MatlabParamParser::wrap(bool(thiz));
        });
        sensor_factory.record("is", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: something more maintainable?
            auto thiz = MatlabParamParser::parse<rs2::sensor>(inv[0]);
            auto type = MatlabParamParser::parse<std::string>(inv[1]);
            if (type == "sensor")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::sensor>());
            else if (type == "roi_sensor")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::roi_sensor>());
            else if (type == "depth_sensor")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::depth_sensor>());
            else if (type == "depth_stereo_sensor")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::depth_stereo_sensor>());
            else {
                mexWarnMsgTxt("rs2::sensor::is: invalid type parameter");
                outv[0] = MatlabParamParser::wrap(false);
            }
        });
        sensor_factory.record("as", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: something more maintainable?
            auto thiz = MatlabParamParser::parse<rs2::sensor>(inv[0]);
            auto type = MatlabParamParser::parse<std::string>(inv[1]);
            if (type == "sensor")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::sensor>());
            else if (type == "roi_sensor")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::roi_sensor>());
            else if (type == "depth_sensor")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::depth_sensor>());
            else if (type == "depth_stereo_sensor")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::depth_stereo_sensor>());
            else {
                mexWarnMsgTxt("rs2::sensor::as: invalid type parameter");
                outv[0] = MatlabParamParser::wrap(uint64_t(0));
            }
        });
        sensor_factory.record("operator==", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto rhs = MatlabParamParser::parse<rs2::sensor>(inv[0]);
            auto lhs = MatlabParamParser::parse<rs2::sensor>(inv[1]);
            MatlabParamParser::wrap(rhs == lhs);
        });
        factory->record(sensor_factory);
    }
    {
        ClassFactory roi_sensor_factory("rs2::roi_sensor");
        // rs2::roi_sensor::constructor(rs2::sensor)                    [?]
        roi_sensor_factory.record("set_region_of_interest", 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::roi_sensor>(inv[0]);
            auto roi = MatlabParamParser::parse<rs2::region_of_interest>(inv[1]);
            thiz.set_region_of_interest(roi);
        });
        roi_sensor_factory.record("get_region_of_interest", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::roi_sensor>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_region_of_interest());
        });
        // rs2::roi_sensor::operator bool                               [?]
        factory->record(roi_sensor_factory);
    }
    {
        ClassFactory depth_sensor_factory("rs2::depth_sensor");
        // rs2::depth_sensor::constructor(rs2::sensor)                  [?]
        depth_sensor_factory.record("get_depth_scale", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::depth_sensor>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_depth_scale());
        });
        // rs2::depth_sensor::operator bool                             [?]
        factory->record(depth_sensor_factory);
    }
    {
        ClassFactory depth_stereo_sensor_factory("rs2::depth_stereo_sensor");
        // rs2::depth_stereo_sensor::constructor(rs2::sensor)           [?]
        depth_stereo_sensor_factory.record("get_stereo_baseline", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::depth_stereo_sensor>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_stereo_baseline());
        });
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
            auto idx = MatlabParamParser::parse<uint32_t>(inv[1]);
            outv[0] = MatlabParamParser::wrap(list[idx]);
            MatlabParamParser::destroy<rs2::device_list>(inv[0]);
        });
        // destructor in case device was never initialized
        device_factory.record("delete#uninit", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::device_list>(inv[0]);
        });
        // destructor in case device was initialized
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
        device_factory.record("operator bool", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            outv[0] = MatlabParamParser::wrap(bool(thiz));
        });
        device_factory.record("is", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: something more maintainable?
            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            auto type = MatlabParamParser::parse<std::string>(inv[1]);
            if (type == "device")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::device>());
            else if (type == "debug_protocol") {
                mexWarnMsgTxt("rs2::device::is: Debug Protocol not supported in MATLAB");
                outv[0] = MatlabParamParser::wrap(false);
            }
            else if (type == "advanced_mode")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs400::advanced_mode>());
            else if (type == "recorder")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::recorder>());
            else if (type == "playback")
                outv[0] = MatlabParamParser::wrap(thiz.is<rs2::playback>());
            else {
                mexWarnMsgTxt("rs2::device::is: invalid type parameter");
                outv[0] = MatlabParamParser::wrap(false);
            }
        });
        device_factory.record("as", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: something more maintainable?
            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            auto type = MatlabParamParser::parse<std::string>(inv[1]);
            if (type == "device")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::device>());
            else if (type == "debug_protocol") {
                mexErrMsgTxt("rs2::device::as: Debug Protocol not supported in MATLAB");
//                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::debug_protocol>());
            }
            else if (type == "advanced_mode")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs400::advanced_mode>());
            else if (type == "recorder")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::recorder>());
            else if (type == "playback")
                outv[0] = MatlabParamParser::wrap(thiz.as<rs2::playback>());
            else {
                mexWarnMsgTxt("rs2::device::as: invalid type parameter");
                outv[0] = MatlabParamParser::wrap(false);
            }
        });
        factory->record(device_factory);
    }
    // rs2::debug_protocol                                              [?]
    // rs2::device_list                                                 [Pure Matlab]

    // rs2_record_playback.hpp
    {
        ClassFactory playback_factory("rs2::playback");
        // rs2::playback::constructor(rs2::device)                      [?]
        playback_factory.record("pause", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::playback>(inv[0]);
            thiz.pause();
        });
        playback_factory.record("resume", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::playback>(inv[0]);
            thiz.resume();
        });
        playback_factory.record("file_name", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::playback>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.file_name());
        });
        playback_factory.record("get_position", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::playback>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_position());
        });
        playback_factory.record("get_duration", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::playback>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_duration());
        });
        playback_factory.record("seek", 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::playback>(inv[0]);
            auto time = MatlabParamParser::parse<std::chrono::nanoseconds>(inv[1]);
            thiz.seek(time);
        });
        playback_factory.record("is_real_time", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::playback>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.is_real_time());
        });
        playback_factory.record("set_real_time", 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::playback>(inv[0]);
            auto real_time = MatlabParamParser::parse<bool>(inv[1]);
            thiz.set_real_time(real_time);
        });
        playback_factory.record("set_playback_speed", 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::playback>(inv[0]);
            auto speed = MatlabParamParser::parse<float>(inv[1]);
            thiz.set_playback_speed(speed);
        });
        // rs2::playback::set_status_changed_callback()                 [?/Callbacks]
        playback_factory.record("current_status", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::playback>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.current_status());
        });
        playback_factory.record("stop", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::playback>(inv[0]);
            thiz.stop();
        });
        factory->record(playback_factory);
    }
    {
        ClassFactory recorder_factory("rs2::recorder");
        // rs2::recorder::constructor(rs2::device)                      [?]
        recorder_factory.record("new#string_device", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto file = MatlabParamParser::parse<std::string>(inv[0]);
            auto device = MatlabParamParser::parse<rs2::device>(inv[1]);
            outv[0] = MatlabParamParser::wrap(rs2::recorder(file, device));
        });
        recorder_factory.record("pause", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::recorder>(inv[0]);
            thiz.pause();
        });
        recorder_factory.record("resume", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::recorder>(inv[0]);
            thiz.resume();
        });
        recorder_factory.record("filename", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::recorder>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.filename());
        });
        factory->record(recorder_factory);
    }

    // rs2_processing.hpp
    // rs2::processing_block                                            [?]
    {
        ClassFactory frame_queue_factory("rs2::frame_queue");
        frame_queue_factory.record("new", 1, 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (inc == 0) {
                outv[0] = MatlabParamParser::wrap(rs2::frame_queue());
            }
            else if (inc == 1) {
                auto capacity = MatlabParamParser::parse<unsigned int>(inv[0]);
                outv[0] = MatlabParamParser::wrap(rs2::frame_queue(capacity));
            }
            else if (inc == 2) {
                auto capacity = MatlabParamParser::parse<unsigned int>(inv[0]);
                auto keep_frames = MatlabParamParser::parse<bool>(inv[1]);
                outv[0] = MatlabParamParser::wrap(rs2::frame_queue(capacity, keep_frames));
            }
        });
        frame_queue_factory.record("delete", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::frame_queue>(inv[0]);
        });
        // rs2::frame_queue::enqueue(frame)                             [?]
        frame_queue_factory.record("wait_for_frame", 1, 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frame_queue>(inv[0]);
            if (inc == 1) {
                outv[0] = MatlabParamParser::wrap(thiz.wait_for_frame());
            }
            else if (inc == 2) {
                auto timeout_ms = MatlabParamParser::parse<unsigned int>(inv[1]);
                outv[0] = MatlabParamParser::wrap(thiz.wait_for_frame(timeout_ms));
            }
        });
        frame_queue_factory.record("poll_for_frame", 2, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frame_queue>(inv[0]);
            auto type = MatlabParamParser::parse<std::string>(inv[1]);
            if (type == "frame") {
                auto f = rs2::frame();
                outv[0] = MatlabParamParser::wrap(thiz.poll_for_frame(&f));
                outv[1] = MatlabParamParser::wrap(std::move(f));
            } else if (type == "video_frame") {
                auto f = rs2::video_frame(rs2::frame());
                outv[0] = MatlabParamParser::wrap(thiz.poll_for_frame(&f));
                outv[1] = MatlabParamParser::wrap(std::move(f));
            } else if (type == "points"){
                auto f = rs2::points();
                outv[0] = MatlabParamParser::wrap(thiz.poll_for_frame(&f));
                outv[1] = MatlabParamParser::wrap(std::move(f));
            } else if (type == "depth_frame"){
                auto f = rs2::depth_frame(rs2::frame());
                outv[0] = MatlabParamParser::wrap(thiz.poll_for_frame(&f));
                outv[1] = MatlabParamParser::wrap(std::move(f));
            } else if (type == "disparity_frame"){
                auto f = rs2::disparity_frame(rs2::frame());
                outv[0] = MatlabParamParser::wrap(thiz.poll_for_frame(&f));
                outv[1] = MatlabParamParser::wrap(std::move(f));
            } else if (type == "motion_frame"){
                auto f = rs2::motion_frame(rs2::frame());
                outv[0] = MatlabParamParser::wrap(thiz.poll_for_frame(&f));
                outv[1] = MatlabParamParser::wrap(std::move(f));
            } else if (type == "pose_frame"){
                auto f = rs2::pose_frame(rs2::frame());
                outv[0] = MatlabParamParser::wrap(thiz.poll_for_frame(&f));
                outv[1] = MatlabParamParser::wrap(std::move(f));
            } else if (type == "frameset"){
                auto f = rs2::frameset();
                outv[0] = MatlabParamParser::wrap(thiz.poll_for_frame(&f));
                outv[1] = MatlabParamParser::wrap(std::move(f));
            } else {
                mexWarnMsgTxt("rs2::frame_queue::poll_for_frame: invalid type parameter");
                outv[0] = MatlabParamParser::wrap(false);
                outv[1] = MatlabParamParser::wrap(rs2::frame());
            }
        });
        frame_queue_factory.record("capacity", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frame_queue>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.capacity());
        });
        frame_queue_factory.record("keep_frames", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::frame_queue>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.keep_frames());
        });
        factory->record(frame_queue_factory);
    }
// TODO: need to understand how to call matlab functions from within C++ before async things can be implemented.
// TODO: What to do about supports/get_info? Just attach to filter?
// processing_block API is completely async.
//    {
//        ClassFactory processing_block_factory("rs2::processing_block");
//
//        factory->record(processing_block_factory);
//    }
    {
        ClassFactory filter_factory("rs2::filter");
        filter_factory.record("process", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::filter>(inv[0]);
            auto frame = MatlabParamParser::parse<rs2::frame>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.process(frame));
        });
        factory->record(filter_factory);
    }
    {
        ClassFactory pointcloud_factory("rs2::pointcloud");
        pointcloud_factory.record("new", 1, 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (inc == 0) {
                outv[0] = MatlabParamParser::wrap(rs2::pointcloud());
                return;
            }
            auto stream = MatlabParamParser::parse<rs2_stream>(inv[0]);
            if (inc == 1)
                outv[0] = MatlabParamParser::wrap(rs2::pointcloud(stream));
            else {
                auto index = MatlabParamParser::parse<int>(inv[1]);
                outv[0] = MatlabParamParser::wrap(rs2::pointcloud(stream, index));
            }
        });
        pointcloud_factory.record("calculate", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::pointcloud>(inv[0]);
            auto depth = MatlabParamParser::parse<rs2::frame>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.calculate(depth));
        });
        pointcloud_factory.record("map_to", 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::pointcloud>(inv[0]);
            auto mapped = MatlabParamParser::parse<rs2::frame>(inv[1]);
            thiz.map_to(mapped);
        });
        factory->record(pointcloud_factory);
    }
    {
        ClassFactory syncer_factory("rs2::syncer");
        syncer_factory.record("new", 1, 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (inc == 0) {
                outv[0] = MatlabParamParser::wrap(rs2::syncer());
            }
            else if (inc == 1) {
                auto queue_size = MatlabParamParser::parse<int>(inv[0]);
                outv[0] = MatlabParamParser::wrap(rs2::syncer(queue_size));
            }
        });
        syncer_factory.record("delete", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::syncer>(inv[0]);
        });
        syncer_factory.record("wait_for_frames", 1, 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::syncer>(inv[0]);
            if (inc == 1) {
                outv[0] = MatlabParamParser::wrap(thiz.wait_for_frames());
            }
            else if (inc == 2) {
                auto timeout_ms = MatlabParamParser::parse<unsigned int>(inv[1]);
                outv[0] = MatlabParamParser::wrap(thiz.wait_for_frames(timeout_ms));
            }
        });
        syncer_factory.record("poll_for_frames", 2, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::syncer>(inv[0]);
            rs2::frameset fs;
            outv[0] = MatlabParamParser::wrap(thiz.poll_for_frames(&fs));
            outv[1] = MatlabParamParser::wrap(std::move(fs));
        });
        factory->record(syncer_factory);
    }
    {
        ClassFactory align_factory("rs2::align");
        align_factory.record("new", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto align_to = MatlabParamParser::parse<rs2_stream>(inv[0]);
            outv[0] = MatlabParamParser::wrap(rs2::align(align_to));
        });
        // TODO: how does this interact with the processing_block variant? Why are the separate?
        align_factory.record("process", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::align>(inv[0]);
            auto frame = MatlabParamParser::parse<rs2::frameset>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.process(frame));
        });
        factory->record(align_factory);
    }
    {
        ClassFactory colorizer_factory("rs2::colorizer");
        colorizer_factory.record("new", 1, 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (inc == 0) outv[0] = MatlabParamParser::wrap(rs2::colorizer());
            else {
                auto color_scheme = MatlabParamParser::parse<float>(inv[0]);
                outv[0] = MatlabParamParser::wrap(rs2::colorizer(color_scheme));
            }
        });
        colorizer_factory.record("colorize", 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::colorizer>(inv[0]);
            auto depth = MatlabParamParser::parse<rs2::frame>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.colorize(depth));
        });
        factory->record(colorizer_factory);
    }
    {
        ClassFactory decimation_filter_factory("rs2::decimation_filter");
        decimation_filter_factory.record("new", 1, 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (inc == 0) outv[0] = MatlabParamParser::wrap(rs2::decimation_filter());
            else {
                auto magnitude = MatlabParamParser::parse<float>(inv[0]);
                outv[0] = MatlabParamParser::wrap(rs2::decimation_filter(magnitude));
            }
        });
        factory->record(decimation_filter_factory);
    }
    {
        ClassFactory temporal_filter_factory("rs2::temporal_filter");
        temporal_filter_factory.record("new", 1, 0, 3, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (inc == 0) outv[0] = MatlabParamParser::wrap(rs2::temporal_filter());
            else if (inc == 3) {
                auto smooth_alpha = MatlabParamParser::parse<float>(inv[0]);
                auto smooth_delta = MatlabParamParser::parse<float>(inv[1]);
                auto persistence_control = MatlabParamParser::parse<int>(inv[2]);
                outv[0] = MatlabParamParser::wrap(rs2::temporal_filter(smooth_alpha, smooth_delta, persistence_control));
            }
            else mexErrMsgTxt("rs2::temporal_filter::new: Wrong number of Inputs");
        });
        factory->record(temporal_filter_factory);
    }
    {
        ClassFactory spatial_filter_factory("rs2::spatial_filter");
        spatial_filter_factory.record("new", 1, 0, 4, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (inc == 0) outv[0] = MatlabParamParser::wrap(rs2::spatial_filter());
            else if (inc == 4) {
                auto smooth_alpha = MatlabParamParser::parse<float>(inv[0]);
                auto smooth_delta = MatlabParamParser::parse<float>(inv[1]);
                auto magnitude = MatlabParamParser::parse<float>(inv[2]);
                auto hole_fill = MatlabParamParser::parse<float>(inv[3]);
                outv[0] = MatlabParamParser::wrap(rs2::spatial_filter(smooth_alpha, smooth_delta, magnitude, hole_fill));
            }
            else mexErrMsgTxt("rs2::spatial_filter::new: Wrong number of Inputs");
        });
        factory->record(spatial_filter_factory);
    }
    {
        ClassFactory disparity_transform_factory("rs2::disparity_transform");
        disparity_transform_factory.record("new", 1, 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (inc == 0) {
                outv[0] = MatlabParamParser::wrap(rs2::disparity_transform());
            }
            else if (inc == 1) {
                auto transform_to_disparity = MatlabParamParser::parse<bool>(inv[0]);
                outv[0] = MatlabParamParser::wrap(rs2::disparity_transform(transform_to_disparity));
            }
        });
        factory->record(disparity_transform_factory);
    }
    {
        ClassFactory hole_filling_filter_factory("rs2::hole_filling_filter");
        hole_filling_filter_factory.record("new", 1, 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (inc == 0) outv[0] = MatlabParamParser::wrap(rs2::hole_filling_filter());
            else {
                auto mode = MatlabParamParser::parse<int>(inv[0]);
                outv[0] = MatlabParamParser::wrap(rs2::hole_filling_filter(mode));
            }
        });
        factory->record(hole_filling_filter_factory);
    }

    // rs_export.hpp
    {
        ClassFactory save_to_ply_factory("rs2::save_to_ply");
        save_to_ply_factory.record("new", 1, 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (inc == 0) {
                outv[0] = MatlabParamParser::wrap(rs2::save_to_ply());
            }
            else if (inc == 1) {
                auto filename = MatlabParamParser::parse<std::string>(inv[0]);
                outv[0] = MatlabParamParser::wrap(rs2::save_to_ply(filename));
            }
            else if (inc == 2) {
                auto filename = MatlabParamParser::parse<std::string>(inv[0]);
                auto pc = MatlabParamParser::parse<rs2::pointcloud>(inv[1]);
                outv[0] = MatlabParamParser::wrap(rs2::save_to_ply(filename, pc));
            }
        });
        factory->record(save_to_ply_factory);
    }
    {
        ClassFactory save_single_frameset_factory("rs2::save_single_frameset");
        save_single_frameset_factory.record("new", 1, 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (inc == 0) {
                outv[0] = MatlabParamParser::wrap(rs2::save_single_frameset());
            }
            else if (inc == 1) {
                auto filename = MatlabParamParser::parse<std::string>(inv[0]);
                outv[0] = MatlabParamParser::wrap(rs2::save_single_frameset(filename));
            }
        });
        factory->record(save_single_frameset_factory);
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
        context_factory.record("unload_tracking_module", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::context>(inv[0]);
            thiz.unload_tracking_module();
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
        pipeline_profile_factory.record("delete", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::pipeline_profile>(inv[0]);
        });
        pipeline_profile_factory.record("get_streams", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::pipeline_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_streams());
        });
        pipeline_profile_factory.record("get_stream", 1, 2, 3, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::pipeline_profile>(inv[0]);
            auto stream_type = MatlabParamParser::parse<rs2_stream>(inv[1]);
            if (inc == 2)
                outv[0] = MatlabParamParser::wrap(thiz.get_stream(stream_type));
            else {
                auto stream_index = MatlabParamParser::parse<int>(inv[2]);
                outv[0] = MatlabParamParser::wrap(thiz.get_stream(stream_type, stream_index));
            }
        });
        pipeline_profile_factory.record("get_device", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::pipeline_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_device());
        });
        // rs2::pipeline_profile::bool()                                [?]
        factory->record(pipeline_profile_factory);
    }
    {
        ClassFactory config_factory("rs2::config");
        config_factory.record("new", 1, 0, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            outv[0] = MatlabParamParser::wrap(rs2::config());
        });
        config_factory.record("delete", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            MatlabParamParser::destroy<rs2::config>(inv[0]);
        });
        config_factory.record("enable_stream#full", 0, 5, 7, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::config>(inv[0]);
            auto stream_type = MatlabParamParser::parse<rs2_stream>(inv[1]);
            auto stream_index = MatlabParamParser::parse<int>(inv[2]);
            auto width = MatlabParamParser::parse<int>(inv[3]);
            auto height = MatlabParamParser::parse<int>(inv[4]);
            if (inc == 5) {
                thiz.enable_stream(stream_type, stream_index, width, height);
            } else if (inc == 6) {
                auto format = MatlabParamParser::parse<rs2_format>(inv[5]);
                thiz.enable_stream(stream_type, stream_index, width, height, format);
            } else if (inc == 7) {
                auto format = MatlabParamParser::parse<rs2_format>(inv[5]);
                auto framerate = MatlabParamParser::parse<int>(inv[6]);
                thiz.enable_stream(stream_type, stream_index, width, height, format, framerate);
            }
        });
        config_factory.record("enable_stream#stream", 0, 2, 3, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::config>(inv[0]);
            auto stream_type = MatlabParamParser::parse<rs2_stream>(inv[1]);
            if (inc == 2) {
                thiz.enable_stream(stream_type);
            } else if (inc == 3){
                auto stream_index = MatlabParamParser::parse<int>(inv[2]);
                thiz.enable_stream(stream_type, stream_index);
            }
        });
        config_factory.record("enable_stream#size", 0, 4, 6, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::config>(inv[0]);
            auto stream_type = MatlabParamParser::parse<rs2_stream>(inv[1]);
            auto width = MatlabParamParser::parse<int>(inv[2]);
            auto height = MatlabParamParser::parse<int>(inv[3]);
            if (inc == 4) {
                thiz.enable_stream(stream_type, width, height);
            } else if (inc == 5) {
                auto format = MatlabParamParser::parse<rs2_format>(inv[4]);
                thiz.enable_stream(stream_type, width, height, format);
            } else if (inc == 6) {
                auto format = MatlabParamParser::parse<rs2_format>(inv[4]);
                auto framerate = MatlabParamParser::parse<int>(inv[5]);
                thiz.enable_stream(stream_type, width, height, format, framerate);
            }
        });
        config_factory.record("enable_stream#format", 0, 3, 4, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::config>(inv[0]);
            auto stream_type = MatlabParamParser::parse<rs2_stream>(inv[1]);
            auto format = MatlabParamParser::parse<rs2_format>(inv[2]);
            if (inc == 3) {
                thiz.enable_stream(stream_type, format);
            } else if (inc == 4) {
                auto framerate = MatlabParamParser::parse<int>(inv[3]);
                thiz.enable_stream(stream_type, format, framerate);
            }
        });
        config_factory.record("enable_stream#extended", 0, 4, 5, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::config>(inv[0]);
            auto stream_type = MatlabParamParser::parse<rs2_stream>(inv[1]);
            auto stream_index = MatlabParamParser::parse<int>(inv[2]);
            auto format = MatlabParamParser::parse<rs2_format>(inv[3]);
            if (inc == 4) {
                thiz.enable_stream(stream_type, stream_index, format);
            } else if (inc == 5) {
                auto framerate = MatlabParamParser::parse<int>(inv[4]);
                thiz.enable_stream(stream_type, stream_index, format, framerate);
            }
        });
        config_factory.record("enable_all_streams", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::config>(inv[0]);
            thiz.enable_all_streams();
        });
        config_factory.record("enable_device", 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::config>(inv[0]);
            auto serial = MatlabParamParser::parse<std::string>(inv[1]);
            thiz.enable_device(serial);
        });
        config_factory.record("enable_device_from_file", 0, 2, 3, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::config>(inv[0]);
            auto file_name = MatlabParamParser::parse<std::string>(inv[1]);
            if (inc == 2)
                thiz.enable_device_from_file(file_name);
            else if (inc == 3) {
                auto repeat_playback = MatlabParamParser::parse<bool>(inv[2]);
                thiz.enable_device_from_file(file_name, repeat_playback);
            }
        });
        config_factory.record("enable_record_to_file", 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::config>(inv[0]);
            auto file_name = MatlabParamParser::parse<std::string>(inv[1]);
            thiz.enable_record_to_file(file_name);
        });
        config_factory.record("disable_stream", 0, 2, 3, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::config>(inv[0]);
            auto stream = MatlabParamParser::parse<rs2_stream>(inv[1]);
            if (inc == 2)
                thiz.disable_stream(stream);
            else if (inc == 3) {
                auto index = MatlabParamParser::parse<int>(inv[2]);
                thiz.disable_stream(stream, index);
            }
        });
        config_factory.record("disable_all_streams", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::config>(inv[0]);
            thiz.disable_all_streams();
        });
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
        pipeline_factory.record("start", 1, 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::pipeline>(inv[0]);
            if (inc == 1)
                outv[0] = MatlabParamParser::wrap(thiz.start());
            else if (inc == 2) {
                auto config = MatlabParamParser::parse<rs2::config>(inv[1]);
                outv[0] = MatlabParamParser::wrap(thiz.start(config));
            }
        });
        pipeline_factory.record("start#fq", 1, 2, 3, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::pipeline>(inv[0]);
            if (inc == 2) {
                auto fq = MatlabParamParser::parse<rs2::frame_queue>(inv[1]);
                outv[0] = MatlabParamParser::wrap(thiz.start(fq));
            } else if (inc == 3) {
                auto config = MatlabParamParser::parse<rs2::config>(inv[1]);
                auto fq = MatlabParamParser::parse<rs2::frame_queue>(inv[2]);
                outv[0] = MatlabParamParser::wrap(thiz.start(config, fq));
            }
        });
        pipeline_factory.record("stop", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
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
        pipeline_factory.record("poll_for_frames", 2, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::pipeline>(inv[0]);
            rs2::frameset fs;
            outv[0] = MatlabParamParser::wrap(thiz.poll_for_frames(&fs));
            outv[1] = MatlabParamParser::wrap(std::move(fs));
        });
        pipeline_factory.record("get_active_profile", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            auto thiz = MatlabParamParser::parse<rs2::pipeline>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_active_profile());
        });
        factory->record(pipeline_factory);
    }

    // rs.hpp
    {
        ClassFactory free_funcs_factory("rs2");
        free_funcs_factory.record("log_to_console", 0, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[]) {
            auto min_severity = MatlabParamParser::parse<rs2_log_severity>(inv[0]);
            rs2::log_to_console(min_severity);
        });
        free_funcs_factory.record("log_to_file", 0, 1, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[]) {
            auto min_severity = MatlabParamParser::parse<rs2_log_severity>(inv[0]);
            if (inc == 1)
                rs2::log_to_file(min_severity);
            else if (inc == 2) {
                auto file_path = MatlabParamParser::parse<const char *>(inv[1]);
                rs2::log_to_file(min_severity, file_path);
            }
        });
        free_funcs_factory.record("log", 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[]) {
            auto severity = MatlabParamParser::parse<rs2_log_severity>(inv[0]);
            auto message = MatlabParamParser::parse<const char *>(inv[1]);
            rs2::log(severity, message);
        });
        factory->record(free_funcs_factory);
    }

    // rs_advanced_mode.hpp
    {
        ClassFactory advanced_mode_factory("rs400::advanced_mode");
        advanced_mode_factory.record("is_enabled", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[]) {
            auto thiz = MatlabParamParser::parse<rs400::advanced_mode>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.is_enabled());
        });
        advanced_mode_factory.record("toggle_advanced_mode", 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[]) {
            auto thiz = MatlabParamParser::parse<rs400::advanced_mode>(inv[0]);
            auto enable = MatlabParamParser::parse<bool>(inv[1]);
            thiz.toggle_advanced_mode(enable);
        });
        advanced_mode_factory.record("serialize_json", 1, 1, [](int outc, mxArray* outv[], int inc, const mxArray* inv[]) {
            auto thiz = MatlabParamParser::parse<rs400::advanced_mode>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.serialize_json());
        });
        advanced_mode_factory.record("load_json", 0, 2, [](int outc, mxArray* outv[], int inc, const mxArray* inv[]) {
            auto thiz = MatlabParamParser::parse<rs400::advanced_mode>(inv[0]);
            auto json_content = MatlabParamParser::parse<std::string>(inv[1]);
            thiz.load_json(json_content);
        });
        factory->record(advanced_mode_factory);
    }

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
    } catch (...) {
        mexErrMsgTxt("An unknown error occured");
    }
}