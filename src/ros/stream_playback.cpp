// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <chrono>

#include "stream_playback.h"
#include "rosbag/view.h"
#include "ros/file_types.h"
#include "realsense_msgs/compressed_frame_info.h"
#include "realsense_msgs/frame_info.h"
#include "realsense_msgs/stream_info.h"
#include "realsense_msgs/motion_stream_info.h"
#include "realsense_msgs/vendor_data.h"
#include "realsense_msgs/pose.h"
#include "realsense_msgs/occupancy_map.h"

#include "rosgraph_msgs/Log.h"
#include "sensor_msgs/Imu.h"
#include "sensor_msgs/Image.h"
#include "sensor_msgs/CompressedImage.h"
#include "sensor_msgs/TimeReference.h"

#include "std_msgs/Float64.h"
#include "ros/topic.h"

#include "std_msgs/UInt32.h"

using namespace rs::file_format;
using namespace rs::file_format::file_types;

inline std::vector<std::string> get_topics(std::unique_ptr<rosbag::View>& view)
{
    std::vector<std::string> topics;
    for(const rosbag::ConnectionInfo* connection : view->getConnections())
    {
        topics.emplace_back(connection->topic);
    }
    return topics;
}

bool stream_playback::get_file_version_from_file(uint32_t& version) const
{
    rosbag::View view(m_file, rosbag::TopicQuery(get_file_version_topic()));
    if (view.size() == 0)
    {
        return false;
    }

    auto item = *view.begin();
    std_msgs::UInt32Ptr msg = item.instantiate<std_msgs::UInt32>();
    if (msg == nullptr)
    {
        return false;
    }
    version = msg->data;

    return true;
}

stream_playback::stream_playback(const std::string &file_path) : 
    m_frame_source(std::make_shared<librealsense::platform::os_time_service>())
{
    if (file_path.empty())
    {
        throw std::invalid_argument("file_path");
    }

    m_file.open(file_path, rosbag::BagMode::Read);

    uint32_t version = 0;
    if(get_file_version_from_file(version) == false)
    {
        throw std::runtime_error("failed to read file version");
    }

    if(version != get_file_version())
    {
        throw std::runtime_error("unsupported file version");
    }

    m_samples_view = std::unique_ptr<rosbag::View>(new rosbag::View(m_file));
    m_samples_itrator = m_samples_view->begin();
    m_topics = get_topics(m_samples_view);
}

status stream_playback::set_filter(std::vector<std::string> topics)
{
    std::unique_lock<std::mutex> locker(m_mutex);
    if(m_samples_itrator == m_samples_view->end())
    {
        return status::status_file_eof;
    }
    rosbag::MessageInstance sample_msg = *m_samples_itrator;
    auto curr_time = sample_msg.getTime();

    m_samples_view = std::unique_ptr<rosbag::View>(new rosbag::View());

    if(topics.empty() == false)
    {
        auto view = std::unique_ptr<rosbag::View>(new rosbag::View(m_file));
        std::vector<std::string> topics_in_file = get_topics(view);
        for(auto element : topics)
        {
            if(std::find(topics_in_file.begin(), topics_in_file.end(), element) == topics_in_file.end())
            {
                return status::status_item_unavailable;
            }
            m_samples_view->addQuery(m_file, rosbag::TopicQuery(element), curr_time);

        }
        m_topics = topics;
    }
    else
    {
        //the default state of view is to get all topics
        m_samples_view = std::unique_ptr<rosbag::View>(new rosbag::View(m_file, curr_time));
        m_topics = get_topics(m_samples_view);
    }
    m_samples_itrator = m_samples_view->begin();
    return status_no_error;
}

status stream_playback::seek_to_time(file_types::nanoseconds seek_time, std::unique_ptr<rosbag::View>& samples_view) const
{
    ros::Time to_time = ros::TIME_MIN;
    if(seek_time.count() != 0)
    {
        std::chrono::duration<uint32_t> sec = std::chrono::duration_cast<std::chrono::duration<uint32_t>>(seek_time);
        file_types::nanoseconds range = seek_time - std::chrono::duration_cast<file_types::nanoseconds>(sec);
        to_time = ros::Time(sec.count(), std::chrono::duration_cast<std::chrono::duration<uint32_t, std::nano>>(range).count());
    }

    if(m_topics.empty() == true)
    {
        samples_view = std::unique_ptr<rosbag::View>(new rosbag::View(m_file));
    }
    else
    {
        samples_view = std::unique_ptr<rosbag::View>(new rosbag::View());
        for(auto topic : m_topics)
        {
            samples_view->addQuery(m_file, rosbag::TopicQuery(topic), to_time);
        }
    }
    if(samples_view->begin() == samples_view->end())
    {
        return status_invalid_argument;
    }

    return status_no_error;

}

status stream_playback::seek_to_time(file_types::nanoseconds seek_time)
{
    std::unique_lock<std::mutex> locker(m_mutex);
    std::unique_ptr<rosbag::View> samples_view;
    status sts = seek_to_time(seek_time, samples_view);
    if(sts != status_no_error)
    {
        return sts;
    }
    m_samples_view = std::move(samples_view);
    m_samples_itrator = m_samples_view->begin();
    return status_no_error;
}

status stream_playback::read_vendor_data(std::vector<std::shared_ptr<ros_data_objects::vendor_data>>& vendor_data,
                                uint32_t device_id) const
{
    auto vendor_data_topic = ros_data_objects::vendor_data::get_topic(device_id);
    rosbag::View view(m_file, rosbag::TopicQuery(vendor_data_topic));
    if (view.size() == 0)
    {
        return status_item_unavailable;
    }
    for (auto message_instance : view)
    {
        auto vendor_item = create_vendor_data(message_instance);
        if(vendor_item == nullptr)
        {
            return status_file_read_failed;
        }
        vendor_data.push_back(vendor_item);
    }
    return status_no_error;
}

status stream_playback::read_stream_infos(std::vector<std::shared_ptr<ros_data_objects::stream_info>>& stream_infos,
                                               file_types::sample_type type, uint32_t device_id) const
{
    switch(type)
    {
    case file_types::st_image :
    case file_types::st_compressed_image :
    {
        auto topic = ros_data_objects::image_stream_info::get_topic(device_id);
        rosbag::View view(m_file, rosbag::TopicQuery(topic));
        if (view.size() == 0)
        {
            return status_item_unavailable;
        }
        for (auto message_instance : view)
        {
            auto info = create_image_stream_info(message_instance);
            if(info == nullptr)
            {
                return status_file_read_failed;
            }
            stream_infos.push_back(info);
        }
        break;
    }
    case file_types::st_motion :
    {
        auto topic = ros_data_objects::motion_stream_info::get_topic(device_id);
        rosbag::View view(m_file, rosbag::TopicQuery(topic));
        if (view.size() == 0)
        {
            return status_item_unavailable;
        }
        for (auto message_instance : view)
        {
            auto info = create_motion_info(message_instance);
            if(info == nullptr)
            {
                return status_file_read_failed;
            }
            stream_infos.push_back(info);
        }
        break;
    }
    default : return status_param_unsupported;
    }
    return status_no_error;
}

status stream_playback::read_next_sample(std::shared_ptr<ros_data_objects::sample>& sample)
{
    std::unique_lock<std::mutex> locker(m_mutex);
    for( ; m_samples_itrator != m_samples_view->end(); ++m_samples_itrator)
    {
        rosbag::MessageInstance sample_msg = *m_samples_itrator;
        if(sample_msg.isType<sensor_msgs::Image>())
        {
            sample = create_image(sample_msg);
        }
        else if(sample_msg.isType<sensor_msgs::CompressedImage>())
        {
            sample = create_compressed_image(sample_msg);
        }
        else if(sample_msg.isType<sensor_msgs::TimeReference>())
        {
            sample = create_time_sample(sample_msg);
        }
        else if(sample_msg.isType<sensor_msgs::Imu>())
        {
            sample = create_motion_sample(sample_msg);
        }
        else if(sample_msg.getTopic().find("/camera/property/") != std::string::npos)
        {
            sample = create_property(sample_msg);
        }
        else if(sample_msg.isType<realsense_msgs::pose>())
        {
            sample = create_six_dof(sample_msg);
        }
        else if(sample_msg.isType<realsense_msgs::occupancy_map>())
        {
            sample = create_occupancy_map(sample_msg);
        }
        else if(sample_msg.isType<rosgraph_msgs::Log>())
        {
            sample = create_log(sample_msg);
        }
        else
        {
            continue;
        }
        ++m_samples_itrator;
        if(sample == nullptr)
        {
            return status_file_read_failed;
        }
        return status_no_error;
    }

    return status_file_eof;
}

std::shared_ptr<ros_data_objects::compressed_image> stream_playback::create_compressed_image(const rosbag::MessageInstance &image_data) const
{
    sensor_msgs::CompressedImagePtr msg = image_data.instantiate<sensor_msgs::CompressedImage>();
    if (msg == nullptr)
    {
        return nullptr;
    }
    ros_data_objects::compressed_image_info info = {};
    topic image_topic(image_data.getTopic());
    auto device_str = topic(image_data.getTopic()).at(4);
    info.device_id = static_cast<uint32_t>(std::stoll(device_str));
    info.timestamp = seconds(msg->header.stamp.toSec());
    info.stream = topic(image_data.getTopic()).at(2);

    info.frame_number = msg->header.seq;
    info.capture_time = nanoseconds(image_data.getTime().toNSec());

    info.data = std::shared_ptr<uint8_t>(new uint8_t[msg->data.size()],
            [](uint8_t* ptr){delete[] ptr;});

    memcpy(info.data.get(), &msg->data[0], msg->data.size());

    info.compression_size = static_cast<uint32_t>(msg->data.size());

    auto info_topic = ros_data_objects::compressed_image::get_info_topic(image_topic.at(1), std::stoi(image_topic.at(3)));
    rosbag::View view_info(m_file, rosbag::TopicQuery(info_topic), image_data.getTime());
    if (view_info.begin() == view_info.end())
    {
        return nullptr;
    }
    rosbag::MessageInstance frame_info = *view_info.begin();

    realsense_msgs::compressed_frame_infoPtr info_msg =
            frame_info.instantiate<realsense_msgs::compressed_frame_info>();
    if (msg == nullptr)
    {
        return nullptr;
    }
    info.system_time = nanoseconds(info_msg->system_time);
    info.timestamp_domain = static_cast<rs2_timestamp_domain>(info_msg->time_stamp_domain);
    info.height = info_msg->height;
    info.width = info_msg->width;
    info.step = info_msg->step;
    for(auto metadata : info_msg->frame_metadata)
    {
        //TODO: Ziv, make sure this works correctly
        info.metadata[static_cast<rs2_frame_metadata>(metadata.type)] = metadata.data;
    }
    auto sts = conversions::convert(info_msg->encoding, info.format);
    if(sts != status_no_error)
    {
        return nullptr;
    }

    return std::make_shared<ros_data_objects::compressed_image>(info);
}

std::shared_ptr<ros_data_objects::image> stream_playback::create_image(const rosbag::MessageInstance &image_data) const
{
    sensor_msgs::ImagePtr msg = image_data.instantiate<sensor_msgs::Image>();
    if (msg == nullptr)
    {
        return nullptr;
    }
    
    

    
    topic image_topic(image_data.getTopic());
    auto device_str = topic(image_data.getTopic()).at(4);
    
    frame_additional_data additional_data {};
    std::chrono::duration<double, std::micro> timestamp_us = std::chrono::duration_cast<microseconds>(seconds(msg->header.stamp.toSec()));
    additional_data.timestamp = timestamp_us.count();
    additional_data.frame_number = msg->header.seq;                            
    conversions::convert(msg->encoding, additional_data.format);
    std::string stream = topic(image_data.getTopic()).at(2); 
    conversions::convert(stream, additional_data.stream_type);
    additional_data.frame_callback_started = 0; //TODO: What is this?        
    additional_data.fps = 0;   //TODO:  why would a frame hold its fps?      
    additional_data.fisheye_ae_mode = false; //TODO: where should this come from?
    auto info_topic = ros_data_objects::image::get_info_topic(image_topic.at(2), std::stoi(image_topic.at(4)));
    rosbag::View view_info(m_file, rosbag::TopicQuery(info_topic), image_data.getTime());
    if (view_info.begin() == view_info.end())
    {
        return nullptr;
    }
    realsense_msgs::frame_infoPtr info_msg = (*view_info.begin()).instantiate<realsense_msgs::frame_info>();
    if (info_msg == nullptr)
    {
        return nullptr;
    }
    additional_data.timestamp_domain = static_cast<rs2_timestamp_domain>(info_msg->time_stamp_domain); //TODO: Ziv, Need to change this - in case of changes to time_stamp_domain this will break
    additional_data.system_time = nanoseconds(info_msg->system_time).count(); //TODO: Ziv - verify system time is in nanos
    //TODO: Ziv, make sure this works correctly
    //std::copy(info_msg->frame_metadata.data(), info_msg->frame_metadata.data() + std::min(static_cast<uint32_t>(info_msg->frame_metadata.size()), 255u), fad.metadata_blob.begin());
    additional_data.metadata_size = std::min(static_cast<uint32_t>(info_msg->frame_metadata.size()), 255u);

    
    frame_interface* frame = m_frame_source.alloc_frame(RS2_EXTENSION_TYPE_VIDEO_FRAME, msg->data.size(), additional_data, true);
    
    librealsense::video_frame* video_frame = static_cast<librealsense::video_frame*>(frame);
    video_frame->assign(msg->width, msg->height, msg->step, msg->step / msg->width / 8); //TODO: Ziv, is bpp bytes or bits per pixel?
    video_frame->data = msg->data;
    uint32_t device_id = static_cast<uint32_t>(std::stoll(device_str));
    auto capture_time = nanoseconds(image_data.getTime().toNSec());
    librealsense::frame_holder fh{ video_frame };
    return std::make_shared<ros_data_objects::image>(capture_time, device_id, std::move(fh));
}

std::shared_ptr<ros_data_objects::image_stream_info> stream_playback::create_image_stream_info(const rosbag::MessageInstance &info_msg) const
{
    realsense_msgs::stream_infoPtr msg = info_msg.instantiate<realsense_msgs::stream_info>();
    if (msg == nullptr)
    {
        return nullptr;
    }
    ros_data_objects::image_stream_data info = {};
    auto device_str = topic(info_msg.getTopic()).at(3);
    info.device_id = static_cast<uint32_t>(std::stoll(device_str));
    conversions::convert(msg->stream_type, info.type);
    info.fps = msg->fps;
    info.width = msg->width;
    info.height = msg->height;
    conversions::convert(msg->encoding, info.format);
    info.intrinsics.fx = static_cast<float>(msg->camera_info.K[0]);
    info.intrinsics.ppx = static_cast<float>(msg->camera_info.K[2]);
    info.intrinsics.fy = static_cast<float>(msg->camera_info.K[4]);
    info.intrinsics.ppy = static_cast<float>(msg->camera_info.K[5]);
    conversions::convert(msg->camera_info.distortion_model, info.intrinsics.model);

    for(uint32_t i = 0; i < msg->camera_info.D.size(); ++i )
    {
        info.intrinsics.coeffs[i] = static_cast<float>((&msg->camera_info.D[0])[i]);
    }
    conversions::convert(msg->stream_extrinsics.extrinsics, info.stream_extrinsics.extrinsics_data);
    info.stream_extrinsics.reference_point_id = msg->stream_extrinsics.reference_point_id;

    return std::make_shared<ros_data_objects::image_stream_info>(info);
}

std::shared_ptr<ros_data_objects::log> stream_playback::create_log(const rosbag::MessageInstance &message) const
{
    rosgraph_msgs::LogPtr msg = message.instantiate<rosgraph_msgs::Log>();
    ros_data_objects::log_info info = {};
    info.level = msg->level;
    info.message = msg->msg;
    info.file = msg->file;
    info.function = msg->function;
    info.line =  msg->line;
    return std::make_shared<ros_data_objects::log>(info);
}

std::shared_ptr<ros_data_objects::motion_sample> stream_playback::create_motion_sample(const rosbag::MessageInstance &message) const
{
    sensor_msgs::ImuPtr msg = message.instantiate<sensor_msgs::Imu>();
    if (msg == nullptr)
    {
        return nullptr;
    }
    ros_data_objects::motion_info info = {};
    auto device_str = topic(message.getTopic()).at(4);
    info.device_id = static_cast<uint32_t>(std::stoll(device_str));
    info.frame_number = msg->header.seq;
    info.timestamp = seconds(msg->header.stamp.toSec());

    if(message.getTopic() == ros_data_objects::motion_sample::get_topic(file_types::motion_type::accel, info.device_id))
    {
        info.type = file_types::motion_type::accel;
        info.data[0] = static_cast<float>(msg->linear_acceleration.x);
        info.data[1] = static_cast<float>(msg->linear_acceleration.y);
        info.data[2] = static_cast<float>(msg->linear_acceleration.z);
    }else if(message.getTopic() == ros_data_objects::motion_sample::get_topic(file_types::motion_type::gyro, info.device_id))
    {
        info.type = file_types::motion_type::gyro;
        info.data[0] = static_cast<float>(msg->angular_velocity.x);
        info.data[1] = static_cast<float>(msg->angular_velocity.y);
        info.data[2] = static_cast<float>(msg->angular_velocity.z);
    }else
    {
        return nullptr;
    }

    info.capture_time = nanoseconds(message.getTime().toNSec());

    return std::make_shared<ros_data_objects::motion_sample>(info);
}

std::shared_ptr<ros_data_objects::motion_stream_info> stream_playback::create_motion_info(const rosbag::MessageInstance &message) const
{
    realsense_msgs::motion_stream_infoPtr msg = message.instantiate<realsense_msgs::motion_stream_info>();
    if (msg == nullptr)
    {
        return nullptr;
    }
    ros_data_objects::motion_stream_data info = {};
    auto device_str = topic(message.getTopic()).at(3);
    info.device_id = static_cast<uint32_t>(std::stoll(device_str));
    info.fps = msg->fps;
    if(conversions::convert(msg->motion_type, info.type) == false)
    {
        return nullptr;
    }

    conversions::convert(msg->stream_intrinsics, info.intrinsics);
    conversions::convert(msg->stream_extrinsics.extrinsics, info.stream_extrinsics.extrinsics_data);
    info.stream_extrinsics.reference_point_id = msg->stream_extrinsics.reference_point_id;

    return std::make_shared<ros_data_objects::motion_stream_info>(info);
}

std::shared_ptr<ros_data_objects::occupancy_map> stream_playback::create_occupancy_map(const rosbag::MessageInstance &message) const
{
    realsense_msgs::occupancy_mapPtr msg = message.instantiate<realsense_msgs::occupancy_map>();
    ros_data_objects::occupancy_map_info info = {};
    info.accuracy = msg->accuracy;
    info.reserved = msg->reserved;
    info.tile_count = msg->tile_count;

    info.tiles = std::shared_ptr<float_t>(new float_t[msg->tiles.size()],
            [](float_t* ptr){delete[] ptr;});

    memcpy(info.tiles.get(), &msg->tiles[0], msg->tiles.size());
    auto device_str = topic(message.getTopic()).at(3);
    info.device_id = static_cast<uint32_t>(std::stoll(device_str));
    info.capture_time = nanoseconds(message.getTime().toNSec());
    return std::make_shared<ros_data_objects::occupancy_map>(info);
}

std::shared_ptr<ros_data_objects::property> stream_playback::create_property(const rosbag::MessageInstance &message) const
{
    std_msgs::Float64Ptr msg = message.instantiate<std_msgs::Float64>();
    ros_data_objects::property_info info = {};
    info.value = msg->data;
    info.key = topic(message.getTopic()).at(3);
    auto device_str = topic(message.getTopic()).at(4);
    info.device_id = static_cast<uint32_t>(std::stoll(device_str));
    info.capture_time = nanoseconds(message.getTime().toNSec());
    return std::make_shared<ros_data_objects::property>(info);
}

std::shared_ptr<ros_data_objects::pose> stream_playback::create_six_dof(const rosbag::MessageInstance &message) const
{
    realsense_msgs::posePtr msg = message.instantiate<realsense_msgs::pose>();

    ros_data_objects::pose_info info = {};
	info.translation.x = static_cast<float>(msg->translation.x);
	info.translation.y = static_cast<float>(msg->translation.y);
	info.translation.z = static_cast<float>(msg->translation.z);

	info.rotation.x = static_cast<float>(msg->rotation.x);
	info.rotation.y = static_cast<float>(msg->rotation.y);
	info.rotation.z = static_cast<float>(msg->rotation.z);
	info.rotation.w = static_cast<float>(msg->rotation.w);

	info.velocity.x = static_cast<float>(msg->velocity.x);
	info.velocity.y = static_cast<float>(msg->velocity.y);
	info.velocity.z = static_cast<float>(msg->velocity.z);

	info.angular_velocity.x = static_cast<float>(msg->angular_velocity.x);
	info.angular_velocity.y = static_cast<float>(msg->angular_velocity.y);
	info.angular_velocity.z = static_cast<float>(msg->angular_velocity.z);

	info.acceleration.x = static_cast<float>(msg->acceleration.x);
	info.acceleration.y = static_cast<float>(msg->acceleration.y);
	info.acceleration.z = static_cast<float>(msg->acceleration.z);

	info.angular_acceleration.x = static_cast<float>(msg->angular_acceleration.x);
	info.angular_acceleration.y = static_cast<float>(msg->angular_acceleration.y);
	info.angular_acceleration.z = static_cast<float>(msg->angular_acceleration.z);

    info.timestamp = nanoseconds(msg->timestamp);
	info.system_timestamp = nanoseconds(msg->system_timestamp);

    auto device_str = topic(message.getTopic()).at(3);
    info.device_id = static_cast<uint32_t>(std::stoll(device_str));
    info.capture_time = nanoseconds(message.getTime().toNSec());
    return std::make_shared<ros_data_objects::pose>(info);
}

std::shared_ptr<ros_data_objects::time_sample> stream_playback::create_time_sample(const rosbag::MessageInstance &message) const
{
    sensor_msgs::TimeReferencePtr msg = message.instantiate<sensor_msgs::TimeReference>();
    if (msg == nullptr)
    {
        return nullptr;
    }
    ros_data_objects::time_sample_info info = {};
    info.frame_number = msg->header.seq;
    info.timestamp = seconds(msg->header.stamp.toSec());
    info.source = msg->source;
    info.capture_time = nanoseconds(message.getTime().toNSec());
    auto device_str = topic(message.getTopic()).at(2);
    info.device_id = static_cast<uint32_t>(std::stoll(device_str));

    return std::make_shared<ros_data_objects::time_sample>(info);
}

std::shared_ptr<ros_data_objects::vendor_data> stream_playback::create_vendor_data(const rosbag::MessageInstance &message) const
{
    realsense_msgs::vendor_dataPtr msg = message.instantiate<realsense_msgs::vendor_data>();
    if (msg == nullptr)
    {
        return nullptr;
    }
    ros_data_objects::vendor_info info = {};
    info.name = msg->name;
    info.value = msg->value;
    auto device_str = topic(message.getTopic()).at(2);
    info.device_id = static_cast<uint32_t>(std::stoll(device_str));

    return std::make_shared<ros_data_objects::vendor_data>(info);
}
status stream_playback::get_file_duration(file_types::nanoseconds& duration) const
{
    std::unique_lock<std::mutex> locker(m_mutex);
    std::unique_ptr<rosbag::View> samples_view;
    auto first_non_frame_time = ros::TIME_MIN.toNSec()+1;
    status sts = seek_to_time(file_types::nanoseconds(first_non_frame_time), samples_view);
    if(sts != status_no_error)
    {
        return sts;
    }
    auto samples_itrator = samples_view->begin();
    auto first_frame_time = samples_itrator->getTime();
    auto total_time = samples_view->getEndTime() - first_frame_time;
    duration = file_types::nanoseconds(total_time.toNSec());
    return status_no_error;
}
