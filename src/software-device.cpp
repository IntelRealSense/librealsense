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
        _metadata_parsers = md_constant_parser::create_metadata_parser_map();
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
            return profile->get_unique_id() == video_stream.uid;
        } ) != _profiles.end());

        if (exist)
        {
            LOG_WARNING("Video stream unique ID already exist!");
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

    std::shared_ptr<stream_profile_interface> software_sensor::add_motion_stream(rs2_motion_stream motion_stream)
    {
        auto exist = (std::find_if(_profiles.begin(), _profiles.end(), [&](std::shared_ptr<stream_profile_interface> profile)
        {
            return profile->get_unique_id() == motion_stream.uid;
        }) != _profiles.end());

        if (exist)
        {
            LOG_WARNING("Motion stream unique ID already exist!");
            throw rs2::error("Stream unique ID already exist!");
        }

        auto profile = std::make_shared<motion_stream_profile>(
            platform::stream_profile{ 0, 0, (uint32_t)motion_stream.fps, 0 });
        profile->set_format(motion_stream.fmt);
        profile->set_framerate(motion_stream.fps);
        profile->set_stream_index(motion_stream.index);
        profile->set_stream_type(motion_stream.type);
        profile->set_unique_id(motion_stream.uid);
        profile->set_intrinsics([=]() {return motion_stream.intrinsics; });
        _profiles.push_back(profile);

        return profile;
    }

    std::shared_ptr<stream_profile_interface> software_sensor::add_pose_stream(rs2_pose_stream pose_stream)
    {
        auto exist = (std::find_if(_profiles.begin(), _profiles.end(), [&](std::shared_ptr<stream_profile_interface> profile)
        {
            return profile->get_unique_id() == pose_stream.uid;
        }) != _profiles.end());

        if (exist)
        {
            LOG_WARNING("Pose stream unique ID already exist!");
            throw rs2::error("Stream unique ID already exist!");
        }

        auto profile = std::make_shared<pose_stream_profile>(
            platform::stream_profile{ 0, 0, (uint32_t)pose_stream.fps, 0 });
        profile->set_format(pose_stream.fmt);
        profile->set_framerate(pose_stream.fps);
        profile->set_stream_index(pose_stream.index);
        profile->set_stream_type(pose_stream.type);
        profile->set_unique_id(pose_stream.uid);
        _profiles.push_back(profile);

        return profile;
    }

    stream_profiles software_sensor::init_stream_profiles()
    {
        return _profiles;
    }

    void software_sensor::open(const stream_profiles& requests)
    {
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("open(...) failed. Software device is streaming!");
        else if (_is_opened)
            throw wrong_api_call_sequence_exception("open(...) failed. Software device is already opened!");
        _is_opened = true;
        set_active_streams(requests);
    }

    void software_sensor::close()
    {
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("close() failed. Software device is streaming!");
        else if (!_is_opened)
            throw wrong_api_call_sequence_exception("close() failed. Software device was not opened!");
        _is_opened = false;
        set_active_streams({});
    }

    void software_sensor::start(frame_callback_ptr callback)
    {
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("start_streaming(...) failed. Software device is already streaming!");
        else if (!_is_opened)
            throw wrong_api_call_sequence_exception("start_streaming(...) failed. Software device was not opened!");
        _source.get_published_size_option()->set(0);
        _source.init(_metadata_parsers);
        _source.set_sensor(this->shared_from_this());
        _source.set_callback(callback);
        _is_streaming = true;
        raise_on_before_streaming_changes(true);
    }

    void software_sensor::stop()
    {
        if (!_is_streaming)
            throw wrong_api_call_sequence_exception("stop_streaming() failed. Software device is not streaming!");

        _is_streaming = false;
        raise_on_before_streaming_changes(false);
        _source.flush();
        _source.reset();
    }


    void software_sensor::set_metadata(rs2_frame_metadata_value key, rs2_metadata_type value)
    {
        _metadata_map[key] = value;
    }

    void software_sensor::on_video_frame(rs2_software_video_frame software_frame)
    {
        frame_additional_data data;
        data.timestamp = software_frame.timestamp;
        data.timestamp_domain = software_frame.domain;
        data.frame_number = software_frame.frame_number;

        data.metadata_size = 0;
        for (auto i : _metadata_map)
        {
            auto size_of_enum = sizeof(rs2_frame_metadata_value);
            auto size_of_data = sizeof(rs2_metadata_type);
            if (data.metadata_size + size_of_enum + size_of_data > 255)
            {
                continue; //stop adding metadata to frame
            }
            memcpy(data.metadata_blob.data() + data.metadata_size, &i.first, size_of_enum);
            data.metadata_size += static_cast<uint32_t>(size_of_enum);
            memcpy(data.metadata_blob.data() + data.metadata_size, &i.second, size_of_data);
            data.metadata_size += static_cast<uint32_t>(size_of_data);
        }

        rs2_extension extension = software_frame.profile->profile->get_stream_type() == RS2_STREAM_DEPTH ?
            RS2_EXTENSION_DEPTH_FRAME : RS2_EXTENSION_VIDEO_FRAME;

        auto frame = _source.alloc_frame(extension, 0, data, false);
        if (!frame)
        {
            LOG_WARNING("Dropped video frame. alloc_frame(...) returned nullptr");
            return;
        }
        auto vid_profile = dynamic_cast<video_stream_profile_interface*>(software_frame.profile->profile);
        auto vid_frame = dynamic_cast<video_frame*>(frame);
        vid_frame->assign(vid_profile->get_width(), vid_profile->get_height(), software_frame.stride, software_frame.bpp * 8);

        frame->set_stream(std::dynamic_pointer_cast<stream_profile_interface>(software_frame.profile->profile->shared_from_this()));
        frame->attach_continuation(frame_continuation{ [=]() {
            software_frame.deleter(software_frame.pixels);
        }, software_frame.pixels });
        _source.invoke_callback(frame);
    }

    void software_sensor::on_motion_frame(rs2_software_motion_frame software_frame)
    {
        frame_additional_data data;
        data.timestamp = software_frame.timestamp;
        data.timestamp_domain = software_frame.domain;
        data.frame_number = software_frame.frame_number;

        data.metadata_size = 0;
        for (auto i : _metadata_map)
        {
            auto size_of_enum = sizeof(rs2_frame_metadata_value);
            auto size_of_data = sizeof(rs2_metadata_type);
            memcpy(data.metadata_blob.data() + data.metadata_size, &i.first, size_of_enum);
            data.metadata_size += static_cast<uint32_t>(size_of_enum);
            memcpy(data.metadata_blob.data() + data.metadata_size, &i.second, size_of_data);
            data.metadata_size += static_cast<uint32_t>(size_of_data);
        }

        auto frame = _source.alloc_frame(RS2_EXTENSION_MOTION_FRAME, 0, data, false);
        if (!frame)
        {
            LOG_WARNING("Dropped motion frame. alloc_frame(...) returned nullptr");
            return;
        }
        frame->set_stream(std::dynamic_pointer_cast<stream_profile_interface>(software_frame.profile->profile->shared_from_this()));
        frame->attach_continuation(frame_continuation{ [=]() {
            software_frame.deleter(software_frame.data);
        }, software_frame.data });
        _source.invoke_callback(frame);
    }

    void software_sensor::on_pose_frame(rs2_software_pose_frame software_frame)
    {
        frame_additional_data data;
        data.timestamp = software_frame.timestamp;
        data.timestamp_domain = software_frame.domain;
        data.frame_number = software_frame.frame_number;

        data.metadata_size = 0;
        for (auto i : _metadata_map)
        {
            auto size_of_enum = sizeof(rs2_frame_metadata_value);
            auto size_of_data = sizeof(rs2_metadata_type);
            memcpy(data.metadata_blob.data() + data.metadata_size, &i.first, size_of_enum);
            data.metadata_size += static_cast<uint32_t>(size_of_enum);
            memcpy(data.metadata_blob.data() + data.metadata_size, &i.second, size_of_data);
            data.metadata_size += static_cast<uint32_t>(size_of_data);
        }

        auto frame = _source.alloc_frame(RS2_EXTENSION_POSE_FRAME, 0, data, false);
        if (!frame)
        {
            LOG_WARNING("Dropped pose frame. alloc_frame(...) returned nullptr");
            return;
        }
        frame->set_stream(std::dynamic_pointer_cast<stream_profile_interface>(software_frame.profile->profile->shared_from_this()));
        frame->attach_continuation(frame_continuation{ [=]() {
            software_frame.deleter(software_frame.data);
        }, software_frame.data });
        _source.invoke_callback(frame);
    }

    void software_sensor::add_read_only_option(rs2_option option, float val)
    {
        register_option(option, std::make_shared<const_value_option>("bypass sensor read only option",
            lazy<float>([=]() { return val; })));
    }

    void software_sensor::update_read_only_option(rs2_option option, float val)
    {
        get_option(option).set(val);
    }
}

