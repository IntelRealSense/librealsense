#pragma once

#include "sensor.h"
#include "device.h"
#include "error-handling.h"
#include <deque>

namespace librealsense
{
    class stdev
    {
    public:
        stdev(unsigned int buffer_size, double default_std);
        void add_value(double val);
        double get_std();

    private:
        unsigned int _buffer_size;
        std::deque<double> _last_values;
        double  _sum;
        double _sumsq;
        double _default_std;
    };

    class time_diff_keeper
    {
    public:
        explicit time_diff_keeper(device* dev, const std::string& name);
        void start();   // must be called AFTER ALL initializations of _hw_monitor.
        ~time_diff_keeper();
        double get_system_hw_time_diff(double crnt_hw_time);

    private:
        bool update_diff_time();
        void polling(dispatcher::cancellable_timer cancellable_timer);

    private:
        device* _device;
        std::string _name;
        double _system_hw_time_diff;
        double _last_sample_hw_time;
        unsigned int _poll_intervals_ms;
        active_object<> _active_object;
        mutable std::recursive_mutex _mtx;
        bool   _skipTimestampCorrection;
        stdev _stdev;
    };

    class global_timestamp_reader : public frame_timestamp_reader
    {
    public:
        global_timestamp_reader(std::unique_ptr<frame_timestamp_reader> device_timestamp_reader, std::shared_ptr<time_diff_keeper> timediff);

        rs2_time_t get_frame_timestamp(const request_mapping& mode, const platform::frame_object& fo) override;
        unsigned long long get_frame_counter(const request_mapping& mode, const platform::frame_object& fo) const override;
        rs2_timestamp_domain get_frame_timestamp_domain(const request_mapping & mode, const platform::frame_object& fo) const override;
        void reset() override;

    private:
        std::unique_ptr<frame_timestamp_reader> _device_timestamp_reader;
        std::weak_ptr<time_diff_keeper> _time_diff_keeper;
        mutable std::recursive_mutex _mtx;
    };
}
