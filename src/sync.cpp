// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "sync.h"

namespace rsimpl2
{
    void syncer::dispatch_frame(frame_holder f)
    {
        using namespace std;

        unique_lock<recursive_mutex> lock(impl->mutex);
        auto stream_type = f.frame->get()->get_stream_type();
        impl->streams[stream_type].queue.enqueue(move(f));

        lock.unlock();
        if (stream_type == impl->key_stream) impl->cv.notify_one();
    }

    frameset syncer::wait_for_frames(int timeout_ms)
    {
        using namespace std;
        unique_lock<recursive_mutex> lock(impl->mutex);
        const auto ready = [this]()
        {
            return impl->streams[impl->key_stream].queue.try_dequeue(
                &impl->streams[impl->key_stream].front);
        };

        frameset result;

        if (!ready())
        {
            if (!impl->cv.wait_for(lock, chrono::milliseconds(timeout_ms), ready))
            {
                return result;
            }
        }

        get_frameset(&result);
        return result;
    }

    bool syncer::poll_for_frames(frameset& frames)
    {
        using namespace std;
        unique_lock<recursive_mutex> lock(impl->mutex);
        if (!impl->streams[impl->key_stream].queue.try_dequeue(
                &impl->streams[impl->key_stream].front)) return false;
        get_frameset(&frames);
        return true;
    }

    double syncer::dist(const frame_holder& a, const frame_holder& b) const
    {
        return std::fabs(a.frame->get()->get_frame_timestamp() -
                         b.frame->get()->get_frame_timestamp());
    }

    void syncer::get_frameset(frameset* frames) const
    {
        frames->clear();
        for (auto i = 0; i < RS2_STREAM_COUNT; i++)
        {
            if (i == impl->key_stream) continue;

            frame_holder res;

            while (true)
            {
                // res <-- front
                if (impl->streams[i].front)
                {
                    res = impl->streams[i].front->get()->get_owner()->clone_frame(impl->streams[i].front);
                    // ignoring return value, if front wasn't copied to res,
                    // it is still better to get rid of it, otherwise,
                    // one single frame can stuck syncronization
                }

                // front <-- deque(q)
                if (!impl->streams[i].queue.try_dequeue(&impl->streams[i].front)) break;

                // if res is a better match, break
                if (res && dist(impl->streams[i].front, impl->streams[impl->key_stream].front) >
                        dist(res, impl->streams[impl->key_stream].front)) break;
            }

            if (res)
            {
                frames->push_back(std::move(res));
            }
        }
        frames->push_back(std::move(impl->streams[impl->key_stream].front));
    }
}

