// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <memory>
#include <iomanip>
#include <ios>      //For std::hexfloat
#include "core/debug.h"
#include "core/serialization.h"
#include "archive.h"
#include "types.h"
#include "stream.h"
#include "rosbag/bag.h"
#include "ros_file_format.h"

namespace librealsense
{
    using namespace device_serializer;

    class ros_writer: public writer
    {
    public:
        explicit ros_writer(const std::string& file, bool compress_while_record) : m_file_path(file)
        {
            LOG_INFO("Compression while record is set to " << (compress_while_record ? "ON" : "OFF"));
            m_bag.open(file, rosbag::BagMode::Write);
            if (compress_while_record)
            {
                m_bag.setCompression(rosbag::CompressionType::LZ4);
            }
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
            }
        }

        void write_frame(const stream_identifier& stream_id, const nanoseconds& timestamp, frame_holder&& frame) 
        {
            if (Is<video_frame>(frame.frame))
            {
                write_video_frame(stream_id, timestamp, std::move(frame));
                return;
            }

            if (Is<motion_frame>(frame.frame))
            {
                write_motion_frame(stream_id, timestamp, std::move(frame));
                return;
            }

            if (Is<pose_frame>(frame.frame))
            {
                write_pose_frame(stream_id, timestamp, std::move(frame));
                return;
            }
        }

        void write_snapshot(uint32_t device_index, const nanoseconds& timestamp, rs2_extension type, const std::shared_ptr<extension_snapshot>& snapshot) override
        {
            write_extension_snapshot(device_index, -1, timestamp, type, snapshot);
        }

        void write_snapshot(const sensor_identifier& sensor_id, const nanoseconds& timestamp, rs2_extension type, const std::shared_ptr<extension_snapshot>& snapshot) override
        {
            write_extension_snapshot(sensor_id.device_index, sensor_id.sensor_index, timestamp, type, snapshot);
        }

        const std::string& get_file_name() const override
        {
            return m_file_path;
        }

    private:
        void write_file_version()
        {
            std_msgs::UInt32 msg;
            msg.data = get_file_version();
            write_message(ros_topic::file_version_topic(), get_static_file_info_timestamp(), msg);
        }


        void write_frame_metadata(const stream_identifier& stream_id, const nanoseconds& timestamp, frame_interface* frame)
        {
            auto metadata_topic = ros_topic::frame_metadata_topic(stream_id);
            diagnostic_msgs::KeyValue system_time;
            system_time.key = SYSTEM_TIME_MD_STR;
            system_time.value = std::to_string(frame->get_frame_system_time());
            write_message(metadata_topic, timestamp, system_time);

            diagnostic_msgs::KeyValue timestamp_domain;
            timestamp_domain.key = TIMESTAMP_DOMAIN_MD_STR;
            timestamp_domain.value = librealsense::get_string(frame->get_frame_timestamp_domain());
            write_message(metadata_topic, timestamp, timestamp_domain);

            for (int i = 0; i < static_cast<rs2_frame_metadata_value>(rs2_frame_metadata_value::RS2_FRAME_METADATA_COUNT); i++)
            {
                rs2_frame_metadata_value type = static_cast<rs2_frame_metadata_value>(i);
                if (frame->supports_frame_metadata(type))
                {
                    auto md = frame->get_frame_metadata(type);
                    diagnostic_msgs::KeyValue md_msg;
                    md_msg.key = librealsense::get_string(type);
                    md_msg.value = std::to_string(md);
                    write_message(metadata_topic, timestamp, md_msg);
                }
            }
        }

        void write_extrinsics(const stream_identifier& stream_id, frame_interface* frame)
        {
            if (m_extrinsics_msgs.find(stream_id) != m_extrinsics_msgs.end())
            {
                return; //already wrote it
            }
            auto& dev = frame->get_sensor()->get_device();
            uint32_t reference_id = 0;
            rs2_extrinsics ext;
            std::tie(reference_id, ext) = dev.get_extrinsics(*frame->get_stream());
            geometry_msgs::Transform tf_msg;
            convert(ext, tf_msg);
            write_message(ros_topic::stream_extrinsic_topic(stream_id, reference_id), get_static_file_info_timestamp(), tf_msg);
            m_extrinsics_msgs[stream_id] = tf_msg;
        }

        realsense_msgs::Notification to_notification_msg(const notification& n)
        {
            realsense_msgs::Notification noti_msg;
            noti_msg.category = get_string(n.category);
            noti_msg.severity = get_string(n.severity);
            noti_msg.description = n.description;
            auto secs = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::duration<double, std::nano>(n.timestamp));
            noti_msg.timestamp = rs2rosinternal::Time(secs.count());
            noti_msg.serialized_data = n.serialized_data;
            return noti_msg;
        }

        void write_notification(const sensor_identifier& sensor_id, const nanoseconds& timestamp, const notification& n)
        {
            realsense_msgs::Notification noti_msg = to_notification_msg(n);
            write_message(ros_topic::notification_topic({ sensor_id.device_index, sensor_id.sensor_index}, n.category), timestamp, noti_msg);
        }

        void write_additional_frame_messages(const stream_identifier& stream_id, const nanoseconds& timestamp, frame_interface* frame)
        {
            try
            {
                write_frame_metadata(stream_id, timestamp, frame);
            }
            catch (std::exception const& e)
            {
                LOG_WARNING("Failed to write frame metadata for " << stream_id << ". Exception: " << e.what());
            }

            try
            {
                write_extrinsics(stream_id, frame);
            }
            catch (std::exception const& e)
            {
                LOG_WARNING("Failed to write stream extrinsics for " << stream_id << ". Exception: " << e.what());
            }
        }

        void write_video_frame(const stream_identifier& stream_id, const nanoseconds& timestamp, frame_holder&& frame)
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
            image.header.stamp = rs2rosinternal::Time(std::chrono::duration<double>(timestamp_ms).count());
            std::string TODO_CORRECT_ME = "0";
            image.header.frame_id = TODO_CORRECT_ME;
            auto image_topic = ros_topic::frame_data_topic(stream_id);
            write_message(image_topic, timestamp, image);
            write_additional_frame_messages(stream_id, timestamp, frame);
        }

        void write_motion_frame(const stream_identifier& stream_id, const nanoseconds& timestamp, frame_holder&& frame)
        {
            sensor_msgs::Imu imu_msg;
            if (!frame)
            {
                throw io_exception("Null frame passed to write_motion_frame");
            }
            
            imu_msg.header.seq = static_cast<uint32_t>(frame.frame->get_frame_number());
            std::chrono::duration<double, std::milli> timestamp_ms(frame.frame->get_frame_timestamp());
            imu_msg.header.stamp = rs2rosinternal::Time(std::chrono::duration<double>(timestamp_ms).count());
            std::string TODO_CORRECT_ME = "0";
            imu_msg.header.frame_id = TODO_CORRECT_ME;
            auto data_ptr = reinterpret_cast<const float*>(frame.frame->get_frame_data());
            if (stream_id.stream_type == RS2_STREAM_ACCEL)
            {
                imu_msg.linear_acceleration.x = data_ptr[0];
                imu_msg.linear_acceleration.y = data_ptr[1];
                imu_msg.linear_acceleration.z = data_ptr[2];
            }
           
            else if (stream_id.stream_type == RS2_STREAM_GYRO)
            {
                imu_msg.angular_velocity.x = data_ptr[0];
                imu_msg.angular_velocity.y = data_ptr[1];
                imu_msg.angular_velocity.z = data_ptr[2]; 
            }
            else
            {
                throw io_exception("Unsupported stream type for a motion frame");
            }

            auto topic = ros_topic::frame_data_topic(stream_id);
            write_message(topic, timestamp, imu_msg);
            write_additional_frame_messages(stream_id, timestamp, frame);
        }

        inline geometry_msgs::Vector3 to_vector3(const float3& f)
        {
            geometry_msgs::Vector3 v;
            v.x = f.x;
            v.y = f.y;
            v.z = f.z;
            return v;
        }

        inline geometry_msgs::Quaternion to_quaternion(const float4& f)
        {
            geometry_msgs::Quaternion q;
            q.x = f.x;
            q.y = f.y;
            q.z = f.z;
            q.w = f.w;
            return q;
        }

        void write_pose_frame(const stream_identifier& stream_id, const nanoseconds& timestamp, frame_holder&& frame)
        {
            auto pose = As<librealsense::pose_frame>(frame.frame);
            if (!frame)
            {
                throw io_exception("Null frame passed to write_motion_frame");
            }
            auto rotation = pose->get_rotation();

            geometry_msgs::Transform transform;
            geometry_msgs::Accel accel;
            geometry_msgs::Twist twist;

            transform.rotation = to_quaternion(pose->get_rotation());
            transform.translation = to_vector3(pose->get_translation());
            accel.angular = to_vector3(pose->get_angular_acceleration());
            accel.linear = to_vector3(pose->get_acceleration());
            twist.angular = to_vector3(pose->get_angular_velocity());
            twist.linear = to_vector3(pose->get_velocity());

            std::string transform_topic = ros_topic::pose_transform_topic(stream_id);
            std::string accel_topic = ros_topic::pose_accel_topic(stream_id);
            std::string twist_topic = ros_topic::pose_twist_topic(stream_id);

            //Write the the pose frame as 3 seperate messages (each with different topic)
            write_message(transform_topic, timestamp, transform);
            write_message(accel_topic, timestamp, accel);
            write_message(twist_topic, timestamp, twist);

            // Write the pose confidence as metadata for the pose frame
            std::string md_topic = ros_topic::frame_metadata_topic(stream_id);
            
            diagnostic_msgs::KeyValue tracker_confidence_msg;
            tracker_confidence_msg.key = TRACKER_CONFIDENCE_MD_STR;
            tracker_confidence_msg.value = std::to_string(pose->get_tracker_confidence());
            write_message(md_topic, timestamp, tracker_confidence_msg);

            diagnostic_msgs::KeyValue mapper_confidence_msg;
            mapper_confidence_msg.key = MAPPER_CONFIDENCE_MD_STR;
            mapper_confidence_msg.value = std::to_string(pose->get_mapper_confidence());
            write_message(md_topic, timestamp, mapper_confidence_msg);

            //Write frame's timestamp as metadata
            diagnostic_msgs::KeyValue frame_timestamp_msg;
            frame_timestamp_msg.key = FRAME_TIMESTAMP_MD_STR;
            frame_timestamp_msg.value = to_string() << std::hexfloat << pose->get_frame_timestamp();
            write_message(md_topic, timestamp, frame_timestamp_msg);
            
            // Write the rest of the frame metadata and stream extrinsics
            write_additional_frame_messages(stream_id, timestamp, frame);
        }

        void write_stream_info(nanoseconds timestamp, const sensor_identifier& sensor_id, std::shared_ptr<stream_profile_interface> profile)
        {
            realsense_msgs::StreamInfo stream_info_msg;
            stream_info_msg.is_recommended = profile->get_tag() & profile_tag::PROFILE_TAG_DEFAULT;
            convert(profile->get_format(), stream_info_msg.encoding);
            stream_info_msg.fps = profile->get_framerate();
            write_message(ros_topic::stream_info_topic({ sensor_id.device_index, sensor_id.sensor_index, profile->get_stream_type(), static_cast<uint32_t>(profile->get_stream_index()) }), timestamp, stream_info_msg);
        }

        void write_streaming_info(nanoseconds timestamp, const sensor_identifier& sensor_id, std::shared_ptr<video_stream_profile_interface> profile)
        {
            write_stream_info(timestamp, sensor_id, profile);
            
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

        void write_streaming_info(nanoseconds timestamp, const sensor_identifier& sensor_id, std::shared_ptr<motion_stream_profile_interface> profile)
        {
            write_stream_info(timestamp, sensor_id, profile);

            realsense_msgs::ImuIntrinsic motion_info_msg;
            
            rs2_motion_device_intrinsic intrinsics{};
            try {
                intrinsics = profile->get_intrinsics();
            }
            catch (...)
            {
                LOG_ERROR("Error trying to get intrinsc data for stream " << profile->get_stream_type() << ", " << profile->get_stream_index());
            }
            //Writing empty in case of failure
            std::copy(&intrinsics.data[0][0], &intrinsics.data[0][0] + motion_info_msg.data.size(), std::begin(motion_info_msg.data));
            std::copy(std::begin(intrinsics.bias_variances) , std::end(intrinsics.bias_variances), std::begin(motion_info_msg.bias_variances));
            std::copy(std::begin(intrinsics.noise_variances), std::end(intrinsics.noise_variances), std::begin(motion_info_msg.noise_variances));

            std::string topic = ros_topic::imu_intrinsic_topic({ sensor_id.device_index, sensor_id.sensor_index, profile->get_stream_type(), static_cast<uint32_t>(profile->get_stream_index()) });
            write_message(topic, timestamp, motion_info_msg);
        }
        void write_streaming_info(nanoseconds timestamp, const sensor_identifier& sensor_id, std::shared_ptr<pose_stream_profile_interface> profile)
        {
            write_stream_info(timestamp, sensor_id, profile);
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

        template <rs2_extension E>
        std::shared_ptr<typename ExtensionToType<E>::type> SnapshotAs(std::shared_ptr<librealsense::extension_snapshot> snapshot)
        {
            auto as_type = As<typename ExtensionToType<E>::type>(snapshot);
            if (as_type == nullptr)
            {
                throw invalid_value_exception(to_string() << "Failed to cast snapshot to \"" << E << "\" (as \"" << ExtensionToType<E>::to_string() << "\")");
            }
            return as_type;
        }
        void write_extension_snapshot(uint32_t device_id, uint32_t sensor_id, const nanoseconds& timestamp, rs2_extension type, std::shared_ptr<librealsense::extension_snapshot> snapshot, bool is_device)
        {
            switch (type)
            {
            case RS2_EXTENSION_INFO:
            {
                auto info = SnapshotAs<RS2_EXTENSION_INFO>(snapshot);
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
                break;
            //case RS2_EXTENSION_OPTION:
            //{
            //    auto option = SnapshotAs<RS2_EXTENSION_OPTION>(snapshot);
            //    write_sensor_option({ device_id, sensor_id }, timestamp, *option);
            //    break;
            //}
            case RS2_EXTENSION_OPTIONS:
            {
                auto options = SnapshotAs<RS2_EXTENSION_OPTIONS>(snapshot);
                write_sensor_options({ device_id, sensor_id }, timestamp, options);
                break;
            }
            case RS2_EXTENSION_VIDEO:
            case RS2_EXTENSION_ROI:
            case RS2_EXTENSION_DEPTH_SENSOR:
            case RS2_EXTENSION_DEPTH_STEREO_SENSOR:
                break;
            case RS2_EXTENSION_VIDEO_PROFILE:
            {
                auto profile = SnapshotAs<RS2_EXTENSION_VIDEO_PROFILE>(snapshot);
                write_streaming_info(timestamp, { device_id, sensor_id }, profile);
                break;
            }
            case RS2_EXTENSION_MOTION_PROFILE:
            {
                auto profile = SnapshotAs<RS2_EXTENSION_MOTION_PROFILE>(snapshot);
                write_streaming_info(timestamp, { device_id, sensor_id }, profile);
                break;
            }
            case RS2_EXTENSION_POSE_PROFILE:
            {
                auto profile = SnapshotAs<RS2_EXTENSION_POSE_PROFILE>(snapshot);
                write_streaming_info(timestamp, { device_id, sensor_id }, profile);
                break;
            }
            default:
                throw invalid_value_exception(to_string() << "Failed to Write Extension Snapshot: Unsupported extension \"" << librealsense::get_string(type) << "\"");
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
        
        void write_sensor_option(device_serializer::sensor_identifier sensor_id, const nanoseconds& timestamp, rs2_option type, const librealsense::option& option)
        {
            float value = option.query();
            const char* str = option.get_description();
            std::string description = str ? std::string(str) : (to_string() << "Read only option of " << librealsense::get_string(type));

            //One message for value
            std_msgs::Float32 option_msg;
            option_msg.data = value;
            write_message(ros_topic::option_value_topic(sensor_id, type), timestamp, option_msg);

            //Another message for description, should be written once per topic

            if (m_written_options_descriptions[sensor_id.sensor_index].find(type) == m_written_options_descriptions[sensor_id.sensor_index].end())
            {
                std_msgs::String option_msg_desc;
                option_msg_desc.data = description;
                write_message(ros_topic::option_description_topic(sensor_id, type), get_static_file_info_timestamp(), option_msg_desc);
                m_written_options_descriptions[sensor_id.sensor_index].insert(type);
            }
        }

        void write_sensor_options(device_serializer::sensor_identifier sensor_id, const nanoseconds& timestamp, std::shared_ptr<options_interface> options)
        {
            for (int i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++)
            {
                auto option_id = static_cast<rs2_option>(i);
                try
                {
                    if (options->supports_option(option_id))
                    {
                        write_sensor_option(sensor_id, timestamp, option_id, options->get_option(option_id));
                    }
                }
                catch (std::exception& e)
                {
                    LOG_WARNING("Failed to get or write option " << option_id << " for sensor " << sensor_id.sensor_index << ". Exception: " << e.what());
                }
            }
        }
        template <typename T>
        void write_message(std::string const& topic, nanoseconds const& time, T const& msg)
        {
            try
            {
                m_bag.write(topic, to_rostime(time), msg);
                LOG_DEBUG("Recorded: \"" << topic << "\" . TS: " << time.count());
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
        std::map<uint32_t, std::set<rs2_option>> m_written_options_descriptions;
    };
}
