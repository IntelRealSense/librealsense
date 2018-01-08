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

    bypass_sensor::bypass_sensor(std::string name, bypass_device* owner)
        : sensor_base(name, owner)
    {

    }

    std::shared_ptr<matcher> bypass_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<std::shared_ptr<matcher>> depth_matchers;

        for (auto& s : _bypass_sensors[0]->_profiles)
            depth_matchers.push_back(std::make_shared<identity_matcher>(s->get_unique_id(), s->get_stream_type()));

        return std::make_shared<frame_number_composite_matcher>(depth_matchers);
    }

    void bypass_sensor::add_video_stream(rs2_stream type, int index, int uid, int width, int height, int fps, rs2_format fmt, rs2_intrinsics intrinsics)
    {
        auto profile = std::make_shared<video_stream_profile>(
            platform::stream_profile{ (uint32_t)width, (uint32_t)height, (uint32_t)fps, 0 });
        profile->set_dims(width, height);
        profile->set_format(fmt);
        profile->set_framerate(fps);
        profile->set_stream_index(index);
        profile->set_stream_type(type);
        profile->set_unique_id(uid);
        profile->set_intrinsics([=]() {return intrinsics; });
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

    void bypass_sensor::on_video_frame(void * pixels, 
        void(*deleter)(void*),
        int stride, int bpp,
        rs2_time_t ts, rs2_timestamp_domain domain,
        int frame_number,
        stream_profile_interface* profile)
    {
        frame_additional_data data;
        data.timestamp = ts;
        data.timestamp_domain = domain;
        data.frame_number = frame_number;
        auto frame = _source.alloc_frame(RS2_EXTENSION_VIDEO_FRAME, 0, data, false);

        auto vid_profile = dynamic_cast<video_stream_profile_interface*>(profile);
        auto vid_frame = dynamic_cast<video_frame*>(frame);
        vid_frame->assign(vid_profile->get_width(), vid_profile->get_height(), stride, bpp);

        frame->set_stream(std::dynamic_pointer_cast<stream_profile_interface>(profile->shared_from_this()));
        frame->attach_continuation(frame_continuation{ [=]() {
            deleter(pixels);
        }, pixels });
        _source.invoke_callback(frame);
    }
}

