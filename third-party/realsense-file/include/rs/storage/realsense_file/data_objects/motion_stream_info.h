// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/storage/realsense_file/data_objects/stream_data.h"

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
            struct DLL_EXPORT motion_stream_data
            {
                file_types::motion_type type;
                uint32_t fps;
                file_types::motion_intrinsics intrinsics;
                file_types::stream_extrinsics stream_extrinsics;
                uint32_t device_id;
            };

            class DLL_EXPORT motion_stream_info : public stream_info
            {
			public:
                virtual ~motion_stream_info(){}

                virtual motion_stream_data get_info() const = 0;
                virtual void set_info(const motion_stream_data& info) = 0;

                static std::string get_topic(uint32_t device_id);

                static std::shared_ptr<motion_stream_info> create(const motion_stream_data& info);
            };
        }
    }
}
