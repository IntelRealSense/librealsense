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
        //LOG_DEBUG("CLinearCoefficients started");
    }

    void CLinearCoefficients::reset()
    {
        _last_values.clear();
        //LOG_DEBUG("CLinearCoefficients::reset");
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
        std::lock_guard<std::recursive_mutex> lock(_add_mtx);   // Redandent as only being read from update_diff_time() and there is a lock there.
        while (_last_values.size() > _buffer_size)
        {
            _last_values.pop_back();
        }
        _last_values.push_front(val);
        calc_linear_coefs();
    }

    void CLinearCoefficients::calc_linear_coefs()
    {
        // Calculate linear coefficients, based on calculus described in: https://www.statisticshowto.datasciencecentral.com/probability-and-statistics/regression-analysis/find-a-linear-regression-equation/
        // Calculate Std 
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
        _base_sample = base_sample;
        _a = a;
        _b = b;
    }

    double CLinearCoefficients::calc_value(double x) const
    {
        std::lock_guard<std::recursive_mutex> lock(_stat_mtx);
        double y(_a * (x - _base_sample._x) + _b + _base_sample._y);
        return y;
    }

    time_diff_keeper::time_diff_keeper(global_time_interface* dev, const unsigned int sampling_interval_ms) :
        _device(dev),
        _poll_intervals_ms(sampling_interval_ms),
        _last_sample_hw_time(1e+200),
        _coefs(15),
        _users_count(0),
        _is_ready(false),
        _active_object([this](dispatcher::cancellable_timer cancellable_timer)
            {
                polling(cancellable_timer);
            })
    {
        //LOG_DEBUG("start new time_diff_keeper ");
    }

    void time_diff_keeper::start()
    {
        std::lock_guard<std::recursive_mutex> lock(_enable_mtx);
        _users_count++;
        LOG_DEBUG("time_diff_keeper::start: _users_count = " << _users_count);
        _active_object.start();
    }

    void time_diff_keeper::stop()
    {
        std::lock_guard<std::recursive_mutex> lock(_enable_mtx);
        if (_users_count <= 0)
            LOG_ERROR("time_diff_keeper users_count <= 0.");

        _users_count--;
        LOG_DEBUG("time_diff_keeper::stop: _users_count = " << _users_count);
        if (_users_count == 0)
        {
            LOG_DEBUG("time_diff_keeper::stop: stop object.");
            _active_object.stop();
            _coefs.reset();
        }
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
            if (!_users_count)
                throw wrong_api_call_sequence_exception("time_diff_keeper::update_diff_time called before object started.");
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            double system_time_start = duration<double, std::milli>(system_clock::now().time_since_epoch()).count();

            double sample_hw_time = _device->get_device_time_ms();
            double system_time_finish = duration<double, std::milli>(system_clock::now().time_since_epoch()).count();
            double system_time((system_time_finish + system_time_start) / 2);
            if (sample_hw_time < _last_sample_hw_time)
            {
                // A time loop happend:
                //LOG_DEBUG("time_diff_keeper::call reset()");
                _coefs.reset();
            }
            _last_sample_hw_time = sample_hw_time;
            CSample crnt_sample(_last_sample_hw_time, system_time);
            _coefs.add_value(crnt_sample);
            _is_ready = true;
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

    double time_diff_keeper::get_system_hw_time(double crnt_hw_time, bool& is_ready)
    {
        static const double possible_loop_time(3000);
        {
            std::lock_guard<std::recursive_mutex> lock(_read_mtx);
            if ((_last_sample_hw_time - crnt_hw_time) > possible_loop_time)
            {
                update_diff_time();
            }
        }
        is_ready = _is_ready;
        if (_is_ready)
            return _coefs.calc_value(crnt_hw_time);
        else        
            return crnt_hw_time;
    }

    global_timestamp_reader::global_timestamp_reader(std::unique_ptr<frame_timestamp_reader> device_timestamp_reader, 
                                                     std::shared_ptr<time_diff_keeper> timediff,
                                                     std::shared_ptr<global_time_option> enable_option) :
        _device_timestamp_reader(std::move(device_timestamp_reader)),
        _time_diff_keeper(timediff),
        _option_is_enabled(enable_option),
        _ts_is_ready(false)
    {
    }

    double global_timestamp_reader::get_frame_timestamp(const request_mapping& mode, const platform::frame_object& fo)
    {
        double frame_time = _device_timestamp_reader->get_frame_timestamp(mode, fo);
        rs2_timestamp_domain ts_domain = _device_timestamp_reader->get_frame_timestamp_domain(mode, fo);
        if (_option_is_enabled->is_true() && ts_domain == RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK)
        {
            auto sp = _time_diff_keeper.lock();
            if (sp)
                frame_time = sp->get_system_hw_time(frame_time, _ts_is_ready);
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
        return (_option_is_enabled->is_true() && _ts_is_ready && ts_domain == RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK) ? RS2_TIMESTAMP_DOMAIN_GLOBAL_TIME : ts_domain;
    }

    void global_timestamp_reader::reset()
    {
        _device_timestamp_reader->reset();
    }

    global_time_interface::global_time_interface() :
        _tf_keeper(std::make_shared<time_diff_keeper>(this, 100))
    {}

    void global_time_interface::enable_time_diff_keeper(bool is_enable)
    {
        if (is_enable)
            _tf_keeper->start();
        else
            _tf_keeper->stop();
    }
}
