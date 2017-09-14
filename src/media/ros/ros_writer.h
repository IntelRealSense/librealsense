// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <memory>
#include "core/debug.h"
#include "core/serialization.h"
#include "archive.h"
#include "types.h"
#include "stream.h"
#include "rosbag/bag.h"
#include "std_msgs/UInt32.h"
#include "diagnostic_msgs/KeyValue.h"
#include "sensor_msgs/Image.h"
#include "realsense_msgs/StreamInfo.h"
#include "sensor_msgs/CameraInfo.h"
#include "ros_file_format.h"

namespace librealsense
{
    using namespace device_serializer;

    class ros_writer: public writer
    {
    public:
        explicit ros_writer(const std::string& file)
        {
            m_bag.open(file, rosbag::BagMode::Write);
            m_bag.setCompression(rosbag::CompressionType::LZ4);
            write_file_version();
        }

        void write_device_description(const librealsense::device_snapshot& device_description) override
        {
            for (auto&& device_extension_snapshot : device_description.get_device_extensions_snapshots().get_snapshots())
            {
                write_extension_snapshot(get_device_index(), get_static_file_info_timestamp(), device_extension_snapshot.first, device_extension_snapshot.second);
            }

            for (auto&& sensors_snapshot : device_description.get_sensors_snapshots())
            {
                for (auto&& sensor_extension_snapshot : sensors_snapshot.get_sensor_extensions_snapshots().get_snapshots())
                {
                    write_extension_snapshot(get_device_index(), sensors_snapshot.get_sensor_index(), get_static_file_info_timestamp(), sensor_extension_snapshot.first, sensor_extension_snapshot.second);
                }
                write_sensor_options(ros_topic::property_topic({ get_device_index(),  sensors_snapshot.get_sensor_index() }), get_static_file_info_timestamp(), sensors_snapshot.get_options());
            }
        }

        void write_frame(const stream_identifier& stream_id, const nanoseconds& timestamp, frame_holder&& frame) override
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

        void write_snapshot(uint32_t device_index, const nanoseconds& timestamp, rs2_extension type, const std::shared_ptr<extension_snapshot > snapshot) override
        {
            write_extension_snapshot(device_index, -1, timestamp, type, snapshot);
        }

        void write_snapshot(const sensor_identifier& sensor_id, const nanoseconds& timestamp, rs2_extension type, const std::shared_ptr<extension_snapshot > snapshot) override
        {
            write_extension_snapshot(sensor_id.device_index, sensor_id.sensor_index, timestamp, type, snapshot);
        }

    private:
        void write_file_version()
        {
            std_msgs::UInt32 msg;
            msg.data = get_file_version();
            write_message(ros_topic::file_version_topic(), get_static_file_info_timestamp(), msg);
        }

        void write_video_frame(stream_identifier stream_id, const nanoseconds& timestamp, const frame_holder& frame)
        {
            sensor_msgs::Image image;
            auto vid_frame = dynamic_cast<librealsense::video_frame*>(frame.frame);
            assert(vid_frame != nullptr);

            image.width = static_cast<uint32_t>(vid_frame->get_width());
            image.height = static_cast<uint32_t>(vid_frame->get_height());
            image.step = static_cast<uint32_t>(vid_frame->get_stride());
            convert(vid_frame->get_stream()->get_format(), image.encoding);
            image.is_bigendian = is_big_endian();
            auto size = vid_frame->get_stride() * vid_frame->get_height();
            auto p_data = vid_frame->get_frame_data();
            image.data.assign(p_data, p_data + size);
            image.header.seq = static_cast<uint32_t>(vid_frame->get_frame_number());
            std::chrono::duration<double, std::milli> timestamp_ms(vid_frame->get_frame_timestamp());
            image.header.stamp = ros::Time(std::chrono::duration<double>(timestamp_ms).count());
            std::string TODO_CORRECT_ME = "0";
            image.header.frame_id = TODO_CORRECT_ME;
            auto image_topic = ros_topic::image_data_topic(stream_id);
            write_message(image_topic, timestamp, image);
            try
            {
                write_image_metadata(stream_id, timestamp, vid_frame);
            }
            catch (std::exception const& e)
            {
                LOG_WARNING("Failed to write image metadata for " << stream_id.device_index << "/" << stream_id.sensor_index << "/" << stream_id.stream_type << "/" << stream_id.stream_index << " ts: " << timestamp.count() << ". Exception: " << e.what());
            }
            try
            {
                write_extrinsics(stream_id, frame);
            }
            catch (std::exception const& e)
            {
                LOG_WARNING("Failed to write stream extrinsics for " << stream_id.device_index << "/" << stream_id.sensor_index << "/" << stream_id.stream_type << "/" << stream_id.stream_index << " ts: " << timestamp.count() << ". Exception: " << e.what());
            }
        }

        void write_extrinsics(const stream_identifier& stream_id, const frame_holder& frame)
        {
            if(m_extrinsics_msgs.find(stream_id) != m_extrinsics_msgs.end())
            {
                return; //already wrote it
            }
            auto& dev = frame.frame->get_sensor()->get_device();
            uint32_t reference_id = 0;
            rs2_extrinsics ext;
            std::tie(reference_id, ext) = dev.get_extrinsics(*frame.frame->get_stream());
            geometry_msgs::Transform tf_msg;
            convert(ext, tf_msg);
            write_message(ros_topic::stream_extrinsic_topic(stream_id, reference_id), get_static_file_info_timestamp(), tf_msg);
            m_extrinsics_msgs[stream_id] = tf_msg;
        }

        void write_image_metadata(const stream_identifier& stream_id, const nanoseconds& timestamp, video_frame* vid_frame)
        {
            auto metadata_topic = ros_topic::image_metadata_topic(stream_id);
            diagnostic_msgs::KeyValue system_time;
            system_time.key = "system_time";
            system_time.value = std::to_string(vid_frame->get_frame_system_time());
            write_message(metadata_topic, timestamp, system_time);

            diagnostic_msgs::KeyValue timestamp_domain;
            timestamp_domain.key = "timestamp_domain";
            timestamp_domain.value = to_string() << vid_frame->get_frame_timestamp_domain();
            write_message(metadata_topic, timestamp, timestamp_domain);

            for (int i = 0; i < static_cast<rs2_frame_metadata_value>(rs2_frame_metadata_value::RS2_FRAME_METADATA_COUNT); i++)
            {
                rs2_frame_metadata_value type = static_cast<rs2_frame_metadata_value>(i);
                if (vid_frame->supports_frame_metadata(type))
                {
                    auto md = vid_frame->get_frame_metadata(type);
                    diagnostic_msgs::KeyValue md_msg;
                    md_msg.key = to_string() << type;
                    md_msg.value = std::to_string(md);
                    write_message(metadata_topic, timestamp, md_msg);
                }
            }
        }

        void write_streaming_info(nanoseconds timestamp, const sensor_identifier& sensor_id, std::shared_ptr<video_stream_profile_interface> profile)
        {
            realsense_msgs::StreamInfo stream_info_msg;
            stream_info_msg.is_recommended = profile->is_default();
            convert(profile->get_format(), stream_info_msg.encoding);
            stream_info_msg.fps = profile->get_framerate();
            write_message(ros_topic::stream_info_topic({ sensor_id.device_index, sensor_id.sensor_index, profile->get_stream_type(), static_cast<uint32_t>(profile->get_stream_index()) }), timestamp, stream_info_msg);

            sensor_msgs::CameraInfo camera_info_msg;
            camera_info_msg.width = profile->get_width();
            camera_info_msg.height = profile->get_height();
            rs2_intrinsics intrinsics{};
            try {
                intrinsics = profile->get_intrinsics();
            }
            catch (...)
            {
                LOG_ERROR("Error trying to get intrinsc data for stream " << profile->get_stream_type() << ", " << profile->get_stream_index());
            }
            camera_info_msg.K[0] = intrinsics.fx;
            camera_info_msg.K[2] = intrinsics.ppx;
            camera_info_msg.K[4] = intrinsics.fy;
            camera_info_msg.K[5] = intrinsics.ppy;
            camera_info_msg.K[8] = 1;
            camera_info_msg.D.assign(std::begin(intrinsics.coeffs), std::end(intrinsics.coeffs));
            camera_info_msg.distortion_model = rs2_distortion_to_string(intrinsics.model);
            write_message(ros_topic::video_stream_info_topic({ sensor_id.device_index, sensor_id.sensor_index, profile->get_stream_type(), static_cast<uint32_t>(profile->get_stream_index()) }), timestamp, camera_info_msg);
        }

        void write_extension_snapshot(uint32_t device_id, const nanoseconds& timestamp, rs2_extension type, std::shared_ptr<librealsense::extension_snapshot> snapshot)
        {
            const auto ignored = 0u;
            write_extension_snapshot(device_id, ignored, timestamp, type, snapshot, true);
        }

        void write_extension_snapshot(uint32_t device_id, uint32_t sensor_id, const nanoseconds& timestamp, rs2_extension type, std::shared_ptr<librealsense::extension_snapshot> snapshot)
        {
            write_extension_snapshot(device_id, sensor_id, timestamp, type, snapshot, false);
        }

        void write_extension_snapshot(uint32_t device_id, uint32_t sensor_id, const nanoseconds& timestamp, rs2_extension type, std::shared_ptr<librealsense::extension_snapshot> snapshot, bool is_device)
        {
            switch (type)
            {
            case RS2_EXTENSION_INFO:
            {
                auto info = As<info_interface>(snapshot);
                if (info == nullptr)
                {
                    throw invalid_value_exception(to_string() << "Failed to cast snapshot with given type \"" << type << "\" to \"" << TypeToExtensionn<info_interface>::to_string() << "\"");
                }
                if (is_device)
                {
                    write_vendor_info(ros_topic::device_info_topic(device_id), timestamp, info);
                }
                else
                {
                    write_vendor_info(ros_topic::sensor_info_topic({ device_id, sensor_id }), timestamp, info);
                }
                break;
            }
            case RS2_EXTENSION_DEBUG:
            case RS2_EXTENSION_MOTION:
            case RS2_EXTENSION_OPTIONS:
            case RS2_EXTENSION_VIDEO:
            case RS2_EXTENSION_ROI:
                break;
            case RS2_EXTENSION_VIDEO_PROFILE:
            {
                auto profile = As<video_stream_profile_interface>(snapshot);
                if (profile == nullptr)
                {
                    throw invalid_value_exception(to_string() << "Failed to cast snapshot with given type \"" << type << "\" to \"" << TypeToExtensionn<info_interface>::to_string() << "\"");
                }
                write_streaming_info(timestamp, { device_id, sensor_id }, profile);
                break;
            }
            default:
                throw invalid_value_exception(to_string() << "Failed to Write Extension Snapshot: Unsupported extension \"" << type << "\"");

            }
        }

        void write_vendor_info(const std::string& topic, nanoseconds timestamp, std::shared_ptr<info_interface> info_snapshot)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(RS2_CAMERA_INFO_COUNT); i++)
            {
                auto camera_info = static_cast<rs2_camera_info>(i);
                if (info_snapshot->supports_info(camera_info))
                {
                    diagnostic_msgs::KeyValue msg;
                    msg.key = rs2_camera_info_to_string(camera_info);
                    msg.value = info_snapshot->get_info(camera_info);
                    write_message(topic, timestamp, msg);
                }
            }
        }

        void write_sensor_options(const std::string& topic, const nanoseconds& timestamp, const std::map<enum rs2_option, float>& options)
        {
            for (auto key_val : options)
            {
                diagnostic_msgs::KeyValue option_msg;
                option_msg.key = rs2_option_to_string(key_val.first);
                option_msg.value = std::to_string(key_val.second);
                write_message(topic, timestamp, option_msg);
            }
        }
        template <typename T>
        void write_message(std::string const& topic, nanoseconds const& time, T const& msg)
        {
            try
            {
                if (time == get_static_file_info_timestamp())
                {
                    m_bag.write(topic, ros::TIME_MIN, msg);
                }
                else
                {
                    auto secs = std::chrono::duration_cast<std::chrono::duration<double>>(time);
                    m_bag.write(topic, ros::Time(secs.count()), msg);
                }
            }
            catch (rosbag::BagIOException& e)
            {
                throw io_exception(to_string() << "Ros Writer failed to write topic: \"" << topic << "\" to file. (Exception message: " << e.what() << ")");
            }
        }

        static uint8_t is_big_endian()
        {
            int num = 1;
            return (*reinterpret_cast<char*>(&num) == 1) ? 0 : 1; //Little Endian: (char)0x0001 => 0x01, Big Endian: (char)0x0001 => 0x00,
        }
        std::map<stream_identifier, geometry_msgs::Transform> m_extrinsics_msgs;
        std::string m_file_path;
        rosbag::Bag m_bag;
    };
}
