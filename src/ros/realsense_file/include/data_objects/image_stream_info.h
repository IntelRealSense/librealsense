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
            struct DLL_EXPORT image_stream_data
            {
                uint32_t width;
                uint32_t height;
                file_types::intrinsics intrinsics;
                file_types::stream_extrinsics stream_extrinsics;
                uint32_t device_id;
                std::string type;
                uint32_t fps;
                file_types::pixel_format format;
            };

            class DLL_EXPORT image_stream_info : public stream_info
            {
            public:
                virtual ~image_stream_info(){}
                static std::string get_topic(const uint32_t& device_id);

                virtual const image_stream_data& get_info() const = 0;
                virtual void set_info(const image_stream_data& info) = 0;

                static std::shared_ptr<image_stream_info> create(const image_stream_data& info);
            };
        }
    }
}
