// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "data_objects/stream_data.h"

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
        * @brief The stream_recorder provides an interface for recording realsense format files
        *
        */
        class DLL_EXPORT stream_recorder
        {
        public:

            virtual ~stream_recorder() = default;

            /**
            * @brief Writes a stream_data object to the file
            *
            * @param[in] data          an object implements the stream_data interface
            * @return status_no_error             Successful execution
            * @return status_param_unsupported    One of the stream data feilds is not supported
            */
            virtual status record(std::shared_ptr<ros_data_objects::stream_data> data) = 0;

			static bool create(const std::string& file_path, std::unique_ptr<stream_recorder>& recorder);
		};
    }
}

