// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>

#include "../../third-party/realsense-file/rosbag/rosbag_storage/include/rosbag/bag.h"
#include "../../third-party/realsense-file/rosbag/rosbag_storage/include/rosbag/view.h"

#include "print_helpers.h"

namespace rosbag_inspector
{
    struct compression_info
    {
        compression_info() : compressed(0), uncompressed(0) {}
        compression_info(const std::tuple<std::string, uint64_t, uint64_t>& t)
        {
            compression_type = std::get<0>(t);
            compressed = std::get<1>(t);
            uncompressed = std::get<2>(t);
        }
        std::string compression_type;
        uint64_t compressed;
        uint64_t uncompressed;
    };

    struct rosbag_content
    {
        rosbag_content(const std::string& file)
        {
            bag.open(file);

            rosbag::View entire_bag_view(bag);

            for (auto&& m : entire_bag_view)
            {
                topics_to_message_types[m.getTopic()].push_back(m.getDataType());
            }

            path = bag.getFileName();
            for (auto rit = path.rbegin(); rit != path.rend(); ++rit)
            {
                if (*rit == '\\' || *rit == '/')
                    break;
                file_name += *rit;
            }
            std::reverse(file_name.begin(), file_name.end());

            version = tmpstringstream() << bag.getMajorVersion() << "." << bag.getMinorVersion();
            file_duration = get_duration(bag);
            size = 1.0 * bag.getSize() / (1024LL * 1024LL);
            compression_info = bag.getCompressionInfo();
        }

        rosbag_content(const rosbag_content& other)
        {
            bag.open(other.path);
            cache = other.cache;
            file_duration = other.file_duration;
            file_name = other.file_name;
            path = other.path;
            version = other.version;
            size = other.size;
            compression_info = other.compression_info;
            topics_to_message_types = other.topics_to_message_types;
        }
        rosbag_content(rosbag_content&& other)
        {
            other.bag.close();
            bag.open(other.path);
            cache = other.cache;
            file_duration = other.file_duration;
            file_name = other.file_name;
            path = other.path;
            version = other.version;
            size = other.size;
            compression_info = other.compression_info;
            topics_to_message_types = other.topics_to_message_types;

            other.cache.clear();
            other.file_duration = std::chrono::nanoseconds::zero();
            other.file_name.clear();
            other.path.clear();
            other.version.clear();
            other.size = 0;
            other.compression_info.compressed = 0;
            other.compression_info.uncompressed = 0;
            other.compression_info.compression_type = "";
            other.topics_to_message_types.clear();
        }
        std::string instanciate_and_cache(const rosbag::MessageInstance& m, uint64_t count)
        {
            auto key = std::make_tuple(m.getCallerId(), m.getDataType(), m.getMD5Sum(), m.getTopic(), m.getTime(), count);
            if (cache.find(key) != cache.end())
            {
                return cache[key];
            }
            std::ostringstream oss;
            oss << m;
            cache[key] = oss.str();
            return oss.str();
        }

        std::chrono::nanoseconds get_duration(const rosbag::Bag& bag)
        {
            rosbag::View only_frames(bag, [](rosbag::ConnectionInfo const* info) {
                std::regex exp(R"RRR(/device_\d+/sensor_\d+/.*_\d+/(image|imu))RRR");
                return std::regex_search(info->topic, exp);
            });
            return std::chrono::nanoseconds((only_frames.getEndTime() - only_frames.getBeginTime()).toNSec());
        }

        std::map<std::tuple<std::string, std::string, std::string, std::string, rs2rosinternal::Time, uint64_t>, std::string> cache;
        std::chrono::nanoseconds file_duration;
        std::string file_name;
        std::string path;
        std::string version;
        double size;
        rosbag_inspector::compression_info compression_info;
        std::map<std::string, std::vector<std::string>> topics_to_message_types;
        rosbag::Bag bag;
    };
}
