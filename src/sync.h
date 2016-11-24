// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_SYNC_H
#define LIBREALSENSE_SYNC_H

#include "archive.h"
#include <atomic>
#include "timestamps.h"
#include <chrono>

namespace rsimpl
{
    class fps_calc
    {
    public:
        fps_calc(unsigned long long in_number_of_frames_to_sampling, int expected_fps)
            : _number_of_frames_to_sample(in_number_of_frames_to_sampling),
            _frame_counter(0),
            _actual_fps(expected_fps)
        {
            _time_samples.reserve(2);
        }
        double calc_fps(std::chrono::time_point<std::chrono::system_clock>& now_time)
        {
            ++_frame_counter;
            if (_frame_counter == _number_of_frames_to_sample || _frame_counter == 1)
            {
                _time_samples.push_back(now_time);
                if (_time_samples.size() == 2)
                {
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        _time_samples[1] - _time_samples[0]).count();
                    _actual_fps = (((double)_frame_counter - 1.) / duration) * 1000.;
                    _frame_counter = 0;
                    _time_samples.clear();
                }
            }

            return _actual_fps;
        }

    private:
        double _actual_fps;
        unsigned long long _number_of_frames_to_sample;
        unsigned long long _frame_counter;
        std::vector<std::chrono::time_point<std::chrono::system_clock>> _time_samples;
    };

    class syncronizing_archive : public frame_archive
    {
    private:
        // This data will be left constant after creation, and accessed from all threads
        subdevice_mode_selection modes[RS_STREAM_NATIVE_COUNT];
        rs_stream key_stream;
        std::vector<rs_stream> other_streams;

        // This data will be read and written exclusively from the application thread
        frameset frontbuffer;

        // This data will be read and written by all threads, and synchronized with a mutex
        std::vector<frame> frames[RS_STREAM_NATIVE_COUNT];
        std::condition_variable_any cv;
        
        void get_next_frames();
        void dequeue_frame(rs_stream stream);
        void discard_frame(rs_stream stream);
        void cull_frames();

        timestamp_corrector            ts_corrector;
    public:
        syncronizing_archive(const std::vector<subdevice_mode_selection> & selection, 
            rs_stream key_stream, 
            std::atomic<uint32_t>* max_size,
            std::atomic<uint32_t>* event_queue_size,
            std::atomic<uint32_t>* events_timeout,
            std::chrono::high_resolution_clock::time_point capture_started = std::chrono::high_resolution_clock::now());
        
        // Application thread API
        void wait_for_frames();
        bool poll_for_frames();

        frameset * wait_for_frames_safe();
        bool poll_for_frames_safe(frameset ** frames);

        double get_frame_metadata(rs_stream stream, rs_frame_metadata frame_metadata) const;
        bool supports_frame_metadata(rs_stream stream, rs_frame_metadata frame_metadata) const;
        const byte * get_frame_data(rs_stream stream) const;
        double get_frame_timestamp(rs_stream stream) const;
        unsigned long long get_frame_number(rs_stream stream) const;
        long long get_frame_system_time(rs_stream stream) const;
        int get_frame_stride(rs_stream stream) const;
        int get_frame_bpp(rs_stream stream) const;

        frameset * clone_frontbuffer();

        // Frame callback thread API
        void commit_frame(rs_stream stream);

        void flush() override;

        void correct_timestamp(rs_stream stream);
        void on_timestamp(rs_timestamp_data data);

    };
}

#endif
