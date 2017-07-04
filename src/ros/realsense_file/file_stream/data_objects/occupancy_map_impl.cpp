// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "include/msgs/realsense_msgs/occupancy_map.h"
#include "file_stream/include/topic.h"
#include "file_stream/include/data_objects/occupancy_map_impl.h"
#include "file_stream/include/conversions.h"
#include "file_stream/include/ros_writer_impl.h"

using namespace rs::file_format;
using namespace rs::file_format::ros_data_objects;
using namespace rs::file_format::file_types;

std::string occupancy_map::get_topic(uint32_t device_id)
{
    return "/camera/rs_occupancy_map/" + std::to_string(device_id);
}

std::shared_ptr<occupancy_map> occupancy_map::create(const occupancy_map_info& info)
{
    return std::make_shared<occupancy_map_impl>(info);
}

occupancy_map_impl::occupancy_map_impl(const occupancy_map_info &info) : m_info(info) {}

file_types::sample_type occupancy_map_impl::get_type() const { return file_types::st_occupancy_map; }

status occupancy_map_impl::write_data(std::shared_ptr<ros_writer> file)
{
    realsense_msgs::occupancy_map msg;
    msg.accuracy = m_info.accuracy;
    msg.reserved = m_info.reserved;
    msg.tile_count = m_info.tile_count;
    msg.tiles.assign(m_info.tiles.get(), m_info.tiles.get() + (m_info.tile_count * 3));

    auto file_instance = std::static_pointer_cast<ros_writer_impl>(file);
    return file_instance->write(get_topic(m_info.device_id), m_info.capture_time, msg);
}

occupancy_map_info occupancy_map_impl::get_info() const
{
    return m_info;
}

void occupancy_map_impl::set_info(const occupancy_map_info &info)
{
    m_info = info;
}
