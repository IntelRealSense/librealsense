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
#include "std_msgs/Float32.h"
#include "std_msgs/String.h"
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
            m_version(0),
            m_metadata_parser_map(create_metadata_parser_map())
        {
            try
            {
                reset(); //Note: calling a virtual function inside c'tor, safe while base function is pure virtual
                m_total_duration = get_file_duration(m_file);
            }
            catch (const std::exception& e)
            {
                //Rethrowing with better clearer message
                throw io_exception(to_string() << "Failed to create ros reader: " << e.what());
            }
        }

        device_snapshot query_device_description(const nanoseconds& time) override
        {
            return read_device_description(time);
        }

        std::shared_ptr<serialized_data> read_next_data() override
        {
            if (m_samples_view == nullptr || m_samples_itrator == m_samples_view->end())
            {
                LOG_DEBUG("End of file reached");
                return std::make_shared<serialized_end_of_file>();
            }

            rosbag::MessageInstance next_msg = *m_samples_itrator;
            ++m_samples_itrator;

            if (next_msg.isType<sensor_msgs::Image>() || next_msg.isType<sensor_msgs::Imu>())
            {
                LOG_DEBUG("Next message is a frame");
                return create_frame(next_msg);
            }

            if (m_version >= 3)
            {
                if (next_msg.isType<std_msgs::Float32>())
                {
                    auto timestamp = to_nanoseconds(next_msg.getTime());
                    auto sensor_id = ros_topic::get_sensor_identifier(next_msg.getTopic());
                    auto option = create_option(m_file, next_msg);
                    return std::make_shared<serialized_option>(timestamp, sensor_id, option.first, option.second);
                }
            }

            std::string err_msg = to_string() << "Unknown message type: " << next_msg.getDataType() << "(Topic: " << next_msg.getTopic() << ")";
            LOG_ERROR(err_msg);
            throw invalid_value_exception(err_msg);
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

        void reset() override
        {
            m_file.close();
            m_file.open(m_file_path, rosbag::BagMode::Read);

            m_version = read_file_version(m_file);
            if (m_version < get_minimum_supported_file_version())
            {
                throw std::runtime_error("unsupported file version");
            }

            m_samples_view = nullptr;
            m_frame_source = std::make_shared<frame_source>();
            m_frame_source->init(m_metadata_parser_map);
            m_initial_device_description = read_device_description(get_static_file_info_timestamp(), true);
        }

        virtual void enable_stream(const std::vector<device_serializer::stream_identifier>& stream_ids) override
        {
            ros::Time start_time = ros::TIME_MIN + ros::Duration{ 0, 1 }; //first non 0 timestamp and afterward
            if (m_samples_view == nullptr) //Starting to stream
            {
                m_samples_view = std::unique_ptr<rosbag::View>(new rosbag::View(m_file, FalseQuery()));
                m_samples_view->addQuery(m_file, OptionsQuery(), start_time);
                m_samples_itrator = m_samples_view->begin();
            }
            else //Already streaming
            {
                if (m_samples_itrator != m_samples_view->end())
                {
                    rosbag::MessageInstance sample_msg = *m_samples_itrator;
                    start_time = sample_msg.getTime();
                }
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

        template <typename ROS_TYPE>      
        static typename ROS_TYPE::ConstPtr instantiate_msg(const rosbag::MessageInstance& msg)
        {
            typename ROS_TYPE::ConstPtr msg_instnance_ptr = msg.instantiate<ROS_TYPE>();
            if (msg_instnance_ptr == nullptr)
            {
                throw io_exception(to_string() 
                    << "Invalid file format, expected " 
                    << ros::message_traits::DataType<ROS_TYPE>::value()
                    << " message but got: " << msg.getDataType()
                    << "(Topic: " << msg.getTopic() << ")");
            }
            return msg_instnance_ptr;
        }
        
        std::shared_ptr<serialized_frame> create_frame(const rosbag::MessageInstance& msg)
        {
            auto next_msg_topic = msg.getTopic();
            auto next_msg_time = msg.getTime();

            nanoseconds timestamp = to_nanoseconds(next_msg_time);
            stream_identifier stream_id = ros_topic::get_stream_identifier(next_msg_topic);

            if (msg.isType<sensor_msgs::Image>())
            {
                frame_holder frame = create_image_from_message(msg);
                return std::make_shared<serialized_frame>(timestamp, stream_id, std::move(frame));
            }

            if (msg.isType<sensor_msgs::Imu>())
            {
                //frame = create_motion_sample(msg);
                //return std::make_shared<serialized_frame>(timestamp, stream_id, std::move(frame));
            }

            std::string err_msg = to_string() << "Unknown frame type: " << msg.getDataType() << "(Topic: " << next_msg_topic << ")";
            LOG_ERROR(err_msg);
            throw invalid_value_exception(err_msg);
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

        static nanoseconds get_file_duration(const rosbag::Bag& file)
        {
            rosbag::View all_frames_view(file, FrameQuery());
            auto streaming_duration = all_frames_view.getEndTime() - all_frames_view.getBeginTime();
            return nanoseconds(streaming_duration.toNSec());
        }

        frame_holder create_image_from_message(const rosbag::MessageInstance &image_data) const
        {
            LOG_DEBUG("Trying to create an image frame from message");

            auto msg = instantiate_msg<sensor_msgs::Image>(image_data);

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
                auto key_val_msg = instantiate_msg<diagnostic_msgs::KeyValue>(message_instance);

                if(key_val_msg->key == "timestamp_domain") //TODO: use constants
                {
                    try
                    {
                        convert(key_val_msg->value, additional_data.timestamp_domain);
                    }
                    catch(const std::exception& e)
                    {
                        LOG_ERROR(e.what());
                        continue;
                    }
                }
                else if (key_val_msg->key == "system_time") //TODO: use constants
                {
                    try
                    {
                        additional_data.system_time = std::stod(key_val_msg->value);
                    }
                    catch(const std::exception& e)
                    {
                        LOG_ERROR(e.what());
                        continue;
                    }
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
            LOG_DEBUG("Created image frame: " << stream_format << " " << video_frame->get_width() << "x" << video_frame->get_height() << " " << stream_format);

            return std::move(fh);
        }

        static uint32_t read_file_version(const rosbag::Bag& file)
        {
            auto version_topic = ros_topic::file_version_topic();
            rosbag::View view(file, rosbag::TopicQuery(version_topic));
            if (view.size() == 0)
            {
                throw io_exception(to_string() << "Invalid file format, file does not contain topic \"" << version_topic << "\"");
            }
            assert(view.size() == 1); //version message is expected to be a single one

            auto item = *view.begin();
            auto msg = instantiate_msg<std_msgs::UInt32>(item);
            return msg->data;
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
            auto tf_msg = instantiate_msg<geometry_msgs::Transform>(msg);
            group_id = ros_topic::get_extrinsic_group_index(msg.getTopic());
            convert(*tf_msg, extrinsic);
            return true;
        }

        static void update_sensor_options(const rosbag::Bag& file, uint32_t sensor_index, const nanoseconds& time, uint32_t file_version, snapshot_collection& sensor_extensions)
        {
            auto sensor_options = read_sensor_options(file, { get_device_index(), sensor_index }, time, file_version);
            sensor_extensions[RS2_EXTENSION_OPTIONS] = sensor_options;

            if (sensor_options->supports_option(RS2_OPTION_DEPTH_UNITS))
            {
                auto&& dpt_opt = sensor_options->get_option(RS2_OPTION_DEPTH_UNITS);
                sensor_extensions[RS2_EXTENSION_DEPTH_SENSOR] = std::make_shared<depth_sensor_snapshot>(dpt_opt.query());

                if (sensor_options->supports_option(RS2_OPTION_STEREO_BASELINE))
                {
                    auto&& bl_opt = sensor_options->get_option(RS2_OPTION_STEREO_BASELINE);
                    sensor_extensions[RS2_EXTENSION_DEPTH_STEREO_SENSOR] = std::make_shared<depth_stereo_sensor_snapshot>(dpt_opt.query(), bl_opt.query());
                }
            }
        }

        device_snapshot read_device_description(const nanoseconds& time, bool reset = false)
        {
            if (time == get_static_file_info_timestamp())
            {
                if (reset)
                {
                    snapshot_collection device_extensions;

                    auto info = read_info_snapshot(ros_topic::device_info_topic(get_device_index()));
                    device_extensions[RS2_EXTENSION_INFO] = info;

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
                            if (try_read_stream_extrinsic(stream_id, reference_id, stream_extrinsic))
                            {
                                extrinsics_map[stream_id] = std::make_pair(reference_id, stream_extrinsic);
                            }
                        }
                        
                        //Update infos
                        auto sensor_info = read_info_snapshot(ros_topic::sensor_info_topic({ get_device_index(), sensor_index }));
                        sensor_extensions[RS2_EXTENSION_INFO] = sensor_info;
                        //Update options
                        update_sensor_options(m_file, sensor_index, time, m_version, sensor_extensions);

                        sensor_descriptions.emplace_back(sensor_index, sensor_extensions, streams_snapshots);
                    }

                    m_initial_device_description = device_snapshot(device_extensions, sensor_descriptions, extrinsics_map);
                }
                return m_initial_device_description;
            }
            else
            {
                //update only:
                auto device_snapshot = m_initial_device_description;
                for (auto& sensor : device_snapshot.get_sensors_snapshots())
                {
                    auto& sensor_extensions = sensor.get_sensor_extensions_snapshots();
                    update_sensor_options(m_file, sensor.get_sensor_index(), time, m_version, sensor_extensions);
                }
                return device_snapshot;
            }
        }

        std::shared_ptr<info_container> read_info_snapshot(const std::string& topic) const
        {
            auto infos = std::make_shared<info_container>();
            std::map<rs2_camera_info, std::string> values;
            rosbag::View view(m_file, rosbag::TopicQuery(topic));
            for (auto message_instance : view)
            {
                diagnostic_msgs::KeyValueConstPtr info_msg = instantiate_msg<diagnostic_msgs::KeyValue>(message_instance);
                try
                {
                    rs2_camera_info info;
                    convert(info_msg->key, info);
                    infos->register_info(info, info_msg->value);
                }
                catch (const std::exception& e)
                {
                    std::cerr << e.what() << std::endl;
                }
            }

            return infos;
        }

        std::set<uint32_t> read_sensor_indices(uint32_t device_index) const
        {
            rosbag::View sensor_infos(m_file, SensorInfoQuery(device_index));
            std::set<uint32_t> sensor_indices;
            for (auto sensor_info : sensor_infos)
            {
                auto msg = instantiate_msg<diagnostic_msgs::KeyValue>(sensor_info);
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

                auto stream_info_msg = instantiate_msg<realsense_msgs::StreamInfo>(infos_view);
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
                    auto video_stream_msg = instantiate_msg<sensor_msgs::CameraInfo>(video_stream_msg_ptr);
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

        static std::string read_option_description(const rosbag::Bag& file, const std::string& topic)
        {
            rosbag::View option_description_view(file, rosbag::TopicQuery(topic));
            if (option_description_view.size() == 0)
            {
                LOG_ERROR("File does not contain topics for: " << topic);
                return "N/A";
            }
            assert(option_description_view.size() == 1); //There should be only 1 message for each option
            auto description_message_instance = *option_description_view.begin();
            auto option_desc_msg = instantiate_msg<std_msgs::String>(description_message_instance);
            return option_desc_msg->data;
        }
        
        /*Until Version 2 (including)*/
        static std::pair<rs2_option, std::shared_ptr<librealsense::option>> create_property(const rosbag::MessageInstance& property_message_instance)
        {
            auto property_msg = instantiate_msg<diagnostic_msgs::KeyValue>(property_message_instance);
            rs2_option id;
            convert(property_msg->key, id);
            float value = std::stof(property_msg->value);
            std::string description = to_string() << "Read only option of " << id;
            return std::make_pair(id, std::make_shared<const_value_option>(description, value));
        }

        /*Starting version 3*/
        static std::pair<rs2_option, std::shared_ptr<librealsense::option>> create_option(const rosbag::Bag& file, const rosbag::MessageInstance& value_message_instance)
        {
            auto option_value_msg = instantiate_msg<std_msgs::Float32>(value_message_instance);
            std::string option_name = ros_topic::get_option_name(value_message_instance.getTopic());
            device_serializer::sensor_identifier sensor_id = ros_topic::get_sensor_identifier(value_message_instance.getTopic());
            rs2_option id;
            convert(option_name, id);
            float value = option_value_msg->data;
            std::string description = read_option_description(file, ros_topic::option_description_topic(sensor_id, id));
            return std::make_pair(id, std::make_shared<const_value_option>(description, value));
        }

        static std::shared_ptr<options_container> read_sensor_options(const rosbag::Bag& file, device_serializer::sensor_identifier sensor_id, const nanoseconds& timestamp, uint32_t file_version)
        {
            auto options = std::make_shared<options_container>();
            if (file_version == 2)
            {
                rosbag::View sensor_options_view(file, rosbag::TopicQuery(ros_topic::property_topic(sensor_id)));
                for (auto message_instance : sensor_options_view)
                {
                    auto id_option = create_property(message_instance);
                    options->register_option(id_option.first, id_option.second);
                }
            }
            else
            {
                //Taking all messages from the beginning of the bag until the time point requested
                for (int i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++)
                {
                    rs2_option id = static_cast<rs2_option>(i);
                    std::string option_topic = ros_topic::option_value_topic(sensor_id, id);
                    rosbag::View option_view(file, rosbag::TopicQuery(option_topic), to_rostime(get_static_file_info_timestamp()), to_rostime(timestamp));
                    auto it = option_view.begin();
                    if (it == option_view.end())
                    {
                        continue;
                    }
                    rosbag::View::iterator last_item;
                    while (it != option_view.end())
                    {
                        last_item = it++;
                    }
                    auto option = create_option(file, *last_item);
                    assert(id == option.first);
                    options->register_option(option.first, option.second);
                }
            }
            return options;
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

        device_snapshot                         m_initial_device_description;
        nanoseconds                             m_total_duration;
        std::string                             m_file_path;
        std::shared_ptr<frame_source>           m_frame_source;
        rosbag::Bag                             m_file;
        std::unique_ptr<rosbag::View>           m_samples_view;
        rosbag::View::iterator                  m_samples_itrator;
        std::vector<std::string>                m_enabled_streams_topics;
        std::shared_ptr<metadata_parser_map>    m_metadata_parser_map;
        std::shared_ptr<context>                m_context;
        uint32_t                                m_version;
    };


}
