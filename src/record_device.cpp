// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>
#include <core/debug.h>
#include "record_device.h"

librealsense::record_device::record_device(std::shared_ptr<librealsense::device_interface> device,
                                      std::shared_ptr<librealsense::device_serializer::writer> serializer):
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

librealsense::record_device::~record_device()
{
    (*m_write_thread)->stop();
}
librealsense::sensor_interface& librealsense::record_device::get_sensor(size_t i)
{
    return *(m_sensors[i]);
}
size_t librealsense::record_device::get_sensors_count() const
{
    return m_sensors.size();
}

//template <typename T, typename P>
//bool Is(P* device)
//{
//    return dynamic_cast<T*>(device) != nullptr;
//}

void librealsense::record_device::write_header()
{
    auto device_extensions_md = get_extensions_snapshots(m_device.get());

    std::vector<sensor_snapshot> sensors_md;
    for (size_t j = 0; j < m_device->get_sensors_count(); ++j)
    {
        auto& sensor = m_device->get_sensor(j);
        auto sensor_extensions_md = get_extensions_snapshots(&sensor);
        sensors_md.emplace_back(sensor_extensions_md);
    }

    m_writer->write_device_description({device_extensions_md, sensors_md});
}

std::chrono::nanoseconds librealsense::record_device::get_capture_time()
{
    auto now = std::chrono::high_resolution_clock::now();
    return (now - m_capture_time_base) - m_record_pause_time;
}

void librealsense::record_device::write_data(size_t sensor_index, std::shared_ptr<frame_interface> f)
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
const std::string& librealsense::record_device::get_info(rs2_camera_info info) const
{
    throw not_implemented_exception(__FUNCTION__);
}
bool librealsense::record_device::supports_info(rs2_camera_info info) const
{
    throw not_implemented_exception(__FUNCTION__);
}
const librealsense::sensor_interface& librealsense::record_device::get_sensor(size_t i) const
{
    throw not_implemented_exception(__FUNCTION__);
}
void librealsense::record_device::hardware_reset()
{
    throw not_implemented_exception(__FUNCTION__);
}
rs2_extrinsics librealsense::record_device::get_extrinsics(size_t from,
                                                      rs2_stream from_stream,
                                                      size_t to,
                                                      rs2_stream to_stream) const
{
    throw not_implemented_exception(__FUNCTION__);
}

/**
 * Go over the extendable instance and find all extensions
 * @tparam T
 * @param extendable
 * @return
 */
template<typename T>
std::vector<std::shared_ptr<librealsense::extension_snapshot>> librealsense::record_device::get_extensions_snapshots(T* extendable)
{

    //No support for extensions with more than a single type - i.e every extension has exactly one type in rs2_extension_type
    std::vector<std::shared_ptr<extension_snapshot>> snapshots;
    for (int i = 0; i < static_cast<int>(RS2_EXTENSION_TYPE_COUNT); ++i)
    {
        rs2_extension_type ext = static_cast<rs2_extension_type>(i);
        switch(ext)
        {
            case RS2_EXTENSION_TYPE_DEBUG:
            {
                auto api = dynamic_cast<librealsense::debug_interface*>(extendable);
                if (api)
                {
                    std::shared_ptr<debug_interface> p;
                    api->recordable<debug_interface>::create_snapshot(p);
                    //TODO: Ziv, Make sure dynamic cast indeed works
                    snapshots.push_back(std::dynamic_pointer_cast<extension_snapshot>(p));
                }
                break;
            }
            case RS2_EXTENSION_TYPE_INFO:
            {
                auto api = dynamic_cast<librealsense::info_interface*>(extendable);
                if (api)
                {
                    std::shared_ptr<info_interface> p;
                    api->recordable<info_interface>::create_snapshot(p);
                    //TODO: Ziv, Make sure dynamic cast indeed works
                    snapshots.push_back(std::dynamic_pointer_cast<extension_snapshot>(p));
                }
                break;
            }
            case RS2_EXTENSION_TYPE_MOTION:
            {
                //librealsense::motion_sensor_interface
                break;
            }
            case RS2_EXTENSION_TYPE_OPTIONS:
            {
                //librealsense::options_interface
                //TODO: Ziv, handle
                break;
            }
            case RS2_EXTENSION_TYPE_UNKNOWN:
            {
                LOG_WARNING("Instance has an extension with an unknown extension type");
                assert(0);
                break;
            }
            case RS2_EXTENSION_TYPE_VIDEO:
            {
                //librealsense::video_sensor_interface
                break;
            }
            case RS2_EXTENSION_TYPE_ROI:
            {
                //librealsense::roi_sensor_interface
                break;
            }
            case RS2_EXTENSION_TYPE_COUNT:
            default:
                throw invalid_value_exception("No such extension type");
        }
    }
    return snapshots;
}
void* librealsense::record_device::extend_to(rs2_extension_type extension_type)
{
    return nullptr;
}

librealsense::record_sensor::~record_sensor()
{

}
librealsense::record_sensor::record_sensor(sensor_interface& sensor,
                                      librealsense::record_sensor::frame_interface_callback_t on_frame):
    m_sensor(sensor),
    m_record_callback(on_frame),
    m_is_pause(false),
    m_is_recording(false)
{

}
std::vector<librealsense::stream_profile> librealsense::record_sensor::get_principal_requests()
{
    m_sensor.get_principal_requests();
}
void librealsense::record_sensor::open(const std::vector<librealsense::stream_profile>& requests)
{
    m_sensor.open(requests);
}
void librealsense::record_sensor::close()
{
    m_sensor.close();
}
librealsense::option& librealsense::record_sensor::get_option(rs2_option id)
{
    m_sensor.get_option(id);
}
const librealsense::option& librealsense::record_sensor::get_option(rs2_option id) const
{
    m_sensor.get_option(id);
}
const std::string& librealsense::record_sensor::get_info(rs2_camera_info info) const
{
    m_sensor.get_info(info);
}
bool librealsense::record_sensor::supports_info(rs2_camera_info info) const
{
    m_sensor.supports_info(info);
}
bool librealsense::record_sensor::supports_option(rs2_option id) const
{
    m_sensor.supports_option(id);
}

void librealsense::record_sensor::register_notifications_callback(librealsense::notifications_callback_ptr callback)
{
    m_sensor.register_notifications_callback(std::move(callback));
}

class my_frame_callback : public rs2_frame_callback
{
    std::function<void(rs2_frame*)> on_frame_function;
public:
    explicit my_frame_callback(std::function<void(rs2_frame*)> on_frame) : on_frame_function(on_frame) {}

    void on_frame(rs2_frame * fref) override
    {
        on_frame_function(fref);
    }

    void release() override { delete this; }
};
void librealsense::record_sensor::start(frame_callback_ptr callback)
{
    if(m_frame_callback != nullptr)
    {
        return; //already started
    }

    frame_callback f;
    auto record_cb = [this, callback](rs2_frame* f)
    {
        m_record_callback(std::make_shared<mock_frame>(m_sensor, f->get()));
        callback->on_frame(f);
    };
    m_frame_callback = std::make_shared<my_frame_callback>(record_cb);

    m_sensor.start(m_frame_callback);
}
void librealsense::record_sensor::stop()
{
    m_sensor.stop();
    m_frame_callback.reset();
}
bool librealsense::record_sensor::is_streaming() const
{
    return m_sensor.is_streaming();
}
void* librealsense::record_sensor::extend_to(rs2_extension_type extension_type)
{
    switch (extension_type)
    {

        case RS2_EXTENSION_TYPE_DEBUG:
        {
//            if(m_extensions.find())
//            {
//                return m_extensions[extension_type].get();
//            }
            auto ptr = dynamic_cast<debug_interface*>(&m_sensor);
            if(!ptr)
            {
                throw invalid_value_exception(std::string("Sensor is not of type ") + typeid(debug_interface).name());
            }
            std::shared_ptr<debug_interface> api;
            ptr->recordable<debug_interface>::create_recordable(api, [this](std::shared_ptr<extension_snapshot> e)
            {
                m_record_callback(std::make_shared<extension_snapshot_frame>(m_sensor, e));
            });
            //m_extensions[extension_type] = d;
            //TODO: Ziv, Verify this doesn't result in memory leaks
            return api.get();
        }
        case RS2_EXTENSION_TYPE_INFO:break;
        case RS2_EXTENSION_TYPE_MOTION:break;
        case RS2_EXTENSION_TYPE_OPTIONS:break;
        //case RS2_EXTENSION_TYPE_UNKNOWN:break;
        case RS2_EXTENSION_TYPE_VIDEO:break;
        case RS2_EXTENSION_TYPE_ROI:break;
        //case RS2_EXTENSION_TYPE_COUNT:break;
        default:
            throw invalid_value_exception(std::string("extension_type ") + std::to_string(extension_type) + " is not supported");
    }
}

double librealsense::mock_frame::get_timestamp() const
{
    return 0;
}
rs2_timestamp_domain librealsense::mock_frame::get_timestamp_domain() const
{
    return rs2_timestamp_domain::RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME;
}
unsigned int librealsense::mock_frame::get_stream_index() const
{
    return 0;
}
const uint8_t* librealsense::mock_frame::get_data() const
{
    return m_frame->data.data();
}
size_t librealsense::mock_frame::get_data_size() const
{
    return m_frame->data.size();
}
const librealsense::sensor_interface& librealsense::mock_frame::get_sensor() const
{
    return m_sensor;
}
librealsense::mock_frame::mock_frame(librealsense::sensor_interface& s, frame* f) :
    m_sensor(s),
    m_frame(f)
{

}



