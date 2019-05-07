// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#include "global_timestamp_reader.h"
#include <chrono>

namespace librealsense
{
    CLinearCoefficients::CLinearCoefficients(unsigned int buffer_size) :
        _base_sample(0, 0),
        _buffer_size(buffer_size)
    {
        LOG_DEBUG("CLinearCoefficients started");
    }

    void CLinearCoefficients::reset()
    {
        _last_values.clear();
        LOG_DEBUG("CLinearCoefficients::reset");
    }

    CSample& CSample::operator-=(const CSample& other)
    {
        _x -= other._x;
        _y -= other._y;
        return *this;
    }

    CSample& CSample::operator+=(const CSample& other)
    {
        _x += other._x;
        _y += other._y;
        return *this;
    }

    bool CLinearCoefficients::is_full() const 
    {
        return _last_values.size() >= _buffer_size;
    }

    void CLinearCoefficients::add_value(CSample val)
    {
        LOG_DEBUG("CLinearCoefficients::add_value - in");
        std::lock_guard<std::recursive_mutex> lock(_add_mtx);   // Redandent as only being read from update_diff_time() and there is a lock there.
        LOG_DEBUG("CLinearCoefficients::add_value - lock");
        while (_last_values.size() > _buffer_size)
        {
            _last_values.pop_back();
        }
        _last_values.push_front(val);
        calc_linear_coefs();
        LOG_DEBUG("CLinearCoefficients::add_value - unlock");
    }

    void CLinearCoefficients::calc_linear_coefs()
    {
        // Calculate linear coefficients, based on calculus described in: https://www.statisticshowto.datasciencecentral.com/probability-and-statistics/regression-analysis/find-a-linear-regression-equation/
        // Calculate Std 
        LOG_DEBUG("CLinearCoefficients::calc_linear_coefs - in");
        double sum_x(0);
        double sum_y(0);
        double sum_xy(0);
        double sum_x2(0);
        double n(static_cast<double>(_last_values.size()));
        CSample base_sample = _last_values.back();
        double a(1);
        double b(0);
        if (n > 1)
        {
            for (auto sample = _last_values.begin(); sample != _last_values.end(); sample++)
            {
                CSample crnt_sample(*sample);
                crnt_sample -= base_sample;
                sum_x += crnt_sample._x;
                sum_y += crnt_sample._y;
                sum_xy += (crnt_sample._x * crnt_sample._y);
                sum_x2 += (crnt_sample._x * crnt_sample._x);
            }
            b = (sum_y*sum_x2 - sum_x * sum_xy) / (n*sum_x2 - sum_x * sum_x);
            a = (n*sum_xy - sum_x * sum_y) / (n*sum_x2 - sum_x * sum_x);
        }
        std::lock_guard<std::recursive_mutex> lock(_stat_mtx);
        LOG_DEBUG("CLinearCoefficients::calc_linear_coefs - lock");
        _base_sample = base_sample;
        _a = a;
        _b = b;
        LOG_DEBUG("CLinearCoefficients::calc_linear_coefs - unlock");
    }

    double CLinearCoefficients::calc_value(double x) const
    {
        LOG_DEBUG("CLinearCoefficients::calc_value - in");
        std::lock_guard<std::recursive_mutex> lock(_stat_mtx);
        LOG_DEBUG("CLinearCoefficients::calc_value - lock");
        double y(_a * (x - _base_sample._x) + _b + _base_sample._y);
        LOG_DEBUG("CLinearCoefficients::calc_value - unlock");
        return y;
    }

    time_diff_keeper::time_diff_keeper(device* dev, const unsigned int sampling_interval_ms) :
        _device(dev),
        _poll_intervals_ms(sampling_interval_ms),
        _last_sample_hw_time(1e+200),
        _coefs(15),
        _active_object([this](dispatcher::cancellable_timer cancellable_timer)
            {
                polling(cancellable_timer);
            })
    {
        LOG_DEBUG("start new time_diff_keeper ");
    }

    void time_diff_keeper::start()
    {
        _active_object.start();
    }

    time_diff_keeper::~time_diff_keeper()
    {
        _active_object.stop();
    }

    bool time_diff_keeper::update_diff_time()
    {
        using namespace std::chrono;
        try
        {
            LOG_DEBUG("time_diff_keeper::update_diff_time - in");
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            LOG_DEBUG("time_diff_keeper::update_diff_time - lock");
            double system_time = static_cast<double>(duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count()) * TIMESTAMP_USEC_TO_MSEC * TIMESTAMP_USEC_TO_MSEC;
            double sample_hw_time = _device->get_device_time();
            if (sample_hw_time < _last_sample_hw_time)
            {
                // A time loop happend:
                LOG_DEBUG("time_diff_keeper::call reset()");
                _coefs.reset();
            }
            _last_sample_hw_time = sample_hw_time;
            CSample crnt_sample(_last_sample_hw_time, system_time);
            _coefs.add_value(crnt_sample);
            LOG_DEBUG("time_diff_keeper::update_diff_time - unlock");
            return true;
        }
        catch (const wrong_api_call_sequence_exception& ex)
        {
            LOG_DEBUG("Temporary skip during time_diff_keeper polling: " << ex.what());
        }
        catch (const std::exception& ex)
        {
            LOG_ERROR("Error during time_diff_keeper polling: " << ex.what());
        }
        catch (...)
        {
            LOG_ERROR("Unknown error during time_diff_keeper polling!");
        }
        return false;
    }

    void time_diff_keeper::polling(dispatcher::cancellable_timer cancellable_timer)
    {
        unsigned int time_to_sleep = _poll_intervals_ms + _coefs.is_full() * (9 * _poll_intervals_ms);
        if (cancellable_timer.try_sleep(time_to_sleep))
        {
            update_diff_time();
        }
        else
        {
            LOG_DEBUG("Notification: time_diff_keeper polling loop is being shut-down");
        }
    }

    double time_diff_keeper::get_system_hw_time(double crnt_hw_time)
    {
        static const double possible_loop_time(3000);
        {
            LOG_DEBUG("time_diff_keeper::get_system_hw_time - in");
            std::lock_guard<std::recursive_mutex> lock(_read_mtx);
            LOG_DEBUG("time_diff_keeper::get_system_hw_time - lock");
            if ((_last_sample_hw_time - crnt_hw_time) > possible_loop_time)
            {
                update_diff_time();
            }
            LOG_DEBUG("time_diff_keeper::get_system_hw_time - unlock");
        }
        return _coefs.calc_value(crnt_hw_time);
    }

    global_timestamp_reader::global_timestamp_reader(std::unique_ptr<frame_timestamp_reader> device_timestamp_reader, std::shared_ptr<time_diff_keeper> timediff) :
        _device_timestamp_reader(std::move(device_timestamp_reader)),
        _time_diff_keeper(timediff)
    {
        LOG_DEBUG("global_timestamp_reader created");
    }

    double global_timestamp_reader::get_frame_timestamp(const request_mapping& mode, const platform::frame_object& fo)
    {
        double frame_time = _device_timestamp_reader->get_frame_timestamp(mode, fo);
        rs2_timestamp_domain ts_domain = _device_timestamp_reader->get_frame_timestamp_domain(mode, fo);
        if (ts_domain == RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK)
        {
            auto sp = _time_diff_keeper.lock();
            if (sp)
                frame_time = sp->get_system_hw_time(frame_time);
            else
                LOG_DEBUG("Notification: global_timestamp_reader - time_diff_keeper is being shut-down");
        }
        return frame_time;
    }


    unsigned long long global_timestamp_reader::get_frame_counter(const request_mapping& mode, const platform::frame_object& fo) const
    {
        return _device_timestamp_reader->get_frame_counter(mode, fo);
    }

    rs2_timestamp_domain global_timestamp_reader::get_frame_timestamp_domain(const request_mapping& mode, const platform::frame_object& fo) const
    {
        rs2_timestamp_domain ts_domain = _device_timestamp_reader->get_frame_timestamp_domain(mode, fo);
        return (ts_domain == RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK) ? RS2_TIMESTAMP_DOMAIN_GLOBAL_TIME : ts_domain;
    }

    void global_timestamp_reader::reset()
    {
        _device_timestamp_reader->reset();
    }


}
