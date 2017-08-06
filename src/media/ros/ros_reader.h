// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <chrono>
#include <mutex>
#include <regex>
#include "rosbag/view.h"
#include "file_types.h"
#include "sensor_msgs/Imu.h"
#include "sensor_msgs/Image.h"
#include "diagnostic_msgs/KeyValue.h"
#include "std_msgs/UInt32.h"
#include "topic.h"
#include <core/serialization.h>
#include <media/device_snapshot.h>
#include "file_types.h"

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
            for (int i = 0; i < frm.additional_data.metadata_blob.size(); i += pair_size)
            {
                rs2_frame_metadata type = *(reinterpret_cast<rs2_frame_metadata*>(frm.additional_data.metadata_blob[i]));
                if (_type == type)
                {
                    result = *(reinterpret_cast<rs2_metadata_t*>(frm.additional_data.metadata_blob[i + 1]));
                    return true;
                }
            }
            return false;
        }
        rs2_frame_metadata _type;
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
    private:
        std::regex _exp;
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
            m_metadata_parser_map = create_metadata_parser_map();
        }

        device_snapshot query_device_description() override
        {
            return m_device_description;
        }

        device_serializer::status read_frame(device_serializer::nanoseconds& timestamp, device_serializer::stream_identifier& stream_id, frame_holder& frame) override
        {
            if (m_samples_itrator == m_samples_view->end())
            {
                return device_serializer::status_file_eof;
            }

            rosbag::MessageInstance next_frame_msg = *m_samples_itrator;

            stream_id = ros_topic::get_stream_identifier(next_frame_msg.getTopic());
            timestamp = device_serializer::nanoseconds(next_frame_msg.getTime().toNSec());

            if (next_frame_msg.isType<sensor_msgs::Image>())
            {
                frame = read_image_from_message(next_frame_msg);
                return device_serializer::status::status_no_error;
            }
           
            if (next_frame_msg.isType<sensor_msgs::Imu>())
            {
                //frame = create_motion_sample(sample_msg);
            }

            throw invalid_value_exception(to_string() << "Unknown frame type: " << next_frame_msg.getDataType() << "(Topic: " << next_frame_msg.getTopic() << ")");
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

            m_samples_view = std::unique_ptr<rosbag::View>(new rosbag::View(m_file));
            m_samples_itrator = m_samples_view->begin();
            m_topics = get_topics(m_samples_view);
            m_frame_source.reset();
            m_frame_source.init(m_metadata_parser_map);
            m_first_frame_time = get_first_frame_timestamp();
            m_device_description = read_device_description();
        }

        virtual void set_filter(const device_serializer::stream_identifier& stream_id) override
        {
			try 
            {
				throw not_implemented_exception(__FUNCTION__);
			}
			catch(not_implemented_exception& e)
			{
				LOG_ERROR(e.what());
			}
        }
        virtual void clear_filter(const device_serializer::stream_identifier& stream_id) override
        {
            try
            {
                throw not_implemented_exception(__FUNCTION__);
            }
            catch (not_implemented_exception& e)
            {
                LOG_ERROR(e.what());
            }
        }

        const std::string& get_file_name() const override
        {
            return m_file_path;
        }
    private:

        device_serializer::status set_filter(std::vector<std::string> topics)
        {
            if(m_samples_itrator == m_samples_view->end())
            {
                return device_serializer::status_file_eof;
            }
            rosbag::MessageInstance sample_msg = *m_samples_itrator;
            auto curr_time = sample_msg.getTime();

            m_samples_view = std::unique_ptr<rosbag::View>(new rosbag::View());

            if(topics.empty() == false)
            {
                auto view = std::unique_ptr<rosbag::View>(new rosbag::View(m_file));
                std::vector<std::string> topics_in_file = get_topics(view);
                for(auto element : topics)
                {
                    if(std::find(topics_in_file.begin(), topics_in_file.end(), element) == topics_in_file.end())
                    {
                        return device_serializer::status_item_unavailable;
                    }
                    m_samples_view->addQuery(m_file, rosbag::TopicQuery(element), curr_time);

                }
                m_topics = topics;
            }
            else
            {
                //the default state of view is to get all topics
                m_samples_view = std::unique_ptr<rosbag::View>(new rosbag::View(m_file, curr_time));
                m_topics = get_topics(m_samples_view);
            }
            m_samples_itrator = m_samples_view->begin();
            return device_serializer::status_no_error;
        }

        device_serializer::status get_file_duration(device_serializer::nanoseconds& duration) const
        {
            std::unique_ptr<rosbag::View> samples_view;
            auto first_non_frame_time = ros::TIME_MIN.toNSec()+1;
            auto sts = seek_to_time(device_serializer::nanoseconds(first_non_frame_time), samples_view);
            if(sts != device_serializer::status_no_error)
            {
                return sts;
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
            rosbag::View frame_metadata_view(m_file, rosbag::TopicQuery(info_topic), image_data.getTime());
            uint32_t total_md_size = 0;
            for (auto message_instance : frame_metadata_view)
            {
                auto key_val_msg = message_instance.instantiate<diagnostic_msgs::KeyValue>();
                if (key_val_msg == nullptr)
                {
                    throw io_exception(to_string() << "Expected KeyValue message but got: " << message_instance.getDataType() << "(Topic: " << message_instance.getTopic() << ")");
                }
                if(key_val_msg->key == "timestamp_domain")
                {
                    additional_data.timestamp_domain = static_cast<rs2_timestamp_domain>(std::stoi(key_val_msg->value));
                }
                else if (key_val_msg->key == "system_time")
                {
                    additional_data.system_time = std::stoll(key_val_msg->value);
                }
                else
                {
                    rs2_frame_metadata type;
                    conversions::convert(key_val_msg->key, type);

                    auto size_of_enum = sizeof(rs2_frame_metadata);
                    rs2_metadata_t md = static_cast<rs2_metadata_t>(std::stoll(key_val_msg->value));
                    auto size_of_data = sizeof(rs2_metadata_t);
                    if (total_md_size + size_of_enum + size_of_data > 255)
                    {
                        continue;; //stop adding metadata to frame
                    }
                    memcpy(additional_data.metadata_blob.data() + total_md_size, &type, size_of_enum);
                    total_md_size += size_of_enum;
                    memcpy(additional_data.metadata_blob.data() + total_md_size, &md, size_of_data);
                    total_md_size += size_of_data;
                }
            }

            frame_interface* frame = m_frame_source.alloc_frame(RS2_EXTENSION_VIDEO_FRAME, msg->data.size(), additional_data, true);
            if (frame == nullptr)
                return nullptr;

            librealsense::video_frame* video_frame = static_cast<librealsense::video_frame*>(frame);
            video_frame->assign(msg->width, msg->height, msg->step, msg->step / msg->width / 8); //TODO: Ziv, is bpp bytes or bits per pixel?

            rs2_format stream_format;
            conversions::convert(msg->encoding, stream_format);
            frame->get_stream()->set_format(stream_format);
            frame->get_stream()->set_stream_index(stream_id.stream_index);
            frame->get_stream()->set_stream_type(stream_id.stream_type);
            //TODO: Ziv, frame->set_stream()
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

        rs2_camera_info rs2_camera_info_from_string(const std::string& info)
        {
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_NAME            ) ) return RS2_CAMERA_INFO_NAME;
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_SERIAL_NUMBER   ) ) return RS2_CAMERA_INFO_SERIAL_NUMBER;
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_FIRMWARE_VERSION) ) return RS2_CAMERA_INFO_FIRMWARE_VERSION;
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_LOCATION        ) ) return RS2_CAMERA_INFO_LOCATION;
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_DEBUG_OP_CODE   ) ) return RS2_CAMERA_INFO_DEBUG_OP_CODE;
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_ADVANCED_MODE   ) ) return RS2_CAMERA_INFO_ADVANCED_MODE;
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_PRODUCT_ID      ) ) return RS2_CAMERA_INFO_PRODUCT_ID;
            if(info == rs2_camera_info_to_string(RS2_CAMERA_INFO_CAMERA_LOCKED   ) ) return RS2_CAMERA_INFO_CAMERA_LOCKED;
            throw std::runtime_error(info + " cannot be converted to rs2_camera_info");
        }

        device_snapshot read_device_description()
        {
            snapshot_collection device_extensions;

            std::shared_ptr<info_snapshot> info = read_info_snapshot(ros_topic::device_info_topic(get_device_index()));
            device_extensions[RS2_EXTENSION_INFO ] = info;
            std::vector<sensor_snapshot> sensor_descriptions;
            uint32_t sensor_count = read_sensor_count(get_device_index());
            for (uint32_t sensor_index = 0; sensor_index < sensor_count; sensor_index++)
            {
                snapshot_collection sensor_extensions;
                auto streams_snapshots = read_stream_info(ros_topic::sensor_info_topic({ get_device_index(), sensor_index }));

               /* std::shared_ptr<options_snapshot> options = options_container read_options();
                sensor_extensions[RS2_EXTENSION_OPTIONS ] = options;*/

                std::shared_ptr<info_snapshot> sensor_info = read_info_snapshot(ros_topic::sensor_info_topic({ get_device_index(), sensor_index }));
                sensor_extensions[RS2_EXTENSION_INFO ] = sensor_info;

                sensor_descriptions.emplace_back(sensor_extensions, streams_snapshots);
            }

            return device_snapshot(device_extensions, sensor_descriptions);
        }

        std::shared_ptr<info_snapshot> read_info_snapshot(const std::string& topic)
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
                    rs2_camera_info info = rs2_camera_info_from_string(info_msg->key);
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
            rosbag::View sensor_infos(m_file, RegexTopicQuery("/device_" + std::to_string(device_index) + R"(/sensor_(\d)+/info))"));
            std::set<uint32_t> sensor_indices;
            for (auto sensor_info : sensor_infos)
            {
                auto msg = sensor_info.instantiate<diagnostic_msgs::KeyValue>();
                if(msg == nullptr)
                {
                    throw io_exception(to_string() << "Expected KeyValue message but got " << sensor_info.getDataType() << "(Topic: " << sensor_info.getTopic() << ")");
                }
                sensor_indices.insert(static_cast<uint32_t>(std::stoll(msg->key)));
            }
            return sensor_indices.size();
        }

        std::vector<snapshot_collection> read_stream_info(const std::string& topic)
        {
            std::vector<snapshot_collection> streams_snapshots;

//            std::vector<stream_profile> profiles;
//            std::vector<rs2_intrinsics> intrinsics_info;
//            std::vector<rs2_stream> streams;
//
//            std::vector<std::shared_ptr<file_format::ros_data_objects::stream_info>> stream_info;
//            auto ros_retval = read_stream_infos(stream_info,file_format::file_types::sample_type::st_image, sensor_id);
//            if (ros_retval !=file_format::status::status_no_error)
//            {
//                throw io_exception("Failed to read stream profiles");
//            }
//     
//            for(auto item : stream_info)
//            {
//                std::shared_ptr<file_format::ros_data_objects::image_stream_info> image_item =
//                    std::static_pointer_cast<file_format::ros_data_objects::image_stream_info>(item);
//               file_format::ros_data_objects::image_stream_data info = image_item->get_info();
//                stream_profile profile {};
//                profile.fps = info.fps;
//                profile.height = info.height;
//                profile.width = info.width;
//                profile.format = info.format;
//                profile.stream = info.type;
//
//                rs2_intrinsics intrinsics_item = info.intrinsics;
//                intrinsics_item.height = info.height;
//                intrinsics_item.width = info.width;
//                profiles.push_back(profile);
//                intrinsics_info.push_back(intrinsics_item);
//                streams.push_back(profile.stream);
//                //TODO: get intrinsics
//                //if(extrinsics.find(sensor_index) == extrinsics.end())
//                //{
//                //    std::pair<core::extrinsics, uint64_t> extrinsics_pair;
//                //    extrinsics_pair.first = conversions::convert(info.stream_extrinsics.extrinsics_data);
//                //    extrinsics_pair.second = info.stream_extrinsics.reference_point_id;
//                //    extrinsics[sensor_index] = extrinsics_pair;
//                //}
//            }
            return streams_snapshots;
        }

        std::shared_ptr<options_container> read_options()
        {
            return nullptr;
        }

        static std::vector<std::string> get_topics(std::unique_ptr<rosbag::View>& view)
        {
            std::vector<std::string> topics;
            for(const rosbag::ConnectionInfo* connection : view->getConnections())
            {
                topics.emplace_back(connection->topic);
            }
            return topics;
        }

        device_snapshot m_device_description;
        device_serializer::nanoseconds m_first_frame_time;
        device_serializer::nanoseconds m_total_duration;
        std::string m_file_path;
        librealsense::frame_source m_frame_source;
        rosbag::Bag                     m_file;
        std::unique_ptr<rosbag::View>   m_samples_view;
        rosbag::View::iterator          m_samples_itrator;
        std::vector<std::string>        m_topics;
        std::shared_ptr<metadata_parser_map> m_metadata_parser_map;
    };


}
