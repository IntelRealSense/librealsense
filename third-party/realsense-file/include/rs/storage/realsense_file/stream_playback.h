// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <vector>
#include "data_objects/stream_data.h"
#include "data_objects/vendor_data.h"
#include "data_objects/stream_data.h"
#include "rs/storage/realsense_file/file_types.h"

#ifdef _WIN32
#ifdef realsense_file_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif /* realsense_file_EXPORTS */
#else /* defined (_WIN32) */
#define DLL_EXPORT
#endif

namespace rs
{
    namespace file_format
    {


        /**
        * @brief The stream_playback provides an interface for playing realsense format files
        *
        */
        class DLL_EXPORT stream_playback
        {
        public:
            virtual ~stream_playback() = default;
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
                                            uint32_t device_id) const = 0;

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
            virtual status set_filter(std::vector<std::string> topics) = 0;

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
            virtual status seek_to_time(file_types::nanoseconds begin) = 0;

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
                                             file_types::sample_type type, uint32_t device_id) const = 0;

            /**
            * @brief Returns the next sample by arrival time
            *
            * @param[out] sample            An object implements the sample interface
            * @return status_no_error               Successful execution
            * @return status_file_read_failed       Failed to read from file
            * @return status_file_eof               The file reached the end
            */
            virtual status read_next_sample(std::shared_ptr<ros_data_objects::sample>& sample) = 0;

            /**
             * @brief Returns the total duration of the file
             * @param[out] duration   On successful execution will hold the file duration (in nanoseconds)
             * @return status_no_error on successful execution
             */
            virtual status get_file_duration(file_types::nanoseconds& duration) const = 0;

			static bool create(const std::string& file_path, std::unique_ptr<stream_playback>& player);

        };
    }
}
