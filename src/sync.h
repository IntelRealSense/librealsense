// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include "types.h"
#include "archive.h"

#include <stdint.h>
#include <vector>
#include <mutex>
#include <deque>
#include <cmath>
#include <memory>

namespace rsimpl2
{
    typedef std::vector<frame_holder> frameset;

    class sync_interface
    {
    public:
        virtual void dispatch_frame(frame_holder f) = 0;
        virtual frameset wait_for_frames(int timeout_ms = 5000) = 0;
        virtual bool poll_for_frames(frameset& frames) = 0;
        virtual ~sync_interface() = default;
    };

    class syncer : public sync_interface
    {
    public:
        explicit syncer(rs2_stream key_stream = RS2_STREAM_DEPTH)
            : impl(new shared_impl())
        {
            impl->key_stream = key_stream;
        }

        void set_key_stream(rs2_stream stream)
        {
            impl->key_stream = stream;
        }

        void dispatch_frame(frame_holder f) override;

        frameset wait_for_frames(int timeout_ms = 5000) override;

        bool poll_for_frames(frameset& frames) override;
    private:
        struct stream_pipe
        {
            stream_pipe() : queue(4) {}
            frame_holder front;
            single_consumer_queue<frame_holder> queue;

            ~stream_pipe()
            {
            }
        };

        struct shared_impl
        {
            std::recursive_mutex mutex;
            std::condition_variable_any cv;
            rs2_stream key_stream;
            stream_pipe streams[RS2_STREAM_COUNT];
        };

        std::shared_ptr<shared_impl> impl;

        double dist(const frame_holder& a, const frame_holder& b) const;

        void get_frameset(frameset* frames) const;
    };
}
