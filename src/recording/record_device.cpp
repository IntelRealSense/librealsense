// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <core/debug.h>
#include "record_device.h"

librealsense::record_device::record_device(std::shared_ptr<librealsense::device_interface> device,
                                      std::shared_ptr<librealsense::device_serializer::writer> serializer):
    m_write_thread([](){return std::make_shared<dispatcher>(std::numeric_limits<unsigned int>::max());}),
    m_is_recording(true),
    m_record_pause_time(0)
{
    //TODO: validation can probably be removed
    if (device == nullptr)
    {
        throw invalid_value_exception("device is null");
    }

    if (serializer == nullptr)
    {
        throw invalid_value_exception("serializer is null");
    }

    serializer->reset();
    m_device = device;
    m_ros_writer = serializer;
    m_sensors = create_record_sensors(m_device);
    assert(m_sensors.empty() == false);
}

std::vector<std::shared_ptr<record_sensor>> record_device::create_record_sensors(std::shared_ptr<device_interface> device)
{
    std::vector<std::shared_ptr<record_sensor>> record_sensors;
    for (size_t sensor_index = 0; sensor_index < device->get_sensors_count(); sensor_index++)
    {
        auto& sensor = device->get_sensor(sensor_index);

        auto recording_sensor = std::make_shared<record_sensor>(sensor,
            [this, sensor_index](frame_holder f/*, notifications_callback_ptr& sensor_notification_handler*/)
            {
                //Should be called when the live sensor raises a frame
                std::call_once(m_first_call_flag, [this]()
                {
                    //Initialize
                    m_capture_time_base = std::chrono::high_resolution_clock::now();
                    m_cached_data_size = 0;
                    (*m_write_thread)->start();
                });

                write_data(sensor_index, std::move(f)/*,sensor_notification_handler*/);
            });
        record_sensors.push_back(recording_sensor);
    }
    return record_sensors;
}

librealsense::record_device::~record_device()
{
    (*m_write_thread)->stop();
}
librealsense::sensor_interface& librealsense::record_device::get_sensor(size_t i)
{
    return *(m_sensors.at(i));
}
size_t librealsense::record_device::get_sensors_count() const
{
    return m_sensors.size();
}

void librealsense::record_device::write_header()
{
    auto device_extensions_md = get_extensions_snapshots(m_device.get());

    std::vector<sensor_snapshot> sensors_md;
    for (size_t j = 0; j < m_device->get_sensors_count(); ++j)
    {
        auto& sensor = m_device->get_sensor(j);
        auto sensor_extensions_md = get_extensions_snapshots(&sensor);
        sensors_md.emplace_back(sensor_extensions_md, sensor.get_principal_requests());
    }

    m_ros_writer->write_device_description({device_extensions_md, sensors_md, {/*TODO: get extrinsics*/}});
}

std::chrono::nanoseconds librealsense::record_device::get_capture_time() const
{
    auto now = std::chrono::high_resolution_clock::now();
    return (now - m_capture_time_base) - m_record_pause_time;
}

void librealsense::record_device::write_data(size_t sensor_index, librealsense::frame_holder frame/*, notifications_callback_ptr& sensor_notification_handler*/)
{
    //TODO: restore: uint64_t data_size = frame.frame->get_frame_data_size();
    uint64_t cached_data_size = m_cached_data_size /*+ data_size*/;
    if (cached_data_size > MAX_CACHED_DATA_SIZE)
    {

        //notification n { RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR, 0, RS2_LOG_SEVERITY_WARN, "Recorder reached maximum cache size, frame dropped"};
        //rs2_notification noti(&n);
        //sensor_notification_handler->on_notification(&noti);
        LOG_ERROR("Recorder reached maximum cache size, frame dropped");
        return;
    }

    m_cached_data_size = cached_data_size;
    auto capture_time = get_capture_time();
    //TODO: Ziv, remove usage of shared pointer when frame_holder is copyable
    auto frame_holder_ptr = std::make_shared<frame_holder>();
    *frame_holder_ptr = std::move(frame);
    auto worker = [this, frame_holder_ptr, sensor_index, capture_time/*, data_size*/](dispatcher::cancellable_timer t) {
        if (m_is_recording == false)
        {
            return; //Recording is paused
        }
        std::call_once(m_first_frame_flag, [&]()
        {
            try
            {
                write_header(); //TODO: This could throw an exception - notify and close recorder
            }
            catch (const std::exception& e)
            {
                std::cout << e.what() << std::endl;
            }
        });
        m_ros_writer->write(capture_time, static_cast<uint32_t>(sensor_index), std::move(*frame_holder_ptr));
        std::lock_guard<std::mutex> locker(m_mutex);
        /*m_cached_data_size -= data_size;*/
    };
    
    (*m_write_thread)->invoke(worker);
        
}
const std::string& librealsense::record_device::get_info(rs2_camera_info info) const
{
    //info has no setter, it does not change - nothing to record
    return m_device->get_info(info);
}
bool librealsense::record_device::supports_info(rs2_camera_info info) const
{
    //info has no setter, it does not change - nothing to record
    return m_device->supports_info(info);
}
const librealsense::sensor_interface& librealsense::record_device::get_sensor(size_t i) const
{
    return *m_sensors.at(i);
}
void librealsense::record_device::hardware_reset()
{
    m_device->hardware_reset();
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
snapshot_collection librealsense::record_device::get_extensions_snapshots(T* extendable)
{
    //No support for extensions with more than a single type - i.e every extension has exactly one type in rs2_extension_type
    snapshot_collection snapshots;
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
                    //std::shared_ptr<debug_interface> p;
                    //TODO: Ziv, use api->create_snapshot(p); //recordable<debug_interface>::
                    //TODO: Ziv, Make sure dynamic cast indeed works
                    //snapshots.push_back(std::dynamic_pointer_cast<extension_snapshot>(p));
                }
                break;
            }
            case RS2_EXTENSION_TYPE_INFO:
            {
                auto api = dynamic_cast<librealsense::info_interface*>(extendable);
                if (api)
                {
                    std::shared_ptr<info_interface> p;
                    api->create_snapshot(p); //recordable<info_interface>::
                    //TODO: Ziv, Make sure dynamic cast indeed works
                    snapshots[ext] = std::dynamic_pointer_cast<extension_snapshot>(p);
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
            case RS2_EXTENSION_TYPE_UNKNOWN: 
                //[[fallthrough]];
            case RS2_EXTENSION_TYPE_DEPTH_SENSOR://TODO: Ziv, handle these extensiosn
                //[[fallthrough]];
            case RS2_EXTENSION_TYPE_VIDEO_FRAME:
                //[[fallthrough]];
            case RS2_EXTENSION_TYPE_MOTION_FRAME:
                //[[fallthrough]];
            case RS2_EXTENSION_TYPE_COMPOSITE_FRAME:
                //[[fallthrough]];
            case RS2_EXTENSION_TYPE_POINTS:
                //[[fallthrough]];
            case RS2_EXTENSION_TYPE_ADVANCED_MODE:
                //[[fallthrough]];
            case RS2_EXTENSION_TYPE_COUNT:
                continue;
            default:
                throw invalid_value_exception("record_device::get_extensions_snapshots: No such extension type");
        }
    }
    return snapshots;
}
bool librealsense::record_device::extend_to(rs2_extension_type extension_type, void** ext)
{
    return false;
}
void librealsense::record_device::pause_recording()
{
    (*m_write_thread)->invoke([this](dispatcher::cancellable_timer c)
    {

        if (m_is_recording == false)
            return;

        //unregister_callbacks();
        m_time_of_pause = std::chrono::high_resolution_clock::now();
        m_is_recording = false;
    });
}
void librealsense::record_device::resume_recording()
{
    (*m_write_thread)->invoke([this](dispatcher::cancellable_timer c)
    {
        if (m_is_recording)
            return;

        m_record_pause_time += (std::chrono::high_resolution_clock::now() - m_time_of_pause);
        //register_callbacks();
        m_is_recording = true;
    });
}


/* TODO: remove :)
 .----------------.  .----------------.  .-----------------. .----------------.  .----------------.  .----------------. 
| .--------------. || .--------------. || .--------------. || .--------------. || .--------------. || .--------------. |
| |    _______   | || |  _________   | || | ____  _____  | || |    _______   | || |     ____     | || |  _______     | |
| |   /  ___  |  | || | |_   ___  |  | || ||_   \|_   _| | || |   /  ___  |  | || |   .'    `.   | || | |_   __ \    | |
| |  |  (__ \_|  | || |   | |_  \_|  | || |  |   \ | |   | || |  |  (__ \_|  | || |  /  .--.  \  | || |   | |__) |   | |
| |   '.___`-.   | || |   |  _|  _   | || |  | |\ \| |   | || |   '.___`-.   | || |  | |    | |  | || |   |  __ /    | |
| |  |`\____) |  | || |  _| |___/ |  | || | _| |_\   |_  | || |  |`\____) |  | || |  \  `--'  /  | || |  _| |  \ \_  | |
| |  |_______.'  | || | |_________|  | || ||_____|\____| | || |  |_______.'  | || |   `.____.'   | || | |____| |___| | |
| |              | || |              | || |              | || |              | || |              | || |              | |
| '--------------' || '--------------' || '--------------' || '--------------' || '--------------' || '--------------' |
 '----------------'  '----------------'  '----------------'  '----------------'  '----------------'  '----------------' 
*/

librealsense::record_sensor::record_sensor(sensor_interface& sensor,
                                      librealsense::record_sensor::frame_interface_callback_t on_frame):
    m_sensor(sensor),
    m_record_callback(on_frame),
    m_is_pause(false),
    m_is_recording(false),
    m_user_notification_callback(nullptr, [](rs2_notifications_callback* n) {})
{
    std::cout << "Record sensor created" << std::endl;
}

librealsense::record_sensor::~record_sensor()
{
}

std::vector<librealsense::stream_profile> librealsense::record_sensor::get_principal_requests()
{
    return m_sensor.get_principal_requests();
}

void librealsense::record_sensor::open(const std::vector<librealsense::stream_profile>& requests)
{
    m_sensor.open(requests);
    //m_curr_configurations = requests;
    //TODO: write to file
}
void librealsense::record_sensor::close()
{
    m_sensor.close();
}
librealsense::option& librealsense::record_sensor::get_option(rs2_option id)
{
    //std::shared_ptr<option> o;
    //m_sensor.get_option(id).create_recordable(o, [this](std::shared_ptr<extension_snapshot> e) {
    //    m_record_callback();
    //});
    //return o;
    return m_sensor.get_option(id);
}
const librealsense::option& librealsense::record_sensor::get_option(rs2_option id) const
{
    return m_sensor.get_option(id);
}
const std::string& librealsense::record_sensor::get_info(rs2_camera_info info) const
{
    return m_sensor.get_info(info);
}
bool librealsense::record_sensor::supports_info(rs2_camera_info info) const
{
    return m_sensor.supports_info(info);
}
bool librealsense::record_sensor::supports_option(rs2_option id) const
{
    return m_sensor.supports_option(id);
}

void librealsense::record_sensor::register_notifications_callback(notifications_callback_ptr callback)
{
    class my_notifications_callback : public rs2_notifications_callback
    {
        std::function<void(rs2_notification*)> on_notification_function;
    public:
        my_notifications_callback(std::function<void(rs2_notification*)>  on_notification) : on_notification_function(on_notification) {}

        void on_notification(rs2_notification* _notification) override
        {
            on_notification_function(_notification);
        }
        void release() override { delete this; }
    };
    auto cb = new my_notifications_callback([&](rs2_notification* n)
    {
        m_user_notification_callback->on_notification(n);
    });
    m_sensor.register_notifications_callback({ cb, [](rs2_notifications_callback* p) { p->release(); } });
}

class my_frame_callback : public rs2_frame_callback
{
    std::function<void(frame_holder)> on_frame_function;
public:
    explicit my_frame_callback(std::function<void(frame_holder)> on_frame) : on_frame_function(on_frame) {}

    void on_frame(rs2_frame * fref) override
    {
        on_frame_function({ (frame_interface*)fref });
    }

    void release() override { delete this; }
};
void librealsense::record_sensor::start(frame_callback_ptr callback)
{
    if(m_frame_callback != nullptr)
    {
        return; //already started
    }

    //TODO: Handle case where live sensor is already streaming
    auto record_cb = [this, callback](frame_holder frame)
    {
        m_record_callback(frame.clone());
        frame_interface* ref = nullptr;
        std::swap(frame.frame, ref);
        callback->on_frame((rs2_frame*)ref);
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

bool librealsense::record_sensor::extend_to(rs2_extension_type extension_type, void** ext)
{
    switch (extension_type)
    {
        case RS2_EXTENSION_TYPE_DEBUG:
        {
            auto ptr = dynamic_cast<debug_interface*>(&m_sensor);
            if(!ptr)
            {
                throw invalid_value_exception(std::string("Sensor is not of type ") + typeid(debug_interface).name());
            }
            std::shared_ptr<debug_interface> api;
//           TODO: Ziv, use create_recordable
//            ptr->create_recordable(api, [this](std::shared_ptr<extension_snapshot> e) //recordable<debug_interface>::
//            {
//                m_record_callback(std::make_shared<extension_snapshot_frame>(m_sensor, e));
//            });
            //m_extensions[extension_type] = d;
            //TODO: Ziv, Verify this doesn't result in memory leaks
            *ext = api.get();
            return true;
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
    return false;
}

const device_interface& record_sensor::get_device()
{
    throw not_implemented_exception(__FUNCTION__);
}

rs2_extrinsics record_sensor::get_extrinsics_to(rs2_stream from, const sensor_interface& other, rs2_stream to) const
{
    throw not_implemented_exception(__FUNCTION__);
}

const std::vector<platform::stream_profile>& record_sensor::get_curr_configurations() const
{
    return m_curr_configurations;
}

std::shared_ptr<matcher> record_device::create_matcher(rs2_stream stream) const
{
    return m_device->create_matcher(stream);
}
