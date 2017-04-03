#include "archive.h"

using namespace rsimpl2;

frame_archive::frame_archive(std::atomic<uint32_t>* in_max_frame_queue_size,
                             std::shared_ptr<uvc::time_service> ts,
                             std::shared_ptr<uvc::uvc_device> dev)
    : max_frame_queue_size(in_max_frame_queue_size),
      mutex(), recycle_frames(true), _time_service(ts),
      device(dev)

{
    published_frames_count = 0;
}

void frame_archive::unpublish_frame(frame* frame)
{
    if (frame)
    {
        log_frame_callback_end(frame);
        std::unique_lock<std::recursive_mutex> lock(mutex);

        if (is_valid(frame->get_stream_type()))
            --published_frames_count;

        if (recycle_frames)
        {
            freelist.push_back(std::move(*frame));
        }
        lock.unlock();

        published_frames.deallocate(frame);
    }
}

frame* frame_archive::publish_frame(frame&& frame)
{
    if (is_valid(frame.get_stream_type()) &&
        published_frames_count >= *max_frame_queue_size)
    {
        LOG_DEBUG("stream_type is invalid OR user didn't release frame resource.");
        return nullptr;
    }
    auto new_frame = published_frames.allocate();
    if (new_frame)
    {
        if (is_valid(frame.get_stream_type())) ++published_frames_count;
        *new_frame = std::move(frame);
    }
    return new_frame;
}

rs2_frame* frame_archive::clone_frame(rs2_frame* frame)
{
    auto new_ref = detached_refs.allocate();
    if (new_ref)
    {
        *new_ref = *frame;
    }
    return new_ref;
}

void frame_archive::release_frame_ref(rs2_frame* ref)
{
    detached_refs.deallocate(ref);
}

// Allocate a new frame in the backbuffer, potentially recycling a buffer from the freelist
frame frame_archive::alloc_frame(const size_t size, const frame_additional_data& additional_data, bool requires_memory)
{
    frame backbuffer;
    //const size_t size = modes[stream].get_image_size(stream);
    {
        std::lock_guard<std::recursive_mutex> guard(mutex);

        if (requires_memory)
        {
            // Attempt to obtain a buffer of the appropriate size from the freelist
            for (auto it = begin(freelist); it != end(freelist); ++it)
            {
                if (it->data.size() == size)
                {
                    backbuffer = std::move(*it);
                    freelist.erase(it);
                    break;
                }
            }
        }

        // Discard buffers that have been in the freelist for longer than 1s
        for (auto it = begin(freelist); it != end(freelist);)
        {
            if (additional_data.timestamp > it->additional_data.timestamp + 1000) it = freelist.erase(it);
            else ++it;
        }
    }

    if (requires_memory)
    {
        backbuffer.data.resize(size); // TODO: Allow users to provide a custom allocator for frame buffers
    }
    backbuffer.additional_data = additional_data;
    return backbuffer;
}

rs2_frame* frame_archive::track_frame(frame& f)
{
    std::unique_lock<std::recursive_mutex> lock(mutex);

    auto published_frame = f.publish(shared_from_this());
    if (published_frame)
    {
        rs2_frame new_ref(published_frame); // allocate new frame_ref to ref-counter the now published frame
        return clone_frame(&new_ref);
    }

    LOG_DEBUG("publish(...) failed");
    return nullptr;
}

void frame_archive::flush()
{
    published_frames.stop_allocation();
    detached_refs.stop_allocation();
    callback_inflight.stop_allocation();
    recycle_frames = false;

    auto callbacks_inflight = callback_inflight.get_size();
    if (callbacks_inflight > 0)
    {
        LOG_WARNING(callbacks_inflight << " callbacks are still running on some other threads. Waiting until all callbacks return...");
    }
    // wait until user is done with all the stuff he chose to borrow
    callback_inflight.wait_until_empty();

    {
        std::lock_guard<std::recursive_mutex> guard(mutex);
        freelist.clear();
    }

    pending_frames = published_frames.get_size();
    if (pending_frames > 0)
    {
        LOG_WARNING("The user was holding on to "
            << std::dec << pending_frames << " frames after stream 0x"
            << std::hex << this << " stopped" << std::dec);
    }
    // frames and their frame refs are not flushed, by design
}

frame_archive::~frame_archive()
{
    if (pending_frames > 0)
    {
        LOG_WARNING("All frames from stream 0x"
            << std::hex << this << " are now released by the user");
    }
}

void frame::release()
{

    if (ref_count.fetch_sub(1) == 1)
    {
        on_release();
        owner->unpublish_frame(this);
    }
}

frame* frame::publish(std::shared_ptr<frame_archive> new_owner)
{
    owner = new_owner;
    return owner->publish_frame(std::move(*this));
}

double frame::get_frame_metadata(const rs2_frame_metadata& /*frame_metadata*/) const
{
    throw not_implemented_exception("unsupported metadata type");
}

bool frame::supports_frame_metadata(const rs2_frame_metadata& /*frame_metadata*/) const
{
    return false;
}

const byte* frame::get_frame_data() const
{
    const byte* frame_data = data.data();

    if (on_release.get_data())
    {
        frame_data = static_cast<const byte*>(on_release.get_data());
        /*if (additional_data.pad < 0)
        {
            frame_data += static_cast<int>(additional_data.stride_x *additional_data.bpp*(-additional_data.pad) + (-additional_data.pad)*additional_data.bpp);
        }*/
    }

    return frame_data;
}

rs2_timestamp_domain frame::get_frame_timestamp_domain() const
{
    return additional_data.timestamp_domain;
}

rs2_time_t frame::get_frame_timestamp() const
{
    return additional_data.timestamp;
}

unsigned long long frame::get_frame_number() const
{
    return additional_data.frame_number;
}

rs2_time_t frame::get_frame_system_time() const
{
    return additional_data.system_time;
}

int frame::get_width() const
{
    return additional_data.width;
}

int frame::get_height() const
{
    return additional_data.height;
}

int frame::get_framerate() const
{
    return additional_data.fps;
}

int frame::get_stride() const
{
    return (additional_data.stride * additional_data.bpp) / 8;
}

int frame::get_bpp() const
{
    return additional_data.bpp;
}

void frame::update_frame_callback_start_ts(rs2_time_t ts)
{
    additional_data.frame_callback_started = ts;
}

rs2_format frame::get_format() const
{
    return additional_data.format;
}
rs2_stream frame::get_stream_type() const
{
    return additional_data.stream_type;
}

rs2_time_t frame::get_frame_callback_start_time_point() const
{
    return additional_data.frame_callback_started;
}

void frame_archive::log_frame_callback_end(frame* frame) const
{
    if (frame)
    {
        auto callback_ended = _time_service->get_time();
        auto callback_warning_duration = 1000 / (frame->additional_data.fps + 1);
        auto callback_duration = callback_ended - frame->get_frame_callback_start_time_point();

        LOG_DEBUG("CallbackFinished," << rsimpl2::get_string(frame->get_stream_type()) << "," << frame->get_frame_number()
            << ",DispatchedAt," << callback_ended);

        if (callback_duration > callback_warning_duration)
        {
            LOG_DEBUG("Frame Callback [" << rsimpl2::get_string(frame->get_stream_type())
                     << "#" << std::dec << frame->additional_data.frame_number
                     << "] overdue. (Duration: " << callback_duration
                     << "ms, FPS: " << frame->additional_data.fps << ", Max Duration: " << callback_warning_duration << "ms)");
        }
    }
}

void rs2_frame::log_callback_start(rs2_time_t timestamp) const
{
    if (get())
    {
        get()->update_frame_callback_start_ts(timestamp);
        LOG_DEBUG("CallbackStarted," << rsimpl2::get_string(get()->get_stream_type()) << "," << get()->get_frame_number() << ",DispatchedAt," << timestamp);
    }
}

void rs2_frame::log_callback_end(rs2_time_t timestamp) const
{
    if (get())
    {
        auto callback_warning_duration = 1000.f / (get()->additional_data.fps + 1);
        auto callback_duration = timestamp - get()->get_frame_callback_start_time_point();

        LOG_DEBUG("CallbackFinished," << rsimpl2::get_string(get()->get_stream_type()) << "," << get()->get_frame_number() << ",DispatchedAt," << timestamp);

        if (callback_duration > callback_warning_duration)
        {
            LOG_INFO("Frame Callback " << rsimpl2::get_string(get()->get_stream_type())
                     << "#" << std::dec << get()->additional_data.frame_number
                     << "overdue. (Duration: " << callback_duration
                     << "ms, FPS: " << get()->additional_data.fps << ", Max Duration: " << callback_warning_duration << "ms)");
        }
    }
}
