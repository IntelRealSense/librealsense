// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>
#include "record_device.h"

rsimpl2::record_device::record_device(std::shared_ptr<rsimpl2::device_interface> device,
                                      std::shared_ptr<rsimpl2::device_serializer> serializer):
    m_write_thread([](){return std::make_shared<dispatcher>(std::numeric_limits<unsigned int>::max());}),
    m_is_first_event(true),
    m_is_recording(true),
    m_record_pause_time(0),
    m_device(device),
    m_writer(serializer)
{
    if (device == nullptr)
    {
        throw std::invalid_argument("device");
    }

    if (serializer == nullptr)
    {
        throw std::invalid_argument("serializer");
    }

    serializer->reset();

    for (size_t i = 0; i < m_device->get_sensors_count(); i++)
    {
        auto& sensor = m_device->get_sensor(i);

        auto recording_sensor = std::make_shared<record_sensor>(sensor,
                                                                [this, i](std::shared_ptr<frame_interface> f)
                                                                {
                                                                    std::call_once(m_first_call_flag, [this]()
                                                                    {
                                                                        m_capture_time_base = std::chrono::high_resolution_clock::now();
                                                                        m_cached_data_size = 0;
                                                                    });

                                                                    write_data(i, f);
                                                                });
        m_sensors.push_back(recording_sensor);
    }
}

rsimpl2::record_device::~record_device()
{
    (*m_write_thread)->stop();
}
rsimpl2::sensor_interface& rsimpl2::record_device::get_sensor(size_t i)
{
    return *(m_sensors[i]);
}
size_t rsimpl2::record_device::get_sensors_count() const
{
    return m_sensors.size();
}

void rsimpl2::record_device::write_header()
{
    throw not_implemented_exception(__FUNCTION__);
}

std::chrono::nanoseconds rsimpl2::record_device::get_capture_time()
{
    auto now = std::chrono::high_resolution_clock::now();
    return (now - m_capture_time_base) - m_record_pause_time;
}

void rsimpl2::record_device::write_data(size_t sensor_index, std::shared_ptr<frame_interface> f)
{
    uint64_t data_size = f->get_data_size();
    uint64_t cached_data_size = m_cached_data_size + data_size;
    if (cached_data_size > MAX_CACHED_DATA_SIZE)
    {
        LOG_ERROR("frame drop occurred");
        return;
    }

    m_cached_data_size = cached_data_size;
    auto capture_time = get_capture_time();

    (*m_write_thread)->invoke([this, sensor_index, capture_time, f, data_size](dispatcher::cancellable_timer t)
                                 {
                                     if(m_is_recording == false)
                                     {
                                         return;
                                     }

                                     if(m_is_first_event)
                                     {
                                         try
                                         {
                                             write_header();
                                             m_writer->write({capture_time, sensor_index, f});
                                         }
                                         catch (const std::exception& e)
                                         {
                                             LOG_ERROR("Error read thread");
                                         }

                                         m_is_first_event = false;
                                     }

                                     std::lock_guard<std::mutex> locker(m_mutex);
                                     m_cached_data_size -= data_size;
                                 });
}








rsimpl2::record_sensor::~record_sensor()
{

}
rsimpl2::record_sensor::record_sensor(sensor_interface& sensor,
                                      rsimpl2::record_sensor::ziv_frame_callback_t on_frame):
    m_sensor(sensor),
    m_record_callback(on_frame),
    m_is_pause(false),
    m_is_recording(false)
{

}
std::vector<rsimpl2::stream_profile> rsimpl2::record_sensor::get_principal_requests()
{
    m_sensor.get_principal_requests();
}
void rsimpl2::record_sensor::open(const std::vector<rsimpl2::stream_profile>& requests)
{
    m_sensor.open(requests);
}
void rsimpl2::record_sensor::close()
{
    m_sensor.close();
}
rsimpl2::option& rsimpl2::record_sensor::get_option(rs2_option id)
{
    m_sensor.get_option(id);
}
const rsimpl2::option& rsimpl2::record_sensor::get_option(rs2_option id) const
{
    m_sensor.get_option(id);
}
const std::string& rsimpl2::record_sensor::get_info(rs2_camera_info info) const
{
    m_sensor.get_info(info);
}
bool rsimpl2::record_sensor::supports_info(rs2_camera_info info) const
{
    m_sensor.supports_info(info);
}
bool rsimpl2::record_sensor::supports_option(rs2_option id) const
{
    m_sensor.supports_option(id);
}
rsimpl2::region_of_interest_method& rsimpl2::record_sensor::get_roi_method() const
{
    m_sensor.get_roi_method();
}
void rsimpl2::record_sensor::set_roi_method(std::shared_ptr<rsimpl2::region_of_interest_method> roi_method)
{
    m_sensor.set_roi_method(roi_method);
}
void rsimpl2::record_sensor::register_notifications_callback(rsimpl2::notifications_callback_ptr callback)
{
    m_sensor.register_notifications_callback(std::move(callback));
}
rsimpl2::pose rsimpl2::record_sensor::get_pose() const
{
    m_sensor.get_pose();
}

void rsimpl2::record_sensor::start(frame_callback_ptr callback)
{
    if(m_frame_callback != nullptr)
    {
        return; //already started
    }
    auto record_cb = [this, callback](rs2::frame f)
    {
        //TODO: how to pass same frame (when type is "frame")
        auto data = std::make_shared<mock_frame>(m_sensor, f.get_data(), static_cast<size_t>(f.get_stride_in_bytes()*f.get_height()));
        m_record_callback(data);
        //callback->on_frame(f.);
    };
    m_frame_callback = std::make_shared<rs2::frame_callback<decltype(record_cb)>>(record_cb);
    m_sensor.start(std::move(callback));
}
void rsimpl2::record_sensor::stop()
{
    m_sensor.stop();
    m_frame_callback.reset();
}

double rsimpl2::mock_frame::get_timestamp() const
{
    return 0;
}
rs2_timestamp_domain rsimpl2::mock_frame::get_timestamp_domain() const
{
    return rs2_timestamp_domain::RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME;
}
unsigned int rsimpl2::mock_frame::get_stream_index() const
{
    return 0;
}
const uint8_t* rsimpl2::mock_frame::get_data() const
{
    return static_cast<const uint8_t*>(m_data);
}
size_t rsimpl2::mock_frame::get_data_size() const
{
    return m_size;
}
const rsimpl2::sensor_interface& rsimpl2::mock_frame::get_sensor() const
{
    return m_sensor;
}
rsimpl2::mock_frame::mock_frame(rsimpl2::sensor_interface& s, const void* data, size_t size) :
    m_sensor(s),
    m_data(data),
    m_size(size)
{

}
