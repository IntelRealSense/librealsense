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
    // TODO:
    // 1.a rs.h
    // 1.a.1 rs2_intrinsics (?)
    // 1.c rs_device.h
    // 1.c.1 rs2_motion_device_intrinsic (?)
    // 1.d rs_frame.h
    // 1.d.1 rs2_vertex (?)
    // 1.d.2 rs2_pixel (?)
    // 1.h rs_sensor.h
    // 1.h.1 rs2_extrinsics
    // 2. rs_types.hpp

    // rs_context.hpp
    // TODO:
    // 1. event_information (?)
    // 2. devices_changed_callback (?)
    {
        ClassFactory context_factory;
        // This lambda feels like it should be possible to generate automatically with templates
        context_factory.record("new", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::context::context: Wrong number of outputs"); }
            if (inc != 0) { mexErrMsgTxt("rs2::context::context: Wrong number of inputs"); }

            outv[0] = MatlabParamParser::wrap(rs2::context());
        });
        context_factory.record("delete", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 0) { mexErrMsgTxt("rs2::context::~context: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::context::~context: Wrong number of inputs"); }

            MatlabParamParser::destroy<rs2::context>(inv[0]);
        });
        context_factory.record("query_devices", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::context::query_devices: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::context::query_devices: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::context>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.query_devices());
        });
        context_factory.record("query_all_sensors", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::context::query_all_sensors: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::context::query_all_sensors: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::context>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.query_all_sensors());
        });
        context_factory.record("get_sensor_parent", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::context::get_sensor_parent: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::context::get_sensor_parent: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::context>(inv[0]);
            auto sensor = MatlabParamParser::parse<rs2::sensor>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.get_sensor_parent(sensor));
        });
        // TODO:
        // 1. set_devices_changed_callback
        context_factory.record("load_device", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::context::load_device: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::context::load_device: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::context>(inv[0]);
            auto file = MatlabParamParser::parse<std::string>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.load_device(file));
        });
        context_factory.record("unload_device", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 0) { mexErrMsgTxt("rs2::context::unload_device: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::context::unload_device: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::context>(inv[0]);
            auto file = MatlabParamParser::parse<std::string>(inv[1]);
            thiz.unload_device(file);
        });
        factory->record("rs2::context", context_factory);
    }
    {
        ClassFactory device_hub_factory;
        device_hub_factory.record("new", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::device_hub::device_hub: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::device_hub::device_hub: Wrong number of inputs"); }

            auto ctx = MatlabParamParser::parse<rs2::context>(inv[0]);
            outv[0] = MatlabParamParser::wrap(rs2::device_hub(ctx));
        });
        device_hub_factory.record("delete", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::device_hub::~device_hub: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::device_hub::~device_hub: Wrong number of inputs"); }

            MatlabParamParser::destroy<rs2::device_hub>(inv[0]);
        });
        device_hub_factory.record("wait_for_device", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::device_hub::wait_for_device: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::device_hub::wait_for_device: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::device_hub>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.wait_for_device());
        });
        device_hub_factory.record("is_connected", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::device_hub::is_connected: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::device_hub::is_connected: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::device_hub>(inv[0]);
            auto dev = MatlabParamParser::parse<rs2::device>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.is_connected(dev));
        });
        factory->record("rs2::device_hub", device_hub_factory);
    }

    // rs_device.hpp
    {
        ClassFactory device_factory;
        // extra helper function for constructing device from device_list
        device_factory.record("init", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::device::device: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::device::device: Wrong number of inputs"); }

            auto list = MatlabParamParser::parse<rs2::device_list>(inv[0]);
            auto idx = MatlabParamParser::parse<uint64_t>(inv[1]);
            outv[0] = MatlabParamParser::wrap(list[idx]);
            MatlabParamParser::destroy<rs2::device_list>(inv[0]);
        });
        // deconstructor in case device was never initialized
        device_factory.record("delete_uninit", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 0) { mexErrMsgTxt("rs2::device::~device: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::device::~device: Wrong number of inputs"); }

            MatlabParamParser::destroy<rs2::device_list>(inv[0]);
        });
        // deconstructor in case device was initialized
        device_factory.record("delete_init", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 0) { mexErrMsgTxt("rs2::device::~device: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::device::~device: Wrong number of inputs"); }

            MatlabParamParser::destroy<rs2::device>(inv[0]);
        });
        device_factory.record("query_sensors", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::device::query_sensors: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::device::query_sensors: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.query_sensors());
        });
        device_factory.record("first", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: better, more maintainable implementation?
            if (outc != 1) { mexErrMsgTxt("rs2::device::first: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::device::first: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            auto type = MatlabParamParser::parse<std::string>(inv[1]);
            try {
                if (type == "sensor")
                    outv[0] = MatlabParamParser::wrap(thiz.first<rs2::sensor>());
                else if (type == "roi_sensor")
                    outv[0] = MatlabParamParser::wrap(thiz.first<rs2::roi_sensor>());
                else if (type == "depth_sensor")
                    outv[0] = MatlabParamParser::wrap(thiz.first<rs2::depth_sensor>());
                else mexErrMsgTxt("rs2::device::first: Could not find requested sensor type!");
            }
            catch (rs2::error) {
                mexErrMsgTxt("rs2::device::first: Could not find requested sensor type!");
            }
        });
        device_factory.record("supports", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::device::supports: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::device::supports: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            auto info = MatlabParamParser::parse<rs2_camera_info>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.supports(info));
        });
        device_factory.record("get_info", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::device::get_info: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::device::get_info: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            auto info = MatlabParamParser::parse<rs2_camera_info>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.get_info(info));
        });
        device_factory.record("hardware_reset", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 0) { mexErrMsgTxt("rs2::device::hardware_reset: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::device::hardware_reset: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            thiz.hardware_reset();
        });
        // TODO:
        // 1. operator= (?)
        // 2. default constructor (?)
        // 3. operator bool (?)
        // WONTDO:
        // 1. get - no meaning in matlab
        device_factory.record("is", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::device::is: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::device::is: Wrong number of inputs"); }

            // TODO: should this take a string instead of a rs2_extension? maybe both options?
            // TODO: something more maintainable?
            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            auto type = MatlabParamParser::parse<std::string>(inv[1]);
            try {
                if (type == "device")
                    outv[0] = MatlabParamParser::wrap(thiz.is<rs2::device>());
                else if (type == "debug_protocol") {
                    mexWarnMsgTxt("rs2::device::is: Debug Protocol not supported in MATLAB");
                    outv[0] = MatlabParamParser::wrap(thiz.is<rs2::debug_protocol>());
                } else if (type == "advanced_mode") {
                    mexWarnMsgTxt("rs2::device::is: Advanced Mode not supported in MATLAB");
                    outv[0] = MatlabParamParser::wrap(false);
                } else if (type == "recorder")
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
/*        device_factory.record("as", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: Implement in matlab with help of is function?
            if (outc != 1) { mexErrMsgTxt("rs2::device::as: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::device::as: Wrong number of inputs"); }

            // TODO: take string instead of rs2_extension
            auto thiz = MatlabParamParser::parse<rs2::device>(inv[0]);
            auto type = MatlabParamParser::parse<rs2_extension>(inv[1]);
            switch (type) {
            case RS2_EXTENSION_DEBUG: mexErrMsgTxt("rs2::device::as: Debug Protocol not supported in MATLAB"); break;
                //outv[0] = MatlabParamParser::wrap(thiz.as<rs2::debug_protocol>()); break;
            // TODO: the rest
            case RS2_EXTENSION_ADVANCED_MODE: mexErrMsgTxt("rs2::device::as: Advanced Mode not supported in MATLAB"); break;
            case RS2_EXTENSION_RECORD: mexErrMsgTxt("rs2::device::as: Record not supported in MATLAB"); break;
                //outv[0] = MatlabParamParser::wrap(thiz.as<rs2::recorder>()); break;
            case RS2_EXTENSION_PLAYBACK: mexErrMsgTxt("rs2::device::as: Playback not supported in MATLAB"); break;
                //outv[0] = MatlabParamParser::wrap(thiz.as<rs2::playback>()); break;
                // TODO: Should I return a nullptr?
            default: mexErrMsgTxt("rs2::device::as: Extension not supported.");
            }
        });*/
        factory->record("rs2::device", device_factory);
    }
    // TODO:
    // 1. debug_protocol (?)
    // WONTDO:
    // 1. device_list - emulated by parsing/wrapping layer

    // rs_frame.hpp
    {
        ClassFactory stream_profile_factory;
        stream_profile_factory.record("new", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::stream_profile::stream_profile: Wrong number of outputs"); }
            if (inc != 0) { mexErrMsgTxt("rs2::stream_profile::stream_profile: Wrong number of inputs"); }

            outv[0] = MatlabParamParser::wrap(rs2::stream_profile());
        });
        stream_profile_factory.record("delete", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
        if (outc != 0) { mexErrMsgTxt("rs2::stream_profile::~stream_profile: Wrong number of outputs"); }
        if (inc != 1) { mexErrMsgTxt("rs2::stream_profile::~stream_profile: Wrong number of inputs"); }

        MatlabParamParser::destroy<rs2::stream_profile>(inv[0]);
        });
        stream_profile_factory.record("stream_index", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::stream_profile::stream_index: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::stream_profile::stream_index: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.stream_index());
        });
        stream_profile_factory.record("stream_type", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::stream_profile::stream_type: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::stream_profile::stream_type: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.stream_type());
        });
        stream_profile_factory.record("format", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::stream_profile::format: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::stream_profile::format: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.format());
        });
        stream_profile_factory.record("fps", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::stream_profile::fps: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::stream_profile::fps: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.fps());
        });
        stream_profile_factory.record("unique_id", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::stream_profile::unique_id: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::stream_profile::unique_id: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.unique_id());
        });
        stream_profile_factory.record("clone", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::stream_profile::clone: Wrong number of outputs"); }
            if (inc != 4) { mexErrMsgTxt("rs2::stream_profile::clone: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            auto type = MatlabParamParser::parse<rs2_stream>(inv[1]);
            auto index = MatlabParamParser::parse<int>(inv[2]);
            auto format = MatlabParamParser::parse<rs2_format>(inv[3]);
            outv[0] = MatlabParamParser::wrap(thiz.clone(type, index, format));
        });
        // TODO:
        // 1.a.1 is
        // 1.a.2 as
        stream_profile_factory.record("stream_name", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::stream_profile::stream_name: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::stream_profile::stream_name: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.stream_name());
        });
        stream_profile_factory.record("is_default", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::stream_profile::is_default: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::stream_profile::is_default: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.is_default());
        });
        // TODO:
        // 1.a.3 operator bool (?)
        // WONTDO:
        // 1.a.4 get - no meaning in matlab
/*        stream_profile_factory.record("get_extrinsics_to", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::stream_profile::get_extrinsics_to: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::stream_profile::get_extrinsics_to: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::stream_profile>(inv[0]);
            auto to = MatlabParamParser::parse<rs2::stream_profile>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.get_extrinsics_to(to));
        });*/
        factory->record("rs2::stream_profile", stream_profile_factory);
    }
    // TODO:
    // 1.b video_stream_profile
    {
        ClassFactory frame_factory;
        // TODO:
        // 1.c.1  new (?)
        frame_factory.record("delete", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 0) { mexErrMsgTxt("rs2::frame::~frame: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::frame::~frame: Wrong number of inputs"); }

            MatlabParamParser::destroy<rs2::frame>(inv[0]);
        });
        // 1.c.3  operator= (how?)
        // 1.c.4  copy constructor (?)
        // 1.c.5  swap (?)
        // 1.c.6  operator bool (?)
        frame_factory.record("get_timestamp", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::frame::get_timestamp: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::frame::get_timestamp: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_timestamp());
        });
        frame_factory.record("get_frame_timestamp_domain", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::frame::get_frame_timestamp_domain: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::frame::get_frame_timestamp_domain: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_frame_timestamp_domain());
        });
        frame_factory.record("get_frame_metadata", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::frame::get_frame_metadata: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::frame::get_frame_metadata: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            auto frame_metadata = MatlabParamParser::parse<rs2_frame_metadata_value>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.get_frame_metadata(frame_metadata));
        });
        frame_factory.record("supports_frame_metadata", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::frame::supports_frame_metadata: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::frame::supports_frame_metadata: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            auto frame_metadata = MatlabParamParser::parse<rs2_frame_metadata_value>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.supports_frame_metadata(frame_metadata));
        });
        frame_factory.record("get_frame_number", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::frame::get_frame_number: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::frame::get_frame_number: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_frame_number());
        });
        frame_factory.record("get_data", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::frame::get_data: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::frame::get_data: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            if (auto vf = thiz.as<rs2::video_frame>()) {
                /*switch (vf.get_profile().format()) {
                case RS2_FORMAT_RGB8: case RS2_FORMAT_BRG2:
                    outv[0] = MatlabParamParser::wrap_array<uint8_t>(vf.get_data(), )
                }*/
                size_t dims[] = { static_cast<size_t>(vf.get_height() * vf.get_stride_in_bytes()) };
                outv[0] = MatlabParamParser::wrap_array<uint8_t>(reinterpret_cast<const uint8_t*>(vf.get_data()), /*1,*/ dims);
            } else {
                uint8_t byte = *reinterpret_cast<const uint8_t*>(thiz.get_data());
                outv[0] = MatlabParamParser::wrap(std::move(byte));
                mexWarnMsgTxt("Can't detect frame dims, sending only first byte");
            }
        });
        frame_factory.record("get_profile", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::frame::get_profile: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::frame::get_profile: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_frame_number());
        });
        // 1.c.14 is
        // 1.c.15 as
        factory->record("rs2::frame", frame_factory);
    }
    {
        ClassFactory video_frame_factory;
        // 1.d.1 new (?)
        video_frame_factory.record("delete", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 0) { mexErrMsgTxt("rs2::video_frame::~video_frame: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::video_frame::~video_frame: Wrong number of inputs"); }

            MatlabParamParser::destroy<rs2::video_frame>(inv[0]);
        });
        video_frame_factory.record("get_width", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::video_frame::get_width: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::video_frame::get_width: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::video_frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_width());
        });
        video_frame_factory.record("get_height", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::video_frame::get_height: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::video_frame::get_height: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::video_frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_height());
        });
        video_frame_factory.record("get_stride_in_bytes", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::video_frame::get_stride_in_bytes: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::video_frame::get_stride_in_bytes: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::video_frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_stride_in_bytes());
        });
        video_frame_factory.record("get_bits_per_pixel", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::video_frame::get_bits_per_pixel: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::video_frame::get_bits_per_pixel: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::video_frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_bits_per_pixel());
        });
        video_frame_factory.record("get_bytes_per_pixel", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::video_frame::get_bytes_per_pixel: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::video_frame::get_bytes_per_pixel: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::video_frame>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.get_bytes_per_pixel());
        });
        factory->record("rs2::video_frame", video_frame_factory);
    }
    // 1.e vertex
    // 1.f texture_coordinate
    // 1.g points
    {
        ClassFactory depth_frame_factory;
        // 1.h.1 new (?)
        depth_frame_factory.record("delete", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 0) { mexErrMsgTxt("rs2::depth_frame::~depth_frame: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::depth_frame::~depth_frame: Wrong number of inputs"); }

            MatlabParamParser::destroy<rs2::depth_frame>(inv[0]);
        });
        depth_frame_factory.record("get_distance", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::depth_frame::get_distance: Wrong number of outputs"); }
            if (inc != 3) { mexErrMsgTxt("rs2::depth_frame::get_distance: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::depth_frame>(inv[0]);
            auto x = MatlabParamParser::parse<int>(inv[1]);
            auto y = MatlabParamParser::parse<int>(inv[2]);
            outv[0] = MatlabParamParser::wrap(thiz.get_distance(x, y));
        });
        factory->record("rs2::depth_frame", depth_frame_factory);
    }
    // 1.i disparity_frame
    {
        ClassFactory frameset_factory;
        // TODO:
        // 1.j.1 new (?)
        frameset_factory.record("delete", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 0) { mexErrMsgTxt("rs2::frameset::~frameset: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::frameset::~frameset: Wrong number of inputs"); }

            MatlabParamParser::destroy<rs2::frameset>(inv[0]);
        });
        frameset_factory.record("first_or_default", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::frameset::first_or_default: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::frameset::first_or_default: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::frameset>(inv[0]);
            auto s = MatlabParamParser::parse<rs2_stream>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.first_or_default(s));
        });
        frameset_factory.record("first", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::frameset::first: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::frameset::first: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::frameset>(inv[0]);
            auto s = MatlabParamParser::parse<rs2_stream>(inv[1]);
            try {
                outv[0] = MatlabParamParser::wrap(thiz.first(s));
            } catch (std::exception &e) {
                mexErrMsgTxt(e.what());
            }
        });
        frameset_factory.record("get_depth_frame", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::frameset::get_depth_frame: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::frameset::get_depth_frame: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::frameset>(inv[0]);
            try {
                outv[0] = MatlabParamParser::wrap(thiz.get_depth_frame());
            } catch (std::exception &e) {
                mexErrMsgTxt(e.what());
            }
        });
        frameset_factory.record("get_color_frame", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::frameset::get_color_frame: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::frameset::get_color_frame: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::frameset>(inv[0]);
            try {
                outv[0] = MatlabParamParser::wrap(thiz.get_color_frame());
            }
            catch (std::exception &e) {
                mexErrMsgTxt(e.what());
            }
        });
        frameset_factory.record("size", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::frameset::size: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::frameset::size: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::frameset>(inv[0]);
            outv[0] = MatlabParamParser::wrap(thiz.size());
        });
        // 1.j.8 foreach (?) (req: callbacks)
        // 1.j.9 operator[] (how?)
        // 1.j.10 iterator (?)
        // 1.j.11 begin (?) (req: iterator)
        // 1.j.12 end (?) (req: iterator)
        factory->record("rs2::frameset", frameset_factory);
    }

    // 2. rs_processing.hpp
    // 3. rs_record_playback.hpp

    // rs_sensor.hpp
    // TODO:
    // 1. notification (?)
    // 2. notifications_callback (?)
    // 3. frame_callback (?)
    // 4. options (?)
    /*{
        ClassFactory options_factory;
        options_factory.record("supports", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::options::supports: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::options::supports: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::options>(inv[0]);
            auto option = MatlabParamParser::parse<rs2_option>(inv[1]);

            outv[0] = MatlabParamParser::wrap(thiz.supports(option));
        });
        options_factory.record("get_option_description", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::options::get_option_description: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::options::get_option_description: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::options>(inv[0]);
            auto option = MatlabParamParser::parse<rs2_option>(inv[1]);

            outv[0] = MatlabParamParser::wrap(thiz.get_option_description(option));
        });
        factory->record("rs2::options", options_factory);
    }*/
    {
        ClassFactory sensor_factory;
        sensor_factory.record("delete", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 0) { mexErrMsgTxt("rs2::sensor::~sensor: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::sensor::~sensor: Wrong number of inputs"); }

            MatlabParamParser::destroy<rs2::sensor>(inv[0]);
        });
        // TODO:
        //  1. supports [rs2_option] (from options)
        //  2. get_option_description (from options)
        //  3. get_option_value_description (from options)
        //  4. get_option (from options)
        //  5. get_option_range (from options)
        //  6. set_option (from options)
        //  7. is_option_read_only (from options)
        //  8. operator= (from options) (?)
        //  9. open [stream_profile]
        // 10. supports [rs2_camera_info]
        // 11. get_info
        // 12. open [vector<stream_profile>]
        // 13. close
        // 14. start
        // 15. stop
        // 16. set_notifications_callback (?)
        // 17. get_stream_profiles
        // 18. get_motion_intrinsics
        // 19. operator= (?)
        // 20. operator bool (?)
        // 21. get (?)
        // 22. is (?)
        // 23. as (?)
        // 24. operator==
        factory->record("rs2::sensor", sensor_factory);
    }
    // TODO:
    // 1. roi_sensor (?)
    // 2. depth_sensor (?)

    // rs_pipeline.hpp
    // TODO:
    // 1. pipeline_profile
    // 2. config
    {
        ClassFactory pipeline_factory;
        // TODO:
        pipeline_factory.record("new", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 1) { mexErrMsgTxt("rs2::pipeline::pipeline: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::pipeline::pipeline: Wrong number of inputs"); }

            auto ctx = MatlabParamParser::parse<rs2::context>(inv[0]);
            outv[0] = MatlabParamParser::wrap(rs2::pipeline(ctx));
        });
        pipeline_factory.record("delete", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            if (outc != 0) { mexErrMsgTxt("rs2::pipeline::~pipeline: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::pipeline::~pipeline: Wrong number of inputs"); }

            MatlabParamParser::destroy<rs2::pipeline>(inv[0]);
        });
        pipeline_factory.record("start:void", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: add output parameter after binding pipeline_profile
            if (outc != 0) { mexErrMsgTxt("rs2::pipeline::start: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::pipeline::start: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::pipeline>(inv[0]);
            thiz.start();
        });
//        pipeline_factory.record("start:config", ...)
        pipeline_factory.record("stop", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: add output parameter after binding pipeline_profile
            if (outc != 0) { mexErrMsgTxt("rs2::pipeline::stop: Wrong number of outputs"); }
            if (inc != 1) { mexErrMsgTxt("rs2::pipeline::stop: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::pipeline>(inv[0]);
            thiz.stop();
        });
        pipeline_factory.record("wait_for_frames", [](int outc, mxArray* outv[], int inc, const mxArray* inv[])
        {
            // TODO: add output parameter after binding pipeline_profile
            if (outc != 1) { mexErrMsgTxt("rs2::pipeline::wait_for_frames: Wrong number of outputs"); }
            if (inc != 2) { mexErrMsgTxt("rs2::pipeline::wait_for_frames: Wrong number of inputs"); }

            auto thiz = MatlabParamParser::parse<rs2::pipeline>(inv[0]);
            auto timeout_ms = MatlabParamParser::parse<unsigned int>(inv[1]);
            outv[0] = MatlabParamParser::wrap(thiz.wait_for_frames(timeout_ms));
        });
        // 3.g poll_for_frames (req: frameset) (how?)
        // 3.h get_active_profile (req: pipeline_profile)
        factory->record("rs2::pipeline", pipeline_factory);
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

    auto func = factory->get(MatlabParamParser::parse<std::string>(inParams[0]), MatlabParamParser::parse<std::string>(inParams[1]));
    if (!func) {
        mexErrMsgTxt("Unknown Command received.");
        return;
    }

    func(nOutParams, outParams, nInParams-2, inParams+2); // "eat" the two function specifiers
}