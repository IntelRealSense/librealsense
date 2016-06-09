#pragma once
#ifndef LIBREALSENSE_TIMESTAMPS_H
#define LIBREALSENSE_TIMESTAMPS_H

#include "../include/librealsense/rs.h"     // Inherit all type definitions in the public API
#include <deque>
#include <condition_variable>
#include <mutex>


namespace rsimpl
{
    struct frame_interface
    { 
        virtual ~frame_interface() {}
        virtual int get_frame_number() const = 0;
        virtual void set_timestamp(int new_ts) = 0;
        virtual rs_stream get_stream_type()const = 0;
    };

    class concurrent_queue{
    public:
        void push_back_data(rs_timestamp_data data);
        bool pop_front_data();
        bool erase(rs_timestamp_data data);
        bool correct(const rs_event_source& source_id, frame_interface& frame);
        unsigned size();

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
        ~timestamp_corrector() override;
        void on_timestamp(rs_timestamp_data data) override;
        void correct_timestamp(frame_interface& frame, rs_stream stream) override;
        void release() override  {delete this;}

    private:
        void update_source_id(rs_event_source& source_id, const rs_stream stream);

        std::mutex mtx;
        concurrent_queue data_queue;
        std::condition_variable cv;
    };



} // namespace rsimpl
#endif // LIBREALSENSE_TIMESTAMPS_H
