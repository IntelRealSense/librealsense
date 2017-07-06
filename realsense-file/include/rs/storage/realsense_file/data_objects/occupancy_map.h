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
            struct DLL_EXPORT occupancy_map_info
            {
                uint8_t accuracy;
                uint8_t reserved;
                uint16_t tile_count;
                std::shared_ptr<float> tiles;
                file_types::nanoseconds capture_time;
                uint32_t device_id;

            };

            class DLL_EXPORT occupancy_map : public sample
            {
            public:
                virtual ~occupancy_map() = default;

                virtual occupancy_map_info get_info() const = 0;
                virtual void set_info(const occupancy_map_info& info) = 0;

                static std::string get_topic(uint32_t device_id);
                static std::shared_ptr<occupancy_map> create(const occupancy_map_info& info);
            };
        }
    }
}


