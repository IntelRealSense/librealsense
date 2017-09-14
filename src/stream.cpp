// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "stream.h"

namespace librealsense
{
    stream::stream(rs2_stream stream_type, int index)
        : _index(index), _type(stream_type)
    {
        _uid = environment::get_instance().generate_stream_id();
    }

    int stream::get_stream_index() const
    {
        return _index;
    }

    void stream::set_stream_index(int index)
    {
        _index = index;
    }

    rs2_stream stream::get_stream_type() const
    {
        return _type;
    }

    void stream::set_stream_type(rs2_stream stream)
    {
        _type = stream;
    }

    stream_profile_base::stream_profile_base(platform::stream_profile sp)
        : backend_stream_profile(std::move(sp))
    {
        _c_ptr = &_c_wrapper;
        _c_wrapper.profile = this;
        _c_wrapper.clone = nullptr;
    }

    int stream_profile_base::get_stream_index() const
    {
        return _index;
    }

    void stream_profile_base::set_stream_index(int index)
    {
        _index = index;
    }

    rs2_stream stream_profile_base::get_stream_type() const
    {
        return _type;
    }

    void stream_profile_base::set_stream_type(rs2_stream stream)
    {
        _type = stream;
    }

    rs2_format stream_profile_base::get_format() const
    {
        return _format;
    }

    void stream_profile_base::set_format(rs2_format format)
    {
        _format = format;
    }

    uint32_t stream_profile_base::get_framerate() const
    {
        return _framerate;
    }

    void stream_profile_base::set_framerate(uint32_t val)
    {
        _framerate = val;
    }

    bool stream_profile_base::is_default() const
    {
        return _is_default;
    }

    void stream_profile_base::make_default()
    {
        _is_default = true;
    }

    size_t stream_profile_base::get_size() const
    {
        return get_image_bpp(get_format()) * get_framerate();
    }

    std::shared_ptr<stream_profile_interface> stream_profile_base::clone() const
    {
        auto res = std::make_shared<stream_profile_base>(get_backend_profile());
        res->set_unique_id(environment::get_instance().generate_stream_id());
        return res;
    }

    rs2_stream_profile* stream_profile_base::get_c_wrapper() const
    {
        return _c_ptr;
    }

    void stream_profile_base::set_c_wrapper(rs2_stream_profile* wrapper)
    {
        _c_ptr = wrapper;
    }
}

