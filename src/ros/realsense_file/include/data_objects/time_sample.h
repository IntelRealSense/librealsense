// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
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
			
            struct DLL_EXPORT time_sample_info
            {
                file_types::nanoseconds capture_time;
                uint32_t device_id;
                std::string source;
                file_types::seconds timestamp;
                uint32_t frame_number;
            };

            class DLL_EXPORT time_sample : public sample
            {
            public:
                virtual ~time_sample() = default;

                virtual time_sample_info get_info() const = 0;
                virtual void set_info(const time_sample_info& info) = 0;

                static std::string get_topic(uint32_t device_id);

                static std::shared_ptr<time_sample> create(const time_sample_info& info);

            };
        }
    }
}


