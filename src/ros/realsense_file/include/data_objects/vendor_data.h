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
            struct DLL_EXPORT vendor_info
            {
                std::string name;
                std::string value;
                uint32_t device_id;
            };

            class DLL_EXPORT vendor_data : public stream_data
            {
            public:
                virtual ~vendor_data() = default;

                virtual vendor_info get_info() const = 0;
                virtual void set_info(const vendor_info& info) = 0;

                static std::string get_topic(const uint32_t& device_id = -1);
                static std::shared_ptr<vendor_data> create(const vendor_info& info);
            };
        }
    }
}


