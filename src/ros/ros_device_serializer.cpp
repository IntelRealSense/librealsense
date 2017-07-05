// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <core/debug.h>
#include "ros_device_serializer.h"

//TODO: Ziv, move to better location
uint32_t DEVICE_INDEX = (std::numeric_limits<uint32_t>::max)(); //braces are for windows compilation
std::string SENSOR_COUNT { "sensor_count" };
rs::file_format::file_types::microseconds FIRST_FRAME_TIMESTAMP { 0 };

rsimpl2::ros_device_serializer::ros_device_serializer(std::string file):
    m_file(file)
{
    //TODO: Ziv, have stream_writer throw this error
    if(!std::ofstream(file).good())
    {
        throw std::invalid_argument(std::string("File ") + file + " is invalid or cannot be opened");
    }
}

//////////////////////////////////////////////////////////
///            ros_device_serializer                   ///
//////////////////////////////////////////////////////////

std::shared_ptr<rsimpl2::device_serializer::writer> rsimpl2::ros_device_serializer::get_writer()
{
    return m_writer;
}
std::shared_ptr<rsimpl2::device_serializer::reader> rsimpl2::ros_device_serializer::get_reader()
{
    return m_reader;
}

//////////////////////////////////////////////////////////
///      ros_device_serializer::ros_writer             ///
//////////////////////////////////////////////////////////

void rsimpl2::ros_device_serializer::ros_writer::write_device_description(const rsimpl2::device_snapshot& device_description)
{
    for (auto&& device_extension_snapshot : device_description.get_device_extensions_snapshots())
    {
        write_extension_snapshot(DEVICE_INDEX, device_extension_snapshot);
    }

    //TODO: Ziv, throw if no sensors snapshots exist?

    uint32_t sensor_index = 0;
    for (auto&& sensors_snapshot : device_description.get_sensors_snapshots())
    {
        for (auto&& sensor_extension_snapshot : sensors_snapshot.get_extensions_snapshots())
        {
            write_extension_snapshot(sensor_index, sensor_extension_snapshot);
        }
    }
}
void rsimpl2::ros_device_serializer::ros_writer::write(rsimpl2::device_serializer::storage_data data)
{
    throw not_implemented_exception(__FUNCTION__);
}
void rsimpl2::ros_device_serializer::ros_writer::reset()
{
    throw not_implemented_exception(__FUNCTION__);
}

void rsimpl2::ros_device_serializer::ros_writer::write_extension_snapshot(uint32_t id,
                                                                          std::shared_ptr<rsimpl2::extension_snapshot> snapshot)
{
    if (Is<rsimpl2::info_interface>(snapshot))
    {
        std::cout << "Remove me !!! info_interface " << id << " : " << snapshot.get() << std::endl;
        //write_vendor_info(snapshot, id);
    }

    if (Is<rsimpl2::debug_interface>(snapshot))
    {
        std::cout << "Remove me !!! debug_interface " << id << " : " << snapshot.get() << std::endl;
        auto timestamp_ns = rs::file_format::file_types::nanoseconds(FIRST_FRAME_TIMESTAMP);
        //write_property(snapshot, id, timestamp_ns);
    }
}


//////////////////////////////////////////////////////////
///      ros_device_serializer::ros_reader             ///
//////////////////////////////////////////////////////////

rsimpl2::device_snapshot rsimpl2::ros_device_serializer::ros_reader::query_device_description()
{
    throw not_implemented_exception(__FUNCTION__);
}
rsimpl2::device_serializer::storage_data rsimpl2::ros_device_serializer::ros_reader::read()
{
    throw not_implemented_exception(__FUNCTION__);
}
void rsimpl2::ros_device_serializer::ros_reader::seek_to_time(std::chrono::nanoseconds time)
{
    throw not_implemented_exception(__FUNCTION__);
}
std::chrono::nanoseconds rsimpl2::ros_device_serializer::ros_reader::query_duration() const
{
    throw not_implemented_exception(__FUNCTION__);
}
void rsimpl2::ros_device_serializer::ros_reader::reset()
{

}