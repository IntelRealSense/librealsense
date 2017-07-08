// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

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
        class DLL_EXPORT ros_writer
        {
        public:

          virtual ~ros_writer() = default;
        };
    }
}
