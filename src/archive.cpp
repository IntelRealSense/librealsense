
#include "archive.h"
#include <algorithm>

using namespace rsimpl;

frame_archive::frame_archive(const std::vector<subdevice_mode_selection>& selection, std::atomic<uint32_t>* in_max_frame_queue_size, std::chrono::high_resolution_clock::time_point capture_started)
    : max_frame_queue_size(in_max_frame_queue_size), mutex(), capture_started(capture_started)
{
    // Store the mode selection that pertains to each native stream
    for (auto & mode : selection)
    {
        for (auto & o : mode.get_outputs())
        {
            modes[o.first] = mode;
        }
    }

    for(auto s : {RS_STREAM_DEPTH, RS_STREAM_INFRARED, RS_STREAM_INFRARED2, RS_STREAM_COLOR, RS_STREAM_FISHEYE})
    {
        published_frames_per_stream[s] = 0;
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
    if (frame)
    {
        log_frame_callback_end(frame);
        std::lock_guard<std::recursive_mutex> lock(mutex);

        if (is_valid(frame->get_stream_type()))
            --published_frames_per_stream[frame->get_stream_type()];

        freelist.push_back(std::move(*frame));
        published_frames.deallocate(frame);
       
    }
}

frame_archive::frame* frame_archive::publish_frame(frame&& frame)
{
    if (is_valid(frame.get_stream_type()) &&
        published_frames_per_stream[frame.get_stream_type()] >= *max_frame_queue_size)
    {
        return nullptr;
    }
    auto new_frame = published_frames.allocate();
    if (new_frame)
    {
        if (is_valid(frame.get_stream_type())) ++published_frames_per_stream[frame.get_stream_type()];
        *new_frame = std::move(frame);
    }
    return new_frame;
}

frame_archive::frame_ref* frame_archive::detach_frame_ref(frameset* frameset, rs_stream stream)
{
    auto new_ref = detached_refs.allocate();
    if (new_ref)
    {
        *new_ref = frameset->detach_ref(stream);
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
byte * frame_archive::alloc_frame(rs_stream stream, const frame_additional_data& additional_data, bool requires_memory)
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

        // Discard buffers that have been in the freelist for longer than 1s
        for (auto it = begin(freelist); it != end(freelist);)
        {
            if (additional_data.timestamp > it->additional_data.timestamp + 1000) it = freelist.erase(it);
            else ++it;
        }
    }
    
    if (requires_memory)
    {
        backbuffer[stream].data.resize(size); // TODO: Allow users to provide a custom allocator for frame buffers
    }
    backbuffer[stream].update_owner(this);
    backbuffer[stream].additional_data = additional_data;
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
        on_release();
        owner->unpublish_frame(this);
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
        buffer[i].disable_continuation();
        buffer[i] = frame_ref(nullptr);
    }
}

double frame_archive::frame_ref::get_frame_metadata(rs_frame_metadata frame_metadata) const
{
    return frame_ptr ? frame_ptr->get_frame_metadata(frame_metadata) : 0;
}

bool frame_archive::frame_ref::supports_frame_metadata(rs_frame_metadata frame_metadata) const
{
    return frame_ptr ? frame_ptr->supports_frame_metadata(frame_metadata) : 0;
}

const byte* frame_archive::frame_ref::get_frame_data() const
{
    return frame_ptr ? frame_ptr->get_frame_data() : nullptr;
}

double frame_archive::frame_ref::get_frame_timestamp() const
{
    return frame_ptr ? frame_ptr->get_frame_timestamp(): 0;
}

unsigned long long frame_archive::frame_ref::get_frame_number() const
{
    return frame_ptr ? frame_ptr->get_frame_number() : 0;
}

long long frame_archive::frame_ref::get_frame_system_time() const
{
    return frame_ptr ? frame_ptr->get_frame_system_time() : 0;
}

rs_timestamp_domain frame_archive::frame_ref::get_frame_timestamp_domain() const
{
    return frame_ptr ? frame_ptr->get_frame_timestamp_domain() : RS_TIMESTAMP_DOMAIN_COUNT;
}

int frame_archive::frame_ref::get_frame_width() const
{
    return frame_ptr ? frame_ptr->get_width() : 0;
}

int frame_archive::frame_ref::get_frame_height() const
{
    return frame_ptr ? frame_ptr->get_height() : 0;
}

int frame_archive::frame_ref::get_frame_framerate() const
{
    return frame_ptr ? frame_ptr->get_framerate() : 0;
}

int frame_archive::frame_ref::get_frame_stride() const
{
    return frame_ptr ? frame_ptr->get_stride() : 0;
}

int frame_archive::frame_ref::get_frame_bpp() const
{
    return frame_ptr ? frame_ptr->get_bpp() : 0;
}

rs_format frame_archive::frame_ref::get_frame_format() const
{
    return frame_ptr ? frame_ptr->get_format() : RS_FORMAT_COUNT;
}

rs_stream frame_archive::frame_ref::get_stream_type() const
{
    return frame_ptr ? frame_ptr->get_stream_type() : RS_STREAM_COUNT;
}

std::chrono::high_resolution_clock::time_point frame_archive::frame_ref::get_frame_callback_start_time_point() const
{
    return frame_ptr ? frame_ptr->get_frame_callback_start_time_point() :  std::chrono::high_resolution_clock::now();
}

void frame_archive::frame_ref::update_frame_callback_start_ts(std::chrono::high_resolution_clock::time_point ts)
{
    frame_ptr->update_frame_callback_start_ts(ts);
}

double frame_archive::frame::get_frame_metadata(rs_frame_metadata frame_metadata) const
{
    if (!supports_frame_metadata(frame_metadata))
        throw std::logic_error("unsupported metadata type");

    switch (frame_metadata)
    {
        case RS_FRAME_METADATA_ACTUAL_EXPOSURE:
            return additional_data.exposure_value;
            break;
        case RS_FRAME_METADATA_ACTUAL_FPS:
            return additional_data.actual_fps;
            break;
        default:
            throw std::logic_error("unsupported metadata type");
            break;
    }
}

bool frame_archive::frame::supports_frame_metadata(rs_frame_metadata frame_metadata) const
{
    for (auto & md : *additional_data.supported_metadata_vector) if (md == frame_metadata) return true;
    return false;
}

const byte* frame_archive::frame::get_frame_data() const
{
	const byte* frame_data = data.data();;

    if (on_release.get_data())
    {
        frame_data = static_cast<const byte*>(on_release.get_data());
        if (additional_data.pad < 0)
        {
            frame_data += (int)(additional_data.stride_x *additional_data.bpp*(-additional_data.pad) + (-additional_data.pad)*additional_data.bpp);
        }
    }

    return frame_data;
}

rs_timestamp_domain frame_archive::frame::get_frame_timestamp_domain() const
{
    return additional_data.timestamp_domain;
}

double frame_archive::frame::get_frame_timestamp() const
{
    return additional_data.timestamp;
}

unsigned long long frame_archive::frame::get_frame_number() const
{
    return additional_data.frame_number;
}

long long frame_archive::frame::get_frame_system_time() const
{
    return additional_data.system_time;
}

int frame_archive::frame::get_width() const
{
    return additional_data.width;
}

int frame_archive::frame::get_height() const
{
    return additional_data.stride_y ? std::min(additional_data.height, additional_data.stride_y) : additional_data.height;
}

int frame_archive::frame::get_framerate() const
{
    return additional_data.fps;
}

int frame_archive::frame::get_stride() const
{
    return (additional_data.stride_x * additional_data.bpp) / 8;
}

int frame_archive::frame::get_bpp() const
{
    return additional_data.bpp;
}


void frame_archive::frame::update_frame_callback_start_ts(std::chrono::high_resolution_clock::time_point ts)
{
    additional_data.frame_callback_started = ts;
}


rs_format frame_archive::frame::get_format() const
{
    return additional_data.format;
}
rs_stream frame_archive::frame::get_stream_type() const
{
    return additional_data.stream_type;
}

std::chrono::high_resolution_clock::time_point frame_archive::frame::get_frame_callback_start_time_point() const
{
    return additional_data.frame_callback_started;
}

void frame_archive::log_frame_callback_end(frame* frame)
{
    auto callback_ended = std::chrono::high_resolution_clock::now();
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(callback_ended - capture_started).count();
    auto callback_warning_duration = 1000 / (frame->additional_data.fps+1);
    auto callback_duration = std::chrono::duration_cast<std::chrono::milliseconds>(callback_ended - frame->get_frame_callback_start_time_point()).count();

    if (callback_duration > callback_warning_duration)
    {
        LOG_INFO("Frame Callback took too long to complete. (Duration: " << callback_duration << "ms, FPS: " << frame->additional_data.fps << ", Max Duration: " << callback_warning_duration << "ms)");
    }

    LOG_DEBUG("CallbackFinished," << rsimpl::get_string(frame->get_stream_type()) << "," << frame->get_frame_number() << ",DispatchedAt," << ts);
}

void frame_archive::frame_ref::log_callback_start(std::chrono::high_resolution_clock::time_point capture_start_time)
{
    auto callback_start_time = std::chrono::high_resolution_clock::now();
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(callback_start_time - capture_start_time).count();
    LOG_DEBUG("CallbackStarted," << rsimpl::get_string(get_stream_type()) << "," << get_frame_number() << ",DispatchedAt," << ts);
}
