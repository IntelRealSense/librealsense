// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <core/serialization.h>
#include <ros/stream_playback.h>

namespace librealsense
{

    class ros_reader: public device_serializer::reader
    {
        //TODO: Ziv, move to better location
        uint32_t DEVICE_INDEX = (std::numeric_limits<uint32_t>::max)(); //braces are for windows compilation
        std::string SENSOR_COUNT { "sensor_count" };
        rs::file_format::file_types::microseconds FIRST_FRAME_TIMESTAMP { 0 };
    public:
        ros_reader(const std::string& file) :
            m_stream_playback(file)
        {
            reset();
        }

        device_snapshot query_device_description() override
        {
            return m_device_description;
        }
        rs::file_format::status read(std::chrono::nanoseconds& timestamp, uint32_t& sensor_index, frame_holder& frame) override
        {
            std::unique_lock<std::mutex> locker(m_mutex);

            ////read cached properties avaialble after seek
            //if(m_cached_properties.empty() == false)
            //{
            //    cached_property properties_sensor_list = (*m_cached_properties.begin());
            //    *obj = properties_sensor_list.property;
            //    properties_sensor_list.property->add_ref();
            //    sensor_index = properties_sensor_list.sensor_id;
            //    timestamp = properties_sensor_list.timestamp;
            //    m_cached_properties.pop_back();
            //    return error_code::no_error;
            //}

            return read_sample(sensor_index, timestamp, frame);
        }

        void seek_to_time(std::chrono::nanoseconds time) override
        {
            throw not_implemented_exception(__FUNCTION__);
        }
        std::chrono::nanoseconds query_duration() const override
        {
            throw not_implemented_exception(__FUNCTION__);
        }
        void reset() override
        {
            m_first_frame_time = FIRST_FRAME_TIMESTAMP;
            m_device_description = read_metadata();
        }
        virtual void set_filter(uint32_t m_sensor_index, const std::vector<stream_profile>& vector) override
        {
            //TODO: Ziv, throw not_implemented_exception(__FUNCTION__);
        }
    private:
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

//        error_code seek_to_time(uint64_t time_microseconds)
//        {
//            if(m_reader == nullptr)
//            {
//                return read_failed;
//            }
//            auto seek_time = m_first_frame_time + rs::file_format::file_types::microseconds(time_microseconds);
//            auto time_interval = rs::file_format::file_types::nanoseconds(seek_time);
//
//            std::unique_lock<std::mutex> locker(m_mutex);
//            auto retval = set_properties_state(seek_time.count());
//            if(retval != error_code::no_error)
//            {
//                LOG_ERROR("Failed to seek_to_time " << time_microseconds << ", set_properties_state returned " << retval);
//                return retval;
//            }
//            auto sts = m_reader->seek_to_time(time_interval);
//            if(sts != rs::file_format::status_no_error)
//            {
//                LOG_ERROR("Failed to seek_to_time " << time_microseconds << ", m_reader->seek_to_time(" <<  time_interval.count() << ") returned " << sts);
//                return read_failed;
//            }
//            return error_code::no_error;
//        }
//        bool is_empty_intrinsics(const rs::core::intrinsics& intrinsics)
//        {
//            auto empty = rs::core::intrinsics{};
//            return empty == intrinsics;
//        }
//        error_code query_duration(uint64_t& duration) const
//        {
//            file_types::nanoseconds time;
//            if(m_reader->get_file_duration(time) != rs::file_format::status::status_no_error)
//            {
//                return error_code::read_failed;
//            }
//            duration = std::chrono::duration_cast<std::chrono::duration<uint64_t, std::micro>>(time).count();
//            return error_code::no_error;
//        }
//
//        bool is_empty_extrinsics(const rs::core::extrinsics& e)
//        {
//            constexpr uint8_t testblock [sizeof(rs::core::extrinsics)] {};
//            memset ((void*)testblock, 0, sizeof testblock);
//
//            return !memcmp (testblock, &e, sizeof(rs::core::extrinsics));
//        }
//        bool create_extrinsics_object(std::map<uint32_t, std::pair<rs::core::extrinsics, uint64_t>>& extrinsics,
//                                                            rs::core::data_object** extrinsics_object)
//        {
//            if(extrinsics.size() == 0)
//            {
//                return false;
//            }
//
//            std::vector<uint32_t> sensors_indices;
//            std::vector<rs::core::extrinsics> extrinsics_vec;
//            std::vector<uint64_t> reference_points_ids;
//
//            for(auto extrinsics_item : extrinsics)
//            {
//                sensors_indices.push_back(extrinsics_item.first);
//                extrinsics_vec.push_back(extrinsics_item.second.first);
//                reference_points_ids.push_back(extrinsics_item.second.second);
//            }
//
//            return rs::data_objects::common::extrinsics_data_object::create(sensors_indices.data(),
//                                                                            extrinsics_vec.data(),
//                                                                            reference_points_ids.data(),
//                                                                            static_cast<uint32_t>(sensors_indices.size()),
//                                                                            extrinsics_object);
//
//        }
//
        device_snapshot read_metadata()
        {
            snapshot_collection device_extensions;

            std::shared_ptr<info_snapshot> info = read_info_snapshot(DEVICE_INDEX);
            device_extrinsics extrinsics = read_device_extrinsics();
            device_extensions[RS2_EXTENSION_TYPE_INFO] = info;
            std::vector<sensor_snapshot> sensor_descriptions;
            uint32_t sensor_count = read_sensor_count();
            for (uint32_t sensor_index = 0; sensor_index < sensor_count; sensor_index++)
            {
                snapshot_collection sensor_extensions;
                std::vector<stream_profile> streaming_profiles = read_stream_info();

               /* std::shared_ptr<options_snapshot> options = options_container read_options();
                sensor_extensions[RS2_EXTENSION_TYPE_OPTIONS] = options;*/

                std::shared_ptr<info_snapshot> sensor_info = read_info_snapshot(sensor_index);
                sensor_extensions[RS2_EXTENSION_TYPE_INFO] = sensor_info;
                sensor_descriptions.emplace_back(sensor_extensions, streaming_profiles);
            }

            return device_snapshot(device_extensions, sensor_descriptions, extrinsics);
        }


       
        rs::file_format::status read_sample(uint32_t &sensor_index, std::chrono::nanoseconds& timestamp, frame_holder& frame)
        {
            std::shared_ptr<rs::file_format::ros_data_objects::sample> sample;
            auto reader_retval = m_stream_playback.read_next_sample(sample);
            if(reader_retval != rs::file_format::status_no_error)
            {
                return reader_retval;
            }

            if(sample->get_type() == rs::file_format::file_types::sample_type::st_image)
            {
                auto image = std::static_pointer_cast<rs::file_format::ros_data_objects::image>(sample);
                image->get_data(sensor_index, timestamp, frame);
                return rs::file_format::status::status_no_error;
            }
            //else if(sample->get_type() == rs::file_format::file_types::sample_type::st_motion)
            //{
            //    return read_motion(std::static_pointer_cast<ros_data_objects::motion_sample>(sample),
            //                       sensor_index, timestamp, obj);
            //}
            //else if(sample->get_type() == rs::file_format::file_types::sample_type::st_property)
            //{
            //    return read_property(std::static_pointer_cast<ros_data_objects::property>(sample),
            //                         sensor_index, timestamp, obj);
            //}
            //else if (sample->get_type() == rs::file_format::file_types::sample_type::st_pose)
            //{
            //    return read_pose(std::static_pointer_cast<ros_data_objects::pose>(sample),
            //                     sensor_index, timestamp, obj);
            //}
            
            return rs::file_format::status_param_unsupported;
        }
//
//        error_code read_pose(std::shared_ptr<ros_data_objects::pose> sample,
//                                                                           uint32_t &sensor_index, uint64_t &timestamp, core::data_object **obj)
//        {
//            if (sample == nullptr || obj == nullptr)
//            {
//                return error_code::invalid_handle;
//            }
//
//            std::shared_ptr<rs::file_format::ros_data_objects::pose> tracking =
//                std::static_pointer_cast<ros_data_objects::pose>(sample);
//
//            rs::core::pose pose = {};
//            rs::file_format::ros_data_objects::pose_info pose_data = tracking->get_info();
//
//            pose = conversions::convert(pose_data);
//
//            sensor_index = pose_data.device_id;
//            timestamp = pose_data.capture_time.count() / 1000;
//
//
//            if (rs::data_objects::tracking::pose_data_object::create(pose, obj) == false)
//            {
//                return error_code::data_object_creation_failed;
//            }
//            return error_code::no_error;
//        }
//
//        bool get_sensor_count(std::vector<std::shared_ptr<ros_data_objects::vendor_data>>& vendor_data, uint32_t& sensor_count)
//        {
//            for(auto data : vendor_data)
//            {
//                if(data->get_info().name == SENSOR_COUNT)
//                {
//                    auto value = data->get_info().value;
//                    sensor_count = std::stoi(value);
//                    return true;
//                }
//            }
//            return false;
//        }
//
//        error_code read_vendor_info(std::map<uint32_t, rs::utils::ref_count_ptr<rs::core::data_object>> &infos,
//                                                                                  uint32_t& sensor_count)
//        {
//            std::vector<std::shared_ptr<ros_data_objects::vendor_data>> vendor_data;
//            auto sts = m_reader->read_vendor_data(vendor_data, DEVICE_INDEX);
//            if(sts == rs::file_format::status_item_unavailable)
//            {
//                sensor_count = 0;
//                return error_code::no_error;
//            }
//            if(sts != rs::file_format::status_no_error || vendor_data.size() == 0)
//            {
//                return error_code::read_failed;
//            }
//
//            if(get_sensor_count(vendor_data, sensor_count) == false)
//            {
//                return error_code::read_failed;
//            }
//
//            //removing SENSOR_COUNT from the vendor_data since it is the only vendor data with vendor_data.name that is not a rs::core::guid
//            vendor_data.erase(
//                std::remove_if(vendor_data.begin(), vendor_data.end(),
//                               [](std::shared_ptr<ros_data_objects::vendor_data>& info) { return info->get_info().name == SENSOR_COUNT; }),
//                vendor_data.end());
//
//            //At this point all vendor_data should be in the form (guid, string)
//            rs::utils::ref_count_ptr<rs::core::data_object> obj;
//            auto retval = create_vendor_info_object(vendor_data, &obj);
//            if(retval != no_error)
//            {
//                return retval;
//            }
//            infos[DEVICE_INDEX] = obj;
//
//            for(uint32_t i = 0; i < m_sensor_count + 1; ++i)
//            {
//                std::vector<std::shared_ptr<ros_data_objects::vendor_data>> sensor_vendor_data;
//                if(m_reader->read_vendor_data(sensor_vendor_data, i) != rs::file_format::status_no_error)
//                {
//                    continue;
//                }
//
//                rs::utils::ref_count_ptr<rs::core::data_object> obj;
//                auto retval = create_vendor_info_object(sensor_vendor_data, &obj);
//                if(retval != no_error)
//                {
//                    return retval;
//                }
//                infos[i] = obj;
//
//            }
//            return error_code::no_error;
//        }
//
//        error_code create_vendor_info_object(std::vector<std::shared_ptr<ros_data_objects::vendor_data>>& vendor_data, rs::core::data_object** obj)
//        {
//            std::vector<rs::core::guid> guids;
//            std::vector<std::string> values;
//
//            for(auto data : vendor_data)
//            {
//                rs::core::guid id;
//                if(rs::utils::string_to_guid(data->get_info().name, id) == false)
//                {
//                    return error_code::read_failed;
//                }
//                guids.push_back(id);
//                values.push_back(data->get_info().value);
//            }
//
//            std::vector<const char*> as_char_arrays;
//            std::transform(values.begin(), values.end(), std::back_inserter(as_char_arrays), [](const std::string& s){ return s.c_str();});
//
//
//            if(rs::data_objects::common::info_data_object::create(guids.data(), as_char_arrays.data(),
//                                                                  static_cast<uint32_t>(guids.size()), obj) == false)
//            {
//                return error_code::data_object_creation_failed;
//            }
//            return error_code::no_error;
//        }
//
//
//        error_code read_motion(std::shared_ptr<ros_data_objects::motion_sample> sample,
//                                                                             uint32_t &sensor_index, uint64_t &timestamp, core::data_object **obj)
//        {
//            if(sample == nullptr || obj == nullptr)
//            {
//                return error_code::invalid_handle;
//            }
//
//            std::shared_ptr<rs::file_format::ros_data_objects::motion_sample> motion =
//                std::static_pointer_cast<ros_data_objects::motion_sample>(sample);
//
//            rs::core::motion_sample motion_sample = {};
//            rs::file_format::ros_data_objects::motion_info motion_info = motion->get_info();
//            sensor_index = motion_info.device_id;
//            if(conversions::convert(motion_info.type, motion_sample.type) == false)
//            {
//                return error_code::invalid_handle;
//            }
//            motion_sample.timestamp = motion_info.timestamp.count() * 1000;
//            motion_sample.frame_number = motion_info.frame_number;
//            motion_sample.data[0] = motion_info.data[0];
//            motion_sample.data[1] = motion_info.data[1];
//            motion_sample.data[2] = motion_info.data[2];
//
//            timestamp = motion_info.capture_time.count() / 1000;
//
//            if(rs::data_objects::motion::motion_data_object::create(motion_sample, obj) == false)
//            {
//                return error_code::data_object_creation_failed;
//            }
//            return error_code::no_error;
//        }
//
//        bool copy_image_metadata(const std::map<rs::file_format::file_types::metadata_type, std::vector<uint8_t>> source,
//                                                       rs::core::metadata_interface* target)
//        {
//            if(target == nullptr)
//            {
//                LOG_WARN("failed to create metadata");
//                return true;
//            }
//            for(auto data : source)
//            {
//                rs::core::metadata_type type = static_cast<rs::core::metadata_type>(data.first);
//                std::vector<uint8_t> buffer = data.second;
//                auto retval = target->add_metadata(type, buffer.data(), static_cast<uint32_t>(buffer.size()));
//                if(retval != rs::core::status::status_no_error)
//                {
//                    LOG_ERROR("failed to read metadata");
//                    continue;
//                }
//            }
//
//            return true;
//        }
//
//
//        bool copy_image_metadata(const rs::core::metadata_interface* source,
//                                                       std::map<rs::file_format::file_types::metadata_type, std::vector<uint8_t>>& target)
//        {
//            if (source == nullptr)
//            {
//                LOG_ERROR("failed to create metadata");
//                return false;
//            }
//            for (int i = 0;; i++)
//            {
//                metadata_type type;
//                if (source->query_available_metadata_type(i, type) == false)
//                {
//                    break;
//                }
//
//                uint32_t size = source->query_buffer_size(type);
//                if (size == 0)
//                {
//                    LOG_ERROR("failed to query frame metadata size");
//                    continue;
//                }
//
//                std::vector<uint8_t> buffer(size);
//                if(source->get_metadata(type, buffer.data()) != size)
//                {
//                    LOG_ERROR("failed to query frame metadata");
//                    continue;
//                }
//                auto ros_type = static_cast<rs::file_format::file_types::metadata_type>(type);
//                buffer.swap(target[ros_type]);
//            }
//
//            return true;
//        }
//
//
//        error_code read_property(std::shared_ptr<ros_data_objects::property> sample,
//                                                                               uint32_t &sensor_index, uint64_t &timestamp,
//                                                                               core::data_object **obj)
//        {
//            if(sample == nullptr || obj == nullptr)
//            {
//                return error_code::invalid_handle;
//            }
//
//            auto property_sample = std::static_pointer_cast<ros_data_objects::property>(sample);
//            sensor_index = property_sample->get_info().device_id;
//            timestamp = property_sample->get_info().capture_time.count() / 1000;
//
//            rs::core::guid id;
//            if(rs::utils::string_to_guid(property_sample->get_info().key, id) == false)
//            {
//                return error_code::read_failed;
//            }
//            double value;
//            value = property_sample->get_info().value;
//
//            if(rs::data_objects::common::properties_data_object::create(&id, &value, 1,
//                                                                        obj) == false)
//            {
//                return error_code::data_object_creation_failed;
//            }
//            if(m_propertiesper_sensor.find(sensor_index) == m_propertiesper_sensor.end())
//            {
//                auto pair = std::pair<uint32_t, std::vector<core::guid>>(sensor_index, {id});
//                m_propertiesper_sensor.insert(pair);
//            }else
//            {
//                auto it = m_propertiesper_sensor.at(sensor_index);
//                if(std::find(it.begin(), it.end(), id) == it.end())
//                {
//                    m_propertiesper_sensor.at(sensor_index).push_back(id);
//                }
//            }
//            return error_code::no_error;
//
//        }
//
//        error_code read_properties_metadata(std::map<uint32_t,
//                                                                                                   std::vector<rs::utils::ref_count_ptr<rs::core::data_object>> > &properties)
//        {
//            uint64_t time_min = FIRST_FRAME_TIMESTAMP.count();
//            uint64_t timestamp_microseconds = time_min;
//            uint32_t sensor_index = 0;
//
//            m_propertiesper_sensor.clear();
//
//            while(timestamp_microseconds == time_min)
//            {
//                rs::utils::ref_count_ptr<rs::core::data_object> obj;
//                if(read(sensor_index, timestamp_microseconds, &obj) != no_error)
//                {
//                    return no_error;
//                }
//
//                if(timestamp_microseconds > time_min)
//                {
//                    continue;
//                }
//
//                rs::utils::ref_count_ptr<rs::extensions::common::properties_extension> properties_data_extension;
//                if (obj->extend_to(rs::extensions::common::properties_extension::ID(), (void**)&properties_data_extension) == false)
//                {
//                    continue;
//                }
//
//                if(properties.find(sensor_index) == properties.end())
//                {
//                    auto pair = std::pair<int32_t, std::vector<rs::utils::ref_count_ptr<rs::core::data_object>>>(sensor_index, {obj});
//                    properties.insert(pair);
//                }else
//                {
//                    properties.at(sensor_index).push_back(obj);
//                }
//            }
//            auto timestamp_us = rs::file_format::file_types::microseconds(timestamp_microseconds);;
//            auto retval = m_reader->seek_to_time(timestamp_us);
//            if (retval != rs::file_format::status_no_error)
//            {
//                return read_failed;
//            }
//
//            m_first_frame_time = timestamp_us;
//            return no_error;
//        }
//
//        error_code set_properties_state(uint64_t seek_time_us)
//        {
//            if(file_types::microseconds(seek_time_us) == m_first_frame_time || m_propertiesper_sensor.empty())
//            {
//                return no_error;
//            }
//
//            //Get all topics from all sensors
//            std::vector<std::string> topics;
//            for(auto sensor_properties : m_propertiesper_sensor)
//            {
//                for(auto guid : sensor_properties.second)
//                {
//                    std::stringstream ss;
//                    ss << guid;
//                    std::string topic = ros_data_objects::property::get_topic(ss.str(), sensor_properties.first);
//                    topics.push_back(topic);
//                }
//            }
//
//            auto prev_timestamp = FIRST_FRAME_TIMESTAMP; //TODO: query from reader once stream_playback adds get_position
//            std::vector<std::string> prev_filters{}; //TODO: query from reader once we add support for filtering for serializer
//            rs::utils::scope_guard restore_state([this, prev_filters, prev_timestamp](){
//                //At the end of this function we should return to the state we were when we entered it
//                //Seeking to beginning of file in order to set the filters back (which at this point of code has no more frames- the iterator read all data)
//                if(m_reader->seek_to_time(rs::file_format::file_types::nanoseconds(FIRST_FRAME_TIMESTAMP)) != rs::file_format::status_no_error)
//                {
//                    LOG_ERROR("Failed to seek to " << FIRST_FRAME_TIMESTAMP.count() << " while restoring serializer state");
//                    return;
//                }
//                if(m_reader->set_filter(prev_filters) != rs::file_format::status_no_error)
//                {
//                    LOG_ERROR("Failed to set filter while restoring serializer state");
//                    return;
//                }
//                if(m_reader->seek_to_time(rs::file_format::file_types::nanoseconds(prev_timestamp)) != rs::file_format::status_no_error)
//                {
//                    LOG_ERROR("Failed to seek to " << prev_timestamp.count() << " while restoring serializer state");
//                    return;
//                }
//            });
//
//
//            //Seek to beginning of the file (after the header, i.e. first frame)
//            auto retval = m_reader->seek_to_time(m_first_frame_time);
//            if(retval != rs::file_format::status_no_error)
//            {
//                return read_failed;
//            }
//            //Add the properties topics as filters
//            if(m_reader->set_filter(topics) != rs::file_format::status_no_error)
//            {
//                return read_failed;
//            }
//
//            //remove all properties we read so far (will be updated at end of this function)
//            //TODO: restore them in case of failure?
//            m_cached_properties.clear();
//            std::vector<cached_property> properties;
//
//            //Get all properties up to time seek_time
//            uint64_t timestamp = m_first_frame_time.count();
//            while(timestamp < seek_time_us)
//            {
//                uint32_t sensor_index = 0;
//                //read_sample should only read properties (because we set the filter the properties only)
//                rs::utils::ref_count_ptr<rs::core::data_object> obj;
//                if (read_sample(sensor_index, timestamp, &obj) != no_error)
//                {
//                    break;
//                }
//
//                if (timestamp >= seek_time_us)
//                {
//                    continue;
//                }
//
//                rs::utils::ref_count_ptr<rs::extensions::common::properties_extension> properties_data_extension;
//                if (obj->extend_to(rs::extensions::common::properties_extension::ID(), (void**) &properties_data_extension) == false)
//                {
//                    return read_failed;
//                }
//
//                auto properties_it = std::find_if(properties.begin(), properties.end(), [&sensor_index](cached_property& i)
//                {
//                    return (i.sensor_id == sensor_index);
//                });
//
//                if (properties_it == properties.end())
//                {
//                    cached_property property = {sensor_index, timestamp, obj};
//                    properties.emplace_back(property);
//                }
//                else
//                {
//                    rs::utils::ref_count_ptr<rs::core::metadata_extension> metadata_extension;
//                    if ((*properties_it).property->extend_to(rs::core::metadata_extension::ID(), (void**) &metadata_extension) == false)
//                    {
//                        return read_failed;
//                    }
//                    if (metadata_extension->update(obj) == false)
//                    {
//                        (*properties_it).property = obj;
//                    }
//                    //update to latest property timestamp
//                    (*properties_it).timestamp = timestamp;
//                }
//            }
//            m_cached_properties = properties;
//            return no_error;
//        }
//
//        error_code read_stream_info(std::map<uint32_t, std::vector<rs::utils::ref_count_ptr<rs::core::data_object>> > &stream_infos,
//                                                                                  std::map<uint32_t, std::pair<rs::core::extrinsics, uint64_t>>& extrinsics)
//        {
//            for(uint32_t sensor_index = 0; sensor_index < m_sensor_count; ++sensor_index)
//            {
//                std::vector<std::shared_ptr<ros_data_objects::stream_info>> stream_info;
//                auto ros_retval = m_reader->read_stream_infos(stream_info,
//                                                              file_types::sample_type::st_image, sensor_index);
//                if (ros_retval != rs::file_format::status::status_no_error)
//                {
//                    continue;
//                }
//
//                std::vector<rs::sensors::camera::streaming_profile> profiles;
//                std::vector<rs::core::intrinsics> intrinsics_info;
//                std::vector<rs::core::stream_type> streams;
//                for(auto item : stream_info)
//                {
//                    std::shared_ptr<rs::file_format::ros_data_objects::image_stream_info> image_item =
//                        std::static_pointer_cast<ros_data_objects::image_stream_info>(item);
//                    rs::file_format::ros_data_objects::image_stream_data info = image_item->get_info();
//                    rs::sensors::camera::streaming_profile profile = {};
//                    profile.fps = info.fps;
//                    profile.height = info.height;
//                    profile.width = info.width;
//                    if(conversions::convert(info.format, profile.format) == false)
//                    {
//                        return error_code::invalid_handle;
//                    }
//                    if(conversions::convert(info.type, profile.stream) == false)
//                    {
//                        return error_code::invalid_handle;
//                    }
//                    rs::core::intrinsics intrinsics_item = {};
//                    if(conversions::convert(info.intrinsics, intrinsics_item) == false)
//                    {
//                        return error_code::invalid_handle;
//                    }
//                    intrinsics_item.height = info.height;
//                    intrinsics_item.width = info.width;
//                    profiles.push_back(profile);
//                    intrinsics_info.push_back(intrinsics_item);
//                    streams.push_back(profile.stream);
//
//                    if(extrinsics.find(sensor_index) == extrinsics.end())
//                    {
//                        std::pair<rs::core::extrinsics, uint64_t> extrinsics_pair;
//                        extrinsics_pair.first = conversions::convert(info.stream_extrinsics.extrinsics_data);
//                        extrinsics_pair.second = info.stream_extrinsics.reference_point_id;
//                        extrinsics[sensor_index] = extrinsics_pair;
//                    }
//                }
//                rs::utils::ref_count_ptr<rs::core::data_object> obj_profile_info;
//                if(rs::data_objects::camera::streaming_data_object::create(profiles.data(),
//                                                                           static_cast<uint32_t>(profiles.size()), &obj_profile_info) == false)
//                {
//                    return error_code::data_object_creation_failed;
//                }
//
//                stream_infos[sensor_index].push_back(obj_profile_info);
//
//                //If there is any extrinsic not empty, then it's a valid extension otherwise it is default value and is not a device metadata
//                if(std::any_of(intrinsics_info.begin(), intrinsics_info.end(), [&](const rs::core::intrinsics& i) { return is_empty_intrinsics(i) == false; }))
//                {
//                    rs::utils::ref_count_ptr<rs::core::data_object> obj_intrinsics;
//                    if (rs::data_objects::camera::camera_intrinsics_data_object::create(streams.data(), intrinsics_info.data(),
//                                                                                        static_cast<uint32_t>(intrinsics_info
//                                                                                            .size()), &obj_intrinsics) == false)
//                    {
//                        return error_code::data_object_creation_failed;
//                    }
//                    stream_infos[sensor_index].push_back(obj_intrinsics);
//
//                    rs::utils::ref_count_ptr<rs::core::data_object> obj_intrinsics_per_profile;
//                    if(rs::data_objects::camera::camera_intrinsics_per_stream_data_object::create(profiles.data(), intrinsics_info.data(),
//                                                                                                  static_cast<uint32_t>(intrinsics_info.size()), &obj_intrinsics_per_profile) == false)
//                    {
//                        return error_code::data_object_creation_failed;
//                    }
//                    stream_infos[sensor_index].push_back(obj_intrinsics_per_profile);
//                }
//            }
//            return error_code::no_error;
//        }
//
//        error_code read_motion_info(std::map<uint32_t, rs::utils::ref_count_ptr<rs::core::data_object>> &infos,
//                                                                                  std::map<uint32_t, std::pair<rs::core::extrinsics, uint64_t>>& extrinsics)
//        {
//            for(uint32_t sensor_index = 0; sensor_index < m_sensor_count; ++sensor_index)
//            {
//                std::vector<std::shared_ptr<ros_data_objects::stream_info>> stream_info;
//                auto ros_retval = m_reader->read_stream_infos(stream_info,
//                                                              rs::file_format::file_types::sample_type::st_motion, sensor_index);
//                if (ros_retval != rs::file_format::status::status_no_error)
//                {
//                    continue;
//                }
//
//                std::vector<rs::sensors::motion::motion_profile> profiles;
//                std::map<rs::core::motion_type, rs::core::motion_device_intrinsics> motion_intrinsics;
//                rs::utils::ref_count_ptr<rs::core::data_object> obj;
//                for(auto item : stream_info)
//                {
//                    std::shared_ptr<rs::file_format::ros_data_objects::motion_stream_info> motion_item =
//                        std::static_pointer_cast<ros_data_objects::motion_stream_info>(item);
//                    rs::file_format::ros_data_objects::motion_stream_data info = motion_item->get_info();
//                    rs::sensors::motion::motion_profile profile = {};
//                    profile.fps = info.fps;
//                    if(conversions::convert(info.type, profile.type) == false)
//                    {
//                        return error_code::invalid_handle;
//                    }
//                    rs::core::motion_device_intrinsics intrinsics = conversions::convert(info.intrinsics);
//                    motion_intrinsics[profile.type] = intrinsics;
//                    if(extrinsics.find(sensor_index) == extrinsics.end())
//                    {
//                        std::pair<rs::core::extrinsics, uint64_t> extrinsics_pair;
//                        extrinsics_pair.first = conversions::convert(info.stream_extrinsics.extrinsics_data);
//                        extrinsics_pair.second = info.stream_extrinsics.reference_point_id;
//                        extrinsics[sensor_index] = extrinsics_pair;
//
//                    }
//
//                    profiles.push_back(profile);
//                }
//
//                std::vector<rs::core::motion_type>  supported_intrinsics_types;
//                std::vector<rs::core::motion_device_intrinsics> intrinsics_values;
//                for (auto&& intrinsic : motion_intrinsics)
//                {
//                    supported_intrinsics_types.push_back(intrinsic.first);
//                    intrinsics_values.push_back(intrinsic.second);
//                }
//
//
//                if(rs::data_objects::motion::motion_config_data_object::create(profiles.data(),
//                                                                               static_cast<uint32_t>(profiles.size()),
//                                                                               supported_intrinsics_types.data(),
//                                                                               intrinsics_values.data(),
//                                                                               static_cast<uint32_t>(intrinsics_values.size()),
//                                                                               &obj) == false)
//                {
//                    return error_code::data_object_creation_failed;
//                }
//                infos[sensor_index] = obj;
//            }
//            return error_code::no_error;
//
//        }
//

        std::shared_ptr<info_snapshot> ros_reader::read_info_snapshot(uint32_t id)
        {
            std::vector<std::shared_ptr<rs::file_format::ros_data_objects::vendor_data>> vendor_data;
            if (m_stream_playback.read_vendor_data(vendor_data, id) != rs::file_format::status_no_error)
            {
                return nullptr;
            }
            std::map<rs2_camera_info, std::string> values;
            for (auto data : vendor_data)
            {
                try
                {
                    rs2_camera_info info = rs2_camera_info_from_string(data->get_info().name);
                    values[info] = data->get_info().value;
                }
                catch (const std::exception& e)
                {
                    std::cerr << e.what() << std::endl;
                }
            }
            return std::make_shared<info_snapshot>(values);
        }

        device_extrinsics ros_reader::read_device_extrinsics()
        {
            return {};
        }

        uint32_t ros_reader::read_sensor_count() const
        {
            std::vector<std::shared_ptr<rs::file_format::ros_data_objects::vendor_data>> vendor_data;
            auto sts = m_stream_playback.read_vendor_data(vendor_data, DEVICE_INDEX);
            if (sts == rs::file_format::status_item_unavailable)
            {
                return 0;
            }

            for (auto data : vendor_data)
            {
                if (data->get_info().name == SENSOR_COUNT)
                {
                    auto value = data->get_info().value;
                    return std::stoi(value);
                }
            }
            return 0;
        }

        std::vector<stream_profile> ros_reader::read_stream_info()
        {
            std::vector<stream_profile> profiles;
            return profiles;
        }

        std::shared_ptr<options_container> ros_reader::read_options()
        {
            return nullptr;
        }
        device_snapshot m_device_description;
        rs::file_format::file_types::microseconds m_first_frame_time;
        //std::vector <cached_property> m_cached_properties;
        //std::map <uint32_t, std::vector<rs::core::guid>> m_propertiesper_sensor;
        std::mutex m_mutex;
        uint32_t m_sensor_count;
        rs::file_format::stream_playback m_stream_playback;
    };


}
