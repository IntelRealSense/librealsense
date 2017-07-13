// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <memory>
#include <string>

#include "core/serialization.h"
#include "ros/file_types.h"
#include "core/debug.h"
#include "archive.h"
#include "ros/data_objects/vendor_data.h"
#include "ros/data_objects/image.h"
#include "ros/data_objects/stream_data.h"
#include "ros/ros_writer.h"

namespace librealsense
{
    class ros_device_serializer_writer: public device_serializer::writer
    {
        //TODO: Ziv, move to better location
        uint32_t DEVICE_INDEX = (std::numeric_limits<uint32_t>::max)(); //braces are for windows compilation
        std::string SENSOR_COUNT { "sensor_count" };
        rs::file_format::file_types::microseconds FIRST_FRAME_TIMESTAMP { 0 };
    public:
        ros_device_serializer_writer(const std::string& file) :
            m_file_path(file),
            m_writer(new rs::file_format::ros_writer(file))
        {
            internal_reset(false);
        }

        void write_device_description(const librealsense::device_snapshot& device_description) override
        {
            for (auto&& device_extension_snapshot : device_description.get_device_extensions_snapshots().get_snapshots())
            {
                write_extension_snapshot(DEVICE_INDEX, device_extension_snapshot.first, device_extension_snapshot.second);
            }

            uint32_t sensor_index = 0;
            for (auto&& sensors_snapshot : device_description.get_sensors_snapshots())
            {
                for (auto&& sensor_extension_snapshot : sensors_snapshot.get_sensor_extensions_snapshots().get_snapshots())
                {
                    write_extension_snapshot(sensor_index, sensor_extension_snapshot.first, sensor_extension_snapshot.second);
                }
            }
            write_sensor_count(device_description.get_sensors_snapshots().size());
        }

        void reset() override
        {
			internal_reset(true);
        }

        void write(std::chrono::nanoseconds timestamp, uint32_t sensor_index, librealsense::frame_holder&& frame) override
        {
            auto timestamp_ns = rs::file_format::file_types::nanoseconds(timestamp);


            if (Is<video_frame>(frame.frame->get()))
            {
                auto image_obj = std::make_shared<rs::file_format::ros_data_objects::image>(timestamp, sensor_index, std::move(frame));
                record(image_obj);
                return;
            }

//            if (Is<motion_frame>(data.frame.get()))
//            {
//                write_motion(data);
//                return;
//            }
//
//            if (Is<pose_frame>(data.frame.get()))
//            {
//                write_pose(data);
//                return;
//            }
        }

        void write(const librealsense::device_serializer::snapshot_box& data) override
        {
            if (data.snapshot == nullptr)
            {
                throw invalid_value_exception("null frame");
            }
            if(Is<info_interface>(data.snapshot))
            {
                write_vendor_info(data);
                return;
            }
            if(Is<options_interface>(data.snapshot))
            {
                //write_property(data);
                return;
            }
            if(Is<debug_interface>(data.snapshot))
            {
                write_debug_info(data);
                return;
            }
        }

    private:
		void internal_reset(bool recreate_device)
		{
			if (recreate_device)
			{
				m_writer.reset();
                m_writer.reset(new rs::file_format::ros_writer(m_file_path));
			}
			m_first_frame_time = FIRST_FRAME_TIMESTAMP;
		}

//        error_code write_device_description(core::device_description_interface *description)
//        {
//            if (description == nullptr)
//            {
//                return error_code::invalid_handle;
//            }
//
//            auto retval = write_device_metadata(description);
//            if(retval != error_code::no_error)
//            {
//                return retval;
//            }
//
//            uint32_t sensor_description_count = description->sensor_description_count();
//            for (uint32_t sensor_index = 0; sensor_index<sensor_description_count; sensor_index++)
//            {
//                rs::utils::ref_count_ptr<rs::core::sensor_description_interface> sensor;
//                if(description->query_sensor_description(sensor_index, &sensor) == false)
//                {
//                    return write_failed;
//                }
//
//                auto retval = write_sensor_metadata(sensor, sensor_index, description);
//                if(retval != error_code::no_error)
//                {
//                    return retval;
//                }
//            }
//
//            write_sensor_count(sensor_description_count);
//
//            return error_code::no_error;
//        }

        void write_extension_snapshot(uint32_t id, rs2_extension_type type, std::shared_ptr<librealsense::extension_snapshot> snapshot)
        {    
            switch (type)
            {
            case RS2_EXTENSION_TYPE_UNKNOWN:
                throw invalid_value_exception("Unknown extension");
            case RS2_EXTENSION_TYPE_DEBUG:
                break;
            case RS2_EXTENSION_TYPE_INFO:
                break;
            case RS2_EXTENSION_TYPE_MOTION:
                break;
            case RS2_EXTENSION_TYPE_OPTIONS:
                break;
            case RS2_EXTENSION_TYPE_VIDEO:
                break;
            case RS2_EXTENSION_TYPE_ROI:
                break;
            case RS2_EXTENSION_TYPE_VIDEO_FRAME:
                break;
            case RS2_EXTENSION_TYPE_MOTION_FRAME:
                break;
            case RS2_EXTENSION_TYPE_COUNT:
                break;
            default:
                throw invalid_value_exception("Unsupported extension");

            }
            if (Is<librealsense::info_interface>(snapshot))
            {
                //std::cout << "Remove me !!! info_interface " << id << " : " << snapshot.get() << std::endl;
                //write_vendor_info(snapshot, id);
                return;
            }
            if (Is<librealsense::options_interface>(snapshot))
            {
                //std::cout << "Remove me !!! options_interface " << id << " : " << snapshot.get() << std::endl;
                //write_vendor_info(snapshot, id);
                return;
            }
            if (Is<librealsense::debug_interface>(snapshot))
            {
                //std::cout << "Remove me !!! debug_interface " << id << " : " << snapshot.get() << std::endl;
                //auto timestamp_ns = rs::file_format::file_types::nanoseconds(FIRST_FRAME_TIMESTAMP);
                //write_property(snapshot, id, timestamp_ns);
                return;
            }
        }
        void write_debug_info(const device_serializer::snapshot_box& box)
        {

        }
            
        void write_vendor_info(const device_serializer::snapshot_box& box)
        {
            info_interface* info = dynamic_cast<info_interface*>(box.snapshot.get());
            assert(info != nullptr);
            for (uint32_t i = 0; i < static_cast<uint32_t>(RS2_CAMERA_INFO_COUNT); i++)
            {
                auto camera_info = static_cast<rs2_camera_info>(i);
                if(!info->supports_info(camera_info))
                {
                    continue;
                }

                rs::file_format::ros_data_objects::vendor_info vendor_info = {};
                vendor_info.device_id = box.sensor_index;
                vendor_info.name =  rs2_camera_info_to_string(camera_info);
                vendor_info.value = info->get_info(camera_info);

                auto msg = std::make_shared<rs::file_format::ros_data_objects::vendor_data>(vendor_info);
                record(msg);
            }
        }

        void write_sensor_count(size_t sensor_count)
        {
            rs::file_format::ros_data_objects::vendor_info vendor_info = {};
            vendor_info.device_id = DEVICE_INDEX;
            vendor_info.name = SENSOR_COUNT;
            vendor_info.value = std::to_string(sensor_count);

            auto msg = std::make_shared<rs::file_format::ros_data_objects::vendor_data>(vendor_info);
            record(msg);
        }

//        error_code write_property(rs::extensions::common::properties_extension* pinfo,
//                                                                                uint32_t sensor_index,
//                                                                                rs::file_format::file_types::nanoseconds timestamp)
//        {
//            if(pinfo == nullptr)
//            {
//                return error_code::invalid_handle;
//            }
//
//            rs::utils::ref_count_ptr<rs::extensions::common::properties_extension> info_ref = pinfo;
//
//            for (uint32_t i = 0; ; i++)
//            {
//                rs::core::guid id;
//                double value;
//                if (info_ref->query_property(i, id, value) == false)
//                    break;
//
//                rs::file_format::ros_data_objects::property_info property_info = {};
//                property_info.device_id = sensor_index;
//                std::stringstream ss;
//                ss << id;
//                property_info.key = ss.str();
//                property_info.value = value;
//                property_info.capture_time = timestamp;
//                std::shared_ptr<rs::file_format::ros_data_objects::property> msg =
//                    ros_data_objects::property::create(property_info);
//                if(msg == nullptr)
//                {
//                    return error_code::write_failed;
//                }
//
//                if(m_stream_recorder.record(msg) != rs::file_format::status_no_error)
//                {
//                    return error_code::write_failed;
//                }
//            }
//
//            return error_code::no_error;
//        }
//
//        error_code write_stream_info(sensors::camera::streaming_extension* p_stream_info,
//                                                                                   sensors::camera::camera_intrinsics_extension* p_intrinsics_info,
//                                                                                   rs::extensions::common::extrinsics_extension* p_extrinsics_info,
//                                                                                   uint32_t sensor_index)
//        {
//            if(p_stream_info == nullptr)
//            {
//                return error_code::invalid_handle;
//            }
//
//            rs::utils::ref_count_ptr<sensors::camera::streaming_extension> stream_info_ref = p_stream_info;
//            rs::utils::ref_count_ptr<sensors::camera::camera_intrinsics_extension> intrinsics_info_ref = p_intrinsics_info;
//            rs::utils::ref_count_ptr<rs::extensions::common::extrinsics_extension> extrinsics_info_ref = p_extrinsics_info;
//
//            for(uint32_t i = 0; i < stream_info_ref->get_profile_count(); ++i)
//            {
//                rs::file_format::ros_data_objects::image_stream_data image_stream_info = {};
//                rs::sensors::camera::streaming_profile profile;
//                if(stream_info_ref == nullptr ||
//                    stream_info_ref->query_active_profile(i, profile) == false)
//                {
//                    continue;
//                }
//                image_stream_info.device_id = sensor_index;
//                image_stream_info.fps = profile.fps;
//                image_stream_info.height = profile.height;
//                image_stream_info.width = profile.width;
//
//                if(conversions::convert(profile.stream, image_stream_info.type) == false)
//                {
//                    return error_code::invalid_handle;
//                }
//
//                if(conversions::convert(profile.format, image_stream_info.format) == false)
//                {
//                    return error_code::invalid_handle;
//                }
//
//                if(extrinsics_info_ref != nullptr)
//                {
//                    rs::core::extrinsics extrinsics;
//                    uint64_t ref_point = 0;
//                    extrinsics_info_ref->get_extrinsics(sensor_index, extrinsics, ref_point);
//                    image_stream_info.stream_extrinsics.extrinsics_data = conversions::convert(extrinsics);
//                    image_stream_info.stream_extrinsics.reference_point_id = ref_point;
//                }
//
//                //set model to None instead of empty string to allow empty intrinsics
//                image_stream_info.intrinsics.model = rs::file_format::file_types::distortion::DISTORTION_NONE;
//                rs::core::intrinsics intrinsics;
//                if (intrinsics_info_ref != nullptr &&
//                    (intrinsics_info_ref->get_intrinsics(profile.stream, intrinsics)) == true)
//                {
//                    if(conversions::convert(intrinsics, image_stream_info.intrinsics) == false)
//                    {
//                        return error_code::invalid_handle;
//                    }
//                }
//
//                std::shared_ptr<rs::file_format::ros_data_objects::image_stream_info> msg =
//                    ros_data_objects::image_stream_info::create(image_stream_info);
//                if(msg == nullptr)
//                {
//                    return error_code::write_failed;
//                }
//
//                if(m_stream_recorder.record(msg) != rs::file_format::status_no_error)
//                {
//                    return error_code::write_failed;
//                }
//
//            }
//            return error_code::no_error;
//        }
//
//        error_code write_motion_info(rs::sensors::motion::motion_extension* p_motion_info,
//                                                                                   rs::extensions::common::extrinsics_extension* p_extrinsics_info,
//                                                                                   uint32_t sensor_index)
//        {
//            if(p_motion_info == nullptr)
//            {
//                return error_code::invalid_handle;
//            }
//            rs::utils::ref_count_ptr<rs::sensors::motion::motion_extension> motion_info_ref = p_motion_info;
//            rs::utils::ref_count_ptr<rs::extensions::common::extrinsics_extension> extrinsics_info_ref = p_extrinsics_info;
//
//            for(uint32_t i = 0;; ++i)
//            {
//                sensors::motion::motion_profile profile;
//                rs::file_format::ros_data_objects::motion_stream_data motion_stream_info = {};
//                if(motion_info_ref->query_active_profile(i, profile) == false)
//                {
//                    return error_code::no_error;
//                }
//                motion_stream_info.device_id = sensor_index;
//                motion_stream_info.fps = profile.fps;
//
//                rs::file_format::file_types::motion_type type;
//                if(conversions::convert(profile.type, type) == false)
//                {
//                    return error_code::invalid_handle;
//                }
//                motion_stream_info.type = type;
//
//                rs::core::motion_device_intrinsics intrinsics;
//
//                if (motion_info_ref->get_intrinsics(profile.type, intrinsics) == true)
//                {
//                    motion_stream_info.intrinsics = conversions::convert(intrinsics);
//                }
//
//                if(extrinsics_info_ref != nullptr)
//                {
//                    rs::core::extrinsics extrinsics;
//                    uint64_t ref_point = 0;
//                    extrinsics_info_ref->get_extrinsics(sensor_index, extrinsics, ref_point);
//                    motion_stream_info.stream_extrinsics.extrinsics_data = conversions::convert(extrinsics);
//                    motion_stream_info.stream_extrinsics.reference_point_id = ref_point;
//                }
//
//                std::shared_ptr<rs::file_format::ros_data_objects::motion_stream_info> msg =
//                    ros_data_objects::motion_stream_info::create(motion_stream_info);
//                if(msg == nullptr)
//                {
//                    return error_code::write_failed;
//                }
//
//                if(m_stream_recorder.record(msg) != rs::file_format::status_no_error)
//                {
//                    return error_code::write_failed;
//                }
//            }
//            return error_code::no_error;
//        }
//
//        error_code write_motion(const core::motion_sample &sample,
//                                                                              int32_t sensor_index,
//                                                                              rs::file_format::file_types::nanoseconds timestamp)
//        {
//
//            rs::file_format::ros_data_objects::motion_info info = {};
//            info.device_id = sensor_index;
//
//            if(conversions::convert(sample.type, info.type) == false)
//            {
//                return error_code::invalid_handle;
//            }
//            info.capture_time = timestamp;
//
//            std::chrono::duration<double, std::milli> timestamp_ms(sample.timestamp);
//            info.timestamp = rs::file_format::file_types::seconds(timestamp_ms);
//
//            info.frame_number = static_cast<uint32_t>(sample.frame_number);
//            std::memcpy(info.data, sample.data, sizeof(sample.data));
//
//            std::shared_ptr<rs::file_format::ros_data_objects::motion_sample> msg =
//                ros_data_objects::motion_sample::create(info);
//            if(msg == nullptr)
//            {
//                return error_code::write_failed;
//            }
//
//            if(m_stream_recorder.record(msg) != rs::file_format::status_no_error)
//            {
//                return error_code::write_failed;
//            }
//
//            return error_code::no_error;
//        }
//
//        error_code write_pose(const core::pose &sample,
//                                                                            uint32_t sensor_index,
//                                                                            rs::file_format::file_types::nanoseconds timestamp)
//        {
//            rs::file_format::ros_data_objects::pose_info info = {};
//
//            info = conversions::convert(sample);
//
//            info.capture_time = timestamp;
//            info.device_id = sensor_index;
//
//            std::shared_ptr<rs::file_format::ros_data_objects::pose> msg =
//                ros_data_objects::pose::create(info);
//            if (msg == nullptr)
//            {
//                return error_code::write_failed;
//            }
//
//            if (m_stream_recorder.record(msg) != rs::file_format::status_no_error)
//            {
//                return error_code::write_failed;
//            }
//
//            return error_code::no_error;
//        }
//
//
//        error_code write_sensor_metadata(core::query_metadata_interface *metadata,
//                                                                                       uint32_t sensor_index, core::query_metadata_interface *device)
//        {
//            if(metadata == nullptr)
//            {
//                return error_code::invalid_handle;
//            }
//
//            rs::utils::ref_count_ptr<rs::sensors::camera::camera_intrinsics_extension> camera_intrinsics;
//            rs::utils::ref_count_ptr<rs::sensors::camera::streaming_extension> streaming_info;
//            rs::utils::ref_count_ptr<rs::extensions::common::extrinsics_extension> extrinsics_extension;
//
//            for(int j = 0; ; ++j)
//            {
//                rs::utils::ref_count_ptr<rs::core::data_object> obj;
//                if(device->query_metadata(j, &obj) == false)
//                {
//                    break;
//                }
//
//                if (obj->extend_to(rs::extensions::common::extrinsics_extension::ID(), (void**)&extrinsics_extension) == true)
//                {
//                    break;
//                }
//            }
//
//            for(int j = 0; ; ++j)
//            {
//                rs::utils::ref_count_ptr<rs::core::data_object> obj;
//                if(metadata->query_metadata(j, &obj) == false)
//                {
//                    break;
//                }
//
//                rs::utils::ref_count_ptr<rs::extensions::common::info_extension> info_extension;
//                if (obj->extend_to(rs::extensions::common::info_extension::ID(), (void**)&info_extension) == true)
//                {
//                    auto retval = write_vendor_info(info_extension, sensor_index);
//                    if(retval != error_code::no_error)
//                    {
//                        return retval;
//                    }
//                }
//                rs::utils::ref_count_ptr<rs::extensions::common::properties_extension> property_extension;
//                if (obj->extend_to(rs::extensions::common::properties_extension::ID(), (void**)&property_extension) == true)
//                {
//                    auto timestamp_ns = rs::file_format::file_types::nanoseconds(FIRST_FRAME_TIMESTAMP);
//                    auto retval = write_property(property_extension, sensor_index, timestamp_ns);
//                    if(retval != error_code::no_error)
//                    {
//                        return retval;
//                    }
//                }
//                rs::utils::ref_count_ptr<rs::sensors::camera::camera_intrinsics_extension> camera_intrinsics_extension;
//                if (obj->extend_to(rs::sensors::camera::camera_intrinsics_extension::ID(), (void**)&camera_intrinsics_extension) == true)
//                {
//                    camera_intrinsics = camera_intrinsics_extension;
//                }
//                rs::utils::ref_count_ptr<rs::sensors::camera::streaming_extension> streaming_extension;
//                if (obj->extend_to(rs::sensors::camera::streaming_extension::ID(), (void**)&streaming_extension) == true)
//                {
//                    streaming_info = streaming_extension;
//                }
//                rs::utils::ref_count_ptr<rs::sensors::motion::motion_extension> motion_extension;
//                if (obj->extend_to(rs::sensors::motion::motion_extension::ID(), (void**)&motion_extension) == true)
//                {
//                    auto retval = write_motion_info(motion_extension, extrinsics_extension, sensor_index);
//                    if(retval != error_code::no_error)
//                    {
//                        return retval;
//                    }
//                }
//            }
//            if(streaming_info != nullptr)
//            {
//                write_stream_info(streaming_info, camera_intrinsics, extrinsics_extension, sensor_index);
//            }
//            return  error_code::no_error;
//        }
//
//        error_code write_device_metadata(core::query_metadata_interface *metadata)
//        {
//            if(metadata == nullptr)
//            {
//                return error_code::invalid_handle;
//            }
//            for(int j = 0; ; ++j)
//            {
//                rs::utils::ref_count_ptr<rs::core::data_object> obj;
//                if(metadata->query_metadata(j, &obj) == false)
//                {
//                    break;
//                }
//
//                rs::utils::ref_count_ptr<rs::extensions::common::info_extension> info_extension;
//                if (obj->extend_to(rs::extensions::common::info_extension::ID(), (void**)&info_extension) == true)
//                {
//                    auto retval = write_vendor_info(info_extension, DEVICE_INDEX);
//                    if(retval != error_code::no_error)
//                    {
//                        return retval;
//                    }
//                }
//                rs::utils::ref_count_ptr<rs::extensions::common::properties_extension> property_extension;
//                if (obj->extend_to(rs::extensions::common::properties_extension::ID(), (void**)&property_extension) == true)
//                {
//                    auto timestamp_ns = rs::file_format::file_types::nanoseconds(FIRST_FRAME_TIMESTAMP);
//                    auto retval = write_property(property_extension, DEVICE_INDEX, timestamp_ns);
//                    if(retval != error_code::no_error)
//                    {
//                        return retval;
//                    }
//                }
//            }
//            return  error_code::no_error;
//        }



        /**
        * @brief Writes a stream_data object to the file
        *
        * @param[in] data          an object implements the stream_data interface
        * @return status_no_error             Successful execution
        * @return status_param_unsupported    One of the stream data feilds is not supported
        */
        void record(std::shared_ptr<rs::file_format::ros_data_objects::stream_data> data)
        {
            data->write_data(*m_writer);
        }

        rs::file_format::file_types::microseconds m_first_frame_time;
        std::unique_ptr<rs::file_format::ros_writer> m_writer;
        std::string m_file_path;
    };

}