// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "stream.h"

namespace librealsense
{
    stream::stream(std::shared_ptr<context> ctx): _ctx(move(ctx))
    {
        _index = ctx->generate_stream_id();
    }

    context& stream::get_context() const
    {
        return *_ctx;
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

    stream_profile_base::stream_profile_base(std::shared_ptr<context> ctx): _ctx(move(ctx))
    {
        _c_ptr = &_c_wrapper;
        _c_wrapper.profile = this;
        _c_wrapper.clone = nullptr;
    }

    context& stream_profile_base::get_context() const
    {
        return *_ctx;
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

    int stream_profile_base::get_framerate() const
    {
        return _framerate;
    }

    bool stream_profile_base::is_recommended() const
    {
        return _is_recommended;
    }

    void stream_profile_base::make_recommended()
    {
        _is_recommended = true;
    }

    size_t stream_profile_base::get_size() const
    {
        return 0;
    }

    std::shared_ptr<stream_profile_interface> stream_profile_base::clone() const
    {
        return std::make_shared<stream_profile_base>(_ctx);
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

