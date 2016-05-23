
#include "archive.h"

using namespace rsimpl;

frame_archive::frame_archive(const std::vector<subdevice_mode_selection>& selection) : mutex()
{
    // Store the mode selection that pertains to each native stream
    for (auto & mode : selection)
    {
        for (auto & o : mode.get_outputs())
        {
            modes[o.first] = mode;
        }
    }
}

frame_archive::frameset* frame_archive::clone_frameset(frameset* frameset)
{
    auto new_set = published_sets.allocate();
    if (new_set)
    {
        *new_set = *frameset;
    }
    return new_set;
}

void frame_archive::unpublish_frame(frame* frame)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    freelist.push_back(std::move(*frame));
    published_frames.deallocate(frame);
}

frame_archive::frame* frame_archive::publish_frame(frame&& frame)
{
    auto new_frame = published_frames.allocate();
    if (new_frame)
    {
        *new_frame = std::move(frame);
    }
    return new_frame;
}

frame_archive::frame_ref* frame_archive::detach_frame_ref(frameset* frameset, rs_stream stream)
{
    auto new_ref = detached_refs.allocate();
    if (new_ref)
    {
        *new_ref = std::move(frameset->detach_ref(stream));
    }
    return new_ref;
}

frame_archive::frame_ref* frame_archive::clone_frame(frame_ref* frameset)
{
    auto new_ref = detached_refs.allocate();
    if (new_ref)
    {
        *new_ref = *frameset;
    }
    return new_ref;
}

// Allocate a new frame in the backbuffer, potentially recycling a buffer from the freelist
byte * frame_archive::alloc_frame(rs_stream stream, int timestamp, int frame_number, long long system_time, bool requires_memory)
{
    const size_t size = modes[stream].get_image_size(stream);

    {
        std::lock_guard<std::recursive_mutex> guard(mutex);

        if (requires_memory)
        {
            // Attempt to obtain a buffer of the appropriate size from the freelist
            for (auto it = begin(freelist); it != end(freelist); ++it)
            {
                if (it->data.size() == size)
                {
                    backbuffer[stream] = std::move(*it);
                    freelist.erase(it);
                    break;
                }
            }
        }

        if (stream != RS_STREAM_FISHEYE) // TODO: W/O until we will achieve frame timestamp
        {
            // Discard buffers that have been in the freelist for longer than 1s
            for (auto it = begin(freelist); it != end(freelist);)
            {
                if (timestamp > it->timestamp + 1000) it = freelist.erase(it);
                else ++it;
            }
        }
        
    }

    if (requires_memory)
    {
        backbuffer[stream].data.resize(size); // TODO: Allow users to provide a custom allocator for frame buffers
    }

    backbuffer[stream].update_owner(this);
    backbuffer[stream].timestamp = timestamp;
    backbuffer[stream].frame_number = frame_number;
	backbuffer[stream].system_time = system_time;
    return backbuffer[stream].data.data();
}

void frame_archive::attach_continuation(rs_stream stream, frame_continuation&& continuation)
{
    backbuffer[stream].attach_continuation(std::move(continuation));
}

frame_archive::frame_ref* frame_archive::track_frame(rs_stream stream)
{
    std::unique_lock<std::recursive_mutex> lock(mutex);

    auto published_frame = backbuffer[stream].publish();
    if (published_frame)
    {
        frame_ref new_ref(published_frame); // allocate new frame_ref to ref-counter the now published frame
        return clone_frame(&new_ref);
    }

    return nullptr;
}

void frame_archive::flush()
{
    published_frames.stop_allocation();
    published_sets.stop_allocation();
    detached_refs.stop_allocation();

    // wait until user is done with all the stuff he chose to borrow
    detached_refs.wait_until_empty();
    published_frames.wait_until_empty();
    published_sets.wait_until_empty();
}

void frame_archive::frame::release()
{
    if (ref_count.fetch_sub(1) == 1)
    {
        owner->unpublish_frame(this);
        on_release();
    }
}

frame_archive::frame* frame_archive::frame::publish()
{
    return owner->publish_frame(std::move(*this));
}

frame_archive::frame_ref frame_archive::frameset::detach_ref(rs_stream stream)
{
    return std::move(buffer[stream]);
}

void frame_archive::frameset::place_frame(rs_stream stream, frame&& new_frame)
{
    auto published_frame = new_frame.publish();
    if (published_frame)
    {
        frame_ref new_ref(published_frame); // allocate new frame_ref to ref-counter the now published frame
        buffer[stream] = std::move(new_ref); // move the new handler in, release a ref count on previous frame
    }
}

void frame_archive::frameset::cleanup()
{
    for (auto i = 0; i < RS_STREAM_NATIVE_COUNT; i++)
    {
        buffer[i] = frame_ref(nullptr);
    }
}

const byte* frame_archive::frame_ref::get_frame_data() const
{
    return frame_ptr ? frame_ptr->get_frame_data() : nullptr;
}

int frame_archive::frame_ref::get_frame_timestamp() const
{
    return frame_ptr ? frame_ptr->timestamp : 0;
}

int frame_archive::frame_ref::get_frame_number() const
{
    return frame_ptr ? frame_ptr->frame_number : 0;
}

long long frame_archive::frame_ref::get_frame_system_time() const
{
	return frame_ptr ? frame_ptr->system_time : 0;
}

const byte* frame_archive::frame::get_frame_data() const
{
    if (on_release.get_data())
        return (const byte*)on_release.get_data();
    return data.data();
}

