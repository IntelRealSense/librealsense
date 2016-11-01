#include "timestamps.h"
#include "sync.h"
#include <algorithm>
#include <iostream>
#include <unistd.h>
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

size_t concurrent_queue::size()
{
    lock_guard<mutex> lock(mtx);

    return data_queue.size();
}

bool concurrent_queue::correct( frame_interface& frame)
{
    lock_guard<mutex> lock(mtx);
   // mtx.lock();
    
   // std::cout << data_queue.size() << std::endl;
    auto it = find_if(data_queue.begin(), data_queue.end(),
                      [&](const rs_timestamp_data& element) {
        return ((frame.get_frame_number() == element.frame_number));
    });

    if (it != data_queue.end())
    {
        frame.set_timestamp(it->timestamp);
       // mtx.unlock();
        return true;
    }
    //mtx.unlock();
    return false;
}

timestamp_corrector::timestamp_corrector(std::atomic<uint32_t>* queue_size, std::atomic<uint32_t>* timeout)
    :event_queue_size(queue_size), events_timeout(timeout)
{
}

timestamp_corrector::~timestamp_corrector()
{
}

void timestamp_corrector::on_timestamp(rs_timestamp_data data)
{
   // unique_lock<mutex> lock(mtx);
    //mtx.lock();
    if (data_queue[data.source_id].size() <= *event_queue_size)
        data_queue[data.source_id].push_back_data(data);
    if (data_queue[data.source_id].size() > *event_queue_size)
        data_queue[data.source_id].pop_front_data();
    // std::cout << "received: " << data.frame_number << std::endl;
   // lock.unlock();
   // cv.notify_one();
  // mtx.unlock();
}

void timestamp_corrector::update_source_id(rs_event_source& source_id, const rs_stream stream)
{
    switch(stream)
    {
    case RS_STREAM_DEPTH:
    case RS_STREAM_COLOR:
    case RS_STREAM_INFRARED:
    case RS_STREAM_INFRARED2:
        source_id = RS_EVENT_IMU_DEPTH_CAM;
        break;
    case RS_STREAM_FISHEYE:
        source_id = RS_EVENT_IMU_MOTION_CAM;
        break;
    default:
        throw std::runtime_error(to_string() << "Unsupported source stream requested " << rs_stream_to_string(stream));
    }
}

bool timestamp_corrector::correct_timestamp(frame_interface& frame, rs_stream stream)
{


    bool res;
    rs_event_source source_id;
    update_source_id(source_id, stream);


    /*if (!(res = data_queue[source_id].correct(frame)))
    {
        const auto ready = [&]() { res =  data_queue[source_id].correct(frame); };
        res = cv.wait_for(lock, std::chrono::milliseconds((*events_timeout)), ready);
        cv.wait_for(lock,chrono::duration::milliseconds(*event_timeout));
        cout << "got new res" << std::endl;

    }*/

    auto start_time = std::chrono::high_resolution_clock::now();
    while (!res && std::chrono::duration_cast<chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count() < (*events_timeout)) {
       // unique_lock<mutex> lock(mtx);
       // mtx.lock();
       // std::cout << "Searching for: " << frame.get_frame_number() << std::endl;
        res = data_queue[source_id].correct(frame);
        //cv.wait_for(lock,std::chrono::milliseconds((*events_timeout)));
        //mtx.unlock();
       // usleep(1000);
    }

    if (res)
    {
        frame.set_timestamp_domain(RS_TIMESTAMP_DOMAIN_MICROCONTROLLER);
    }
    return res;
}
