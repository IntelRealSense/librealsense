// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "sensor.h"
#include "device.h"
#include "error-handling.h"
#include <deque>

namespace librealsense
{
    class LRS_EXTENSION_API global_time_option : public bool_option
    {
    public:
        global_time_option() {}
        // TODO: expose this outwards
        const char* get_description() const override { return "Enable/Disable global timestamp."; }
    };

    class CSample
    {
    public:
        CSample(double x, double y) :
            _x(x), _y(y) {};
        CSample& operator-=(const CSample& other);
        CSample& operator+=(const CSample& other);

    public:
        double _x;
        double _y;
    };

    class CLinearCoefficients
    {
    public:
        CLinearCoefficients(unsigned int buffer_size);
        void reset();
        void add_value(CSample val);
        double calc_value(double x) const;
        bool is_full() const;

    private:
        void calc_linear_coefs();

    private:
        unsigned int _buffer_size;
        std::deque<CSample> _last_values;
        double _b, _a;    //Linear regression coeffitions.
        CSample _base_sample;
        mutable std::recursive_mutex _add_mtx;
        mutable std::recursive_mutex _stat_mtx;
    };

    class time_diff_keeper
    {
    public:
        explicit time_diff_keeper(device* dev, const unsigned int sampling_interval_ms);
        void start();   // must be called AFTER ALL initializations of _hw_monitor.
        void stop();
        ~time_diff_keeper();
        double get_system_hw_time(double crnt_hw_time);

    private:
        bool update_diff_time();
        void polling(dispatcher::cancellable_timer cancellable_timer);

    private:
        device* _device;
        double _last_sample_hw_time;
        unsigned int _poll_intervals_ms;
        std::atomic<unsigned int> _users_count;
        active_object<> _active_object;
        mutable std::recursive_mutex _mtx;      // Watch the update process
        mutable std::recursive_mutex _read_mtx; // Watch only 1 reader at a time.
        CLinearCoefficients _coefs;
    };

    class global_timestamp_reader : public frame_timestamp_reader
    {
    public:
        global_timestamp_reader(std::unique_ptr<frame_timestamp_reader> device_timestamp_reader, 
                                std::shared_ptr<time_diff_keeper> timediff,
                                std::shared_ptr<global_time_option>);

        rs2_time_t get_frame_timestamp(const request_mapping& mode, const platform::frame_object& fo) override;
        unsigned long long get_frame_counter(const request_mapping& mode, const platform::frame_object& fo) const override;
        rs2_timestamp_domain get_frame_timestamp_domain(const request_mapping & mode, const platform::frame_object& fo) const override;
        void reset() override;

    private:
        std::unique_ptr<frame_timestamp_reader> _device_timestamp_reader;
        std::weak_ptr<time_diff_keeper> _time_diff_keeper;
        mutable std::recursive_mutex _mtx;
        std::shared_ptr<global_time_option> _option_is_enabled;
    };
}
