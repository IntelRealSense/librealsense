#include "global_timestamp_reader.h"
#include <chrono>

namespace librealsense
{
    time_diff_keeper::time_diff_keeper(device* dev) :
        _device(dev),
        _poll_intervals_ms(500),
        _system_hw_time_diff(-1e+200),
        _skipTimestampCorrection(false),
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
        try
        {
            double hw_time_now = _device->get_device_time();
            double system_time = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

            double diff = system_time - hw_time_now;
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
                double iirWeight = 0.05;
                // Transfer lag distribution is asynmmetry. Sometimes big latancy are happens. To reduce the affect. 
                if (diff > _system_hw_time_diff) {
                    iirWeight *= 0.1;
                }
                _system_hw_time_diff = (1.0 - iirWeight) * _system_hw_time_diff + iirWeight * diff;
            }
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

    double time_diff_keeper::get_system_hw_time_diff()
    {
        //std::cout << "_system_hw_time_diff: " << std::setprecision(20) << _system_hw_time_diff << std::endl;
        while (_system_hw_time_diff < 0)
        {
            std::cout << "***** NOT initialized **** " << std::endl;
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
            frame_time += _time_diff_keeper->get_system_hw_time_diff();
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
