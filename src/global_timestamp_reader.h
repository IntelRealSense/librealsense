#pragma once

#include "sensor.h"
#include "device.h"
#include "error-handling.h"

namespace librealsense
{
    class time_diff_keeper
    {
    public:
        explicit time_diff_keeper(device* dev);
        ~time_diff_keeper();
        double get_system_hw_time_diff();

    private:
        bool update_diff_time();
        void polling(dispatcher::cancellable_timer cancellable_timer);

    private:
        device* _device;
        double _system_hw_time_diff;
        unsigned int _poll_intervals_ms;
        active_object<> _active_object;
        mutable std::recursive_mutex _mtx;
        bool   _skipTimestampCorrection;
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
        std::shared_ptr<time_diff_keeper> _time_diff_keeper;
        mutable std::recursive_mutex _mtx;
    };
}
