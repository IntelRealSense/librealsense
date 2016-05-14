#include "sync.h"

using namespace rsimpl;

frame_archive::frame_archive(const std::vector<subdevice_mode_selection> & selection, rs_stream key_stream) : key_stream(key_stream)
{
    // Store the mode selection that pertains to each native stream
    for(auto & mode : selection)
    {
        for(auto & o : mode.get_outputs())
        {
            modes[o.first] = mode;
        }
    }

    // Enumerate all streams we need to keep synchronized with the key stream
    for(auto s : {RS_STREAM_DEPTH, RS_STREAM_INFRARED, RS_STREAM_INFRARED2, RS_STREAM_COLOR})
    {
        if(is_stream_enabled(s) && s != key_stream) other_streams.push_back(s);
    }

    // Allocate an empty image for each stream, and move it to the frontbuffer
    // This allows us to assume that get_frame_data/get_frame_timestamp always return valid data
    alloc_frame(key_stream, 0);
    frontbuffer.place_frame(key_stream, std::move(backbuffer[key_stream]));
    for(auto s : other_streams)
    {
        alloc_frame(s, 0);
        frontbuffer.place_frame(s, std::move(backbuffer[s]));
    }
}

const byte * frame_archive::get_frame_data(rs_stream stream) const 
{ 
    return frontbuffer.get_frame_data(stream);
}

int frame_archive::get_frame_timestamp(rs_stream stream) const
{ 
    return frontbuffer.get_frame_timestamp(stream);
}

frame_archive::frameset* frame_archive::clone_frontbuffer()
{
	return clone_frameset(&frontbuffer);
}

// Block until the next coherent frameset is available
void frame_archive::wait_for_frames()
{
    std::unique_lock<std::mutex> lock(mutex);
    const auto ready = [this]() { return !frames[key_stream].empty(); };
    if (!ready() && !cv.wait_for(lock, std::chrono::seconds(5), ready)) throw std::runtime_error("Timeout waiting for frames.");
    get_next_frames();
}

// If a coherent frameset is available, obtain it and return true, otherwise return false immediately
bool frame_archive::poll_for_frames()
{
    // TODO: Implement a user-specifiable timeout for how long to wait before returning false?
    std::unique_lock<std::mutex> lock(mutex);
    if(frames[key_stream].empty()) return false;
    get_next_frames();
    return true;
}

frame_archive::frameset* frame_archive::wait_for_frames_safe()
{
	frameset * result = nullptr;
	do
	{
		std::unique_lock<std::mutex> lock(mutex);
		const auto ready = [this]() { return !frames[key_stream].empty(); };
		if (!ready() && !cv.wait_for(lock, std::chrono::seconds(5), ready)) throw std::runtime_error("Timeout waiting for frames.");
		get_next_frames();
		result = clone_frontbuffer();
	} 
	while (!result);
	return result;
}

bool frame_archive::poll_for_frames_safe(frameset** frameset)
{
    // TODO: Implement a user-specifiable timeout for how long to wait before returning false?
    std::unique_lock<std::mutex> lock(mutex);
    if (frames[key_stream].empty()) return false;
    get_next_frames();
	auto result = clone_frontbuffer();
	if (result)
	{
		*frameset = result;
		return true;
	}
	return false;
}

// Move frames from the queues to the frontbuffers to form the next coherent frameset
void frame_archive::get_next_frames()
{
    // Always dequeue a frame from the key stream
    dequeue_frame(key_stream);

    // Dequeue from other streams if the new frame is closer to the timestamp of the key stream than the old frame
    for(auto s : other_streams)
    {
        if (!frames[s].empty() && abs(frames[s].front().timestamp - frontbuffer.get_frame_timestamp(key_stream)) <= abs(frontbuffer.get_frame_timestamp(s) - frontbuffer.get_frame_timestamp(key_stream)))
        {
            dequeue_frame(s);
        }
    }
}

void frame_archive::release_frameset(frameset* frameset)
{
	published_sets.deallocate(frameset);
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

void frame_archive::release_frame_ref(frame_ref* ref)
{
	detached_refs.deallocate(ref);
}

// Allocate a new frame in the backbuffer, potentially recycling a buffer from the freelist
byte * frame_archive::alloc_frame(rs_stream stream, int timestamp) 
{ 
    const size_t size = modes[stream].get_image_size(stream);

    {
        std::lock_guard<std::mutex> guard(mutex);

        // Attempt to obtain a buffer of the appropriate size from the freelist
        for(auto it = begin(freelist); it != end(freelist); ++it)
        {
            if(it->data.size() == size)
            {
                backbuffer[stream] = std::move(*it);
                freelist.erase(it);
                break;
            }
        }

        // Discard buffers that have been in the freelist for longer than 1s
        for(auto it = begin(freelist); it != end(freelist); )
        {
            if(timestamp > it->timestamp + 1000) it = freelist.erase(it);
            else ++it;
        }
    }

    backbuffer[stream].update_owner(this);
    backbuffer[stream].data.resize(size); // TODO: Allow users to provide a custom allocator for frame buffers
    backbuffer[stream].timestamp = timestamp;
    return backbuffer[stream].data.data();
}

// Move a frame from the backbuffer to the back of the queue
void frame_archive::commit_frame(rs_stream stream) 
{
    std::unique_lock<std::mutex> lock(mutex);
    frames[stream].push_back(std::move(backbuffer[stream]));
    cull_frames();
    lock.unlock();
    if(!frames[key_stream].empty()) cv.notify_one();
}

frame_archive::frame_ref* frame_archive::track_frame(rs_stream stream)
{
	std::unique_lock<std::mutex> lock(mutex);

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

	frontbuffer.cleanup(); // frontbuffer also holds frame references, since its content is publicly available through get_frame_data

	// wait until user is done with all the stuff he chose to borrow
	detached_refs.wait_until_empty();
	published_frames.wait_until_empty();
	published_sets.wait_until_empty();
}

// Discard all frames which are older than the most recent coherent frameset
void frame_archive::cull_frames()
{
    // Never keep more than four frames around in any given stream, regardless of timestamps
    for(auto s : {RS_STREAM_DEPTH, RS_STREAM_COLOR, RS_STREAM_INFRARED, RS_STREAM_INFRARED2})
    {
        while(frames[s].size() > 4)
        {
            discard_frame(s);
        }
    }

    // Cannot do any culling unless at least one frame is enqueued for each enabled stream
    if(frames[key_stream].empty()) return;
    for(auto s : other_streams) if(frames[s].empty()) return;

    // We can discard frames from the key stream if we have at least two and the latter is closer to the most recent frame of all other streams than the former
    while(true)
    {
        if(frames[key_stream].size() < 2) break;
        const int t0 = frames[key_stream][0].timestamp, t1 = frames[key_stream][1].timestamp;

        bool valid_to_skip = true;
        for(auto s : other_streams)
        {
            if(abs(t0 - frames[s].back().timestamp) < abs(t1 - frames[s].back().timestamp))
            {
                valid_to_skip = false;
                break;
            }
        }
        if(!valid_to_skip) break;

        discard_frame(key_stream);
    }

    // We can discard frames for other streams if we have at least two and the latter is closer to the next key stream frame than the former
    for(auto s : other_streams)
    {
        while(true)
        {
            if(frames[s].size() < 2) break;
            const int t0 = frames[s][0].timestamp, t1 = frames[s][1].timestamp;

            if(abs(t0 - frames[key_stream].front().timestamp) < abs(t1 - frames[key_stream].front().timestamp)) break;
            discard_frame(s);
        }
    }
}

// Move a single frame from the head of the queue to the front buffer, while recycling the front buffer into the freelist
void frame_archive::dequeue_frame(rs_stream stream)
{
    frontbuffer.place_frame(stream, std::move(frames[stream].front())); // the frame will move to free list once there are no external references to it
    frames[stream].erase(begin(frames[stream]));
}

// Move a single frame from the head of the queue directly to the freelist
void frame_archive::discard_frame(rs_stream stream)
{
    freelist.push_back(std::move(frames[stream].front()));
    frames[stream].erase(begin(frames[stream]));    
}

frame_archive::frame& frame_archive::frame::operator=(frame&& r)
{ 
	data = move(r.data); 
	timestamp = r.timestamp; 
	owner = r.owner;
	ref_count = r.ref_count.exchange(0);
	return *this; 
}

void frame_archive::frame::release()
{
	if (ref_count.fetch_sub(1) == 1)
	{
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
		buffer[i] = frame_ref(nullptr);
	}
}

frame_archive::frame_ref::frame_ref(frame* frame): frame_ptr(frame)
{
	if (frame) frame->acquire();
}

frame_archive::frame_ref::frame_ref(const frame_ref& other): frame_ptr(other.frame_ptr)
{
	if (frame_ptr) frame_ptr->acquire();
}

frame_archive::frame_ref::frame_ref(frame_ref&& other): frame_ptr(other.frame_ptr)
{
	other.frame_ptr = nullptr;
}

frame_archive::frame_ref& frame_archive::frame_ref::operator=(frame_ref other)
{
	swap(other);
	return *this;
}

frame_archive::frame_ref::~frame_ref()
{
	if (frame_ptr) frame_ptr->release();
}

void frame_archive::frame_ref::swap(frame_ref& other)
{
	std::swap(frame_ptr, other.frame_ptr);
}

const byte* frame_archive::frame_ref::get_frame_data() const
{
	return frame_ptr ? frame_ptr->data.data() : nullptr;
}

int frame_archive::frame_ref::get_frame_timestamp() const
{
	return frame_ptr ? frame_ptr->timestamp : 0;
}