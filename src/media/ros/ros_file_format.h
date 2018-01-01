// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <chrono>
#include "librealsense2/rs.h"
#include "sensor_msgs/image_encodings.h"
#include "geometry_msgs/Transform.h"
#include "metadata-parser.h"
#include "option.h"
#include "rosbag/structures.h"
#include <regex>
#include "stream.h"
#include "types.h"

namespace librealsense
{
    inline void convert(rs2_format source, std::string& target)
    {
        switch (source)
        {
        case RS2_FORMAT_Z16: target = sensor_msgs::image_encodings::MONO16;     return;
        case RS2_FORMAT_RGB8: target = sensor_msgs::image_encodings::RGB8;       return;
        case RS2_FORMAT_BGR8: target = sensor_msgs::image_encodings::BGR8;       return;
        case RS2_FORMAT_RGBA8: target = sensor_msgs::image_encodings::RGBA8;      return;
        case RS2_FORMAT_BGRA8: target = sensor_msgs::image_encodings::BGRA8;      return;
        case RS2_FORMAT_Y8: target = sensor_msgs::image_encodings::TYPE_8UC1;  return;
        case RS2_FORMAT_Y16: target = sensor_msgs::image_encodings::TYPE_16UC1; return;
        case RS2_FORMAT_RAW8: target = sensor_msgs::image_encodings::MONO8;      return;
        case RS2_FORMAT_UYVY: target = sensor_msgs::image_encodings::YUV422;     return;
        default: target = rs2_format_to_string(source);
        }
    }

    inline void convert(const std::string& source, rs2_format& target)
    {
        if (source == sensor_msgs::image_encodings::MONO16) { target = RS2_FORMAT_Z16; return; }
        if (source == sensor_msgs::image_encodings::RGB8) { target = RS2_FORMAT_RGB8; return; }
        if (source == sensor_msgs::image_encodings::BGR8) { target = RS2_FORMAT_BGR8; return; }
        if (source == sensor_msgs::image_encodings::RGBA8) { target = RS2_FORMAT_RGBA8; return; }
        if (source == sensor_msgs::image_encodings::BGRA8) { target = RS2_FORMAT_BGRA8; return; }
        if (source == sensor_msgs::image_encodings::TYPE_8UC1) { target = RS2_FORMAT_Y8; return; }
        if (source == sensor_msgs::image_encodings::TYPE_16UC1) { target = RS2_FORMAT_Y16; return; }
        if (source == sensor_msgs::image_encodings::MONO8) { target = RS2_FORMAT_RAW8; return; }
        if (source == sensor_msgs::image_encodings::YUV422) { target = RS2_FORMAT_UYVY; return; }
        if (!try_parse(source, target))
        {
            throw std::runtime_error(to_string() << "Failed to convert source: \"" << "\" to matching rs2_format");
        }
    }

    inline void convert(const std::string& source, rs2_stream& target)
    {
        if(!try_parse(source, target))
        {
            throw std::runtime_error(to_string() << "Failed to convert source: \"" << "\" to matching rs2_stream");
        }
    }

    inline void convert(const std::string& source, rs2_distortion& target)
    {
        if (!try_parse(source, target))
        {
            throw std::runtime_error(to_string() << "Failed to convert source: \"" << "\" to matching rs2_distortion");
        }
    }

    inline void convert(const std::string& source, rs2_option& target)
    {
        if (!try_parse(source, target))
        {
            throw std::runtime_error(to_string() << "Failed to convert source: \"" << "\" to matching rs2_optin");
        }
    }

    inline void convert(const std::string& source, rs2_frame_metadata_value& target)
    {
        if (!try_parse(source, target))
        {
            throw std::runtime_error(to_string() << "Failed to convert source: \"" << "\" to matching rs2_frame_metadata");
        }
    }

    inline void convert(const std::string& source, rs2_camera_info& target)
    {
        if (!try_parse(source, target))
        {
            throw std::runtime_error(to_string() << "Failed to convert source: \"" << "\" to matching rs2_camera_info");
        }
    }

    inline void convert(const std::string& source, rs2_timestamp_domain& target)
    {
        if (!try_parse(source, target))
        {
            throw std::runtime_error(to_string() << "Failed to convert source: \"" << "\" to matching rs2_timestamp_domain");
        }
    }


    /*
    quat2rot(), rot2quat()
    ------------------

    rotation matrix is column major
    m[3][3] <==> r[9]

    [00, 01, 02] <==> [ r[0], r[3], r[6] ]
    m = [10, 11, 12] <==> [ r[1], r[4], r[7] ]
    [20, 21, 22] <==> [ r[2], r[5], r[8] ]
    */

    inline void quat2rot(const geometry_msgs::Transform::_rotation_type& q, float(&r)[9])
    {
        r[0] = 1 - 2 * q.y*q.y - 2 * q.z*q.z;  //  m00 = 1 - 2 * qy2 - 2 * qz2
        r[3] = 2 * q.x*q.y - 2 * q.z*q.w;      //  m01 = 2 * qx*qy - 2 * qz*qw
        r[6] = 2 * q.x*q.z + 2 * q.y*q.w;      //  m02 = 2 * qx*qz + 2 * qy*qw
        r[1] = 2 * q.x*q.y + 2 * q.z*q.w;      //  m10 = 2 * qx*qy + 2 * qz*qw
        r[4] = 1 - 2 * q.x*q.x - 2 * q.z*q.z;  //  m11 = 1 - 2 * qx2 - 2 * qz2
        r[7] = 2 * q.y*q.z - 2 * q.x*q.w;      //  m12 = 2 * qy*qz - 2 * qx*qw
        r[2] = 2 * q.x*q.z - 2 * q.y*q.w;      //  m20 = 2 * qx*qz - 2 * qy*qw
        r[5] = 2 * q.y*q.z + 2 * q.x*q.w;      //  m21 = 2 * qy*qz + 2 * qx*qw
        r[8] = 1 - 2 * q.x*q.x - 2 * q.y*q.y;  //  m22 = 1 - 2 * qx2 - 2 * qy2
    }

    inline void rot2quat(const float(&r)[9], geometry_msgs::Transform::_rotation_type& q)
    {
        q.w = sqrtf(1 + r[0] + r[4] + r[8]) / 2; // qw= sqrt(1 + m00 + m11 + m22) /2
        q.x = (r[5] - r[7]) / (4 * q.w);         // qx = (m21 - m12)/( 4 *qw)
        q.y = (r[6] - r[2]) / (4 * q.w);         // qy = (m02 - m20)/( 4 *qw)
        q.z = (r[1] - r[3]) / (4 * q.w);         // qz = (m10 - m01)/( 4 *qw)
    }

    inline void convert(const geometry_msgs::Transform& source, rs2_extrinsics& target)
    {
        target.translation[0] = source.translation.x;
        target.translation[1] = source.translation.y;
        target.translation[2] = source.translation.z;
        quat2rot(source.rotation, target.rotation);
    }

    inline void convert(const rs2_extrinsics& source, geometry_msgs::Transform& target)
    {
        target.translation.x = source.translation[0];
        target.translation.y = source.translation[1];
        target.translation.z = source.translation[2];
        rot2quat(source.rotation, target.rotation);
    }

    class md_constant_parser : public md_attribute_parser_base
    {
    public:
        md_constant_parser(rs2_frame_metadata_value type) : _type(type) {}
        rs2_metadata_type get(const frame& frm) const override
        {
            rs2_metadata_type v;
            if (try_get(frm, v) == false)
            {
                throw invalid_value_exception("Frame does not support this type of metadata");
            }
            return v;
        }
        bool supports(const frame& frm) const override
        {
            rs2_metadata_type v;
            return try_get(frm, v);
        }
    private:
        bool try_get(const frame& frm, rs2_metadata_type& result) const
        {
            auto pair_size = (sizeof(rs2_frame_metadata_value) + sizeof(rs2_metadata_type));
            const uint8_t* pos = frm.additional_data.metadata_blob.data();
            while (pos <= frm.additional_data.metadata_blob.data() + frm.additional_data.metadata_blob.size())
            {
                const rs2_frame_metadata_value* type = reinterpret_cast<const rs2_frame_metadata_value*>(pos);
                pos += sizeof(rs2_frame_metadata_value);
                if (_type == *type)
                {
                    const rs2_metadata_type* value = reinterpret_cast<const rs2_metadata_type*>(pos);
                    result = *value;
                    return true;
                }
                pos += sizeof(rs2_metadata_type);
            }
            return false;
        }
        rs2_frame_metadata_value _type;
    };

    class FalseQuery {
    public:
        bool operator()(rosbag::ConnectionInfo const* info) const { return false; }
    };

    class RegexTopicQuery
    {
    public:
        RegexTopicQuery(std::string const& regexp) : _exp(regexp)
        {}

        bool operator()(rosbag::ConnectionInfo const* info) const
        {
            return std::regex_search(info->topic, _exp);
        }

        static std::string data_msg_types()
        {
            return "image|imu";
        }

        static std::string stream_prefix(const device_serializer::stream_identifier& stream_id)
        {
           return  to_string() << "/device_" << stream_id.device_index
                               << "/sensor_" << stream_id.sensor_index
                               << "/" << get_string(stream_id.stream_type) << "_" << stream_id.stream_index;
        }

    private:
        std::regex _exp;
    };

    class SensorInfoQuery : public RegexTopicQuery
    {
    public:
        SensorInfoQuery(uint32_t device_index) : RegexTopicQuery(to_string() << "/device_" << device_index << R"RRR(/sensor_(\d)+/info)RRR") {}
    };

    class FrameQuery : public RegexTopicQuery
    {
    public:
        FrameQuery() : RegexTopicQuery(to_string() << R"RRR(/device_\d+/sensor_\d+/.*_\d+)RRR" << "/(" << data_msg_types() << ")/data") {}
    };

    class StreamQuery : public RegexTopicQuery
    {
    public:
        StreamQuery(const device_serializer::stream_identifier& stream_id) :
            RegexTopicQuery(to_string() << stream_prefix(stream_id)
                << "/(" << RegexTopicQuery::data_msg_types() << ")/data")
        {
        }
    };

    class ExtrinsicsQuery : public RegexTopicQuery
    {
    public:
        ExtrinsicsQuery(const device_serializer::stream_identifier& stream_id) :
            RegexTopicQuery(to_string() << stream_prefix(stream_id) << "/tf")
        {
        }
    };

    class OptionsQuery : public RegexTopicQuery
    {
    public:
        OptionsQuery() : 
            RegexTopicQuery(to_string() << R"RRR(/device_\d+/sensor_\d+/option/.*/value)RRR") {}
    };

    class ros_topic
    {
    public:
        static constexpr const char* elements_separator = "/";

        static uint32_t get_device_index(const std::string& topic)
        {
            return get_id("device_", get<1>(topic));
        }

        static uint32_t get_sensor_index(const std::string& topic)
        {
            return get_id("sensor_", get<2>(topic));
        }

        static rs2_stream get_stream_type(const std::string& topic)
        {
            auto stream_with_id = get<3>(topic);
            rs2_stream type;
            convert(stream_with_id.substr(0, stream_with_id.find_first_of("_")), type);
            return type;
        }

        static uint32_t get_stream_index(const std::string& topic)
        {
            auto stream_with_id = get<3>(topic);
            return get_id(stream_with_id.substr(0, stream_with_id.find_first_of("_") + 1), get<3>(topic));
        }

        static device_serializer::sensor_identifier get_sensor_identifier(const std::string& topic)
        {
            return device_serializer::sensor_identifier{ get_device_index(topic),  get_sensor_index(topic) };
        }

        static device_serializer::stream_identifier get_stream_identifier(const std::string& topic)
        {
            return device_serializer::stream_identifier{ get_device_index(topic),  get_sensor_index(topic),  get_stream_type(topic),  get_stream_index(topic) };
        }

        static uint32_t get_extrinsic_group_index(const std::string& topic)
        {
            return std::stoul(get<5>(topic));
        }

        static std::string get_option_name(const std::string& topic)
        {
            return get<4>(topic);
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

        /*version 2 and down*/
        static std::string property_topic(const device_serializer::sensor_identifier& sensor_id)
        {
            return create_from({ device_prefix(sensor_id.device_index), sensor_prefix(sensor_id.sensor_index), "property" });
        }

        /*version 3 and up*/
        static std::string option_value_topic(const device_serializer::sensor_identifier& sensor_id, rs2_option option_type)
        {
            return create_from({ device_prefix(sensor_id.device_index), sensor_prefix(sensor_id.sensor_index), "option", rs2_option_to_string(option_type), "value" });
        }

        /*version 3 and up*/
        static std::string option_description_topic(const device_serializer::sensor_identifier& sensor_id, rs2_option option_type)
        {
            return create_from({ device_prefix(sensor_id.device_index), sensor_prefix(sensor_id.sensor_index), "option", rs2_option_to_string(option_type), "description" });
        }

        static std::string image_data_topic(const device_serializer::stream_identifier& stream_id)
        {
            return create_from({ stream_full_prefix(stream_id), "image", "data" });
        }
        static std::string image_metadata_topic(const device_serializer::stream_identifier& stream_id)
        {
            return create_from({ stream_full_prefix(stream_id), "image", "metadata" });
        }
        static std::string stream_extrinsic_topic(const device_serializer::stream_identifier& stream_id, uint32_t ref_id)
        {
            return create_from({ stream_full_prefix(stream_id), "tf", std::to_string(ref_id) });
        }
        static std::string  additional_info_topic()
        {
            return create_from({ "additional_info" });
        }

        static std::string stream_full_prefix(const device_serializer::stream_identifier& stream_id)
        {
            return create_from({ device_prefix(stream_id.device_index), sensor_prefix(stream_id.sensor_index), stream_prefix(stream_id.stream_type, stream_id.stream_index) }).substr(1); //substr(1) to remove the first "/"
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

            throw std::out_of_range(to_string() << "Requeted index \"" << index << "\" is out of bound of topic: \"" << value << "\"");
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
    };

    /**
    * Incremental number of the RealSense file format version
    * Since we maintain backward compatability, changes to topics/messages are reflected by the version
    */
    constexpr uint32_t get_file_version()
    {
        return 3u;
    }

    constexpr uint32_t get_minimum_supported_file_version()
    {
        return 2u;
    }

    constexpr uint32_t get_device_index()
    {
        return 0; //TODO: change once SDK file supports multiple devices
    }

    constexpr device_serializer::nanoseconds get_static_file_info_timestamp()
    {
        return device_serializer::nanoseconds::min();
    }

    inline device_serializer::nanoseconds to_nanoseconds(const ros::Time& t)
    {
        if (t == ros::TIME_MIN)
            return get_static_file_info_timestamp();

        return device_serializer::nanoseconds(t.toNSec());
    }

    inline ros::Time to_rostime(const device_serializer::nanoseconds& t)
    {
        if (t == get_static_file_info_timestamp())
            return ros::TIME_MIN;
        
        auto secs = std::chrono::duration_cast<std::chrono::duration<double>>(t);
        return ros::Time(secs.count());
    }
}
