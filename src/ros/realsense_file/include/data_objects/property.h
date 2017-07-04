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

            struct DLL_EXPORT property_info
            {
                file_types::nanoseconds capture_time;
                uint32_t device_id;
                std::string key;
                double value;
            };

            class DLL_EXPORT property : public sample
            {
            public:
                virtual ~property() = default;

                virtual property_info get_info() const = 0;
                virtual void set_info(const property_info& info) = 0;

                static std::string get_topic(const std::string& key, uint32_t device_id);

                static std::shared_ptr<property> create(const property_info& info);

            };
        }
    }
}


