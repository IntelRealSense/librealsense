#pragma once
#ifndef LIBREALSENSE_TIMESTAMPS_H
#define LIBREALSENSE_TIMESTAMPS_H

#include "librealsense/rs.h"
#include <deque>
#include <condition_variable>
#include <mutex>


namespace rsimpl
{
    struct frame_info;

    class concurrent_queue{
    public:
        void push_back_data(rs_timestamp_data data);
        bool pop_front_data();
        bool erase(rs_timestamp_data data);
        bool update_timestamp(int frame_number, rs_timestamp_data& data);
        unsigned size();

    private:
        std::deque<rs_timestamp_data> data_queue;
        std::mutex mtx;

    };

    class correct_interface{
    public:
        virtual ~correct_interface() {}
        virtual void on_timestamp(rs_timestamp_data data) = 0;
        virtual void correct_timestamp(frame_info& frame) = 0;
        virtual void release() = 0;
    };


    class correct : public correct_interface{
    public:
        ~correct() override;
        void on_timestamp(rs_timestamp_data data) override;
        void correct_timestamp(frame_info& frame) override;
        void release() override  {delete this;}

    private:
        std::mutex mtx;
        concurrent_queue data_queue;
        std::condition_variable cv;
    };



} // namespace rsimpl
#endif // LIBREALSENSE_TIMESTAMPS_H
