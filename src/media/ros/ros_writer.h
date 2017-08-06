// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <memory>
#include <media/device_snapshot.h>
#include "core/debug.h"
#include "core/serialization.h"
#include "archive.h"
#include "types.h"
#include "media/ros/file_types.h"
#include "stream.h"

#include "file_types.h"
#include "ros/exception.h"
#include "rosbag/bag.h"
#include "std_msgs/UInt32.h"
#include "diagnostic_msgs/KeyValue.h"
#include "topic.h"
#include "sensor_msgs/Image.h"
#include "realsense_msgs/StreamInfo.h"
#include "sensor_msgs/CameraInfo.h"

namespace librealsense
{
    class ros_writer: public device_serializer::writer
    {
        const device_serializer::nanoseconds STATIC_INFO_TIMESTAMP = device_serializer::nanoseconds::min();
    public:
        ros_writer(const std::string& file) :
            m_file_path(file)
        {
            reset();
        }

        void write_device_description(const librealsense::device_snapshot& device_description) override
        {
            for (auto&& device_extension_snapshot : device_description.get_device_extensions_snapshots().get_snapshots())
            {
                auto ignored = 0u;
                write_extension_snapshot(get_device_index(), ignored, STATIC_INFO_TIMESTAMP, device_extension_snapshot.first, device_extension_snapshot.second);
            }
            uint32_t sensor_index = 0;
            for (auto&& sensors_snapshot : device_description.get_sensors_snapshots())
            {
                for (auto&& sensor_extension_snapshot : sensors_snapshot.get_sensor_extensions_snapshots().get_snapshots())
                {
                    write_extension_snapshot(get_device_index(), sensor_index, STATIC_INFO_TIMESTAMP, sensor_extension_snapshot.first, sensor_extension_snapshot.second); 
                }
                sensor_index++;
            }
        }

        void reset() override
        {
            try
            {
                m_bag.close();
                m_bag.open(m_file_path, rosbag::BagMode::Write);
                m_bag.setCompression(rosbag::CompressionType::LZ4);
                write_file_version();
            }
            catch(rosbag::BagIOException& e)
            {
                throw librealsense::invalid_value_exception(e.what());
            }
            catch(std::exception& e)
            {
                throw librealsense::invalid_value_exception(to_string() << "Failed to initialize writer. " << e.what());
            }
            m_first_frame_time = get_first_frame_timestamp();
        }

        void write_frame(const device_serializer::stream_identifier& stream_id, const device_serializer::nanoseconds& timestamp, frame_holder&& frame) override
        {
            if (Is<video_frame>(frame.frame))
            {
                write_video_frame(stream_id, timestamp, std::move(frame));
                return;
            }

           /* if (Is<motion_frame>(data.frame.get()))
            {
                write_motion(data);
                return;
            }

            if (Is<pose_frame>(data.frame.get()))
            {
                write_pose(data);
                return;
            }*/
        }

        void write_snapshot(uint32_t device_index, const device_serializer::nanoseconds& timestamp, rs2_extension type, const std::shared_ptr<extension_snapshot > snapshot) override
        {
            write_extension_snapshot(device_index, -1, timestamp, type, snapshot);
        }
        
        void write_snapshot(const device_serializer::sensor_identifier& sensor_id, const device_serializer::nanoseconds& timestamp, rs2_extension type, const std::shared_ptr<extension_snapshot > snapshot) override
        {
            write_extension_snapshot(sensor_id.device_index, sensor_id.sensor_index, timestamp, type, snapshot);
        }

        void write_file_version()
        {
            std_msgs::UInt32 msg;
            msg.data = get_file_version();
            write_message(get_file_version_topic(), STATIC_INFO_TIMESTAMP, msg);
        }

    private:

        void write_video_frame(device_serializer::stream_identifier stream_id, const device_serializer::nanoseconds& timestamp, const frame_holder& frame)
        {
            sensor_msgs::Image image;
            auto vid_frame = dynamic_cast<librealsense::video_frame*>(frame.frame);
            assert(vid_frame != nullptr);

            image.width = static_cast<uint32_t>(vid_frame->get_width());
            image.height = static_cast<uint32_t>(vid_frame->get_height());
            image.step = static_cast<uint32_t>(vid_frame->get_stride());
            image.encoding = rs2_format_to_string(vid_frame->get_stream()->get_format());
            image.is_bigendian = 0;
            auto size = vid_frame->get_stride() * vid_frame->get_height();
            auto p_data = vid_frame->get_frame_data();
            image.data.assign(p_data, p_data + size);
            image.header.seq = static_cast<uint32_t>(vid_frame->get_frame_number());
            std::chrono::duration<double, std::milli> timestamp_ms(vid_frame->get_frame_timestamp());
            image.header.stamp = ros::Time(std::chrono::duration<double>(timestamp_ms).count());
            auto image_topic = ros_topic::image_data_topic(stream_id);
            write_message(image_topic, timestamp, image);

            auto metadata_topic = ros_topic::image_metadata_topic(stream_id);
            diagnostic_msgs::KeyValue system_time;
            system_time.key = "system_time";
            system_time.value = "0";
            write_message(metadata_topic, timestamp, system_time);

            diagnostic_msgs::KeyValue timestamp_domain;
            timestamp_domain.key = "time_stamp_domain";
            timestamp_domain.value = to_string() << vid_frame->get_frame_timestamp_domain();
            write_message(metadata_topic, timestamp, timestamp_domain);

            for (int i = 0; i < static_cast<rs2_frame_metadata>(rs2_frame_metadata::RS2_FRAME_METADATA_COUNT); i++)
            {
                rs2_frame_metadata type = static_cast<rs2_frame_metadata>(i);
                if (frame.frame->supports_frame_metadata(type))
                {
                    auto md = frame.frame->get_frame_metadata(type);
                    diagnostic_msgs::KeyValue md_msg;
                    md_msg.key = to_string() << type;
                    md_msg.value = std::to_string(md);
                    write_message(metadata_topic, timestamp, md_msg);
                }
            }
        }

        void write_streaming_info(device_serializer::nanoseconds timestamp, const device_serializer::sensor_identifier& sensor_id, std::shared_ptr<video_stream_profile_interface> profile)
        {
            realsense_msgs::StreamInfo stream_info_msg;
            stream_info_msg.unique_id = std::to_string(profile->get_unique_id());
            conversions::convert(profile->get_stream_type(), stream_info_msg.encoding);
            stream_info_msg.fps = profile->get_framerate();
            write_message(ros_topic::stream_info_topic({ sensor_id.device_index, sensor_id.sensor_index, profile->get_stream_type(), static_cast<uint32_t>(profile->get_stream_index()) }), timestamp, stream_info_msg);

            sensor_msgs::CameraInfo camera_info_msg;
            camera_info_msg.width = profile->get_width();
            camera_info_msg.height = profile->get_height();
            camera_info_msg.distortion_model = profile->get_intrinsics().model;
            camera_info_msg.K[0] = profile->get_intrinsics().fx;
            camera_info_msg.K[2] = profile->get_intrinsics().ppx;
            camera_info_msg.K[4] = profile->get_intrinsics().fy;
            camera_info_msg.K[5] = profile->get_intrinsics().ppy;
            camera_info_msg.K[8] = 1;
            camera_info_msg.D.assign(profile->get_intrinsics().coeffs, profile->get_intrinsics().coeffs + 5);
            conversions::convert(profile->get_intrinsics().model, camera_info_msg.distortion_model);
            write_message(ros_topic::stream_info_topic({ sensor_id.device_index, sensor_id.sensor_index, profile->get_stream_type(), static_cast<uint32_t>(profile->get_stream_index()) }), timestamp, camera_info_msg);
        }

        void write_extension_snapshot(uint32_t device_id, uint32_t sensor_id, const device_serializer::nanoseconds& timestamp, rs2_extension type, std::shared_ptr<librealsense::extension_snapshot> snapshot)
        {
            bool is_device_snapshot = Is<device_interface>(snapshot);
            switch (type)
            {
            case RS2_EXTENSION_UNKNOWN:
                throw invalid_value_exception("Unknown extension");
            case RS2_EXTENSION_DEBUG :
                break;
            case RS2_EXTENSION_INFO :
            {
                auto info = As<info_interface>(snapshot);
                if(info == nullptr)
                {
                    throw invalid_value_exception(to_string() << "Failed to cast snapshot with given type \"" << type << "\" to \"" << TypeToExtensionn<info_interface>::to_string() << "\"");
                }
                if (is_device_snapshot) 
                {
                    write_vendor_info(timestamp, device_id, info);
                }
                else
                {
                    write_vendor_info(timestamp, { device_id, sensor_id }, info);
                }
                break;
            }
            case RS2_EXTENSION_MOTION :
                break;
            case RS2_EXTENSION_OPTIONS :
                break;
            case RS2_EXTENSION_VIDEO :
                break;
            case RS2_EXTENSION_ROI :
                break;
            case RS2_EXTENSION_VIDEO_FRAME :
                break;
            case RS2_EXTENSION_MOTION_FRAME :
                break;
            case RS2_EXTENSION_COUNT :
                break;
            case RS2_EXTENSION_VIDEO_PROFILE:
            {   
                auto profile = As<video_stream_profile_interface>(snapshot);
                if (profile == nullptr)
                {
                    throw invalid_value_exception(to_string() << "Failed to cast snapshot with given type \"" << type << "\" to \"" << TypeToExtensionn<info_interface>::to_string() << "\"");
                }
                write_streaming_info(timestamp, { device_id, sensor_id }, profile);
            }
            default:
                throw invalid_value_exception("Unsupported extension");

            }
        }

        void write_vendor_info(device_serializer::nanoseconds timestamp, uint32_t device_index, std::shared_ptr<info_interface> info_snapshot)
        {
            write_vendor_info(ros_topic::device_info_topic(device_index), timestamp, info_snapshot);
        }

        void write_vendor_info(device_serializer::nanoseconds timestamp, const device_serializer::sensor_identifier& sensor_id, std::shared_ptr<info_interface> info_snapshot)
        {
            write_vendor_info(ros_topic::sensor_info_topic(sensor_id), timestamp, info_snapshot);
        }
        
        void write_vendor_info(const std::string& topic, device_serializer::nanoseconds timestamp, std::shared_ptr<info_interface> info_snapshot)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(RS2_CAMERA_INFO_COUNT); i++)
            {
                auto camera_info = static_cast<rs2_camera_info>(i);
                if (info_snapshot->supports_info(camera_info))
                {
                    diagnostic_msgs::KeyValue msg;
                    msg.key = info_snapshot->get_info(camera_info);
                    msg.value = rs2_camera_info_to_string(camera_info);
                    write_message(topic, timestamp, msg);
                }
            }
        }
       
        template <typename T>
        void write_message(std::string const& topic, device_serializer::nanoseconds const& time, T const& msg)
        {
            try
            {
                if (time == STATIC_INFO_TIMESTAMP)
                {
                    m_bag.write(topic, ros::TIME_MIN, msg);
                }
                else
                {
                    //TODO: simply convert time to <double, sec>
                    std::chrono::duration<uint32_t> sec = std::chrono::duration_cast<std::chrono::duration<uint32_t>>(time);
                    device_serializer::nanoseconds nanos = time - std::chrono::duration_cast<device_serializer::nanoseconds>(sec);
                    auto unanos = std::chrono::duration_cast<std::chrono::duration<uint32_t, std::nano>>(nanos);
                    ros::Time capture_time = ros::Time(sec.count(), unanos.count());
                    m_bag.write(topic, capture_time, msg);
                }
            }
            catch (rosbag::BagIOException& e)
            {
                throw io_exception(to_string() << "Ros Writer: Failed to write topic: \"" << topic << "\" to file. (rosbag error: " << e.what() << ")");
            }
        }

        device_serializer::nanoseconds m_first_frame_time;
        std::string m_file_path;
        rosbag::Bag m_bag;
    };
}
