// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "rosbag/bag.h"
#include "ros_file_format.h"

namespace librealsense
{
    using namespace device_serializer;

    class ros_writer: public writer
    {
    public:
        explicit ros_writer(const std::string& file, bool compress_while_record);
        void write_device_description(const librealsense::device_snapshot& device_description) override;
        void write_frame(const stream_identifier& stream_id, const nanoseconds& timestamp, frame_holder&& frame) override;
        void write_snapshot(uint32_t device_index, const nanoseconds& timestamp, rs2_extension type, const std::shared_ptr<extension_snapshot>& snapshot) override;
        void write_snapshot(const sensor_identifier& sensor_id, const nanoseconds& timestamp, rs2_extension type, const std::shared_ptr<extension_snapshot>& snapshot) override;
        const std::string& get_file_name() const override;

    private:
        void write_file_version();
        void write_frame_metadata(const stream_identifier& stream_id, const nanoseconds& timestamp, frame_interface* frame);
        void write_extrinsics(const stream_identifier& stream_id, frame_interface* frame);
        realsense_msgs::Notification to_notification_msg(const notification& n);
        void write_notification(const sensor_identifier& sensor_id, const nanoseconds& timestamp, const notification& n) override;
        void write_additional_frame_messages(const stream_identifier& stream_id, const nanoseconds& timestamp, frame_interface* frame);
        void write_video_frame(const stream_identifier& stream_id, const nanoseconds& timestamp, frame_holder&& frame);
        void write_motion_frame(const stream_identifier& stream_id, const nanoseconds& timestamp, frame_holder&& frame);
        inline geometry_msgs::Vector3 to_vector3(const float3& f);
        inline geometry_msgs::Quaternion to_quaternion(const float4& f);
        void write_pose_frame(const stream_identifier& stream_id, const nanoseconds& timestamp, frame_holder&& frame);
        void write_stream_info(nanoseconds timestamp, const sensor_identifier& sensor_id, std::shared_ptr<stream_profile_interface> profile);
        void write_streaming_info(nanoseconds timestamp, const sensor_identifier& sensor_id, std::shared_ptr<video_stream_profile_interface> profile);
        void write_streaming_info(nanoseconds timestamp, const sensor_identifier& sensor_id, std::shared_ptr<motion_stream_profile_interface> profile);
        void write_streaming_info(nanoseconds timestamp, const sensor_identifier& sensor_id, std::shared_ptr<pose_stream_profile_interface> profile);
        void write_extension_snapshot(uint32_t device_id, const nanoseconds& timestamp, rs2_extension type, std::shared_ptr<librealsense::extension_snapshot> snapshot);
        void write_extension_snapshot(uint32_t device_id, uint32_t sensor_id, const nanoseconds& timestamp, rs2_extension type, std::shared_ptr<librealsense::extension_snapshot> snapshot);

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

        void write_extension_snapshot(uint32_t device_id, uint32_t sensor_id, const nanoseconds& timestamp, rs2_extension type, std::shared_ptr<librealsense::extension_snapshot> snapshot, bool is_device);
        void write_vendor_info(const std::string& topic, nanoseconds timestamp, std::shared_ptr<info_interface> info_snapshot);
        void write_sensor_option(device_serializer::sensor_identifier sensor_id, const nanoseconds& timestamp, rs2_option type, const librealsense::option& option);
        void write_sensor_options(device_serializer::sensor_identifier sensor_id, const nanoseconds& timestamp, std::shared_ptr<options_interface> options);
        void write_l500_data(device_serializer::sensor_identifier sensor_id, const nanoseconds& timestamp, std::shared_ptr<l500_depth_sensor_interface> l500_depth_sensor);

        rs2_extension get_processing_block_extension(const std::shared_ptr<processing_block_interface> block);
        void write_sensor_processing_blocks(device_serializer::sensor_identifier sensor_id, const nanoseconds& timestamp, std::shared_ptr<recommended_proccesing_blocks_interface> proccesing_blocks);

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

        static uint8_t is_big_endian();
        std::map<stream_identifier, geometry_msgs::Transform> m_extrinsics_msgs;
        std::string m_file_path;
        rosbag::Bag m_bag;
        std::map<uint32_t, std::set<rs2_option>> m_written_options_descriptions;
    };
}
