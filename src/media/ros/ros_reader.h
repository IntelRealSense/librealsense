// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <chrono>

#include "rosbag/view.h"
#include "file_types.h"
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
#include "topic.h"

#include "std_msgs/UInt32.h"

#include <mutex>
#include <core/serialization.h>

#include <media/device_snapshot.h>
#include "file_types.h"
#include "status.h"

#include <media/ros/data_objects/vendor_data.h>
#include "media/ros/data_objects/compressed_image.h"
#include "media/ros/data_objects/image.h"
#include "media/ros/data_objects/image_stream_info.h"
#include "media/ros/data_objects/motion_stream_info.h"
#include "media/ros/data_objects/motion_sample.h"
#include "media/ros/data_objects/time_sample.h"
#include "media/ros/data_objects/property.h"
#include "media/ros/data_objects/pose.h"
#include "media/ros/data_objects/occupancy_map.h"
#include "rosbag/view.h"
#include "source.h"


using namespace librealsense::file_format;
using namespace librealsense::file_format::file_types;

namespace librealsense
{
    inline std::vector<std::string> get_topics(std::unique_ptr<rosbag::View>& view)
    {
        std::vector<std::string> topics;
        for(const rosbag::ConnectionInfo* connection : view->getConnections())
        {
            topics.emplace_back(connection->topic);
        }
        return topics;
    }

    /**
 * @brief The stream_playback provides an interface for playing realsense format files
 *
 */
    class stream_playback //TODO: Merge with ros_reader
    {

    public:
        stream_playback(const std::string& file_path):
            m_frame_source(nullptr),
            m_file_path(file_path)
        {
            if (file_path.empty())
            {
                throw std::invalid_argument("file_path");
            }
            reset();
        }

        /**
        * @brief Returns a vector of vendor_data objects for the corresponding device
        *
        * @param[out] vendor_data               Vector of the vendor information
        * @param[in]  device_id                 Device identifier
        * @return status_no_error               Successful execution
        * @return status_item_unavailable       No vendor info data of the requested device id is found in the file
        * @return status_file_read_failed       Failed to read from file
        */
        status read_vendor_data(std::vector<std::shared_ptr<ros_data_objects::vendor_data>>& vendor_data, uint32_t device_id) const
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

        /**
        * @brief Sets the player to play only the requested topics
        *        The player will continue playing from the time before the set_filter call.
        *        To get a stream_data topic, use the static function get_topic for each stream_data type.
        *        To play all the recorded items set the function with an empty vector.
        *
        * @param[in] topics             List of the chosen topics
        * @return status_no_error               Successful execution
        * @return status_item_unavailable       At list one of the topics does not exist in the file
        * @return status_file_eof               The file reached the end
        */
        status set_filter(std::vector<std::string> topics)
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

        /**
        * @brief Set the player to play from the requested timestamp.
        *        The player will play only the chosen streams (if they were set with set_filter)
        *        If the begin time is out of range the status will be status_invalid_argument
        *        To seek to begin time call seek_to_time with begin = 0
        *
        * @param[in] begin             The requested timestamp
        * @return status_no_error               Successful execution
        * @return status_invalid_argument       The begin argument is out of range
        */
        status seek_to_time(file_types::nanoseconds seek_time)
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

        /**
        * @brief Returns a vector of stream_info objects for the corresponding device and stream type
        *
        * @param[out] stream_infos              Vector of the stream information
        * @param[in]  type                      Requested stream type
        * @param[in]  device_id                 Requested device identifier
        * @return status_no_error               Successful execution
        * @return status_item_unavailable       No stream info data of the requested device id  and type is found in the file
        * @return status_file_read_failed       Failed to read from file
        * @return status_param_unsupported      The sample type is not supported by the file format
        */
        status read_stream_infos(std::vector<std::shared_ptr<ros_data_objects::stream_info>>& stream_infos,
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


        /**
        * @brief Returns the next sample by arrival time
        *
        * @param[out] sample            An object implements the sample interface
        * @return status_no_error               Successful execution
        * @return status_file_read_failed       Failed to read from file
        * @return status_file_eof               The file reached the end
        */
        status read_next_sample(std::shared_ptr<ros_data_objects::sample>& sample)
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

        /**
         * @brief Returns the total duration of the file
         * @param[out] duration   On successful execution will hold the file duration (in nanoseconds)
         * @return status_no_error on successful execution
         */
        status get_file_duration(file_types::nanoseconds& duration) const
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

        void reset()
        {
            m_file.close();
            m_file.open(m_file_path, rosbag::BagMode::Read);

            uint32_t version = 0;
            if (get_file_version_from_file(version) == false)
            {
                throw std::runtime_error("failed to read file version");
            }

            if (version != get_file_version())
            {
                throw std::runtime_error("unsupported file version");
            }

            m_samples_view = std::unique_ptr<rosbag::View>(new rosbag::View(m_file));
            m_samples_itrator = m_samples_view->begin();
            m_topics = get_topics(m_samples_view);
            m_frame_source.reset();
            m_frame_source.init(nullptr);
        }

        std::string m_file_path;
    private:
        std::shared_ptr<ros_data_objects::compressed_image> create_compressed_image(const rosbag::MessageInstance &image_data) const
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

        std::shared_ptr<ros_data_objects::image> create_image(const rosbag::MessageInstance &image_data) const
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


            frame_interface* frame = m_frame_source.alloc_frame(RS2_EXTENSION_VIDEO_FRAME, msg->data.size(), additional_data, true);
            if (frame == nullptr)
                return nullptr;

            librealsense::video_frame* video_frame = static_cast<librealsense::video_frame*>(frame);
            video_frame->assign(msg->width, msg->height, msg->step, msg->step / msg->width / 8); //TODO: Ziv, is bpp bytes or bits per pixel?
            video_frame->data = msg->data;
            uint32_t device_id = static_cast<uint32_t>(std::stoll(device_str));
            auto capture_time = nanoseconds(image_data.getTime().toNSec());
            librealsense::frame_holder fh{ video_frame };
            return std::make_shared<ros_data_objects::image>(capture_time, device_id, std::move(fh));
        }

        std::shared_ptr<ros_data_objects::image_stream_info> create_image_stream_info(const rosbag::MessageInstance &info_msg) const
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

        std::shared_ptr<ros_data_objects::motion_sample> create_motion_sample(const rosbag::MessageInstance &message) const
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

        std::shared_ptr<ros_data_objects::motion_stream_info> create_motion_info(const rosbag::MessageInstance &message) const
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

        std::shared_ptr<ros_data_objects::occupancy_map> create_occupancy_map(const rosbag::MessageInstance &message) const
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

        std::shared_ptr<ros_data_objects::property> create_property(const rosbag::MessageInstance &message) const
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

        std::shared_ptr<ros_data_objects::pose> create_six_dof(const rosbag::MessageInstance &message) const
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

        std::shared_ptr<ros_data_objects::time_sample> create_time_sample(const rosbag::MessageInstance &message) const
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

        std::shared_ptr<ros_data_objects::vendor_data> create_vendor_data(const rosbag::MessageInstance &message) const
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
        bool get_file_version_from_file(uint32_t& version) const
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

        status seek_to_time(file_types::nanoseconds seek_time, std::unique_ptr<rosbag::View>& samples_view) const
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
        //std::shared_ptr<librealsense::archive_interface> m_archive;
        librealsense::frame_source m_frame_source;
        rosbag::Bag                     m_file;
        std::unique_ptr<rosbag::View>   m_samples_view;
        rosbag::View::iterator          m_samples_itrator;
        mutable std::mutex              m_mutex;
        std::vector<std::string>        m_topics;
        std::atomic<uint32_t> m_max_frame_queue_size;

    };

    class ros_reader: public device_serializer::reader
    {
        //TODO: Ziv, move to better location
        uint32_t DEVICE_INDEX = (std::numeric_limits<uint32_t>::max)(); //braces are for windows compilation
        std::string SENSOR_COUNT { "sensor_count" };
       file_format::file_types::microseconds FIRST_FRAME_TIMESTAMP { 0 };
    public:
        ros_reader(const std::string& file) :
            m_stream_playback(file)
        {
            reset();
        }

        device_snapshot query_device_description() override
        {
            return m_device_description;
        }
       file_format::status read(std::chrono::nanoseconds& timestamp, uint32_t& sensor_index, frame_holder& frame) override
        {
            std::unique_lock<std::mutex> locker(m_mutex);

            ////read cached properties avaialble after seek
            //if(m_cached_properties.empty() == false)
            //{
            //    cached_property properties_sensor_list = (*m_cached_properties.begin());
            //    *obj = properties_sensor_list.property;
            //    properties_sensor_list.property->add_ref();
            //    sensor_index = properties_sensor_list.sensor_id;
            //    timestamp = properties_sensor_list.timestamp;
            //    m_cached_properties.pop_back();
            //    return error_code::no_error;
            //}

            return read_sample(sensor_index, timestamp, frame);
        }

        void seek_to_time(std::chrono::nanoseconds time) override
        {
            throw not_implemented_exception(__FUNCTION__);
        }
        std::chrono::nanoseconds query_duration() const override
        {
            throw not_implemented_exception(__FUNCTION__);
        }
        void reset() override
        {
            m_stream_playback.reset();
            m_first_frame_time = FIRST_FRAME_TIMESTAMP;
            m_device_description = read_metadata();
        }
        virtual void set_filter(uint32_t m_sensor_index, const std::vector<stream_profile>& vector) override
        {
            //TODO: Ziv, throw not_implemented_exception(__FUNCTION__);
        }

        virtual const std::string& get_file_name() const override
        {
            return m_stream_playback.m_file_path;
        }
    private:
        rs2_camera_info rs2_camera_info_from_string(const std::string& info)
        {
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_NAME            ) ) return RS2_CAMERA_INFO_NAME;
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_SERIAL_NUMBER   ) ) return RS2_CAMERA_INFO_SERIAL_NUMBER;
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_FIRMWARE_VERSION) ) return RS2_CAMERA_INFO_FIRMWARE_VERSION;
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_LOCATION        ) ) return RS2_CAMERA_INFO_LOCATION;
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_DEBUG_OP_CODE   ) ) return RS2_CAMERA_INFO_DEBUG_OP_CODE;
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_ADVANCED_MODE   ) ) return RS2_CAMERA_INFO_ADVANCED_MODE;
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_PRODUCT_ID      ) ) return RS2_CAMERA_INFO_PRODUCT_ID;
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_CAMERA_LOCKED   ) ) return RS2_CAMERA_INFO_CAMERA_LOCKED;
            if(info == "sensor_count") return static_cast<rs2_camera_info>(static_cast<int>(RS2_CAMERA_INFO_COUNT) + 1);
            throw std::runtime_error(info + " cannot be converted to rs2_camera_info");
        }

//        error_code seek_to_time(uint64_t time_microseconds)
//        {
//            if(m_reader == nullptr)
//            {
//                return read_failed;
//            }
//            auto seek_time = m_first_frame_time +file_format::file_types::microseconds(time_microseconds);
//            auto time_interval =file_format::file_types::nanoseconds(seek_time);
//
//            std::unique_lock<std::mutex> locker(m_mutex);
//            auto retval = set_properties_state(seek_time.count());
//            if(retval != error_code::no_error)
//            {
//                LOG_ERROR("Failed to seek_to_time " << time_microseconds << ", set_properties_state returned " << retval);
//                return retval;
//            }
//            auto sts = m_reader->seek_to_time(time_interval);
//            if(sts !=file_format::status_no_error)
//            {
//                LOG_ERROR("Failed to seek_to_time " << time_microseconds << ", m_reader->seek_to_time(" <<  time_interval.count() << ") returned " << sts);
//                return read_failed;
//            }
//            return error_code::no_error;
//        }
//        bool is_empty_intrinsics(constcore::intrinsics& intrinsics)
//        {
//            auto empty =core::intrinsics{};
//            return empty == intrinsics;
//        }
//        error_code query_duration(uint64_t& duration) const
//        {
//            file_types::nanoseconds time;
//            if(m_reader->get_file_duration(time) !=file_format::status::status_no_error)
//            {
//                return error_code::read_failed;
//            }
//            duration = std::chrono::duration_cast<std::chrono::duration<uint64_t, std::micro>>(time).count();
//            return error_code::no_error;
//        }
//
//        bool is_empty_extrinsics(constcore::extrinsics& e)
//        {
//            constexpr uint8_t testblock [sizeof(core::extrinsics)] {};
//            memset ((void*)testblock, 0, sizeof testblock);
//
//            return !memcmp (testblock, &e, sizeof(core::extrinsics));
//        }
//        bool create_extrinsics_object(std::map<uint32_t, std::pair<core::extrinsics, uint64_t>>& extrinsics,
//                                                           core::data_object** extrinsics_object)
//        {
//            if(extrinsics.size() == 0)
//            {
//                return false;
//            }
//
//            std::vector<uint32_t> sensors_indices;
//            std::vector<core::extrinsics> extrinsics_vec;
//            std::vector<uint64_t> reference_points_ids;
//
//            for(auto extrinsics_item : extrinsics)
//            {
//                sensors_indices.push_back(extrinsics_item.first);
//                extrinsics_vec.push_back(extrinsics_item.second.first);
//                reference_points_ids.push_back(extrinsics_item.second.second);
//            }
//
//            returndata_objects::common::extrinsics_data_object::create(sensors_indices.data(),
//                                                                            extrinsics_vec.data(),
//                                                                            reference_points_ids.data(),
//                                                                            static_cast<uint32_t>(sensors_indices.size()),
//                                                                            extrinsics_object);
//
//        }
//
        device_snapshot read_metadata()
        {
            snapshot_collection device_extensions;

            std::shared_ptr<info_snapshot> info = read_info_snapshot(DEVICE_INDEX);
            device_extrinsics extrinsics = read_device_extrinsics();
            device_extensions[RS2_EXTENSION_INFO ] = info;
            std::vector<sensor_snapshot> sensor_descriptions;
            uint32_t sensor_count = read_sensor_count();
            for (uint32_t sensor_index = 0; sensor_index < sensor_count; sensor_index++)
            {
                snapshot_collection sensor_extensions;
                std::vector<stream_profile> streaming_profiles = read_stream_info(sensor_index);

               /* std::shared_ptr<options_snapshot> options = options_container read_options();
                sensor_extensions[RS2_EXTENSION_OPTIONS ] = options;*/

                std::shared_ptr<info_snapshot> sensor_info = read_info_snapshot(sensor_index);
                sensor_extensions[RS2_EXTENSION_INFO ] = sensor_info;
                sensor_descriptions.emplace_back(sensor_extensions, streaming_profiles);
            }

            return device_snapshot(device_extensions, sensor_descriptions, extrinsics);
        }

       file_format::status read_sample(uint32_t &sensor_index, std::chrono::nanoseconds& timestamp, frame_holder& frame)
        {
            std::shared_ptr<file_format::ros_data_objects::sample> sample;
            auto reader_retval = m_stream_playback.read_next_sample(sample);
            if(reader_retval !=file_format::status_no_error)
            {
                return reader_retval;
            }

            if(sample->get_type() ==file_format::file_types::sample_type::st_image)
            {
                auto image = std::static_pointer_cast<file_format::ros_data_objects::image>(sample);
                image->get_data(sensor_index, timestamp, frame);
                return file_format::status::status_no_error;
            }
            //else if(sample->get_type() ==file_format::file_types::sample_type::st_motion)
            //{
            //    return read_motion(std::static_pointer_cast<ros_data_objects::motion_sample>(sample),
            //                       sensor_index, timestamp, obj);
            //}
            //else if(sample->get_type() ==file_format::file_types::sample_type::st_property)
            //{
            //    return read_property(std::static_pointer_cast<ros_data_objects::property>(sample),
            //                         sensor_index, timestamp, obj);
            //}
            //else if (sample->get_type() ==file_format::file_types::sample_type::st_pose)
            //{
            //    return read_pose(std::static_pointer_cast<ros_data_objects::pose>(sample),
            //                     sensor_index, timestamp, obj);
            //}
            
            return file_format::status_param_unsupported;
        }
//
//        error_code read_pose(std::shared_ptr<ros_data_objects::pose> sample,
//                                                                           uint32_t &sensor_index, uint64_t &timestamp, core::data_object **obj)
//        {
//            if (sample == nullptr || obj == nullptr)
//            {
//                return error_code::invalid_handle;
//            }
//
//            std::shared_ptr<file_format::ros_data_objects::pose> tracking =
//                std::static_pointer_cast<ros_data_objects::pose>(sample);
//
//           core::pose pose = {};
//           file_format::ros_data_objects::pose_info pose_data = tracking->get_info();
//
//            pose = conversions::convert(pose_data);
//
//            sensor_index = pose_data.device_id;
//            timestamp = pose_data.capture_time.count() / 1000;
//
//
//            if (data_objects::tracking::pose_data_object::create(pose, obj) == false)
//            {
//                return error_code::data_object_creation_failed;
//            }
//            return error_code::no_error;
//        }
//
//        bool get_sensor_count(std::vector<std::shared_ptr<ros_data_objects::vendor_data>>& vendor_data, uint32_t& sensor_count)
//        {
//            for(auto data : vendor_data)
//            {
//                if(data->get_info().name == SENSOR_COUNT)
//                {
//                    auto value = data->get_info().value;
//                    sensor_count = std::stoi(value);
//                    return true;
//                }
//            }
//            return false;
//        }
//
//        error_code read_vendor_info(std::map<uint32_t,utils::ref_count_ptr<core::data_object>> &infos,
//                                                                                  uint32_t& sensor_count)
//        {
//            std::vector<std::shared_ptr<ros_data_objects::vendor_data>> vendor_data;
//            auto sts = m_reader->read_vendor_data(vendor_data, DEVICE_INDEX);
//            if(sts ==file_format::status_item_unavailable)
//            {
//                sensor_count = 0;
//                return error_code::no_error;
//            }
//            if(sts !=file_format::status_no_error || vendor_data.size() == 0)
//            {
//                return error_code::read_failed;
//            }
//
//            if(get_sensor_count(vendor_data, sensor_count) == false)
//            {
//                return error_code::read_failed;
//            }
//
//            //removing SENSOR_COUNT from the vendor_data since it is the only vendor data with vendor_data.name that is not acore::guid
//            vendor_data.erase(
//                std::remove_if(vendor_data.begin(), vendor_data.end(),
//                               [](std::shared_ptr<ros_data_objects::vendor_data>& info) { return info->get_info().name == SENSOR_COUNT; }),
//                vendor_data.end());
//
//            //At this point all vendor_data should be in the form (guid, string)
//           utils::ref_count_ptr<core::data_object> obj;
//            auto retval = create_vendor_info_object(vendor_data, &obj);
//            if(retval != no_error)
//            {
//                return retval;
//            }
//            infos[DEVICE_INDEX] = obj;
//
//            for(uint32_t i = 0; i < m_sensor_count + 1; ++i)
//            {
//                std::vector<std::shared_ptr<ros_data_objects::vendor_data>> sensor_vendor_data;
//                if(m_reader->read_vendor_data(sensor_vendor_data, i) !=file_format::status_no_error)
//                {
//                    continue;
//                }
//
//               utils::ref_count_ptr<core::data_object> obj;
//                auto retval = create_vendor_info_object(sensor_vendor_data, &obj);
//                if(retval != no_error)
//                {
//                    return retval;
//                }
//                infos[i] = obj;
//
//            }
//            return error_code::no_error;
//        }
//
//        error_code create_vendor_info_object(std::vector<std::shared_ptr<ros_data_objects::vendor_data>>& vendor_data,core::data_object** obj)
//        {
//            std::vector<core::guid> guids;
//            std::vector<std::string> values;
//
//            for(auto data : vendor_data)
//            {
//               core::guid id;
//                if(utils::string_to_guid(data->get_info().name, id) == false)
//                {
//                    return error_code::read_failed;
//                }
//                guids.push_back(id);
//                values.push_back(data->get_info().value);
//            }
//
//            std::vector<const char*> as_char_arrays;
//            std::transform(values.begin(), values.end(), std::back_inserter(as_char_arrays), [](const std::string& s){ return s.c_str();});
//
//
//            if(data_objects::common::info_data_object::create(guids.data(), as_char_arrays.data(),
//                                                                  static_cast<uint32_t>(guids.size()), obj) == false)
//            {
//                return error_code::data_object_creation_failed;
//            }
//            return error_code::no_error;
//        }
//
//
//        error_code read_motion(std::shared_ptr<ros_data_objects::motion_sample> sample,
//                                                                             uint32_t &sensor_index, uint64_t &timestamp, core::data_object **obj)
//        {
//            if(sample == nullptr || obj == nullptr)
//            {
//                return error_code::invalid_handle;
//            }
//
//            std::shared_ptr<file_format::ros_data_objects::motion_sample> motion =
//                std::static_pointer_cast<ros_data_objects::motion_sample>(sample);
//
//           core::motion_sample motion_sample = {};
//           file_format::ros_data_objects::motion_info motion_info = motion->get_info();
//            sensor_index = motion_info.device_id;
//            if(conversions::convert(motion_info.type, motion_sample.type) == false)
//            {
//                return error_code::invalid_handle;
//            }
//            motion_sample.timestamp = motion_info.timestamp.count() * 1000;
//            motion_sample.frame_number = motion_info.frame_number;
//            motion_sample.data[0] = motion_info.data[0];
//            motion_sample.data[1] = motion_info.data[1];
//            motion_sample.data[2] = motion_info.data[2];
//
//            timestamp = motion_info.capture_time.count() / 1000;
//
//            if(data_objects::motion::motion_data_object::create(motion_sample, obj) == false)
//            {
//                return error_code::data_object_creation_failed;
//            }
//            return error_code::no_error;
//        }
//
//        bool copy_image_metadata(const std::map<file_format::file_types::metadata_type, std::vector<uint8_t>> source,
//                                                      core::metadata_interface* target)
//        {
//            if(target == nullptr)
//            {
//                LOG_WARN("failed to create metadata");
//                return true;
//            }
//            for(auto data : source)
//            {
//               core::metadata_type type = static_cast<core::metadata_type>(data.first);
//                std::vector<uint8_t> buffer = data.second;
//                auto retval = target->add_metadata(type, buffer.data(), static_cast<uint32_t>(buffer.size()));
//                if(retval !=core::status::status_no_error)
//                {
//                    LOG_ERROR("failed to read metadata");
//                    continue;
//                }
//            }
//
//            return true;
//        }
//
//
//        bool copy_image_metadata(constcore::metadata_interface* source,
//                                                       std::map<file_format::file_types::metadata_type, std::vector<uint8_t>>& target)
//        {
//            if (source == nullptr)
//            {
//                LOG_ERROR("failed to create metadata");
//                return false;
//            }
//            for (int i = 0;; i++)
//            {
//                metadata_type type;
//                if (source->query_available_metadata_type(i, type) == false)
//                {
//                    break;
//                }
//
//                uint32_t size = source->query_buffer_size(type);
//                if (size == 0)
//                {
//                    LOG_ERROR("failed to query frame metadata size");
//                    continue;
//                }
//
//                std::vector<uint8_t> buffer(size);
//                if(source->get_metadata(type, buffer.data()) != size)
//                {
//                    LOG_ERROR("failed to query frame metadata");
//                    continue;
//                }
//                auto ros_type = static_cast<file_format::file_types::metadata_type>(type);
//                buffer.swap(target[ros_type]);
//            }
//
//            return true;
//        }
//
//
//        error_code read_property(std::shared_ptr<ros_data_objects::property> sample,
//                                                                               uint32_t &sensor_index, uint64_t &timestamp,
//                                                                               core::data_object **obj)
//        {
//            if(sample == nullptr || obj == nullptr)
//            {
//                return error_code::invalid_handle;
//            }
//
//            auto property_sample = std::static_pointer_cast<ros_data_objects::property>(sample);
//            sensor_index = property_sample->get_info().device_id;
//            timestamp = property_sample->get_info().capture_time.count() / 1000;
//
//           core::guid id;
//            if(utils::string_to_guid(property_sample->get_info().key, id) == false)
//            {
//                return error_code::read_failed;
//            }
//            double value;
//            value = property_sample->get_info().value;
//
//            if(data_objects::common::properties_data_object::create(&id, &value, 1,
//                                                                        obj) == false)
//            {
//                return error_code::data_object_creation_failed;
//            }
//            if(m_propertiesper_sensor.find(sensor_index) == m_propertiesper_sensor.end())
//            {
//                auto pair = std::pair<uint32_t, std::vector<core::guid>>(sensor_index, {id});
//                m_propertiesper_sensor.insert(pair);
//            }else
//            {
//                auto it = m_propertiesper_sensor.at(sensor_index);
//                if(std::find(it.begin(), it.end(), id) == it.end())
//                {
//                    m_propertiesper_sensor.at(sensor_index).push_back(id);
//                }
//            }
//            return error_code::no_error;
//
//        }
//
//        error_code read_properties_metadata(std::map<uint32_t,
//                                                                                                   std::vector<utils::ref_count_ptr<core::data_object>> > &properties)
//        {
//            uint64_t time_min = FIRST_FRAME_TIMESTAMP.count();
//            uint64_t timestamp_microseconds = time_min;
//            uint32_t sensor_index = 0;
//
//            m_propertiesper_sensor.clear();
//
//            while(timestamp_microseconds == time_min)
//            {
//               utils::ref_count_ptr<core::data_object> obj;
//                if(read(sensor_index, timestamp_microseconds, &obj) != no_error)
//                {
//                    return no_error;
//                }
//
//                if(timestamp_microseconds > time_min)
//                {
//                    continue;
//                }
//
//               utils::ref_count_ptr<extensions::common::properties_extension> properties_data_extension;
//                if (obj->extend_to(extensions::common::properties_extension::ID(), (void**)&properties_data_extension) == false)
//                {
//                    continue;
//                }
//
//                if(properties.find(sensor_index) == properties.end())
//                {
//                    auto pair = std::pair<int32_t, std::vector<utils::ref_count_ptr<core::data_object>>>(sensor_index, {obj});
//                    properties.insert(pair);
//                }else
//                {
//                    properties.at(sensor_index).push_back(obj);
//                }
//            }
//            auto timestamp_us =file_format::file_types::microseconds(timestamp_microseconds);;
//            auto retval = m_reader->seek_to_time(timestamp_us);
//            if (retval !=file_format::status_no_error)
//            {
//                return read_failed;
//            }
//
//            m_first_frame_time = timestamp_us;
//            return no_error;
//        }
//
//        error_code set_properties_state(uint64_t seek_time_us)
//        {
//            if(file_types::microseconds(seek_time_us) == m_first_frame_time || m_propertiesper_sensor.empty())
//            {
//                return no_error;
//            }
//
//            //Get all topics from all sensors
//            std::vector<std::string> topics;
//            for(auto sensor_properties : m_propertiesper_sensor)
//            {
//                for(auto guid : sensor_properties.second)
//                {
//                    std::stringstream ss;
//                    ss << guid;
//                    std::string topic = ros_data_objects::property::get_topic(ss.str(), sensor_properties.first);
//                    topics.push_back(topic);
//                }
//            }
//
//            auto prev_timestamp = FIRST_FRAME_TIMESTAMP; //TODO: query from reader once stream_playback adds get_position
//            std::vector<std::string> prev_filters{}; //TODO: query from reader once we add support for filtering for serializer
//           utils::scope_guard restore_state([this, prev_filters, prev_timestamp](){
//                //At the end of this function we should return to the state we were when we entered it
//                //Seeking to beginning of file in order to set the filters back (which at this point of code has no more frames- the iterator read all data)
//                if(m_reader->seek_to_time(file_format::file_types::nanoseconds(FIRST_FRAME_TIMESTAMP)) !=file_format::status_no_error)
//                {
//                    LOG_ERROR("Failed to seek to " << FIRST_FRAME_TIMESTAMP.count() << " while restoring serializer state");
//                    return;
//                }
//                if(m_reader->set_filter(prev_filters) !=file_format::status_no_error)
//                {
//                    LOG_ERROR("Failed to set filter while restoring serializer state");
//                    return;
//                }
//                if(m_reader->seek_to_time(file_format::file_types::nanoseconds(prev_timestamp)) !=file_format::status_no_error)
//                {
//                    LOG_ERROR("Failed to seek to " << prev_timestamp.count() << " while restoring serializer state");
//                    return;
//                }
//            });
//
//
//            //Seek to beginning of the file (after the header, i.e. first frame)
//            auto retval = m_reader->seek_to_time(m_first_frame_time);
//            if(retval !=file_format::status_no_error)
//            {
//                return read_failed;
//            }
//            //Add the properties topics as filters
//            if(m_reader->set_filter(topics) !=file_format::status_no_error)
//            {
//                return read_failed;
//            }
//
//            //remove all properties we read so far (will be updated at end of this function)
//            //TODO: restore them in case of failure?
//            m_cached_properties.clear();
//            std::vector<cached_property> properties;
//
//            //Get all properties up to time seek_time
//            uint64_t timestamp = m_first_frame_time.count();
//            while(timestamp < seek_time_us)
//            {
//                uint32_t sensor_index = 0;
//                //read_sample should only read properties (because we set the filter the properties only)
//               utils::ref_count_ptr<core::data_object> obj;
//                if (read_sample(sensor_index, timestamp, &obj) != no_error)
//                {
//                    break;
//                }
//
//                if (timestamp >= seek_time_us)
//                {
//                    continue;
//                }
//
//               utils::ref_count_ptr<extensions::common::properties_extension> properties_data_extension;
//                if (obj->extend_to(extensions::common::properties_extension::ID(), (void**) &properties_data_extension) == false)
//                {
//                    return read_failed;
//                }
//
//                auto properties_it = std::find_if(properties.begin(), properties.end(), [&sensor_index](cached_property& i)
//                {
//                    return (i.sensor_id == sensor_index);
//                });
//
//                if (properties_it == properties.end())
//                {
//                    cached_property property = {sensor_index, timestamp, obj};
//                    properties.emplace_back(property);
//                }
//                else
//                {
//                   utils::ref_count_ptr<core::metadata_extension> metadata_extension;
//                    if ((*properties_it).property->extend_to(core::metadata_extension::ID(), (void**) &metadata_extension) == false)
//                    {
//                        return read_failed;
//                    }
//                    if (metadata_extension->update(obj) == false)
//                    {
//                        (*properties_it).property = obj;
//                    }
//                    //update to latest property timestamp
//                    (*properties_it).timestamp = timestamp;
//                }
//            }
//            m_cached_properties = properties;
//            return no_error;
//        }
//
//        error_code read_stream_info(std::map<uint32_t, std::vector<utils::ref_count_ptr<core::data_object>> > &stream_infos,
//                                                                                  std::map<uint32_t, std::pair<core::extrinsics, uint64_t>>& extrinsics)
//        {
//            for(uint32_t sensor_index = 0; sensor_index < m_sensor_count; ++sensor_index)
//            {
//                std::vector<std::shared_ptr<ros_data_objects::stream_info>> stream_info;
//                auto ros_retval = m_reader->read_stream_infos(stream_info,
//                                                              file_types::sample_type::st_image, sensor_index);
//                if (ros_retval !=file_format::status::status_no_error)
//                {
//                    continue;
//                }
//
//                std::vector<sensors::camera::streaming_profile> profiles;
//                std::vector<core::intrinsics> intrinsics_info;
//                std::vector<core::stream_type> streams;
//                for(auto item : stream_info)
//                {
//                    std::shared_ptr<file_format::ros_data_objects::image_stream_info> image_item =
//                        std::static_pointer_cast<ros_data_objects::image_stream_info>(item);
//                   file_format::ros_data_objects::image_stream_data info = image_item->get_info();
//                   sensors::camera::streaming_profile profile = {};
//                    profile.fps = info.fps;
//                    profile.height = info.height;
//                    profile.width = info.width;
//                    if(conversions::convert(info.format, profile.format) == false)
//                    {
//                        return error_code::invalid_handle;
//                    }
//                    if(conversions::convert(info.type, profile.stream) == false)
//                    {
//                        return error_code::invalid_handle;
//                    }
//                   core::intrinsics intrinsics_item = {};
//                    if(conversions::convert(info.intrinsics, intrinsics_item) == false)
//                    {
//                        return error_code::invalid_handle;
//                    }
//                    intrinsics_item.height = info.height;
//                    intrinsics_item.width = info.width;
//                    profiles.push_back(profile);
//                    intrinsics_info.push_back(intrinsics_item);
//                    streams.push_back(profile.stream);
//
//                    if(extrinsics.find(sensor_index) == extrinsics.end())
//                    {
//                        std::pair<core::extrinsics, uint64_t> extrinsics_pair;
//                        extrinsics_pair.first = conversions::convert(info.stream_extrinsics.extrinsics_data);
//                        extrinsics_pair.second = info.stream_extrinsics.reference_point_id;
//                        extrinsics[sensor_index] = extrinsics_pair;
//                    }
//                }
//               utils::ref_count_ptr<core::data_object> obj_profile_info;
//                if(data_objects::camera::streaming_data_object::create(profiles.data(),
//                                                                           static_cast<uint32_t>(profiles.size()), &obj_profile_info) == false)
//                {
//                    return error_code::data_object_creation_failed;
//                }
//
//                stream_infos[sensor_index].push_back(obj_profile_info);
//
//                //If there is any extrinsic not empty, then it's a valid extension otherwise it is default value and is not a device metadata
//                if(std::any_of(intrinsics_info.begin(), intrinsics_info.end(), [&](constcore::intrinsics& i) { return is_empty_intrinsics(i) == false; }))
//                {
//                   utils::ref_count_ptr<core::data_object> obj_intrinsics;
//                    if (data_objects::camera::camera_intrinsics_data_object::create(streams.data(), intrinsics_info.data(),
//                                                                                        static_cast<uint32_t>(intrinsics_info
//                                                                                            .size()), &obj_intrinsics) == false)
//                    {
//                        return error_code::data_object_creation_failed;
//                    }
//                    stream_infos[sensor_index].push_back(obj_intrinsics);
//
//                   utils::ref_count_ptr<core::data_object> obj_intrinsics_per_profile;
//                    if(data_objects::camera::camera_intrinsics_per_stream_data_object::create(profiles.data(), intrinsics_info.data(),
//                                                                                                  static_cast<uint32_t>(intrinsics_info.size()), &obj_intrinsics_per_profile) == false)
//                    {
//                        return error_code::data_object_creation_failed;
//                    }
//                    stream_infos[sensor_index].push_back(obj_intrinsics_per_profile);
//                }
//            }
//            return error_code::no_error;
//        }
//
//        error_code read_motion_info(std::map<uint32_t,utils::ref_count_ptr<core::data_object>> &infos,
//                                                                                  std::map<uint32_t, std::pair<core::extrinsics, uint64_t>>& extrinsics)
//        {
//            for(uint32_t sensor_index = 0; sensor_index < m_sensor_count; ++sensor_index)
//            {
//                std::vector<std::shared_ptr<ros_data_objects::stream_info>> stream_info;
//                auto ros_retval = m_reader->read_stream_infos(stream_info,
//                                                             file_format::file_types::sample_type::st_motion, sensor_index);
//                if (ros_retval !=file_format::status::status_no_error)
//                {
//                    continue;
//                }
//
//                std::vector<sensors::motion::motion_profile> profiles;
//                std::map<core::motion_type,core::motion_device_intrinsics> motion_intrinsics;
//               utils::ref_count_ptr<core::data_object> obj;
//                for(auto item : stream_info)
//                {
//                    std::shared_ptr<file_format::ros_data_objects::motion_stream_info> motion_item =
//                        std::static_pointer_cast<ros_data_objects::motion_stream_info>(item);
//                   file_format::ros_data_objects::motion_stream_data info = motion_item->get_info();
//                   sensors::motion::motion_profile profile = {};
//                    profile.fps = info.fps;
//                    if(conversions::convert(info.type, profile.type) == false)
//                    {
//                        return error_code::invalid_handle;
//                    }
//                   core::motion_device_intrinsics intrinsics = conversions::convert(info.intrinsics);
//                    motion_intrinsics[profile.type] = intrinsics;
//                    if(extrinsics.find(sensor_index) == extrinsics.end())
//                    {
//                        std::pair<core::extrinsics, uint64_t> extrinsics_pair;
//                        extrinsics_pair.first = conversions::convert(info.stream_extrinsics.extrinsics_data);
//                        extrinsics_pair.second = info.stream_extrinsics.reference_point_id;
//                        extrinsics[sensor_index] = extrinsics_pair;
//
//                    }
//
//                    profiles.push_back(profile);
//                }
//
//                std::vector<core::motion_type>  supported_intrinsics_types;
//                std::vector<core::motion_device_intrinsics> intrinsics_values;
//                for (auto&& intrinsic : motion_intrinsics)
//                {
//                    supported_intrinsics_types.push_back(intrinsic.first);
//                    intrinsics_values.push_back(intrinsic.second);
//                }
//
//
//                if(data_objects::motion::motion_config_data_object::create(profiles.data(),
//                                                                               static_cast<uint32_t>(profiles.size()),
//                                                                               supported_intrinsics_types.data(),
//                                                                               intrinsics_values.data(),
//                                                                               static_cast<uint32_t>(intrinsics_values.size()),
//                                                                               &obj) == false)
//                {
//                    return error_code::data_object_creation_failed;
//                }
//                infos[sensor_index] = obj;
//            }
//            return error_code::no_error;
//
//        }
//

        std::shared_ptr<info_snapshot> read_info_snapshot(uint32_t id)
        {
            std::vector<std::shared_ptr<file_format::ros_data_objects::vendor_data>> vendor_data;
            if (m_stream_playback.read_vendor_data(vendor_data, id) !=file_format::status_no_error)
            {
                return nullptr;
            }
            std::map<rs2_camera_info, std::string> values;
            for (auto data : vendor_data)
            {
                try
                {
                    rs2_camera_info info = rs2_camera_info_from_string(data->get_info().name);
                    values[info] = data->get_info().value;
                }
                catch (const std::exception& e)
                {
                    std::cerr << e.what() << std::endl;
                }
            }
            return std::make_shared<info_snapshot>(values);
        }

        device_extrinsics read_device_extrinsics()
        {
            return {};
        }

        uint32_t read_sensor_count() const
        {
            std::vector<std::shared_ptr<file_format::ros_data_objects::vendor_data>> vendor_data;
            auto sts = m_stream_playback.read_vendor_data(vendor_data, DEVICE_INDEX);
            if (sts ==file_format::status_item_unavailable)
            {
                return 0;
            }

            for (auto data : vendor_data)
            {
                if (data->get_info().name == SENSOR_COUNT)
                {
                    auto value = data->get_info().value;
                    return std::stoi(value);
                }
            }
            return 0;
        }

        std::vector<stream_profile> read_stream_info(uint32_t sensor_id)
        {
            std::vector<stream_profile> profiles;
            std::vector<rs2_intrinsics> intrinsics_info;
            std::vector<rs2_stream> streams;

            std::vector<std::shared_ptr<file_format::ros_data_objects::stream_info>> stream_info;
            auto ros_retval = m_stream_playback.read_stream_infos(stream_info,file_format::file_types::sample_type::st_image, sensor_id);
            if (ros_retval !=file_format::status::status_no_error)
            {
                throw io_exception("Failed to read stream profiles");
            }
     
            for(auto item : stream_info)
            {
                std::shared_ptr<file_format::ros_data_objects::image_stream_info> image_item =
                    std::static_pointer_cast<file_format::ros_data_objects::image_stream_info>(item);
               file_format::ros_data_objects::image_stream_data info = image_item->get_info();
                stream_profile profile {};
                profile.fps = info.fps;
                profile.height = info.height;
                profile.width = info.width;
                profile.format = info.format;
                profile.stream = info.type;

                rs2_intrinsics intrinsics_item = info.intrinsics;
                intrinsics_item.height = info.height;
                intrinsics_item.width = info.width;
                profiles.push_back(profile);
                intrinsics_info.push_back(intrinsics_item);
                streams.push_back(profile.stream);
                //TODO: get intrinsics
                //if(extrinsics.find(sensor_index) == extrinsics.end())
                //{
                //    std::pair<core::extrinsics, uint64_t> extrinsics_pair;
                //    extrinsics_pair.first = conversions::convert(info.stream_extrinsics.extrinsics_data);
                //    extrinsics_pair.second = info.stream_extrinsics.reference_point_id;
                //    extrinsics[sensor_index] = extrinsics_pair;
                //}
            }
            return profiles;
        }

        std::shared_ptr<options_container> read_options()
        {
            return nullptr;
        }
        device_snapshot m_device_description;
        file_format::file_types::microseconds m_first_frame_time;
        //std::vector <cached_property> m_cached_properties;
        //std::map <uint32_t, std::vector<core::guid>> m_propertiesper_sensor;
        std::mutex m_mutex;
        stream_playback m_stream_playback;
    };


}
