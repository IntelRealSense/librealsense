// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>
#include <iostream>
#include <iomanip>
#include <core/streaming.h> //TODO: remove
#include <sensor.h>
#include <record_device.h>

#include "tclap/CmdLine.h"

using namespace std;
using namespace TCLAP;

//TODO: remove when a sensor class actually exists in rs2.hpp
class sensor_REMOVE_ME
{
public:
    sensor_REMOVE_ME(rs2::device sensor) : m_sensor(sensor)
    {

    }

    vector<rsimpl2::stream_profile> get_principal_requests()
    {
        throw std::runtime_error("Not Implemented: get_principal_requests()");
    }
    void open(const vector<rsimpl2::stream_profile>& requests)
    {
        std::vector<rs2::stream_profile> profiles;
        std::transform(requests.begin(),requests.end(), std::back_inserter(profiles),[](const rsimpl2::stream_profile& p)
        {
            return rs2::stream_profile{ p.stream, p.width, p.height, p.fps, p.format};
        } );
        m_sensor.open(profiles);
    }
    void close()
    {
        m_sensor.close();
    }
    void start(rsimpl2::frame_callback_ptr callback)
    {
        //m_sensor.start(callback);
        throw std::runtime_error("Not Implementer: start() ");

    }
    void stop()
    {
        m_sensor.stop();
    }
    rsimpl2::option& get_option(rs2_option id)
    {
        throw std::runtime_error("Not Implemented: get_option()");
    }
    const rsimpl2::option& get_option(rs2_option id) const
    {
        throw std::runtime_error("Not Implemented: get_option() const");
    }
    bool supports_option(rs2_option id) const
    {
        return false;
    }
    const string& get_info(rs2_camera_info info) const
    {
        throw std::runtime_error("Not Implemented: get_info() const");
    }
    bool supports_info(rs2_camera_info info) const
    {
        return false;
    }
    void register_notifications_callback(rsimpl2::notifications_callback_ptr callback)
    {
        throw std::runtime_error("Not Implemented: register_notifications_callback()");
    }
    bool is_streaming() const
    {
        throw std::runtime_error("Not Implemented: is_streaming()");
    }

private:
    rs2::device m_sensor;
};
//TODO: remove when a device class actually exists in rs2.hpp
class device_REMOVE_ME_WHEN_DEVICE_CLASS_EXISTS : public rsimpl2::device_interface
{
public:
    device_REMOVE_ME_WHEN_DEVICE_CLASS_EXISTS()
    {
        m_sensors = m_ctx.query_devices();
        if(m_sensors.empty())
            throw std::runtime_error("No device connected");

        for (auto s: m_sensors)
        {
            //m_sensors_interface.emplace_back<sensor_REMOVE_ME>(std::make_shared<rs2_sensor>());
        }
    }
    const string& get_info(rs2_camera_info info) const override
    {
        return m_sensors[0].get_camera_info(info);
    }
    bool supports_info(rs2_camera_info info) const override
    {
        return m_sensors[0].supports(info);
    }
    rsimpl2::sensor_interface& get_sensor(size_t i) override
    {
        throw std::runtime_error("Not Implementer: get_sensor() ");
    }
    const rsimpl2::sensor_interface& get_sensor(size_t i) const override
    {
        throw std::runtime_error("Not Implementer: get_sensor() const");
    }
    size_t get_sensors_count() const override
    {
        throw std::runtime_error("Not Implementer: get_sensor_count() ");
    }
    void hardware_reset() override
    {
        m_sensors[0].hardware_reset();
    }
    rs2_extrinsics get_extrinsics(size_t from, rs2_stream from_stream, size_t to, rs2_stream to_stream) const override
    {
        return m_ctx.get_extrinsics(m_sensors[from],from_stream,m_sensors[to],to_stream);

    }

    const rs2_device* get() const
    {
        return m_sensors[0].get();
    }

    rs2::context m_ctx;
    vector<rs2::device> m_sensors;
    //vector<std::shared_ptr<rs2_sensor>> m_sensors_interface;
};
//TODO: move to rs2.hpp
class recorder
{
public:
    recorder(std::string file, device_REMOVE_ME_WHEN_DEVICE_CLASS_EXISTS device) :
        m_file(file)
    {
        std::ifstream f(file);
        if(!f.good())
            throw std::invalid_argument(std::string("File ") + file + " does not exists or cannot be opened");

        rs2_error* e = nullptr;
        m_serializer = std::shared_ptr<rs2_device_serializer>(
            rs2_create_device_serializer(file.c_str(), &e),
            rs2_delete_device_serializer);
        rs2::error::handle(e);

        e = nullptr;
        m_record_device = std::shared_ptr<rs2_record_device>(
            rs2_create_record_device(m_live_device.get(), m_serializer.get(), &e),
            rs2_delete_record_device);
        rs2::error::handle(e);
    }
private:
    std::string m_file;
    std::shared_ptr<rs2_device_serializer> m_serializer;
    std::shared_ptr<rs2_device> m_live_device; //TODO: use a pointer to the c struct instead (when there will be one)
    std::shared_ptr<rs2_record_device> m_record_device;
};
int main(int argc, const char** argv) try
{
    CmdLine cmd("librealsense cpp-record example", ' ', RS2_API_VERSION_STR);
    ValueArg<std::string> file_path("f", "file_path", "File path for recording output", false, "record_example.bag", "string");
    cmd.add( file_path );
    cmd.parse(argc, argv);

    device_REMOVE_ME_WHEN_DEVICE_CLASS_EXISTS ds5_device;
    recorder record_device(file_path.getValue(), ds5_device);

    std::cout << "Recording to file: " << file_path.getValue() << std::endl;
    return 0;
}
catch (const rs2::error & e)
{
    cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
    return EXIT_FAILURE;
}