// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
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
            struct DLL_EXPORT log_info
            {
                uint8_t level;
                std::string message;
                std::string file;
                std::string function;
                uint32_t line;
                file_types::nanoseconds capture_time;

            };

            class DLL_EXPORT log : public sample
            {
            public:
                virtual ~log() = default;

                virtual log_info get_info() const = 0;
                virtual void set_info(const log_info& info) = 0;

                static std::string get_topic();
                static std::shared_ptr<log> create(const log_info& info);
            };
        }
    }
}

