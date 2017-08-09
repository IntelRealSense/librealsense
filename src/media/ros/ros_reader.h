// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <chrono>
#include <mutex>
#include <regex>
#include <core/serialization.h>
#include <media/device_snapshot.h>
#include "rosbag/view.h"
#include "file_types.h"
#include "topic.h"
#include "sensor_msgs/Imu.h"
#include "sensor_msgs/Image.h"
#include "diagnostic_msgs/KeyValue.h"
#include "std_msgs/UInt32.h"
#include "file_types.h"
#include "realsense_msgs/StreamInfo.h"
#include "sensor_msgs/CameraInfo.h"

namespace librealsense
{
    class md_constant_parser : public md_attribute_parser_base
    {
    public:
        md_constant_parser(rs2_frame_metadata type) : _type(type) {}
        rs2_metadata_t get(const frame& frm) const override
        {
            rs2_metadata_t v;
            if (try_get(frm, v) == false)
            {
                throw invalid_value_exception("Frame does not support this type of metadata");
            }
            return v;
        }
        bool supports(const frame& frm) const override
        {
            rs2_metadata_t v;
            return try_get(frm, v);
        }
    private:
        bool try_get(const frame& frm, rs2_metadata_t& result) const
        {
            auto pair_size = (sizeof(rs2_frame_metadata) + sizeof(rs2_metadata_t));
            const uint8_t* pos = frm.additional_data.metadata_blob.data();
            while(pos <= frm.additional_data.metadata_blob.data() + frm.additional_data.metadata_blob.size())
            {
                const rs2_frame_metadata* type = reinterpret_cast<const rs2_frame_metadata*>(pos);
                pos += sizeof(rs2_frame_metadata);
                if (_type == *type)
                {
                    const rs2_metadata_t* value = reinterpret_cast<const rs2_metadata_t*>(pos);
                    result = *value; 
                    return true;
                }
                pos += sizeof(rs2_metadata_t);
            }
            return false;
        }
        rs2_frame_metadata _type;
    };
    class FalseQuery {
    public: bool operator()(rosbag::ConnectionInfo const* info) const { return false; }
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
        
    private:
        std::regex _exp;
    };
    class SensorInfoQuery : public RegexTopicQuery
    {
    public:
        SensorInfoQuery(uint32_t device_index) : RegexTopicQuery(to_string() << "/device_" << device_index << R"RRR(/sensor_(\d)+/info)RRR"){}
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
                RegexTopicQuery(to_string() << "/device_" << stream_id.device_index
                                            << "/sensor_" << stream_id.sensor_index
                                            << "/" << stream_id.stream_type << "_" << stream_id.stream_index
                                            << "/(" << RegexTopicQuery::data_msg_types() << ")/data")
        {
        }
    };

    class ros_reader: public device_serializer::reader
    {
    public:
        ros_reader(const std::string& file) :
            m_first_frame_time(0),
            m_total_duration(0),
            m_file_path(file),
            m_frame_source(nullptr)
        {

            if (file.empty())
            {
                throw std::invalid_argument("file_path");
            }
            ros_reader::reset();
            auto status = get_file_duration(m_total_duration);
            if(status != device_serializer::status_no_error)
            {
                throw librealsense::io_exception("Failed create reader (Could not extract file duration");
            }
        }

        device_snapshot query_device_description() override
        {
            return m_device_description;
        }

        device_serializer::status read_frame(device_serializer::nanoseconds& timestamp, device_serializer::stream_identifier& stream_id, frame_holder& frame) override
        {
            while(m_samples_itrator != m_samples_view->end())
            {
                rosbag::MessageInstance next_frame_msg = *m_samples_itrator;
                ++m_samples_itrator;
                if (next_frame_msg.isType<sensor_msgs::Image>())
                {
                    stream_id = ros_topic::get_stream_identifier(next_frame_msg.getTopic());
                    timestamp = device_serializer::nanoseconds(next_frame_msg.getTime().toNSec());
                    frame = read_image_from_message(next_frame_msg);
                    return device_serializer::status::status_no_error;
                }
                
                if (next_frame_msg.isType<sensor_msgs::Imu>())
                {
                    //stream_id = ros_topic::get_stream_identifier(next_frame_msg.getTopic());
                    //timestamp = device_serializer::nanoseconds(next_frame_msg.getTime().toNSec());
                    //frame = create_motion_sample(sample_msg);
                    return device_serializer::status::status_no_error;
                }
                LOG_WARNING("Unknown frame type: " << next_frame_msg.getDataType() << "(Topic: " << next_frame_msg.getTopic() << ")");
            }
            return device_serializer::status_file_eof;
        }

        void seek_to_time(const device_serializer::nanoseconds& time) override
        {
            if(time < m_first_frame_time || time > m_total_duration)
            {
                throw invalid_value_exception(to_string() << "Requested time is out of bound of playback file. (Requested = " << time.count() << ", Duration = " << m_total_duration.count() << ")");
            }
            auto seek_time = m_first_frame_time + device_serializer::nanoseconds(time);
            auto time_interval = device_serializer::nanoseconds(seek_time);

            std::unique_ptr<rosbag::View> samples_view;
            auto sts = seek_to_time(seek_time, samples_view);
            if(sts != device_serializer::status_no_error)
            {
                throw invalid_value_exception(to_string() << "Failed to seek_to_time " << time.count() << ", m_reader->seek_to_time(" <<  time_interval.count() << ") returned " << sts);
            }
            m_samples_view = std::move(samples_view);
            m_samples_itrator = m_samples_view->begin();
        }

        device_serializer::nanoseconds query_duration() const override
        {
            return m_total_duration;
        }

        static std::shared_ptr<metadata_parser_map> create_metadata_parser_map()
        {
            auto md_parser_map = std::make_shared<metadata_parser_map>();
            for (int i = 0; i < static_cast<int>(rs2_frame_metadata::RS2_FRAME_METADATA_COUNT); ++i)
            {
                auto frame_md_type = static_cast<rs2_frame_metadata>(i);
                md_parser_map->insert(std::make_pair(frame_md_type, std::make_shared<md_constant_parser>(frame_md_type)));
            }
            return md_parser_map;
        }

        void reset() override
        {
            m_file.close();
            m_file.open(m_file_path, rosbag::BagMode::Read);

            uint32_t version = 0;
            if (get_file_version_from_file(version) == false)
            {
                throw std::runtime_error("failed to read file version");
            }

            if (version != get_file_version())
            {
                throw std::runtime_error("unsupported file version");
            }
            
            m_samples_view = nullptr;
            //m_samples_itrator
            //m_topics = get_topics(m_samples_view);
            m_frame_source.reset();
            m_metadata_parser_map = create_metadata_parser_map();
            m_frame_source.init(m_metadata_parser_map);
            m_first_frame_time = get_first_frame_timestamp();
            m_device_description = read_device_description();
        }

        //TODO: Rename to enable_stream
        virtual void set_filter(const device_serializer::stream_identifier& stream_id) override
        {
            ros::Time curr_time = ros::TIME_MIN;
            if(m_samples_view == nullptr)
            {
                m_samples_view = std::unique_ptr<rosbag::View>(new rosbag::View(m_file, FalseQuery()));
                m_samples_itrator = m_samples_view->begin();
            }
            
            if (m_samples_itrator != m_samples_view->end())
            {
                rosbag::MessageInstance sample_msg = *m_samples_itrator;
                curr_time = sample_msg.getTime();
            }
            
            m_samples_view->addQuery(m_file, StreamQuery(stream_id), curr_time);
            m_samples_itrator = m_samples_view->begin();
        }

        //TODO: rename to disable_stream
        virtual void clear_filter(const device_serializer::stream_identifier& stream_id) override
        {
            //TODO: go over all current topics and remove the ones matching this stream id
            if(m_samples_view == nullptr)
            {
                return;
            }
            ros::Time curr_time;
            if(m_samples_itrator == m_samples_view->end())
            {
                curr_time = m_samples_view->getEndTime();
            }
            else
            {
                rosbag::MessageInstance sample_msg = *m_samples_itrator;
                curr_time = sample_msg.getTime();
            }
            
            m_samples_view = std::unique_ptr<rosbag::View>(new rosbag::View(m_file,FalseQuery()));
            for (auto topic : get_topics(m_samples_view))
            {
                bool should_topic_remain = topic.find(ros_topic::stream_full_prefix(stream_id)) != std::string::npos;
                if (should_topic_remain)
                {
                    m_samples_view->addQuery(m_file, rosbag::TopicQuery(topic), curr_time);
                }
            }
            m_samples_itrator = m_samples_view->begin();

        }

        const std::string& get_file_name() const override
        {
            return m_file_path;
        }
    private:

        device_serializer::status get_file_duration(device_serializer::nanoseconds& duration) const
        {
            std::unique_ptr<rosbag::View> samples_view;
            auto first_non_frame_time = ros::TIME_MIN.toNSec()+1;
            auto sts = seek_to_time(device_serializer::nanoseconds(first_non_frame_time), samples_view);
            if(sts != device_serializer::status_no_error)
            {
                duration = device_serializer::nanoseconds(0);
                return device_serializer::status_no_error;
            }
            auto samples_itrator = samples_view->begin();
            auto first_frame_time = samples_itrator->getTime();
            auto total_time = samples_view->getEndTime() - first_frame_time;
            duration = device_serializer::nanoseconds(total_time.toNSec());
            return device_serializer::status_no_error;
        }

        frame_holder read_image_from_message(const rosbag::MessageInstance &image_data) const
        {
            sensor_msgs::ImagePtr msg = image_data.instantiate<sensor_msgs::Image>();
            if (msg == nullptr)
            {
                throw io_exception(to_string() << "Expected image message but got " << image_data.getDataType());
            }

            frame_additional_data additional_data {};
            std::chrono::duration<double, std::micro> timestamp_us(std::chrono::duration<double>(msg->header.stamp.toSec()));
            additional_data.timestamp = timestamp_us.count();
            additional_data.frame_number = msg->header.seq;
            additional_data.fisheye_ae_mode = false; //TODO: where should this come from?

            auto stream_id = ros_topic::get_stream_identifier(image_data.getTopic());
            auto info_topic = ros_topic::image_metadata_topic(stream_id);
            rosbag::View frame_metadata_view(m_file, rosbag::TopicQuery(info_topic), image_data.getTime(), image_data.getTime());
            uint32_t total_md_size = 0;
            for (auto message_instance : frame_metadata_view)
            {
                auto key_val_msg = message_instance.instantiate<diagnostic_msgs::KeyValue>();
                if (key_val_msg == nullptr)
                {
                    throw io_exception(to_string() << "Expected KeyValue message but got: " << message_instance.getDataType() << "(Topic: " << message_instance.getTopic() << ")");
                }
                if(key_val_msg->key == "timestamp_domain") //TODO: use constants
                {
                    conversions::convert(key_val_msg->value, additional_data.timestamp_domain);
                }
                else if (key_val_msg->key == "system_time") //TODO: use constants
                {
                    additional_data.system_time = std::stod(key_val_msg->value);
                }
                else
                {
                    rs2_frame_metadata type;
                    try
                    {
                        conversions::convert(key_val_msg->key, type);
                    }
                    catch(const std::exception& e)
                    {
                        LOG_ERROR(e.what());
                        continue;
                    }
                    auto size_of_enum = sizeof(rs2_frame_metadata);
                    rs2_metadata_t md = static_cast<rs2_metadata_t>(std::stoll(key_val_msg->value));
                    auto size_of_data = sizeof(rs2_metadata_t);
                    if (total_md_size + size_of_enum + size_of_data > 255)
                    {
                        continue;; //stop adding metadata to frame
                    }
                    memcpy(additional_data.metadata_blob.data() + total_md_size, &type, size_of_enum);
                    total_md_size += static_cast<uint32_t>(size_of_enum);
                    memcpy(additional_data.metadata_blob.data() + total_md_size, &md, size_of_data);
                    total_md_size += static_cast<uint32_t>(size_of_data);
                }
            }

            frame_interface* frame = m_frame_source.alloc_frame(RS2_EXTENSION_VIDEO_FRAME, msg->data.size(), additional_data, true);
            if (frame == nullptr)
            {
                throw invalid_value_exception("Failed to allocate new frame");
            }
            librealsense::video_frame* video_frame = static_cast<librealsense::video_frame*>(frame);
            video_frame->assign(msg->width, msg->height, msg->step, msg->step / msg->width / 8); //TODO: Ziv, is bpp bytes or bits per pixel?

            rs2_format stream_format;
            conversions::convert(msg->encoding, stream_format);
            //attaching a temp stream to the frame. Playback sensor should assign the real stream 
            frame->set_stream(std::make_shared<video_stream_profile>(nullptr, platform::stream_profile{}));
            frame->get_stream()->set_format(stream_format);
            frame->get_stream()->set_stream_index(stream_id.stream_index);
            frame->get_stream()->set_stream_type(stream_id.stream_type);
            video_frame->data = msg->data;
            librealsense::frame_holder fh{ video_frame };
            return std::move(fh);
        }

        bool get_file_version_from_file(uint32_t& version) const
        {
            rosbag::View view(m_file, rosbag::TopicQuery(get_file_version_topic()));
            if (view.size() == 0)
            {
                return false;
            }

            auto item = *view.begin();
            std_msgs::UInt32Ptr msg = item.instantiate<std_msgs::UInt32>();
            if (msg == nullptr)
            {
                return false;
            }
            version = msg->data;

            return true;
        }

        device_serializer::status seek_to_time(const device_serializer::nanoseconds& seek_time, std::unique_ptr<rosbag::View>& samples_view) const
        {
            ros::Time to_time = ros::TIME_MIN;
            if(seek_time.count() != 0)
            {
                std::chrono::duration<uint32_t> sec = std::chrono::duration_cast<std::chrono::duration<uint32_t>>(seek_time);
                device_serializer::nanoseconds range = seek_time - std::chrono::duration_cast<device_serializer::nanoseconds>(sec);
                to_time = ros::Time(sec.count(), std::chrono::duration_cast<std::chrono::duration<uint32_t, std::nano>>(range).count());
            }

            if(m_topics.empty() == true)
            {
                samples_view = std::unique_ptr<rosbag::View>(new rosbag::View(m_file));
            }
            else
            {
                samples_view = std::unique_ptr<rosbag::View>(new rosbag::View());
                for(auto topic : m_topics)
                {
                    samples_view->addQuery(m_file, rosbag::TopicQuery(topic), to_time);
                }
            }
            if(samples_view->begin() == samples_view->end())
            {
                return device_serializer::status_invalid_argument;
            }

            return device_serializer::status_no_error;

        }

        device_snapshot read_device_description() const
        {
            snapshot_collection device_extensions;

            std::shared_ptr<info_snapshot> info = read_info_snapshot(ros_topic::device_info_topic(get_device_index()));
            device_extensions[RS2_EXTENSION_INFO ] = info;
            std::vector<sensor_snapshot> sensor_descriptions;
            uint32_t sensor_count = read_sensor_count(get_device_index());
            for (uint32_t sensor_index = 0; sensor_index < sensor_count; sensor_index++)
            {
                snapshot_collection sensor_extensions;
                auto streams_snapshots = read_stream_info(get_device_index(), sensor_index);
                
               /* std::shared_ptr<options_snapshot> options = options_container read_options();
                sensor_extensions[RS2_EXTENSION_OPTIONS ] = options;*/

                std::shared_ptr<info_snapshot> sensor_info = read_info_snapshot(ros_topic::sensor_info_topic({ get_device_index(), sensor_index }));
                sensor_extensions[RS2_EXTENSION_INFO] = sensor_info;

                sensor_descriptions.emplace_back(sensor_extensions, streams_snapshots);
            }

            return device_snapshot(device_extensions, sensor_descriptions);
        }

        std::shared_ptr<info_snapshot> read_info_snapshot(const std::string& topic) const
        {
            std::map<rs2_camera_info, std::string> values;
            rosbag::View view(m_file, rosbag::TopicQuery(topic));
            for (auto message_instance : view)
            {
                diagnostic_msgs::KeyValueConstPtr info_msg = message_instance.instantiate<diagnostic_msgs::KeyValue>();
                if (info_msg == nullptr)
                {
                    throw io_exception(to_string() << "Expected KeyValue Message but got " << message_instance.getDataType());
                }
                try
                {
                    rs2_camera_info info;
                    conversions::convert(info_msg->key, info);
                    values[info] = info_msg->value;
                }
                catch (const std::exception& e)
                {
                    std::cerr << e.what() << std::endl;
                }
            }

            return std::make_shared<info_snapshot>(values);
        }

        uint32_t read_sensor_count(uint32_t device_index) const
        {
            rosbag::View sensor_infos(m_file, SensorInfoQuery(device_index));
            std::set<uint32_t> sensor_indices;
            for (auto sensor_info : sensor_infos)
            {
                auto msg = sensor_info.instantiate<diagnostic_msgs::KeyValue>();
                if(msg == nullptr)
                {
                    throw io_exception(to_string() << "Expected KeyValue message but got " << sensor_info.getDataType() << "(Topic: " << sensor_info.getTopic() << ")");
                }
                sensor_indices.insert(static_cast<uint32_t>(ros_topic::get_sensor_index(sensor_info.getTopic())));
            }
            return static_cast<uint32_t>(sensor_indices.size());
        }

        stream_profiles read_stream_info(uint32_t device_index, uint32_t sensor_index) const
        {
            stream_profiles streams;
            //The below regex matches both stream info messages and also video \ imu stream info (both have the same prefix)
            rosbag::View stream_infos_view(m_file, RegexTopicQuery("/device_" + std::to_string(device_index) + "/sensor_" + std::to_string(sensor_index) + R"RRR(/(\w)+_(\d)+/info)RRR"));
            for (auto infos_view : stream_infos_view)
            {
                auto stream_id = ros_topic::get_stream_identifier(infos_view.getTopic());

                if (infos_view.isType<realsense_msgs::StreamInfo>() == false)
                {
                    continue;
                }
                
                auto stream_info_msg = infos_view.instantiate<realsense_msgs::StreamInfo>();
                if (stream_info_msg == nullptr)
                {
                    throw io_exception(to_string() << "Expected realsense_msgs::StreamInfo message but got " << infos_view.getDataType() << "(Topic: " << infos_view.getTopic() << ")");
                }

                //auto is_recommended = stream_info_msg->is_recommended;
                auto fps = stream_info_msg->fps;
                rs2_format format;
                conversions::convert(stream_info_msg->encoding, format);

                auto video_stream_topic = ros_topic::video_stream_info_topic(stream_id);
                rosbag::View video_stream_infos_view(m_file, rosbag::TopicQuery(video_stream_topic));
                if (video_stream_infos_view.size() > 0)
                {
                    assert(video_stream_infos_view.size() == 1);
                    auto video_stream_msg_ptr = *video_stream_infos_view.begin();
                    auto video_stream_msg = video_stream_msg_ptr.instantiate<sensor_msgs::CameraInfo>();
                    if (video_stream_msg == nullptr)
                    {
                        throw io_exception(to_string() << "Expected sensor_msgs::CameraInfo message but got " << video_stream_msg_ptr.getDataType() << "(Topic: " << video_stream_msg_ptr.getTopic() << ")");
                    }

                    auto profile = std::make_shared<video_stream_profile>(nullptr, platform::stream_profile{ video_stream_msg->width ,video_stream_msg->height, fps, static_cast<uint32_t>(format) });
                    rs2_intrinsics intrinsics{};
                    intrinsics.height = video_stream_msg->height;
                    intrinsics.width = video_stream_msg->width;
                    intrinsics.fx = video_stream_msg->K[0];
                    intrinsics.ppx = video_stream_msg->K[2];
                    intrinsics.fy = video_stream_msg->K[4];
                    intrinsics.ppy = video_stream_msg->K[5];
                    memcpy(intrinsics.coeffs, video_stream_msg->D.data(), sizeof(intrinsics.coeffs));
                    profile->set_intrinsics([intrinsics]() {return intrinsics; });
                    profile->set_stream_index(stream_id.stream_index);
                    profile->set_stream_type(stream_id.stream_type);
                    profile->set_dims(video_stream_msg->width, video_stream_msg->height);
                    profile->set_format(format);
                    profile->set_framerate(fps);
                    //TODO: set size?
                    //profile->set_recommended(is_recommended);
                    streams.push_back(profile);
                }

                auto imu_stream_topic = ros_topic::imu_intrinsic_topic(stream_id);
                rosbag::View imu_intrinsic_view(m_file, rosbag::TopicQuery(imu_stream_topic));
                if (imu_intrinsic_view.size() > 0)
                {
                    assert(imu_intrinsic_view.size() == 1);
                    //TODO: implement when relevant
                }

                if (video_stream_infos_view.size() == 0 && imu_intrinsic_view.size() == 0)
                {
                    throw io_exception(to_string() << "Every StreamInfo is expected to have a complementary video/imu message, but none was found");
                }
                
            }
            return streams;
        }

        std::shared_ptr<options_container> read_options()
        {
            return nullptr;
        }

        static std::vector<std::string> get_topics(std::unique_ptr<rosbag::View>& view)
        {
            std::vector<std::string> topics;
            auto connections = view->getConnections();
            auto x = std::transform(connections.begin(), connections.end(), std::back_inserter(topics), [](const rosbag::ConnectionInfo* connection) { return connection->topic; });
            return topics;
        }

        device_snapshot                         m_device_description;
        device_serializer::nanoseconds          m_first_frame_time;
        device_serializer::nanoseconds          m_total_duration;
        std::string                             m_file_path;
        librealsense::frame_source              m_frame_source;
        rosbag::Bag                             m_file;
        std::unique_ptr<rosbag::View>           m_samples_view;
        rosbag::View::iterator                  m_samples_itrator;
        std::vector<std::string>                m_topics;
        std::shared_ptr<metadata_parser_map>    m_metadata_parser_map;
    };


}
