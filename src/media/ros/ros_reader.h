// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <core/serialization.h>
#include "rosbag/view.h"
#include "ros_file_format.h"

namespace librealsense
{
    using namespace device_serializer;

    class ros_reader: public device_serializer::reader
    {
    public:
        ros_reader(const std::string& file, const std::shared_ptr<context>& ctx);
        device_snapshot query_device_description(const nanoseconds& time) override;
        std::shared_ptr<serialized_data> read_next_data() override;
        void seek_to_time(const nanoseconds& seek_time) override;
        std::vector<std::shared_ptr<serialized_data>> fetch_last_frames(const nanoseconds& seek_time) override;
        nanoseconds query_duration() const override;
        void reset() override;
        virtual void enable_stream(const std::vector<device_serializer::stream_identifier>& stream_ids) override;
        virtual void disable_stream(const std::vector<device_serializer::stream_identifier>& stream_ids) override;
        const std::string& get_file_name() const override;

    private:

        template <typename ROS_TYPE>
        static typename ROS_TYPE::ConstPtr instantiate_msg(const rosbag::MessageInstance& msg)
        {
            typename ROS_TYPE::ConstPtr msg_instnance_ptr = msg.instantiate<ROS_TYPE>();
            if (msg_instnance_ptr == nullptr)
            {
                throw io_exception(to_string()
                    << "Invalid file format, expected "
                    << rs2rosinternal::message_traits::DataType<ROS_TYPE>::value()
                    << " message but got: " << msg.getDataType()
                    << "(Topic: " << msg.getTopic() << ")");
            }
            return msg_instnance_ptr;
        }

        std::shared_ptr<serialized_frame> create_frame(const rosbag::MessageInstance& msg);
        static nanoseconds get_file_duration(const rosbag::Bag& file, uint32_t version);
        static void get_legacy_frame_metadata(const rosbag::Bag& bag,
            const device_serializer::stream_identifier& stream_id,
            const rosbag::MessageInstance &msg,
            frame_additional_data& additional_data);

        template <typename T>
        static bool safe_convert(const std::string& key, T& val)
        {
            try
            {
                convert(key, val);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR(e.what());
                return false;
            }
            return true;
        }

        static std::map<std::string, std::string> get_frame_metadata(const rosbag::Bag& bag,
            const std::string& topic,
            const device_serializer::stream_identifier& stream_id,
            const rosbag::MessageInstance &msg,
            frame_additional_data& additional_data);
        frame_holder create_image_from_message(const rosbag::MessageInstance &image_data) const;
        frame_holder create_motion_sample(const rosbag::MessageInstance &motion_data) const;
        static inline float3 to_float3(const geometry_msgs::Vector3& v);
        static inline float4 to_float4(const geometry_msgs::Quaternion& q);
        frame_holder create_pose_sample(const rosbag::MessageInstance &msg) const;
        static uint32_t read_file_version(const rosbag::Bag& file);
        bool try_read_legacy_stream_extrinsic(const stream_identifier& stream_id, uint32_t& group_id, rs2_extrinsics& extrinsic) const;
        bool try_read_stream_extrinsic(const stream_identifier& stream_id, uint32_t& group_id, rs2_extrinsics& extrinsic) const;
        static void update_sensor_options(const rosbag::Bag& file, uint32_t sensor_index, const nanoseconds& time, uint32_t file_version, snapshot_collection& sensor_extensions, uint32_t version);
        void update_proccesing_blocks(const rosbag::Bag& file, uint32_t sensor_index, const nanoseconds& time, uint32_t file_version, snapshot_collection& sensor_extensions, uint32_t version, std::string pid, std::string sensor_name);
        bool is_depth_sensor(std::string sensor_name);
        bool is_color_sensor(std::string sensor_name);
        bool is_motion_module_sensor(std::string sensor_name);
        bool is_ds5_PID(int pid);
        bool is_sr300_PID(int pid);
        bool is_l500_PID(int pid);
        std::shared_ptr<recommended_proccesing_blocks_snapshot> read_proccesing_blocks_for_version_under_4(std::string pid, std::string sensor_name, std::shared_ptr<options_interface> options);
        std::shared_ptr<recommended_proccesing_blocks_snapshot> read_proccesing_blocks(const rosbag::Bag& file, device_serializer::sensor_identifier sensor_id, const nanoseconds& timestamp,
            std::shared_ptr<options_interface> options, uint32_t file_version, std::string pid, std::string sensor_name);
        device_snapshot read_device_description(const nanoseconds& time, bool reset = false);
        std::shared_ptr<info_container> read_legacy_info_snapshot(uint32_t sensor_index) const;
        std::shared_ptr<info_container> read_info_snapshot(const std::string& topic) const;
        std::set<uint32_t> read_sensor_indices(uint32_t device_index) const;
        static std::shared_ptr<stream_profile_base> create_pose_profile(uint32_t stream_index, uint32_t fps);
        static std::shared_ptr<motion_stream_profile> create_motion_stream(rs2_stream stream_type, uint32_t stream_index, uint32_t fps, rs2_format format, rs2_motion_device_intrinsic intrinsics);
        static std::shared_ptr<video_stream_profile> create_video_stream_profile(const platform::stream_profile& sp,
            const sensor_msgs::CameraInfo& ci,
            const stream_descriptor& sd);

        stream_profiles read_legacy_stream_info(uint32_t sensor_index) const;
        stream_profiles read_stream_info(uint32_t device_index, uint32_t sensor_index) const;
        static std::string read_option_description(const rosbag::Bag& file, const std::string& topic);
        /*Until Version 2 (including)*/
        static std::pair<rs2_option, std::shared_ptr<librealsense::option>> create_property(const rosbag::MessageInstance& property_message_instance);
        /*Starting version 3*/
        static std::pair<rs2_option, std::shared_ptr<librealsense::option>> create_option(const rosbag::Bag& file, const rosbag::MessageInstance& value_message_instance);
        static std::shared_ptr<librealsense::processing_block_interface> create_processing_block(const rosbag::MessageInstance& value_message_instance, bool& depth_to_disparity, std::shared_ptr<options_interface> options);
        static notification create_notification(const rosbag::Bag& file, const rosbag::MessageInstance& message_instance);
        static std::shared_ptr<options_container> read_sensor_options(const rosbag::Bag& file, device_serializer::sensor_identifier sensor_id, const nanoseconds& timestamp, uint32_t file_version);
        static std::vector<std::string> get_topics(std::unique_ptr<rosbag::View>& view);

        device_snapshot                         m_initial_device_description;
        nanoseconds                             m_total_duration;
        std::string                             m_file_path;
        std::shared_ptr<frame_source>           m_frame_source;
        rosbag::Bag                             m_file;
        std::unique_ptr<rosbag::View>           m_samples_view;
        rosbag::View::iterator                  m_samples_itrator;
        std::vector<std::string>                m_enabled_streams_topics;
        std::shared_ptr<metadata_parser_map>    m_metadata_parser_map;
        std::shared_ptr<context>                m_context;
        uint32_t                                m_version;
    };
}
