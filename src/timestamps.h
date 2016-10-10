#pragma once
#ifndef LIBREALSENSE_TIMESTAMPS_H
#define LIBREALSENSE_TIMESTAMPS_H

#include "../include/librealsense/rs.h"     // Inherit all type definitions in the public API
#include <deque>
#include <condition_variable>
#include <mutex>
#include <atomic>


namespace rsimpl
{
    struct frame_interface
    { 
        virtual ~frame_interface() {}
        virtual double get_frame_metadata(rs_frame_metadata frame_metadata) const = 0;
        virtual bool supports_frame_metadata(rs_frame_metadata frame_metadata) const = 0;
        virtual unsigned long long get_frame_number() const = 0;
        virtual void set_timestamp(double new_ts) = 0;
        virtual void set_timestamp_domain(rs_timestamp_domain timestamp_domain) = 0;
        virtual rs_stream get_stream_type()const = 0;
    };


    class concurrent_queue{
    public:
        void    push_back_data(rs_timestamp_data data);
        bool    pop_front_data();
        bool    erase(rs_timestamp_data data);
        bool    correct(frame_interface& frame);
        size_t  size();

        private:
        std::deque<rs_timestamp_data> data_queue;
        std::mutex mtx;

    };

    class timestamp_corrector_interface{
    public:
        virtual ~timestamp_corrector_interface() {}
        virtual void on_timestamp(rs_timestamp_data data) = 0;
        virtual void correct_timestamp(frame_interface& frame, rs_stream stream) = 0;
        virtual void release() = 0;
    };


    class timestamp_corrector : public timestamp_corrector_interface{
    public:
        timestamp_corrector(std::atomic<uint32_t>* event_queue_size, std::atomic<uint32_t>* events_timeout);
        ~timestamp_corrector() override;
        void on_timestamp(rs_timestamp_data data) override;
        void correct_timestamp(frame_interface& frame, rs_stream stream) override;
        void release() override  {delete this;}

    private:
        void update_source_id(rs_event_source& source_id, const rs_stream stream);

        std::mutex mtx;
        concurrent_queue data_queue[RS_EVENT_SOURCE_COUNT];
        std::condition_variable cv;
        std::atomic<uint32_t>* event_queue_size;
        std::atomic<uint32_t>* events_timeout;

    };



} // namespace rsimpl
#endif // LIBREALSENSE_TIMESTAMPS_H
