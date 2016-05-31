#pragma once
#ifndef LIBREALSENSE_TIMESTAMPS_H
#define LIBREALSENSE_TIMESTAMPS_H

#include "librealsense/rs.h"
#include "sync.h"
#include <deque>
#include <condition_variable>
#include <mutex>


namespace rsimpl
{
    class concurrent_queue{
    public:
        void push_data(rs_timestamp_data data)
        {
            std::lock_guard<std::mutex> lock(mtx);

            data_queue.push_back(data);
        }

        bool pop_data(rs_timestamp_data& data)
        {
            std::lock_guard<std::mutex> lock(mtx);

            if(try_pop(data))
                return true;
            else
                return false;
        }


    private:
        bool try_pop(rs_timestamp_data& data)
        {
            if (!data_queue.size())
                return false;

            data = data_queue[0];
            data_queue.pop_front();

            return true;
        }

        std::deque<rs_timestamp_data> data_queue;
        std::mutex mtx;

    };

    class correct_interface{
    public:
        virtual ~correct_interface() {}
        virtual void on_timestamp(rs_timestamp_data data) = 0;
        virtual int correct_timestamp(frame_info frame) = 0;
        virtual void release() = 0;
    };


    class correct : public correct_interface{
    public:
        ~correct() override;
        void on_timestamp(rs_timestamp_data data) override;
        int correct_timestamp(frame_info frame) override;
        void release() override  {delete this;}

    private:
        std::condition_variable cv;
    };



} // namespace rsimpl
#endif // LIBREALSENSE_TIMESTAMPS_H
