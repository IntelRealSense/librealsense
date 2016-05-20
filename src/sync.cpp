#include "sync.h"

using namespace rsimpl;

syncronizing_archive::syncronizing_archive(const std::vector<subdevice_mode_selection> & selection, rs_stream key_stream)
    : frame_archive(selection), key_stream(key_stream)
{
    // Enumerate all streams we need to keep synchronized with the key stream
    for(auto s : {RS_STREAM_DEPTH, RS_STREAM_INFRARED, RS_STREAM_INFRARED2, RS_STREAM_COLOR, RS_STREAM_FISHEYE})
    {
        if(is_stream_enabled(s) && s != key_stream) other_streams.push_back(s);
    }

    // Allocate an empty image for each stream, and move it to the frontbuffer
    // This allows us to assume that get_frame_data/get_frame_timestamp always return valid data
    alloc_frame(key_stream, 0, 0, true);
    frontbuffer.place_frame(key_stream, std::move(backbuffer[key_stream]));
    for(auto s : other_streams)
    {
        alloc_frame(s, 0, 0, true);
        frontbuffer.place_frame(s, std::move(backbuffer[s]));
    }
}

const byte * syncronizing_archive::get_frame_data(rs_stream stream) const
{ 
    return frontbuffer.get_frame_data(stream);
}

int syncronizing_archive::get_frame_timestamp(rs_stream stream) const
{ 
    return frontbuffer.get_frame_timestamp(stream);
}

frame_archive::frameset* syncronizing_archive::clone_frontbuffer()
{
    return clone_frameset(&frontbuffer);
}

int rsimpl::syncronizing_archive::get_frame_counter(rs_stream stream) const
{
    return frontbuffer.get_frame_number(stream);
}

// Block until the next coherent frameset is available
void syncronizing_archive::wait_for_frames()
{
    std::unique_lock<std::recursive_mutex> lock(mutex);
    const auto ready = [this]() { return !frames[key_stream].empty(); };
    // Evgeni - for debugging incrase the timeout period to 50 (instead of 5 seconds
    if(!ready() && !cv.wait_for(lock, std::chrono::seconds(50), ready)) throw std::runtime_error("Timeout waiting for frames.");
    get_next_frames();
}

// If a coherent frameset is available, obtain it and return true, otherwise return false immediately
bool syncronizing_archive::poll_for_frames()
{
    // TODO: Implement a user-specifiable timeout for how long to wait before returning false?
    std::unique_lock<std::recursive_mutex> lock(mutex);
    if(frames[key_stream].empty()) return false;
    get_next_frames();
    return true;
}

frame_archive::frameset* syncronizing_archive::wait_for_frames_safe()
{
    frameset * result = nullptr;
    do
    {
        std::unique_lock<std::recursive_mutex> lock(mutex);
        const auto ready = [this]() { return !frames[key_stream].empty(); };
        if (!ready() && !cv.wait_for(lock, std::chrono::seconds(5), ready)) throw std::runtime_error("Timeout waiting for frames.");
        get_next_frames();
        result = clone_frontbuffer();
    } 
    while (!result);
    return result;
}

bool syncronizing_archive::poll_for_frames_safe(frameset** frameset)
{
    // TODO: Implement a user-specifiable timeout for how long to wait before returning false?
    std::unique_lock<std::recursive_mutex> lock(mutex);
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
void syncronizing_archive::get_next_frames()
{
    // Always dequeue a frame from the key stream
    dequeue_frame(key_stream);

    // Dequeue from other streams if the new frame is closer to the timestamp of the key stream than the old frame
    for(auto s : other_streams)
    {
        if (!frames[s].empty() && s == RS_STREAM_FISHEYE) // TODO: W/O until we will achieve frame timestamp
        {
            dequeue_frame(s);
            continue;
        }

        if (!frames[s].empty() && abs(frames[s].front().timestamp - frontbuffer.get_frame_timestamp(key_stream)) <= abs(frontbuffer.get_frame_timestamp(s) - frontbuffer.get_frame_timestamp(key_stream)))
        {
            dequeue_frame(s);
        }
    }
}

// Move a frame from the backbuffer to the back of the queue
void syncronizing_archive::commit_frame(rs_stream stream)
{
    std::unique_lock<std::recursive_mutex> lock(mutex);
    frames[stream].push_back(std::move(backbuffer[stream]));
    cull_frames();
    lock.unlock();
    if(!frames[key_stream].empty()) cv.notify_one();
}

void syncronizing_archive::flush()
{
    frontbuffer.cleanup(); // frontbuffer also holds frame references, since its content is publicly available through get_frame_data
    frame_archive::flush();
}


// Discard all frames which are older than the most recent coherent frameset
void syncronizing_archive::cull_frames()
{
    // Never keep more than four frames around in any given stream, regardless of timestamps
    for(auto s : {RS_STREAM_DEPTH, RS_STREAM_COLOR, RS_STREAM_INFRARED, RS_STREAM_INFRARED2, RS_STREAM_FISHEYE})
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
            if (key_stream == RS_STREAM_FISHEYE) // TODO: W/O until we will achieve frame timestamp
                continue;

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
        if (key_stream == RS_STREAM_FISHEYE) // TODO: W/O until we will achieve frame timestamp
            continue;

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
void syncronizing_archive::dequeue_frame(rs_stream stream)
{
    frontbuffer.place_frame(stream, std::move(frames[stream].front())); // the frame will move to free list once there are no external references to it
    frames[stream].erase(begin(frames[stream]));
}

// Move a single frame from the head of the queue directly to the freelist
void syncronizing_archive::discard_frame(rs_stream stream)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    freelist.push_back(std::move(frames[stream].front()));
    frames[stream].erase(begin(frames[stream]));    
}
