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
            struct DLL_EXPORT pose_info
            {
				file_types::vector3 translation;
				file_types::vector4 rotation;
				file_types::vector3 velocity;
				file_types::vector3 angular_velocity;
				file_types::vector3 acceleration;
				file_types::vector3 angular_acceleration;
                file_types::nanoseconds capture_time;
                file_types::nanoseconds timestamp;
				file_types::nanoseconds system_timestamp;
				uint32_t device_id;

            };

            class DLL_EXPORT pose : public sample
            {
            public:
                virtual ~pose() = default;

                virtual pose_info get_info() const = 0;
                virtual void set_info(const pose_info& info) = 0;

                static std::string get_topic(uint32_t device_id);
                static std::shared_ptr<pose> create(const pose_info& info);
            };
        }
    }
}

