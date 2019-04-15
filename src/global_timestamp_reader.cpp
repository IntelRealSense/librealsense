#include "global_timestamp_reader.h"
#include <chrono>

namespace librealsense
{
    stdev::stdev(unsigned int buffer_size, double default_std):
        _buffer_size(buffer_size),
        _default_std(default_std)
    {}

    void stdev::add_value(double val)
    {
        while (_last_values.size() > _buffer_size)
        {
            double old_val = _last_values.back();
            _sum -= old_val;
            _sumsq -= (old_val * old_val);
            _last_values.pop_back();
        }
        _sum += val;
        _sumsq += (val * val);
        _last_values.push_front(val);
    }

    double stdev::get_std()
    {
        size_t n(_last_values.size());
        if (n < _buffer_size) return _default_std;
        return  (_sumsq - (_sum * _sum) / n) / (n - 1);
    }


    time_diff_keeper::time_diff_keeper(device* dev) :
        _device(dev),
        _poll_intervals_ms(100),
        _system_hw_time_diff(-1e+200),
        _last_sample_hw_time(1e+200),
        _skipTimestampCorrection(false),
        _stdev(15, 5.8),
        _active_object([this](dispatcher::cancellable_timer cancellable_timer)
            {
                LOG_INFO("start new time_diff_keeper");
                polling(cancellable_timer);
            })
        {
            _active_object.start();
        };

    time_diff_keeper::~time_diff_keeper()
    {
        _active_object.stop();
    }

    bool time_diff_keeper::update_diff_time()
    {
        const static double diffThresh = 500;
        static std::ofstream fout("times.txt");
        try
        {
            _last_sample_hw_time = _device->get_device_time();
            double system_time = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

            double diff = system_time - _last_sample_hw_time;
            if (fabs(diff - _system_hw_time_diff) > diffThresh)
            {
                //Giant time leap: It happens on initialize 
                LOG_INFO("Timestamp offset : " << diff);
                _system_hw_time_diff = diff;

                //In case of using system time. Time correction is useless.
                if (fabs(diff)<500) {
                    LOG_INFO("Timecorrection is not needed use raw timestamp");
                    _skipTimestampCorrection = true;
                }
            }
            else
            {
                _stdev.add_value(diff - _system_hw_time_diff);
                double iirWeight = 0.05;
                // Transfer lag distribution is asynmmetry. Sometimes big latancy are happens. To reduce the affect. 
                if (diff > _system_hw_time_diff) 
                {
                    iirWeight *= 0.1;
                    std::cout << "get_std() = " << std::fixed << _stdev.get_std() << std::endl;
                    if (diff - _system_hw_time_diff > _stdev.get_std())
                        iirWeight = 0;
                }
                _system_hw_time_diff = (1.0 - iirWeight) * _system_hw_time_diff + iirWeight * diff;
            }
            fout << std::fixed << diff << " , " << _system_hw_time_diff << std::endl;
            return true;
        }
        catch (const wrong_api_call_sequence_exception& ex)
        {
            LOG_DEBUG("Error during time_diff_keeper polling: " << ex.what());
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
        if (cancellable_timer.try_sleep(_poll_intervals_ms))
        {
            if (_skipTimestampCorrection)
                return;
            update_diff_time();
        }
        else
        {
            LOG_DEBUG("Notification: time_diff_keeper polling loop is being shut-down");
        }
    }

    double time_diff_keeper::get_system_hw_time_diff(double crnt_hw_time)
    {
        static const double possible_loop_time(3000);
        while ((_last_sample_hw_time - crnt_hw_time) > possible_loop_time)
        {
            std::cout << "***** NOT initialized **** " << std::endl;
            std::cout << _last_sample_hw_time << " > " << crnt_hw_time << std::endl;
            update_diff_time();
        }
        return _system_hw_time_diff;
    }

    global_timestamp_reader::global_timestamp_reader(std::unique_ptr<frame_timestamp_reader> device_timestamp_reader, std::shared_ptr<time_diff_keeper> timediff) :
        _device_timestamp_reader(std::move(device_timestamp_reader)),
        _time_diff_keeper(timediff)
    {
    }

    double global_timestamp_reader::get_frame_timestamp(const request_mapping& mode, const platform::frame_object& fo)
    {

        double frame_time = _device_timestamp_reader->get_frame_timestamp(mode, fo);
        rs2_timestamp_domain ts_domain = _device_timestamp_reader->get_frame_timestamp_domain(mode, fo);
        if (ts_domain == RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK)
        {
            frame_time += _time_diff_keeper->get_system_hw_time_diff(frame_time);
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
        return (ts_domain == RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK) ? RS2_TIMESTAMP_DOMAIN_HARDWARE_SYSTEM_TIME : ts_domain;
    }

    void global_timestamp_reader::reset()
    {
        _device_timestamp_reader->reset();
    }


}
