#include "timestamps.h"
#include "sync.h"
#include <algorithm>

using namespace rsimpl;
using namespace std;

void concurrent_queue::push_back_data(rs_timestamp_data data)
{
    lock_guard<mutex> lock(mtx);

    data_queue.push_back(data);
}

bool concurrent_queue::pop_front_data()
{
    lock_guard<mutex> lock(mtx);

    if (!data_queue.size())
        return false;

    data_queue.pop_front();

    return true;
}

bool concurrent_queue::erase(rs_timestamp_data data)
{
    lock_guard<mutex> lock(mtx);

    auto it = find_if(data_queue.begin(), data_queue.end(),
                      [&](const rs_timestamp_data& element) {
        return (data.frame_number == element.frame_number);
    });

    if (it != data_queue.end())
    {
        data_queue.erase(it);
        return true;
    }

    return false;
}

unsigned concurrent_queue::size()
{
    lock_guard<mutex> lock(mtx);

    return data_queue.size();
}

bool concurrent_queue::correct(rs_timestamp_data& data)
{
    lock_guard<mutex> lock(mtx);

    auto it = find_if(data_queue.begin(), data_queue.end(),
                      [&](const rs_timestamp_data& element) {
        return ((data.frame_number == element.frame_number) && (data.source_id == element.source_id));
    });

    if (it != data_queue.end())
    {
        data.timestamp = it->timestamp;
        return true;
    }

    return false;
}

timestamp_corrector::~timestamp_corrector()
{ }

void timestamp_corrector::on_timestamp(rs_timestamp_data data)
{
    lock_guard<mutex> lock(mtx);

    data_queue.push_back_data(data);
    if (data_queue.size() > 10)
        data_queue.pop_front_data();

    cv.notify_one();
}

void timestamp_corrector::update_timestamp_data(rs_timestamp_data& data, const frame_info& frame, const rs_stream stream)
{
    data.frame_number = frame.frameCounter;

    switch(stream)
    {
    case RS_STREAM_DEPTH:
    case RS_STREAM_COLOR:
    case RS_STREAM_INFRARED:
    case RS_STREAM_INFRARED2:
        data.source_id = RS_IMU_DEPTH_CAM;        
        break;
    case RS_STREAM_FISHEYE:
        data.source_id = RS_IMU_MOTION_CAM;        
        break;
    }
}

void timestamp_corrector::correct_timestamp(frame_info& frame, rs_stream stream)
{
    unique_lock<mutex> lock(mtx);

    rs_timestamp_data data;
    update_timestamp_data(data, frame, stream);
    if (!data_queue.correct(data))
    {
        const auto ready = [&]() { return data_queue.correct(data); };
        cv.wait_for(lock, std::chrono::milliseconds(1), ready);
    }

    lock.unlock();
}
