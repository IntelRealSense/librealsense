// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include<stdarg.h>

namespace librealsense
{
    class ros_topic
    {
    public:
        static constexpr const char* elements_separator = "/";

        static uint32_t get_device_index(const std::string& topic)
        {
            return get_id("device_", get<0>(topic));
        }
        static uint32_t get_sensor_index(const std::string& topic)
        {
            return get_id("sensor_", get<1>(topic));
        }
        static rs2_stream get_stream_type(const std::string& topic)
        {
            auto stream_with_id = get<2>(topic);
            rs2_stream type;
            conversions::convert(stream_with_id.substr(0, stream_with_id.find_first_of("_")), type);
            return type;
        }

        static uint32_t get_stream_index(const std::string& topic)
        {
            auto stream_with_id = get<2>(topic);
            return get_id(stream_with_id.substr(0, stream_with_id.find_first_of("_") + 1), get<2>(topic));
        }

        static device_serializer::stream_identifier get_stream_identifier(const std::string& topic)
        {
            return device_serializer::stream_identifier{ get_device_index(topic),  get_sensor_index(topic),  get_stream_type(topic),  get_stream_index(topic) };
        }

        static std::string file_version_topic()
        {
            return create_from({ "file_version" });
        }
        static std::string device_info_topic(uint32_t device_id)
        {
            return create_from({ device_prefix(device_id),  "info" });
        }
        static std::string sensor_info_topic(const device_serializer::sensor_identifier& sensor_id)
        {
            return create_from({ device_prefix(sensor_id.device_index), sensor_prefix(sensor_id.sensor_index),  "info" });
        }
        static std::string stream_info_topic(const device_serializer::stream_identifier& stream_id)
        {
            return create_from({ stream_full_prefix(stream_id), "info" });
        }
        static std::string video_stream_info_topic(const device_serializer::stream_identifier& stream_id)
        {
            return create_from({ stream_full_prefix(stream_id), "info", "camera_info" });
        }
        static std::string imu_intrinsic_topic(const device_serializer::stream_identifier& stream_id)
        {
            return create_from({ stream_full_prefix(stream_id), "imu_intrinsic" });
        }
        static std::string property_topic(const device_serializer::sensor_identifier& sensor_id)
        {
            return create_from({ device_prefix(sensor_id.device_index), sensor_prefix(sensor_id.sensor_index), "property" });
        }
        static std::string image_data_topic(const device_serializer::stream_identifier& stream_id)
        {
            return create_from({ stream_full_prefix(stream_id), "image", "data" });
        }
        static std::string image_metadata_topic(const device_serializer::stream_identifier& stream_id)
        {
            return create_from({ stream_full_prefix(stream_id), "image", "metadata" });
        }
        static std::string stream_extrinsic_topic(const device_serializer::stream_identifier& stream_id)
        {
            return create_from({ stream_full_prefix(stream_id), "tf" });
        }
        static std::string  additional_info_topic()
        {
            return create_from({ "additional_info" });
        }
    private:

        static std::string create_from(const std::vector<std::string>& parts)
        {
            std::ostringstream oss;
            oss << elements_separator;
            if (parts.empty() == false)
            {
                std::copy(parts.begin(), parts.end() - 1, std::ostream_iterator<std::string>(oss, elements_separator));
                oss << parts.back();
            }
            return oss.str();
        }

        template<uint32_t index>
        static std::string get(const std::string& value)
        {
            size_t current_pos = 0;
            std::string value_copy = value;
            uint32_t elements_iterator = 0;
            const auto seperator_length = std::string(elements_separator).length();
            while ((current_pos = value_copy.find(elements_separator)) != std::string::npos)
            {
                auto token = value_copy.substr(0, current_pos);
                if (elements_iterator == index)
                {
                    return token;
                }
                value_copy.erase(0, current_pos + seperator_length);
                ++elements_iterator;
            }

            if (elements_iterator == index)
                return value_copy;

            throw std::out_of_range("Requeted index is out of bound");
        }
        static uint32_t get_id(const std::string& prefix, const std::string& str)
        {
            if (str.compare(0, prefix.size(), prefix) != 0)
            {
                throw std::runtime_error("Failed to get id after prefix \"" + prefix + "\"from string \"" + str + "\"");
            }

            std::string id_str = str.substr(prefix.size());
            return static_cast<uint32_t>(std::stoll(id_str));
        }

        static std::string device_prefix(uint32_t device_id)
        {
            return "device_" + std::to_string(device_id);
        }
        static std::string sensor_prefix(uint32_t sensor_id)
        {
            return "sensor_" + std::to_string(sensor_id);
        }
        static std::string stream_prefix(rs2_stream type, uint32_t stream_id)
        {
            return std::string(rs2_stream_to_string(type)) + "_" + std::to_string(stream_id);
        }

        static std::string stream_full_prefix(const device_serializer::stream_identifier& stream_id)
        {
            return create_from({ device_prefix(stream_id.device_index), sensor_prefix(stream_id.sensor_index), stream_prefix(stream_id.stream_type, stream_id.stream_index) });
        }
    };
}
