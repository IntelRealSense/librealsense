// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "core/streaming.h"
#include "context.h"

namespace librealsense
{
    class stream : public stream_interface
    {
    public:
        explicit stream(std::shared_ptr<context> ctx);

        context& get_context() const override;

        int get_stream_index() const override;
        void set_stream_index(int index) override;

        rs2_stream get_stream_type() const override;
        void set_stream_type(rs2_stream stream) override;

    private:
        std::shared_ptr<context> _ctx;
        int _index = 0;
        rs2_stream _type = RS2_STREAM_ANY;
    };

    class stream_profile1 : public stream_profile_interface
    {
    public:
        explicit stream_profile1(std::shared_ptr<context> ctx);

        context& get_context() const override;

        int get_stream_index() const override;
        void set_stream_index(int index) override;

        rs2_stream get_stream_type() const override;
        void set_stream_type(rs2_stream stream) override;

        rs2_format get_format() const override;
        void set_format(rs2_format format) override;

        int get_framerate() const override;

        bool is_recommended() const override;
        void make_recommended() override;

        size_t get_size() const override;

        std::shared_ptr<stream_profile_interface> clone() const override;

        rs2_stream_profile* get_c_wrapper() const override;

        void set_c_wrapper(rs2_stream_profile* wrapper) override;

    private:
        std::shared_ptr<context> _ctx;
        int _index = 0;
        rs2_stream _type = RS2_STREAM_ANY;
        rs2_format _format = RS2_FORMAT_ANY;
        int _framerate = 0;
        bool _is_recommended = false;
        rs2_stream_profile _c_wrapper;
        rs2_stream_profile* _c_ptr = nullptr;
    };
}
