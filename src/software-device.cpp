// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "software-device.h"
#include "stream.h"

namespace librealsense
{
    software_device::software_device()
        : device(std::make_shared<context>(backend_type::standard), {})
    {
        register_info(RS2_CAMERA_INFO_NAME, "Software-Device");
    }

    software_sensor& software_device::add_software_sensor(const std::string& name)
    {
        auto sensor = std::make_shared<software_sensor>(name, this);
        add_sensor(sensor);
        _software_sensors.push_back(sensor);

        return *sensor;
    }

    software_sensor& software_device::get_software_sensor(int index)
    {
        if (index >= _software_sensors.size())
        {
            throw rs2::error("Requested index is out of range!");
        }
        return *_software_sensors[index];
    }

    void software_device::set_matcher_type(rs2_matchers matcher)
    {
        _matcher = matcher;
    }

    software_sensor::software_sensor(std::string name, software_device* owner)
        : sensor_base(name, owner)
    {

    }

    std::shared_ptr<matcher> software_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> profiles;

        for (auto&& s : _software_sensors)
            for (auto&& p : s->get_stream_profiles())
                profiles.push_back(p.get());

        return matcher_factory::create(_matcher, profiles);
    }

    std::shared_ptr<stream_profile_interface> software_sensor::add_video_stream(rs2_video_stream video_stream)
    {
        auto exist = (std::find_if(_profiles.begin(), _profiles.end(), [&](std::shared_ptr<stream_profile_interface> profile)
        {
            if (profile->get_unique_id() == video_stream.uid)
            {
                return true;
            }
            return false;
        } ) != _profiles.end());

        if (exist)
        {
            LOG_WARNING("Stream unique ID already exist!");
            throw rs2::error("Stream unique ID already exist!");
        }

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

        return profile;
    }

    stream_profiles software_sensor::init_stream_profiles()
    {
        return _profiles;
    }

    void software_sensor::open(const stream_profiles& requests)
    {

    }
    void software_sensor::close()
    {

    }

    void software_sensor::start(frame_callback_ptr callback)
    {
        _source.init(_metadata_parsers);
        _source.set_sensor(this->shared_from_this());
        _source.set_callback(callback);
    }

    void software_sensor::stop()
    {
        _source.flush();
        _source.reset();
    }

    void software_sensor::on_video_frame(rs2_software_video_frame software_frame)
    {
        frame_additional_data data;
        data.timestamp = software_frame.timestamp;
        data.timestamp_domain = software_frame.domain;
        data.frame_number = software_frame.frame_number;

        rs2_extension extension = software_frame.profile->profile->get_stream_type() == RS2_STREAM_DEPTH ?
            RS2_EXTENSION_DEPTH_FRAME : RS2_EXTENSION_VIDEO_FRAME;

        auto frame = _source.alloc_frame(extension, 0, data, false);

        auto vid_profile = dynamic_cast<video_stream_profile_interface*>(software_frame.profile->profile);
        auto vid_frame = dynamic_cast<video_frame*>(frame);
        vid_frame->assign(vid_profile->get_width(), vid_profile->get_height(), software_frame.stride, software_frame.bpp);

        frame->set_stream(std::dynamic_pointer_cast<stream_profile_interface>(software_frame.profile->profile->shared_from_this()));
        frame->attach_continuation(frame_continuation{ [=]() {
            software_frame.deleter(software_frame.pixels);
        }, software_frame.pixels });
        _source.invoke_callback(frame);
    }

    void software_sensor::add_read_only_option(rs2_option option, float val)
    {
        register_option(RS2_OPTION_DEPTH_UNITS, std::make_shared<const_value_option>("bypass sensor read only option",
            lazy<float>([=]() { return val; })));
    }

    void software_sensor::update_read_only_option(rs2_option option, float val)
    {
        get_option(option).set(val);
    }
}

