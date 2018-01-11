// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "bypass-device.h"
#include "stream.h"

namespace librealsense
{
    bypass_device::bypass_device()
        : device(std::make_shared<context>(backend_type::standard), {})
    {
        register_info(RS2_CAMERA_INFO_NAME, "Bypass-Sensor");
    }

    bypass_sensor& bypass_device::add_bypass_sensor(const std::string& name)
    {
        auto sensor = std::make_shared<bypass_sensor>(name, this);
        add_sensor(sensor);
        _bypass_sensors.push_back(sensor);

        return *sensor;
    }

    bypass_sensor& bypass_device::get_bypass_sensor(int index)
    {
        return *_bypass_sensors[index];
    }

    void bypass_device::set_matcher_type(rs2_matchers matcher)
    {
        _matcher = matcher;
    }

    bypass_sensor::bypass_sensor(std::string name, bypass_device* owner)
        : sensor_base(name, owner)
    {

    }

    std::shared_ptr<matcher> bypass_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> profiles;

        for (auto&& s : _bypass_sensors)
            for (auto&& p : s->get_stream_profiles())
                profiles.push_back(p.get());

        return matcher_factory::create(_matcher, profiles);
    }

    void bypass_sensor::add_video_stream(rs2_video_stream video_stream)
    {
        auto profile = std::make_shared<video_stream_profile>(
            platform::stream_profile{ (uint32_t)video_stream.width, (uint32_t)video_stream.height, (uint32_t)video_stream.fps, 0 });
        profile->set_dims(video_stream.width, video_stream.height);
        profile->set_format(video_stream.fmt);
        profile->set_framerate(video_stream.fps);
        profile->set_stream_index(video_stream.index);
        profile->set_stream_type(video_stream.type);
        profile->set_unique_id(video_stream.uid);
        profile->set_intrinsics([=]() {return video_stream.intrinsics; });
        _profiles.push_back(profile);
    }

    stream_profiles bypass_sensor::init_stream_profiles()
    {
        return _profiles;
    }

    void bypass_sensor::open(const stream_profiles& requests)
    {

    }
    void bypass_sensor::close()
    {

    }

    void bypass_sensor::start(frame_callback_ptr callback)
    {
        _source.init(_metadata_parsers);
        _source.set_sensor(this->shared_from_this());
        _source.set_callback(callback);
    }

    void bypass_sensor::stop()
    {
        _source.flush();
        _source.reset();
    }

    void bypass_sensor::on_video_frame(rs2_bypass_video_frame bypass_frame)
    {
        frame_additional_data data;
        data.timestamp = bypass_frame.timestamp;
        data.timestamp_domain = bypass_frame.domain;
        data.frame_number = bypass_frame.frame_number;

        rs2_extension extension = bypass_frame.profile->profile->get_stream_type() == RS2_STREAM_DEPTH? 
            RS2_EXTENSION_DEPTH_FRAME: RS2_EXTENSION_VIDEO_FRAME;
       
       auto frame = _source.alloc_frame(extension, 0, data, false);

        auto vid_profile = dynamic_cast<video_stream_profile_interface*>(bypass_frame.profile->profile);
        auto vid_frame = dynamic_cast<video_frame*>(frame);
        vid_frame->assign(vid_profile->get_width(), vid_profile->get_height(), bypass_frame.stride, bypass_frame.bpp);

        frame->set_stream(std::dynamic_pointer_cast<stream_profile_interface>(bypass_frame.profile->profile->shared_from_this()));
        frame->attach_continuation(frame_continuation{ [=]() {
            bypass_frame.deleter(bypass_frame.pixels);
        }, bypass_frame.pixels });
        _source.invoke_callback(frame);
    }

    void bypass_sensor::add_read_only_option(rs2_option option, float val)
    {
        register_option(RS2_OPTION_DEPTH_UNITS, std::make_shared<const_value_option>("bypass sensor read only option",
            lazy<float>([=]() { return val; })));
    }

    void bypass_sensor::update_read_only_option(rs2_option option, float val)
    {
        get_option(option).set(val);
    }
}

