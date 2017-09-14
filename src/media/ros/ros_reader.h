// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <chrono>
#include <mutex>
#include <regex>
#include <core/serialization.h>
#include "rosbag/view.h"
#include "sensor_msgs/Imu.h"
#include "sensor_msgs/Image.h"
#include "diagnostic_msgs/KeyValue.h"
#include "std_msgs/UInt32.h"
#include "realsense_msgs/StreamInfo.h"
#include "sensor_msgs/CameraInfo.h"
#include "ros_file_format.h"

namespace librealsense
{
    using namespace device_serializer;

    class ros_reader: public device_serializer::reader
    {
    public:
        ros_reader(const std::string& file, const std::shared_ptr<context>& ctx) :
            m_total_duration(0),
            m_file_path(file),
            m_context(ctx),
            m_metadata_parser_map(create_metadata_parser_map())
        {
            reset(); //Note: calling a virtual function inside c'tor, safe while base function is pure virtual
            m_total_duration = get_file_duration();
        }

        device_snapshot query_device_description() override
        {
            return m_device_description;
        }

        status read_frame(nanoseconds& timestamp, stream_identifier& stream_id, frame_holder& frame) override
        {
            while(m_samples_view != nullptr && m_samples_itrator != m_samples_view->end())
            {
                rosbag::MessageInstance next_frame_msg = *m_samples_itrator;
                ++m_samples_itrator;
                if (next_frame_msg.isType<sensor_msgs::Image>())
                {
                    stream_id = ros_topic::get_stream_identifier(next_frame_msg.getTopic());
                    timestamp = nanoseconds(next_frame_msg.getTime().toNSec());
                    frame = read_image_from_message(next_frame_msg);
                    return status::status_no_error;
                }

                if (next_frame_msg.isType<sensor_msgs::Imu>())
                {
                    //stream_id = ros_topic::get_stream_identifier(next_frame_msg.getTopic());
                    //timestamp = nanoseconds(next_frame_msg.getTime().toNSec());
                    //frame = create_motion_sample(sample_msg);
                    return status::status_no_error;
                }
                LOG_WARNING("Unknown frame type: " << next_frame_msg.getDataType() << "(Topic: " << next_frame_msg.getTopic() << ")");
            }
            return status_file_eof;
        }

        void seek_to_time(const nanoseconds& seek_time) override
        {
            if (seek_time > m_total_duration)
            {
                throw invalid_value_exception(to_string() << "Requested time is out of playback length. (Requested = " << seek_time.count() << ", Duration = " << m_total_duration.count() << ")");
            }
            auto seek_time_as_secs = std::chrono::duration_cast<std::chrono::duration<double>>(seek_time);
            auto seek_time_as_rostime = ros::Time(seek_time_as_secs.count());

            m_samples_view.reset(new rosbag::View(m_file, FalseQuery()));
            //Using cached topics here and not querying them (before reseting) since a previous call to seek
            // could have changed the view and some streams that should be streaming were dropped.
            //E.g:  Recording Depth+Color, stopping Depth, starting IR, stopping IR and Color. Play IR+Depth: will play only depth, then only IR, then we seek to a point only IR was streaming, and then to 0.
            for (auto topic : m_enabled_streams_topics)
            {
                m_samples_view->addQuery(m_file, rosbag::TopicQuery(topic), seek_time_as_rostime);
            }
            m_samples_itrator = m_samples_view->begin();
        }

        nanoseconds query_duration() const override
        {
            return m_total_duration;
        }

        static std::shared_ptr<metadata_parser_map> create_metadata_parser_map()
        {
            auto md_parser_map = std::make_shared<metadata_parser_map>();
            for (int i = 0; i < static_cast<int>(rs2_frame_metadata_value::RS2_FRAME_METADATA_COUNT); ++i)
            {
                auto frame_md_type = static_cast<rs2_frame_metadata_value>(i);
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
            m_frame_source = std::make_shared<frame_source>();
            m_frame_source->init(m_metadata_parser_map);
            m_device_description = read_device_description();
        }

        virtual void enable_stream(const std::vector<device_serializer::stream_identifier>& stream_ids) override
        {
            ros::Time start_time = ros::TIME_MIN;
            if (m_samples_view == nullptr)
            {
                m_samples_view = std::unique_ptr<rosbag::View>(new rosbag::View(m_file, FalseQuery()));
                m_samples_itrator = m_samples_view->begin();
            }

            if (m_samples_itrator != m_samples_view->end())
            {
                rosbag::MessageInstance sample_msg = *m_samples_itrator;
                start_time = sample_msg.getTime();
            }

            auto currently_streaming = get_topics(m_samples_view);
            //empty the view
            m_samples_view = std::unique_ptr<rosbag::View>(new rosbag::View(m_file, FalseQuery()));

            for (auto&& stream_id : stream_ids)
            {
                //add new stream to view
                m_samples_view->addQuery(m_file, StreamQuery(stream_id), start_time);
            }

            //add already existing streams
            for (auto topic : currently_streaming)
            {
                m_samples_view->addQuery(m_file, rosbag::TopicQuery(topic), start_time);
            }
            m_samples_itrator = m_samples_view->begin();
            m_enabled_streams_topics = get_topics(m_samples_view);
        }

        virtual void disable_stream(const std::vector<device_serializer::stream_identifier>& stream_ids) override
        {
            if (m_samples_view == nullptr)
            {
                return;
            }
            ros::Time curr_time;
            if (m_samples_itrator == m_samples_view->end())
            {
                curr_time = m_samples_view->getEndTime();
            }
            else
            {
                rosbag::MessageInstance sample_msg = *m_samples_itrator;
                curr_time = sample_msg.getTime();
            }
            auto currently_streaming = get_topics(m_samples_view);
            m_samples_view = std::unique_ptr<rosbag::View>(new rosbag::View(m_file, FalseQuery()));
            for (auto topic : currently_streaming)
            {
                //Find if this topic is one of the streams that should be disabled
                auto it = std::find_if(stream_ids.begin(), stream_ids.end(), [&topic](const device_serializer::stream_identifier& s) {
                    //return topic.starts_with(s);
                    return topic.find(ros_topic::stream_full_prefix(s)) != std::string::npos;
                });
                bool should_topic_remain = (it == stream_ids.end());
                if (should_topic_remain)
                {
                    m_samples_view->addQuery(m_file, rosbag::TopicQuery(topic), curr_time);
                }
            }
            m_samples_itrator = m_samples_view->begin();
            m_enabled_streams_topics = get_topics(m_samples_view);
        }

        const std::string& get_file_name() const override
        {
            return m_file_path;
        }

    private:

        nanoseconds get_file_duration() const
        {
            rosbag::View all_frames_view(m_file, FrameQuery());
            auto streaming_duration = all_frames_view.getEndTime() - all_frames_view.getBeginTime();
            return nanoseconds(streaming_duration.toNSec());
        }

        frame_holder read_image_from_message(const rosbag::MessageInstance &image_data) const
        {
            sensor_msgs::ImagePtr msg = image_data.instantiate<sensor_msgs::Image>();
            if (msg == nullptr)
            {
                throw io_exception(to_string() << "Expected image message but got " << image_data.getDataType());
            }

            frame_additional_data additional_data {};
            std::chrono::duration<double, std::milli> timestamp_us(std::chrono::duration<double>(msg->header.stamp.toSec()));
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
                    convert(key_val_msg->value, additional_data.timestamp_domain);
                }
                else if (key_val_msg->key == "system_time") //TODO: use constants
                {
                    additional_data.system_time = std::stod(key_val_msg->value);
                }
                else
                {
                    rs2_frame_metadata_value type;
                    try
                    {
                        convert(key_val_msg->key, type);
                    }
                    catch(const std::exception& e)
                    {
                        LOG_ERROR(e.what());
                        continue;
                    }
                    auto size_of_enum = sizeof(rs2_frame_metadata_value);
                    rs2_metadata_type md = static_cast<rs2_metadata_type>(std::stoll(key_val_msg->value));
                    auto size_of_data = sizeof(rs2_metadata_type);
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
            additional_data.metadata_size = total_md_size;
            frame_interface* frame = m_frame_source->alloc_frame((stream_id.stream_type == RS2_STREAM_DEPTH) ? RS2_EXTENSION_DEPTH_FRAME : RS2_EXTENSION_VIDEO_FRAME,
                                                                msg->data.size(), additional_data, true);
            if (frame == nullptr)
            {
                throw invalid_value_exception("Failed to allocate new frame");
            }
            librealsense::video_frame* video_frame = static_cast<librealsense::video_frame*>(frame);
            video_frame->assign(msg->width, msg->height, msg->step, msg->step / msg->width * 8);
            rs2_format stream_format;
            convert(msg->encoding, stream_format);
            //attaching a temp stream to the frame. Playback sensor should assign the real stream
            frame->set_stream(std::make_shared<video_stream_profile>(platform::stream_profile{}));
            frame->get_stream()->set_format(stream_format);
            frame->get_stream()->set_stream_index(stream_id.stream_index);
            frame->get_stream()->set_stream_type(stream_id.stream_type);
            video_frame->data = msg->data;
            librealsense::frame_holder fh{ video_frame };
            return std::move(fh);
        }

        bool get_file_version_from_file(uint32_t& version) const
        {
            rosbag::View view(m_file, rosbag::TopicQuery(ros_topic::file_version_topic()));
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

        bool try_read_stream_extrinsic(const stream_identifier& stream_id, uint32_t& group_id, rs2_extrinsics& extrinsic) const
        {
            rosbag::View tf_view(m_file, ExtrinsicsQuery(stream_id));
            if (tf_view.size() == 0)
            {
                return false;
            }
            assert(tf_view.size() == 1); //There should be 1 message per stream
            auto msg = *tf_view.begin();
            auto tf_msg = msg.instantiate<geometry_msgs::Transform>();
            if (tf_msg == nullptr)
            {
                throw io_exception(to_string() << "Expected KeyValue message but got " << msg.getDataType() << "(Topic: " << msg.getTopic() << ")");
            }
            group_id = ros_topic::get_extrinsic_group_index(msg.getTopic());
            convert(*tf_msg, extrinsic);
            return true;
        }

        device_snapshot read_device_description() const
        {
            snapshot_collection device_extensions;

            std::shared_ptr<info_snapshot> info = read_info_snapshot(ros_topic::device_info_topic(get_device_index()));
            device_extensions[RS2_EXTENSION_INFO ] = info;
            std::vector<sensor_snapshot> sensor_descriptions;
            auto sensor_indices = read_sensor_indices(get_device_index());
            std::map<stream_identifier, std::pair<uint32_t, rs2_extrinsics>> extrinsics_map;
            for (auto sensor_index : sensor_indices)
            {
                snapshot_collection sensor_extensions;
                auto streams_snapshots = read_stream_info(get_device_index(), sensor_index);
                for (auto stream_profile : streams_snapshots)
                {
                    auto stream_id = stream_identifier{ get_device_index(), sensor_index, stream_profile->get_stream_type(), static_cast<uint32_t>(stream_profile->get_stream_index()) };
                    uint32_t reference_id;
                    rs2_extrinsics stream_extrinsic;
                    if(try_read_stream_extrinsic(stream_id, reference_id, stream_extrinsic))
                    {
                        extrinsics_map[stream_id] = std::make_pair(reference_id, stream_extrinsic);
                    }
                }

                std::shared_ptr<info_snapshot> sensor_info = read_info_snapshot(ros_topic::sensor_info_topic({ get_device_index(), sensor_index }));
                sensor_extensions[RS2_EXTENSION_INFO] = sensor_info;
                std::map<enum rs2_option, float> sensor_options = read_sensor_options(ros_topic::property_topic({ get_device_index(), sensor_index }));
                auto depth_scale_it = sensor_options.find(RS2_OPTION_DEPTH_UNITS);
                if(depth_scale_it != sensor_options.end())
                {
                    sensor_extensions[RS2_EXTENSION_DEPTH_SENSOR] = std::make_shared<depth_sensor_snapshot>(depth_scale_it->second);
                }
                sensor_descriptions.emplace_back(sensor_index, sensor_extensions, sensor_options, streams_snapshots);
            }

            return device_snapshot(device_extensions, sensor_descriptions, extrinsics_map);
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
                    convert(info_msg->key, info);
                    values[info] = info_msg->value;
                }
                catch (const std::exception& e)
                {
                    std::cerr << e.what() << std::endl;
                }
            }

            return std::make_shared<info_snapshot>(values);
        }

        std::set<uint32_t> read_sensor_indices(uint32_t device_index) const
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
            return sensor_indices;
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
                convert(stream_info_msg->encoding, format);

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

                    auto profile = std::make_shared<video_stream_profile>(platform::stream_profile{ video_stream_msg->width ,video_stream_msg->height, fps, static_cast<uint32_t>(format) });
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
        std::map<enum rs2_option, float> read_sensor_options(const std::string& topic) const
        {
            rosbag::View sensor_options_view(m_file, rosbag::TopicQuery(topic));
            std::map<enum rs2_option, float> options;
            for (auto message_instance : sensor_options_view)
            {
                auto property_msg = message_instance.instantiate<diagnostic_msgs::KeyValue>();
                if(property_msg == nullptr)
                {
                    throw io_exception(to_string() << "Expected KeyValue message but got: " << message_instance.getDataType() << "(Topic: " << message_instance.getTopic() << ")");
                }
                rs2_option id;
                convert(property_msg->key, id);
                options[id] = std::stof(property_msg->value);

            }
            return options;
        }
        std::shared_ptr<options_container> read_options()
        {
            return nullptr;
        }

        static std::vector<std::string> get_topics(std::unique_ptr<rosbag::View>& view)
        {
            std::vector<std::string> topics;
            if(view != nullptr)
            {
                auto connections = view->getConnections();
                std::transform(connections.begin(), connections.end(), std::back_inserter(topics), [](const rosbag::ConnectionInfo* connection) { return connection->topic; });
            }
            return topics;
        }

        device_snapshot                         m_device_description;
        nanoseconds                             m_total_duration;
        std::string                             m_file_path;
        std::shared_ptr<frame_source>           m_frame_source;
        rosbag::Bag                             m_file;
        std::unique_ptr<rosbag::View>           m_samples_view;
        rosbag::View::iterator                  m_samples_itrator;
        std::vector<std::string>                m_enabled_streams_topics;
        std::shared_ptr<metadata_parser_map>    m_metadata_parser_map;
        std::shared_ptr<context>                m_context;
    };


}
