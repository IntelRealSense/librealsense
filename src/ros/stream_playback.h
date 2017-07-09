// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once
#include <mutex>
#include <ros/data_objects/vendor_data.h>

#include "stream_playback.h"
#include "data_objects/compressed_image.h"
#include "data_objects/image.h"
#include "data_objects/image_stream_info.h"
#include "data_objects/motion_stream_info.h"
#include "data_objects/motion_sample.h"
#include "data_objects/time_sample.h"
#include "data_objects/property.h"
#include "data_objects/pose.h"
#include "data_objects/occupancy_map.h"
#include "data_objects/log.h"
#include "rosbag/view.h"

namespace rs
{
    namespace file_format
    {
        /**
        * @brief The stream_playback provides an interface for playing realsense format files
        *
        */
        class stream_playback
        {
        public:
            stream_playback(const std::string& file_path);

            /**
            * @brief Returns a vector of vendor_data objects for the corresponding device
            *
            * @param[out] vendor_data               Vector of the vendor information
            * @param[in]  device_id                 Device identifier
            * @return status_no_error               Successful execution
            * @return status_item_unavailable       No vendor info data of the requested device id is found in the file
            * @return status_file_read_failed       Failed to read from file
            */
            virtual status read_vendor_data(std::vector<std::shared_ptr<ros_data_objects::vendor_data>>& vendor_data,
                                            uint32_t device_id) const;

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
            virtual status set_filter(std::vector<std::string> topics);

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
            virtual status seek_to_time(file_types::nanoseconds begin);

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
            virtual status read_stream_infos(std::vector<std::shared_ptr<ros_data_objects::stream_info>>& stream_infos,
                                             file_types::sample_type type, uint32_t device_id) const;

            /**
            * @brief Returns the next sample by arrival time
            *
            * @param[out] sample            An object implements the sample interface
            * @return status_no_error               Successful execution
            * @return status_file_read_failed       Failed to read from file
            * @return status_file_eof               The file reached the end
            */
            virtual status read_next_sample(std::shared_ptr<ros_data_objects::sample>& sample);

            /**
             * @brief Returns the total duration of the file
             * @param[out] duration   On successful execution will hold the file duration (in nanoseconds)
             * @return status_no_error on successful execution
             */
            virtual status get_file_duration(file_types::nanoseconds& duration) const;

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
