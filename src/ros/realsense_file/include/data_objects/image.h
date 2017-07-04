// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <vector>
#include <map>
#include "include/data_objects/stream_data.h"

#ifdef _WIN32 
#ifdef realsense_file_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif /* realsense_file_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif

namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            struct DLL_EXPORT image_info
            {
                std::string stream;
                uint32_t width;
                uint32_t height;
                file_types::pixel_format format;
                uint32_t step;
                file_types::nanoseconds capture_time;
                file_types::seconds timestamp;
                file_types::nanoseconds system_time;
                file_types::timestamp_domain timestamp_domain;
                uint32_t device_id;
                uint32_t frame_number;
                std::shared_ptr<uint8_t> data;
                std::map<file_types::metadata_type, std::vector<uint8_t>> metadata;
            };

            class DLL_EXPORT image : public sample
            {
            public:
                virtual ~image() = default;

                virtual const image_info& get_info() const = 0;
                virtual void set_info(const image_info& info) = 0;

                static std::string get_topic(std::string stream, uint32_t device_id);
                static std::string get_info_topic(std::string stream, uint32_t device_id);

                static std::shared_ptr<image> create(const image_info& info);

            };
        }
    }
}
