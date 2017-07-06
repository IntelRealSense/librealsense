// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once
#include <mutex>

#include "rs/storage/realsense_file/stream_playback.h"
#include "rs/storage/realsense_file/data_objects/compressed_image.h"
#include "rs/storage/realsense_file/data_objects/image.h"
#include "rs/storage/realsense_file/data_objects/image_stream_info.h"
#include "rs/storage/realsense_file/data_objects/motion_stream_info.h"
#include "rs/storage/realsense_file/data_objects/motion_sample.h"
#include "rs/storage/realsense_file/data_objects/time_sample.h"
#include "rs/storage/realsense_file/data_objects/property.h"
#include "rs/storage/realsense_file/data_objects/pose.h"
#include "rs/storage/realsense_file/data_objects/occupancy_map.h"
#include "rs/storage/realsense_file/data_objects/log.h"
#include "rosbag/view.h"

namespace rs
{
    namespace file_format
    {
        class stream_playback_impl : public stream_playback
        {
        public:

            stream_playback_impl(const std::string& file_path);

            virtual ~stream_playback_impl();

            virtual status set_filter(std::vector<std::string> topics) override;

            virtual status seek_to_time(file_types::nanoseconds seek_time) override;

            virtual status read_vendor_data(std::vector<std::shared_ptr<ros_data_objects::vendor_data>>& vendor_data,
                                            uint32_t device_id) const override;
            virtual status read_stream_infos(std::vector<std::shared_ptr<ros_data_objects::stream_info>>& stream_infos,
                                             file_types::sample_type type, uint32_t device_id) const override;

            virtual status read_next_sample(std::shared_ptr<ros_data_objects::sample>& sample) override;
            virtual status get_file_duration(file_types::nanoseconds& duration) const override;

        private:
            std::shared_ptr<ros_data_objects::compressed_image> create_compressed_image(const rosbag::MessageInstance &image_data) const;
            std::shared_ptr<ros_data_objects::image> create_image(const rosbag::MessageInstance &image_data) const;
            std::shared_ptr<ros_data_objects::image_stream_info> create_image_stream_info(const rosbag::MessageInstance &info_msg) const;
            std::shared_ptr<ros_data_objects::log> create_log(const rosbag::MessageInstance &message) const;
            std::shared_ptr<ros_data_objects::motion_sample> create_motion_sample(const rosbag::MessageInstance &message) const;
            std::shared_ptr<ros_data_objects::motion_stream_info> create_motion_info(const rosbag::MessageInstance &message) const;
            std::shared_ptr<ros_data_objects::occupancy_map> create_occupancy_map(const rosbag::MessageInstance &message) const;
            std::shared_ptr<ros_data_objects::property> create_property(const rosbag::MessageInstance &message) const;
            std::shared_ptr<ros_data_objects::pose> create_six_dof(const rosbag::MessageInstance &message) const;
            std::shared_ptr<ros_data_objects::time_sample> create_time_sample(const rosbag::MessageInstance &message) const;
            std::shared_ptr<ros_data_objects::vendor_data> create_vendor_data(const rosbag::MessageInstance &message) const;
            bool get_file_version_from_file(uint32_t& version);
            status seek_to_time(file_types::nanoseconds seek_time, std::unique_ptr<rosbag::View>& samples_view) const;

            rosbag::Bag                     m_file;
            std::unique_ptr<rosbag::View>   m_samples_view;
            rosbag::View::iterator          m_samples_itrator;
            mutable std::mutex              m_mutex;
            std::vector<std::string>        m_topics;


        };
    }
}


