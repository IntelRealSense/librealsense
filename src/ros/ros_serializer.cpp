//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
//
//#include <vector>
//#include <map>
//#include <mutex>
//
//
//#include "conversions.h"
//#include "ros_serializer.h"
//
//
//
//using namespace rs;
//using namespace rs::core;
//using namespace rs::storage::serializers;
//using namespace rs::file_format;
//
//
//const uint32_t DEVICE_INDEX { (std::numeric_limits<uint32_t>::max)() };
//const std::string SENSOR_COUNT { "sensor_count" };
//const rs::file_format::file_types::microseconds FIRST_FRAME_TIMESTAMP { 0 };
//
//
//bool realsense_serializer::is_empty_extrinsics(const rs::core::extrinsics& e)
//{
//    constexpr uint8_t testblock [sizeof(rs::core::extrinsics)] {};
//    memset ((void*)testblock, 0, sizeof testblock);
//
//    return !memcmp (testblock, &e, sizeof(rs::core::extrinsics));
//}
//bool realsense_serializer::create_extrinsics_object(std::map<uint32_t, std::pair<rs::core::extrinsics, uint64_t>>& extrinsics,
//                                     rs::core::data_object** extrinsics_object)
//{
//    if(extrinsics.size() == 0)
//    {
//        return false;
//    }
//
//    std::vector<uint32_t> sensors_indices;
//    std::vector<rs::core::extrinsics> extrinsics_vec;
//    std::vector<uint64_t> reference_points_ids;
//
//    for(auto extrinsics_item : extrinsics)
//    {
//        sensors_indices.push_back(extrinsics_item.first);
//        extrinsics_vec.push_back(extrinsics_item.second.first);
//        reference_points_ids.push_back(extrinsics_item.second.second);
//    }
//
//    return rs::data_objects::common::extrinsics_data_object::create(sensors_indices.data(),
//                                                             extrinsics_vec.data(),
//                                                             reference_points_ids.data(),
//                                                             static_cast<uint32_t>(sensors_indices.size()),
//                                                             extrinsics_object);
//
//}
//
//device_data_serializer::error_code realsense_serializer::read_metadata(core::device_description_interface **description)
//{
//    if(description == nullptr)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//    std::vector<rs::core::data_object*> device_objects;
//    rs::utils::scope_guard device_objects_releaser([&]()
//    {
//        for (auto object : device_objects)
//        {
//            object->release();
//        }
//    });
//
//    std::map<uint32_t, std::vector<rs::utils::ref_count_ptr<rs::core::data_object>>> properties;
//    auto retval = read_properties_metadata(properties);
//    if(retval != device_data_serializer::error_code::no_error)
//    {
//        return retval;
//    }
//
//    std::map<uint32_t, rs::utils::ref_count_ptr<rs::core::data_object>> infos;
//    retval = read_vendor_info(infos, m_sensor_count);
//    if(retval != device_data_serializer::error_code::no_error)
//    {
//        return retval;
//    }
//
//    std::map<uint32_t, std::vector<rs::utils::ref_count_ptr<rs::core::data_object>>> stream_infos;
//    std::map<uint32_t, std::pair<rs::core::extrinsics, uint64_t>> extrinsics;
//    retval = read_stream_info(stream_infos, extrinsics);
//    if(retval != device_data_serializer::error_code::no_error)
//    {
//        return retval;
//    }
//
//    std::map<uint32_t, rs::utils::ref_count_ptr<rs::core::data_object>> motion_infos;
//    retval = read_motion_info(motion_infos, extrinsics);
//    if(retval != device_data_serializer::error_code::no_error)
//    {
//        return retval;
//    }
//
//    std::vector<rs::core::sensor_description_interface*> sensor_descriptions;
//    rs::utils::scope_guard sensor_descriptions_releaser([&]()
//    {
//        for (auto sensor_description : sensor_descriptions)
//        {
//            sensor_description->release();
//        }
//    });
//
//    for (uint32_t sensor_index = 0; sensor_index < m_sensor_count; sensor_index++)
//    {
//        std::vector<rs::core::data_object*> sensor_objects;
//        rs::utils::scope_guard sensor_objects_releaser([&]()
//        {
//            for (auto object : sensor_objects)
//            {
//                object->release();
//            }
//        });
//
//        if(properties.find(sensor_index) != properties.end())
//        {
//            std::for_each(properties.at(sensor_index).begin(),
//                          properties.at(sensor_index).end(),
//                          [](rs::utils::ref_count_ptr<rs::core::data_object> &obj){ obj->add_ref(); });
//            sensor_objects.insert(sensor_objects.end(), properties.at(sensor_index).begin(), properties.at(sensor_index).end());
//        }
//        if(infos.find(sensor_index) != infos.end())
//        {
//            infos.at(sensor_index)->add_ref();
//            sensor_objects.push_back(infos.at(sensor_index));
//        }
//        if(stream_infos.find(sensor_index) != stream_infos.end())
//        {
//            std::for_each(stream_infos.at(sensor_index).begin(),
//                          stream_infos.at(sensor_index).end(),
//                          [](rs::utils::ref_count_ptr<rs::core::data_object> &obj){ obj->add_ref(); });
//            sensor_objects.insert(sensor_objects.end(), stream_infos.at(sensor_index).begin(),
//                                  stream_infos.at(sensor_index).end());
//        }
//        if(motion_infos.find(sensor_index) != motion_infos.end())
//        {
//            motion_infos.at(sensor_index)->add_ref();
//            sensor_objects.push_back(motion_infos.at(sensor_index));
//        }
//
//        rs::utils::ref_count_ptr<rs::core::sensor_description_interface> sensor_description = nullptr;
//        if (rs::storage::common::sensor_description::create(
//                    sensor_objects.data(),
//                    static_cast<uint32_t>(sensor_objects.size()),
//                    &sensor_description) == false)
//            return device_data_serializer::error_code::read_failed;;
//
//        sensor_description->add_ref();
//        sensor_descriptions.emplace_back(sensor_description);
//    }
//    uint32_t device_id = DEVICE_INDEX;
//    if(properties.find(device_id) != properties.end())
//    {
//        std::for_each(properties.at(device_id).begin(),
//                      properties.at(device_id).end(),
//                      [](rs::utils::ref_count_ptr<rs::core::data_object> &obj){ obj->add_ref(); });
//        device_objects.insert(device_objects.end(), properties.at(device_id).begin(), properties.at(device_id).end());
//    }
//
//    if(infos.find(device_id) != infos.end())
//    {
//        infos.at(device_id)->add_ref();
//        device_objects.push_back(infos.at(device_id));
//    }
//
//
//    //If there is any extrinsic not empty, then it's a valid extension otherwise it is default value and is not a device metadata
//    if(std::any_of(extrinsics.begin(), extrinsics.end(), [](const std::pair<uint32_t, std::pair<rs::core::extrinsics, uint64_t>>& e) { return is_empty_extrinsics(e.second.first) == false; }))
//    {
//        rs::utils::ref_count_ptr<rs::core::data_object> extrinsics_data_object;
//        if (create_extrinsics_object(extrinsics, &extrinsics_data_object) == true)
//        {
//            extrinsics_data_object->add_ref();
//            device_objects.push_back(extrinsics_data_object);
//        }
//        else
//        {
//            LOG_ERROR("falied to create extrinsics object");
//        }
//    }
//    if (rs::storage::common::device_description::create(
//                device_objects.data(),
//                static_cast<uint32_t>(device_objects.size()),
//                sensor_descriptions.data(),
//                static_cast<uint32_t>(sensor_descriptions.size()),
//                description) == false)
//    {
//        return device_data_serializer::error_code::read_failed;
//    }
//
//    return device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::write_vendor_info(rs::extensions::common::info_extension* pinfo,
//                                                                           uint32_t sensor_index)
//{
//    if(pinfo == nullptr)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//
//    const rs::utils::ref_count_ptr<rs::extensions::common::info_extension> info_ref = pinfo;
//    for (uint32_t i = 0; ; i++)
//    {
//        rs::core::guid id;
//        const char* info;
//        if ((info = info_ref->query_info(i, id)) == nullptr)
//        {
//            break;
//        }
//
//        rs::file_format::ros_data_objects::vendor_info vendor_info = {};
//        vendor_info.device_id = sensor_index;
//        std::stringstream ss;
//        ss << id;
//        vendor_info.name = ss.str();
//        vendor_info.value = info;
//
//        std::shared_ptr<rs::file_format::ros_data_objects::vendor_data> msg =
//                ros_data_objects::vendor_data::create(vendor_info);
//
//        if(msg == nullptr)
//        {
//            return device_data_serializer::error_code::write_failed;
//        }
//
//        if(m_writer->record(msg) != rs::file_format::status_no_error)
//        {
//            return device_data_serializer::error_code::write_failed;
//        }
//
//    }
//    return device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::write_sensor_count(uint32_t sensor_count)
//{
//    rs::file_format::ros_data_objects::vendor_info vendor_info = {};
//    vendor_info.device_id = DEVICE_INDEX;
//    vendor_info.name = SENSOR_COUNT;
//    vendor_info.value = std::to_string(sensor_count);
//
//    std::shared_ptr<rs::file_format::ros_data_objects::vendor_data> msg =
//            ros_data_objects::vendor_data::create(vendor_info);
//    if(msg == nullptr)
//    {
//        return device_data_serializer::error_code::write_failed;
//    }
//
//    if(m_writer->record(msg) != rs::file_format::status_no_error)
//    {
//        return device_data_serializer::error_code::write_failed;
//    }
//
//    return device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::write_property(rs::extensions::common::properties_extension* pinfo,
//                                                                        uint32_t sensor_index,
//																		rs::file_format::file_types::nanoseconds timestamp)
//{
//    if(pinfo == nullptr)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//
//    rs::utils::ref_count_ptr<rs::extensions::common::properties_extension> info_ref = pinfo;
//
//    for (uint32_t i = 0; ; i++)
//    {
//        rs::core::guid id;
//        double value;
//        if (info_ref->query_property(i, id, value) == false)
//            break;
//
//        rs::file_format::ros_data_objects::property_info property_info = {};
//        property_info.device_id = sensor_index;
//        std::stringstream ss;
//        ss << id;
//        property_info.key = ss.str();
//        property_info.value = value;
//        property_info.capture_time = timestamp;
//        std::shared_ptr<rs::file_format::ros_data_objects::property> msg =
//                ros_data_objects::property::create(property_info);
//        if(msg == nullptr)
//        {
//            return device_data_serializer::error_code::write_failed;
//        }
//
//        if(m_writer->record(msg) != rs::file_format::status_no_error)
//        {
//            return device_data_serializer::error_code::write_failed;
//        }
//    }
//
//    return device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::write_stream_info(sensors::camera::streaming_extension* p_stream_info,
//                                                                           sensors::camera::camera_intrinsics_extension* p_intrinsics_info,
//                                                                           rs::extensions::common::extrinsics_extension* p_extrinsics_info,
//                                                                           uint32_t sensor_index)
//{
//    if(p_stream_info == nullptr)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//
//    rs::utils::ref_count_ptr<sensors::camera::streaming_extension> stream_info_ref = p_stream_info;
//    rs::utils::ref_count_ptr<sensors::camera::camera_intrinsics_extension> intrinsics_info_ref = p_intrinsics_info;
//    rs::utils::ref_count_ptr<rs::extensions::common::extrinsics_extension> extrinsics_info_ref = p_extrinsics_info;
//
//    for(uint32_t i = 0; i < stream_info_ref->get_profile_count(); ++i)
//    {
//        rs::file_format::ros_data_objects::image_stream_data image_stream_info = {};
//        rs::sensors::camera::streaming_profile profile;
//        if(stream_info_ref == nullptr ||
//                stream_info_ref->query_active_profile(i, profile) == false)
//        {
//            continue;
//        }
//        image_stream_info.device_id = sensor_index;
//        image_stream_info.fps = profile.fps;
//        image_stream_info.height = profile.height;
//        image_stream_info.width = profile.width;
//
//        if(conversions::convert(profile.stream, image_stream_info.type) == false)
//        {
//            return error_code::invalid_handle;
//        }
//
//        if(conversions::convert(profile.format, image_stream_info.format) == false)
//        {
//            return error_code::invalid_handle;
//        }
//
//        if(extrinsics_info_ref != nullptr)
//        {
//            rs::core::extrinsics extrinsics;
//            uint64_t ref_point = 0;
//            extrinsics_info_ref->get_extrinsics(sensor_index, extrinsics, ref_point);
//			image_stream_info.stream_extrinsics.extrinsics_data = conversions::convert(extrinsics);
//            image_stream_info.stream_extrinsics.reference_point_id = ref_point;
//        }
//
//        //set model to None instead of empty string to allow empty intrinsics
//        image_stream_info.intrinsics.model = rs::file_format::file_types::distortion::DISTORTION_NONE;
//        rs::core::intrinsics intrinsics;
//        if (intrinsics_info_ref != nullptr &&
//                (intrinsics_info_ref->get_intrinsics(profile.stream, intrinsics)) == true)
//        {
//            if(conversions::convert(intrinsics, image_stream_info.intrinsics) == false)
//            {
//                return error_code::invalid_handle;
//            }
//        }
//
//        std::shared_ptr<rs::file_format::ros_data_objects::image_stream_info> msg =
//                ros_data_objects::image_stream_info::create(image_stream_info);
//        if(msg == nullptr)
//        {
//            return device_data_serializer::error_code::write_failed;
//        }
//
//        if(m_writer->record(msg) != rs::file_format::status_no_error)
//        {
//            return device_data_serializer::error_code::write_failed;
//        }
//
//    }
//    return device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::write_motion_info(rs::sensors::motion::motion_extension* p_motion_info,
//                                                                           rs::extensions::common::extrinsics_extension* p_extrinsics_info,
//                                                                           uint32_t sensor_index)
//{
//    if(p_motion_info == nullptr)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//    rs::utils::ref_count_ptr<rs::sensors::motion::motion_extension> motion_info_ref = p_motion_info;
//    rs::utils::ref_count_ptr<rs::extensions::common::extrinsics_extension> extrinsics_info_ref = p_extrinsics_info;
//
//    for(uint32_t i = 0;; ++i)
//    {
//        sensors::motion::motion_profile profile;
//        rs::file_format::ros_data_objects::motion_stream_data motion_stream_info = {};
//        if(motion_info_ref->query_active_profile(i, profile) == false)
//        {
//            return device_data_serializer::error_code::no_error;
//        }
//        motion_stream_info.device_id = sensor_index;
//        motion_stream_info.fps = profile.fps;
//
//        rs::file_format::file_types::motion_type type;
//        if(conversions::convert(profile.type, type) == false)
//        {
//            return error_code::invalid_handle;
//        }
//        motion_stream_info.type = type;
//
//        rs::core::motion_device_intrinsics intrinsics;
//
//        if (motion_info_ref->get_intrinsics(profile.type, intrinsics) == true)
//        {
//            motion_stream_info.intrinsics = conversions::convert(intrinsics);
//        }
//
//        if(extrinsics_info_ref != nullptr)
//        {
//            rs::core::extrinsics extrinsics;
//            uint64_t ref_point = 0;
//            extrinsics_info_ref->get_extrinsics(sensor_index, extrinsics, ref_point);
//			motion_stream_info.stream_extrinsics.extrinsics_data = conversions::convert(extrinsics);
//            motion_stream_info.stream_extrinsics.reference_point_id = ref_point;
//        }
//
//        std::shared_ptr<rs::file_format::ros_data_objects::motion_stream_info> msg =
//                ros_data_objects::motion_stream_info::create(motion_stream_info);
//        if(msg == nullptr)
//        {
//            return device_data_serializer::error_code::write_failed;
//        }
//
//        if(m_writer->record(msg) != rs::file_format::status_no_error)
//        {
//            return device_data_serializer::error_code::write_failed;
//        }
//    }
//    return device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::write_motion(const core::motion_sample &sample,
//                                                                      int32_t sensor_index,
//																	  rs::file_format::file_types::nanoseconds timestamp)
//{
//
//    rs::file_format::ros_data_objects::motion_info info = {};
//    info.device_id = sensor_index;
//
//    if(conversions::convert(sample.type, info.type) == false)
//    {
//        return error_code::invalid_handle;
//    }
//    info.capture_time = timestamp;
//
//	std::chrono::duration<double, std::milli> timestamp_ms(sample.timestamp);
//	info.timestamp = rs::file_format::file_types::seconds(timestamp_ms);
//
//    info.frame_number = static_cast<uint32_t>(sample.frame_number);
//    std::memcpy(info.data, sample.data, sizeof(sample.data));
//
//    std::shared_ptr<rs::file_format::ros_data_objects::motion_sample> msg =
//            ros_data_objects::motion_sample::create(info);
//    if(msg == nullptr)
//    {
//        return device_data_serializer::error_code::write_failed;
//    }
//
//    if(m_writer->record(msg) != rs::file_format::status_no_error)
//    {
//        return device_data_serializer::error_code::write_failed;
//    }
//
//    return device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::write_pose(const core::pose &sample,
//																	uint32_t sensor_index,
//																	rs::file_format::file_types::nanoseconds timestamp)
//{
//    rs::file_format::ros_data_objects::pose_info info = {};
//
//	info = conversions::convert(sample);
//
//	info.capture_time = timestamp;
//	info.device_id = sensor_index;
//
//	std::shared_ptr<rs::file_format::ros_data_objects::pose> msg =
//		ros_data_objects::pose::create(info);
//	if (msg == nullptr)
//	{
//		return device_data_serializer::error_code::write_failed;
//	}
//
//	if (m_writer->record(msg) != rs::file_format::status_no_error)
//	{
//		return device_data_serializer::error_code::write_failed;
//	}
//
//	return device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::read_pose(std::shared_ptr<ros_data_objects::pose> sample,
//	uint32_t &sensor_index, uint64_t &timestamp, core::data_object **obj)
//{
//	if (sample == nullptr || obj == nullptr)
//	{
//		return device_data_serializer::error_code::invalid_handle;
//	}
//
//	std::shared_ptr<rs::file_format::ros_data_objects::pose> tracking =
//		std::static_pointer_cast<ros_data_objects::pose>(sample);
//
//	rs::core::pose pose = {};
//    rs::file_format::ros_data_objects::pose_info pose_data = tracking->get_info();
//
//	pose = conversions::convert(pose_data);
//
//	sensor_index = pose_data.device_id;
//	timestamp = pose_data.capture_time.count() / 1000;
//
//
//	if (rs::data_objects::tracking::pose_data_object::create(pose, obj) == false)
//	{
//		return device_data_serializer::error_code::data_object_creation_failed;
//	}
//	return device_data_serializer::error_code::no_error;
//}
//
//
//bool realsense_serializer::copy_image_metadata(const rs::core::metadata_interface* source,
//	std::map<rs::file_format::file_types::metadata_type, std::vector<uint8_t>>& target)
//{
//	if (source == nullptr)
//	{
//               LOG_ERROR("failed to create metadata");
//               return false;
//	}
//	for (int i = 0;; i++)
//	{
//		metadata_type type;
//        if (source->query_available_metadata_type(i, type) == false)
//		{
//			break;
//		}
//
//		uint32_t size = source->query_buffer_size(type);
//		if (size == 0)
//		{
//                       LOG_ERROR("failed to query frame metadata size");
//			continue;
//		}
//
//        std::vector<uint8_t> buffer(size);
//        if(source->get_metadata(type, buffer.data()) != size)
//        {
//            LOG_ERROR("failed to query frame metadata");
//            continue;
//        }
//        auto ros_type = static_cast<rs::file_format::file_types::metadata_type>(type);
//        buffer.swap(target[ros_type]);
//	}
//
//	return true;
//}
//
//device_data_serializer::error_code realsense_serializer::write_image(core::image_interface *img,
//                                                                    int32_t sensor_index,
//																	rs::file_format::file_types::nanoseconds timestamp)
//{
//    if(img == nullptr)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//
//    uint32_t buffer_size = img->query_info().height * img->query_info().pitch;
//    std::shared_ptr<uint8_t> buffer = std::shared_ptr<uint8_t>(new uint8_t[buffer_size],
//                                                               [](uint8_t* ptr){delete[] ptr;});
//    memcpy(buffer.get(), img->query_data(), buffer_size);
//    ros_data_objects::image_info info;
//    info.device_id = sensor_index;
//    if(conversions::convert(img->query_stream_type(), info.stream) == false)
//    {
//        return error_code::invalid_handle;
//    }
//    info.width = static_cast<uint32_t>(img->query_info().width);
//    info.height = static_cast<uint32_t>(img->query_info().height);
//    if(conversions::convert(img->query_info().format, info.format) == false)
//    {
//        return error_code::invalid_handle;
//    }
//
//    info.step = static_cast<uint32_t>(img->query_info().pitch);
//    info.capture_time = timestamp;
//
//	std::chrono::duration<double, std::milli> timestamp_ms(img->query_time_stamp());
//    info.timestamp = rs::file_format::file_types::seconds(timestamp_ms);
//
//    if(conversions::convert(img->query_time_stamp_domain(), info.timestamp_domain) == false)
//    {
//        return error_code::invalid_handle;
//    }
//
//    info.frame_number = static_cast<uint32_t>(img->query_frame_number());
//    info.data = buffer;
//
//    if(copy_image_metadata(img->query_metadata(), info.metadata) == false)
//    {
//        return device_data_serializer::write_failed;
//    }
//
//    std::shared_ptr<ros_data_objects::image> image = ros_data_objects::image::create(info);
//    if(image == nullptr)
//    {
//        return device_data_serializer::error_code::write_failed;
//    }
//
//    if(m_writer->record(image) != rs::file_format::status_no_error)
//    {
//        return device_data_serializer::write_failed;
//    }
//
//    return device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::read_property(std::shared_ptr<ros_data_objects::property> sample,
//                                                                       uint32_t &sensor_index, uint64_t &timestamp,
//                                                                       core::data_object **obj)
//{
//    if(sample == nullptr || obj == nullptr)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//
//    auto property_sample = std::static_pointer_cast<ros_data_objects::property>(sample);
//    sensor_index = property_sample->get_info().device_id;
//    timestamp = property_sample->get_info().capture_time.count() / 1000;
//
//    rs::core::guid id;
//    if(rs::utils::string_to_guid(property_sample->get_info().key, id) == false)
//    {
//        return device_data_serializer::error_code::read_failed;
//    }
//    double value;
//    value = property_sample->get_info().value;
//
//    if(rs::data_objects::common::properties_data_object::create(&id, &value, 1,
//                                                                obj) == false)
//    {
//        return device_data_serializer::error_code::data_object_creation_failed;
//    }
//    if(m_propertiesper_sensor.find(sensor_index) == m_propertiesper_sensor.end())
//    {
//        auto pair = std::pair<uint32_t, std::vector<core::guid>>(sensor_index, {id});
//        m_propertiesper_sensor.insert(pair);
//    }else
//    {
//        auto it = m_propertiesper_sensor.at(sensor_index);
//        if(std::find(it.begin(), it.end(), id) == it.end())
//        {
//            m_propertiesper_sensor.at(sensor_index).push_back(id);
//        }
//    }
//    return device_data_serializer::error_code::no_error;
//
//}
//
//device_data_serializer::error_code realsense_serializer::read_properties_metadata(std::map<uint32_t,
//                                                                                  std::vector<rs::utils::ref_count_ptr<rs::core::data_object>> > &properties)
//{
//    uint64_t time_min = FIRST_FRAME_TIMESTAMP.count();
//    uint64_t timestamp_microseconds = time_min;
//    uint32_t sensor_index = 0;
//
//    m_propertiesper_sensor.clear();
//
//    while(timestamp_microseconds == time_min)
//    {
//        rs::utils::ref_count_ptr<rs::core::data_object> obj;
//        if(read(sensor_index, timestamp_microseconds, &obj) != device_data_serializer::no_error)
//        {
//            return device_data_serializer::no_error;
//        }
//
//        if(timestamp_microseconds > time_min)
//        {
//            continue;
//        }
//
//        rs::utils::ref_count_ptr<rs::extensions::common::properties_extension> properties_data_extension;
//        if (obj->extend_to(rs::extensions::common::properties_extension::ID(), (void**)&properties_data_extension) == false)
//        {
//            continue;
//        }
//
//        if(properties.find(sensor_index) == properties.end())
//        {
//            auto pair = std::pair<int32_t, std::vector<rs::utils::ref_count_ptr<rs::core::data_object>>>(sensor_index, {obj});
//            properties.insert(pair);
//        }else
//        {
//            properties.at(sensor_index).push_back(obj);
//        }
//    }
//    auto timestamp_us = rs::file_format::file_types::microseconds(timestamp_microseconds);;
//    auto retval = m_reader->seek_to_time(timestamp_us);
//	if (retval != rs::file_format::status_no_error)
//	{
//        return device_data_serializer::read_failed;
//	}
//
//    m_first_frame_time = timestamp_us;
//    return device_data_serializer::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::set_properties_state(uint64_t seek_time_us)
//{
//    if(file_types::microseconds(seek_time_us) == m_first_frame_time || m_propertiesper_sensor.empty())
//    {
//        return device_data_serializer::no_error;
//    }
//
//    //Get all topics from all sensors
//    std::vector<std::string> topics;
//    for(auto sensor_properties : m_propertiesper_sensor)
//    {
//        for(auto guid : sensor_properties.second)
//        {
//            std::stringstream ss;
//            ss << guid;
//            std::string topic = ros_data_objects::property::get_topic(ss.str(), sensor_properties.first);
//            topics.push_back(topic);
//        }
//    }
//
//    auto prev_timestamp = FIRST_FRAME_TIMESTAMP; //TODO: query from reader once stream_playback adds get_position
//    std::vector<std::string> prev_filters{}; //TODO: query from reader once we add support for filtering for serializer
//    rs::utils::scope_guard restore_state([this, prev_filters, prev_timestamp](){
//       //At the end of this function we should return to the state we were when we entered it
//        //Seeking to beginning of file in order to set the filters back (which at this point of code has no more frames- the iterator read all data)
//        if(m_reader->seek_to_time(rs::file_format::file_types::nanoseconds(FIRST_FRAME_TIMESTAMP)) != rs::file_format::status_no_error)
//        {
//            LOG_ERROR("Failed to seek to " << FIRST_FRAME_TIMESTAMP.count() << " while restoring serializer state");
//            return;
//        }
//        if(m_reader->set_filter(prev_filters) != rs::file_format::status_no_error)
//        {
//            LOG_ERROR("Failed to set filter while restoring serializer state");
//            return;
//        }
//        if(m_reader->seek_to_time(rs::file_format::file_types::nanoseconds(prev_timestamp)) != rs::file_format::status_no_error)
//        {
//            LOG_ERROR("Failed to seek to " << prev_timestamp.count() << " while restoring serializer state");
//            return;
//        }
//    });
//
//
//    //Seek to beginning of the file (after the header, i.e. first frame)
//    auto retval = m_reader->seek_to_time(m_first_frame_time);
//    if(retval != rs::file_format::status_no_error)
//    {
//        return device_data_serializer::read_failed;
//    }
//    //Add the properties topics as filters
//    if(m_reader->set_filter(topics) != rs::file_format::status_no_error)
//    {
//        return device_data_serializer::read_failed;
//    }
//
//    //remove all properties we read so far (will be updated at end of this function)
//    //TODO: restore them in case of failure?
//    m_cached_properties.clear();
//    std::vector<cached_property> properties;
//
//    //Get all properties up to time seek_time
//    uint64_t timestamp = m_first_frame_time.count();
//    while(timestamp < seek_time_us)
//    {
//        uint32_t sensor_index = 0;
//        //read_sample should only read properties (because we set the filter the properties only)
//        rs::utils::ref_count_ptr<rs::core::data_object> obj;
//        if (read_sample(sensor_index, timestamp, &obj) != device_data_serializer::no_error)
//        {
//            break;
//        }
//
//        if (timestamp >= seek_time_us)
//        {
//            continue;
//        }
//
//        rs::utils::ref_count_ptr<rs::extensions::common::properties_extension> properties_data_extension;
//        if (obj->extend_to(rs::extensions::common::properties_extension::ID(), (void**) &properties_data_extension) == false)
//        {
//            return device_data_serializer::read_failed;
//        }
//
//        auto properties_it = std::find_if(properties.begin(), properties.end(), [&sensor_index](cached_property& i)
//        {
//            return (i.sensor_id == sensor_index);
//        });
//
//        if (properties_it == properties.end())
//        {
//            cached_property property = {sensor_index, timestamp, obj};
//            properties.emplace_back(property);
//        }
//        else
//        {
//            rs::utils::ref_count_ptr<rs::core::metadata_extension> metadata_extension;
//            if ((*properties_it).property->extend_to(rs::core::metadata_extension::ID(), (void**) &metadata_extension) == false)
//            {
//                return device_data_serializer::read_failed;
//            }
//            if (metadata_extension->update(obj) == false)
//            {
//                (*properties_it).property = obj;
//            }
//            //update to latest property timestamp
//            (*properties_it).timestamp = timestamp;
//        }
//    }
//    m_cached_properties = properties;
//    return device_data_serializer::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::read_stream_info(std::map<uint32_t, std::vector<rs::utils::ref_count_ptr<rs::core::data_object>> > &stream_infos,
//                                                                          std::map<uint32_t, std::pair<rs::core::extrinsics, uint64_t>>& extrinsics)
//{
//    for(uint32_t sensor_index = 0; sensor_index < m_sensor_count; ++sensor_index)
//    {
//        std::vector<std::shared_ptr<ros_data_objects::stream_info>> stream_info;
//        auto ros_retval = m_reader->read_stream_infos(stream_info,
//                                                      file_types::sample_type::st_image, sensor_index);
//        if (ros_retval != rs::file_format::status::status_no_error)
//        {
//            continue;
//        }
//
//        std::vector<rs::sensors::camera::streaming_profile> profiles;
//        std::vector<rs::core::intrinsics> intrinsics_info;
//        std::vector<rs::core::stream_type> streams;
//        for(auto item : stream_info)
//        {
//            std::shared_ptr<rs::file_format::ros_data_objects::image_stream_info> image_item =
//                    std::static_pointer_cast<ros_data_objects::image_stream_info>(item);
//            rs::file_format::ros_data_objects::image_stream_data info = image_item->get_info();
//            rs::sensors::camera::streaming_profile profile = {};
//            profile.fps = info.fps;
//            profile.height = info.height;
//            profile.width = info.width;
//            if(conversions::convert(info.format, profile.format) == false)
//            {
//                return error_code::invalid_handle;
//            }
//            if(conversions::convert(info.type, profile.stream) == false)
//            {
//                return error_code::invalid_handle;
//            }
//            rs::core::intrinsics intrinsics_item = {};
//            if(conversions::convert(info.intrinsics, intrinsics_item) == false)
//            {
//                return error_code::invalid_handle;
//            }
//            intrinsics_item.height = info.height;
//            intrinsics_item.width = info.width;
//            profiles.push_back(profile);
//            intrinsics_info.push_back(intrinsics_item);
//            streams.push_back(profile.stream);
//
//            if(extrinsics.find(sensor_index) == extrinsics.end())
//            {
//                std::pair<rs::core::extrinsics, uint64_t> extrinsics_pair;
//				extrinsics_pair.first = conversions::convert(info.stream_extrinsics.extrinsics_data);
//                extrinsics_pair.second = info.stream_extrinsics.reference_point_id;
//                extrinsics[sensor_index] = extrinsics_pair;
//            }
//        }
//        rs::utils::ref_count_ptr<rs::core::data_object> obj_profile_info;
//        if(rs::data_objects::camera::streaming_data_object::create(profiles.data(),
//                                                                   static_cast<uint32_t>(profiles.size()), &obj_profile_info) == false)
//        {
//            return device_data_serializer::error_code::data_object_creation_failed;
//        }
//
//        stream_infos[sensor_index].push_back(obj_profile_info);
//
//        //If there is any extrinsic not empty, then it's a valid extension otherwise it is default value and is not a device metadata
//        if(std::any_of(intrinsics_info.begin(), intrinsics_info.end(), [&](const rs::core::intrinsics& i) { return is_empty_intrinsics(i) == false; }))
//        {
//            rs::utils::ref_count_ptr<rs::core::data_object> obj_intrinsics;
//            if (rs::data_objects::camera::camera_intrinsics_data_object::create(streams.data(), intrinsics_info.data(),
//                                                                                static_cast<uint32_t>(intrinsics_info
//                                                                                    .size()), &obj_intrinsics) == false)
//            {
//                return device_data_serializer::error_code::data_object_creation_failed;
//            }
//            stream_infos[sensor_index].push_back(obj_intrinsics);
//
//            rs::utils::ref_count_ptr<rs::core::data_object> obj_intrinsics_per_profile;
//            if(rs::data_objects::camera::camera_intrinsics_per_stream_data_object::create(profiles.data(), intrinsics_info.data(),
//                                                                                          static_cast<uint32_t>(intrinsics_info.size()), &obj_intrinsics_per_profile) == false)
//            {
//                return device_data_serializer::error_code::data_object_creation_failed;
//            }
//            stream_infos[sensor_index].push_back(obj_intrinsics_per_profile);
//        }
//    }
//    return device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::read_motion_info(std::map<uint32_t, rs::utils::ref_count_ptr<rs::core::data_object>> &infos,
//                                                                          std::map<uint32_t, std::pair<rs::core::extrinsics, uint64_t>>& extrinsics)
//{
//    for(uint32_t sensor_index = 0; sensor_index < m_sensor_count; ++sensor_index)
//    {
//        std::vector<std::shared_ptr<ros_data_objects::stream_info>> stream_info;
//        auto ros_retval = m_reader->read_stream_infos(stream_info,
//                                                      rs::file_format::file_types::sample_type::st_motion, sensor_index);
//        if (ros_retval != rs::file_format::status::status_no_error)
//        {
//            continue;
//        }
//
//        std::vector<rs::sensors::motion::motion_profile> profiles;
//        std::map<rs::core::motion_type, rs::core::motion_device_intrinsics> motion_intrinsics;
//        rs::utils::ref_count_ptr<rs::core::data_object> obj;
//        for(auto item : stream_info)
//        {
//            std::shared_ptr<rs::file_format::ros_data_objects::motion_stream_info> motion_item =
//                    std::static_pointer_cast<ros_data_objects::motion_stream_info>(item);
//            rs::file_format::ros_data_objects::motion_stream_data info = motion_item->get_info();
//            rs::sensors::motion::motion_profile profile = {};
//            profile.fps = info.fps;
//            if(conversions::convert(info.type, profile.type) == false)
//            {
//                return error_code::invalid_handle;
//            }
//            rs::core::motion_device_intrinsics intrinsics = conversions::convert(info.intrinsics);
//            motion_intrinsics[profile.type] = intrinsics;
//            if(extrinsics.find(sensor_index) == extrinsics.end())
//            {
//                std::pair<rs::core::extrinsics, uint64_t> extrinsics_pair;
//				extrinsics_pair.first = conversions::convert(info.stream_extrinsics.extrinsics_data);
//                extrinsics_pair.second = info.stream_extrinsics.reference_point_id;
//                extrinsics[sensor_index] = extrinsics_pair;
//
//            }
//
//            profiles.push_back(profile);
//        }
//
//        std::vector<rs::core::motion_type>  supported_intrinsics_types;
//        std::vector<rs::core::motion_device_intrinsics> intrinsics_values;
//        for (auto&& intrinsic : motion_intrinsics)
//        {
//            supported_intrinsics_types.push_back(intrinsic.first);
//            intrinsics_values.push_back(intrinsic.second);
//        }
//
//
//        if(rs::data_objects::motion::motion_config_data_object::create(profiles.data(),
//                                                                       static_cast<uint32_t>(profiles.size()),
//                                                                       supported_intrinsics_types.data(),
//                                                                       intrinsics_values.data(),
//                                                                       static_cast<uint32_t>(intrinsics_values.size()),
//                                                                       &obj) == false)
//        {
//            return device_data_serializer::error_code::data_object_creation_failed;
//        }
//        infos[sensor_index] = obj;
//    }
//    return device_data_serializer::error_code::no_error;
//
//}
//
//device_data_serializer::error_code realsense_serializer::create_vendor_info_object(std::vector<std::shared_ptr<ros_data_objects::vendor_data>>& vendor_data, rs::core::data_object** obj)
//{
//    std::vector<rs::core::guid> guids;
//    std::vector<std::string> values;
//
//    for(auto data : vendor_data)
//    {
//        rs::core::guid id;
//        if(rs::utils::string_to_guid(data->get_info().name, id) == false)
//        {
//            return device_data_serializer::error_code::read_failed;
//        }
//        guids.push_back(id);
//        values.push_back(data->get_info().value);
//    }
//
//    std::vector<const char*> as_char_arrays;
//    std::transform(values.begin(), values.end(), std::back_inserter(as_char_arrays), [](const std::string& s){ return s.c_str();});
//
//
//    if(rs::data_objects::common::info_data_object::create(guids.data(), as_char_arrays.data(),
//                                                          static_cast<uint32_t>(guids.size()), obj) == false)
//    {
//        return device_data_serializer::error_code::data_object_creation_failed;
//    }
//    return device_data_serializer::error_code::no_error;
//}
//
//bool realsense_serializer::get_sensor_count(std::vector<std::shared_ptr<ros_data_objects::vendor_data>>& vendor_data, uint32_t& sensor_count)
//{
//    for(auto data : vendor_data)
//    {
//        if(data->get_info().name == SENSOR_COUNT)
//        {
//            auto value = data->get_info().value;
//            sensor_count = std::stoi(value);
//            return true;
//        }
//    }
//    return false;
//}
//
//device_data_serializer::error_code realsense_serializer::read_vendor_info(std::map<uint32_t, rs::utils::ref_count_ptr<rs::core::data_object>> &infos,
//                                                                          uint32_t& sensor_count)
//{
//    std::vector<std::shared_ptr<ros_data_objects::vendor_data>> vendor_data;
//    auto sts = m_reader->read_vendor_data(vendor_data, DEVICE_INDEX);
//    if(sts == rs::file_format::status_item_unavailable)
//    {
//        sensor_count = 0;
//        return device_data_serializer::error_code::no_error;
//    }
//    if(sts != rs::file_format::status_no_error || vendor_data.size() == 0)
//    {
//        return device_data_serializer::error_code::read_failed;
//    }
//
//    if(get_sensor_count(vendor_data, sensor_count) == false)
//    {
//        return device_data_serializer::error_code::read_failed;
//    }
//
//    //removing SENSOR_COUNT from the vendor_data since it is the only vendor data with vendor_data.name that is not a rs::core::guid
//    vendor_data.erase(
//        std::remove_if(vendor_data.begin(), vendor_data.end(),
//            [](std::shared_ptr<ros_data_objects::vendor_data>& info) { return info->get_info().name == SENSOR_COUNT; }),
//        vendor_data.end());
//
//    //At this point all vendor_data should be in the form (guid, string)
//    rs::utils::ref_count_ptr<rs::core::data_object> obj;
//    auto retval = create_vendor_info_object(vendor_data, &obj);
//    if(retval != device_data_serializer::no_error)
//    {
//        return retval;
//    }
//    infos[DEVICE_INDEX] = obj;
//
//    for(uint32_t i = 0; i < m_sensor_count + 1; ++i)
//    {
//        std::vector<std::shared_ptr<ros_data_objects::vendor_data>> sensor_vendor_data;
//        if(m_reader->read_vendor_data(sensor_vendor_data, i) != rs::file_format::status_no_error)
//        {
//            continue;
//        }
//
//        rs::utils::ref_count_ptr<rs::core::data_object> obj;
//        auto retval = create_vendor_info_object(sensor_vendor_data, &obj);
//        if(retval != device_data_serializer::no_error)
//        {
//            return retval;
//        }
//        infos[i] = obj;
//
//    }
//    return device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::read_motion(std::shared_ptr<ros_data_objects::motion_sample> sample,
//                                                                     uint32_t &sensor_index, uint64_t &timestamp, core::data_object **obj)
//{
//    if(sample == nullptr || obj == nullptr)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//
//    std::shared_ptr<rs::file_format::ros_data_objects::motion_sample> motion =
//            std::static_pointer_cast<ros_data_objects::motion_sample>(sample);
//
//    rs::core::motion_sample motion_sample = {};
//    rs::file_format::ros_data_objects::motion_info motion_info = motion->get_info();
//    sensor_index = motion_info.device_id;
//    if(conversions::convert(motion_info.type, motion_sample.type) == false)
//    {
//        return error_code::invalid_handle;
//    }
//    motion_sample.timestamp = motion_info.timestamp.count() * 1000;
//    motion_sample.frame_number = motion_info.frame_number;
//    motion_sample.data[0] = motion_info.data[0];
//    motion_sample.data[1] = motion_info.data[1];
//    motion_sample.data[2] = motion_info.data[2];
//
//    timestamp = motion_info.capture_time.count() / 1000;
//
//    if(rs::data_objects::motion::motion_data_object::create(motion_sample, obj) == false)
//    {
//        return device_data_serializer::error_code::data_object_creation_failed;
//    }
//    return device_data_serializer::error_code::no_error;
//}
//
//bool realsense_serializer::copy_image_metadata(const std::map<rs::file_format::file_types::metadata_type, std::vector<uint8_t>> source,
//                                                                             rs::core::metadata_interface* target)
//{
//    if(target == nullptr)
//    {
//        LOG_WARN("failed to create metadata");
//        return true;
//    }
//    for(auto data : source)
//    {
//        rs::core::metadata_type type = static_cast<rs::core::metadata_type>(data.first);
//        std::vector<uint8_t> buffer = data.second;
//        auto retval = target->add_metadata(type, buffer.data(), static_cast<uint32_t>(buffer.size()));
//        if(retval != rs::core::status::status_no_error)
//        {
//            LOG_ERROR("failed to read metadata");
//            continue;
//        }
//    }
//
//    return true;
//}
//
//device_data_serializer::error_code realsense_serializer::read_image(std::shared_ptr<ros_data_objects::image> sample,
//                                                                          uint32_t &sensor_index, uint64_t &timestamp, core::data_object **obj)
//{
//    if(sample == nullptr || obj == nullptr)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//
//    std::shared_ptr<rs::file_format::ros_data_objects::image> img =
//            std::static_pointer_cast<ros_data_objects::image>(sample);
//    ros_data_objects::image_info image_info = img->get_info();
//    sensor_index = image_info.device_id;
//    rs::core::pixel_format format;
//
//    if(conversions::convert(image_info.format, format) == false)
//    {
//        return error_code::invalid_handle;
//    }
//
//    rs::core::image_info info{static_cast<int32_t>(image_info.width),
//                static_cast<int32_t>(image_info.height),
//                format,
//                static_cast<int32_t>(image_info.step)};
//
//    int buffer_size = image_info.height * image_info.step;
//    uint8_t* buffer = nullptr;
//    rs::core::release_interface* data_releaser = nullptr;
//    rs::utils::ref_count_ptr<rs::core::image_interface> img_int;
//    rs::utils::scope_guard scope_data_releaser([&buffer, &data_releaser, &img_int]()
//    {
//        if(img_int == nullptr)
//        {
//            if(data_releaser != nullptr)
//            {
//                data_releaser->release();
//            }
//            else
//            {
//                if(buffer != nullptr)
//                {
//                    delete[] buffer;
//                }
//            }
//        }
//    });
//
//    buffer = new uint8_t[buffer_size];
//    data_releaser = new rs::utils::self_releasing_array_data_releaser(buffer);
//
//    memcpy(buffer, image_info.data.get(), buffer_size);
//    rs::core::stream_type stream;
//    if(conversions::convert(image_info.stream, stream) == false)
//    {
//        return error_code::invalid_handle;
//    }
//    rs::core::timestamp_domain timestamp_domain;
//    if(conversions::convert(image_info.timestamp_domain, timestamp_domain) == false)
//    {
//        return error_code::invalid_handle;
//    }
//    img_int = rs::utils::ref_count_ptr<rs::core::image_interface>::make_attached(
//                rs::core::image_interface::create_instance_from_raw_data(&info,
//    {buffer, data_releaser},
//                                                                         stream,
//                                                                         rs::core::image_interface::flag::any,
//                                                                         image_info.timestamp.count() * 1000,
//                                                                         image_info.frame_number,
//                                                                         timestamp_domain));
//    timestamp = image_info.capture_time.count() / 1000;
//
//    if(copy_image_metadata(image_info.metadata, img_int->query_metadata()) == false)
//    {
//        return device_data_serializer::error_code::read_failed;
//    }
//
//    if(rs::data_objects::camera::image_data_object::create(img_int, obj) == false)
//    {
//        return device_data_serializer::error_code::data_object_creation_failed;
//    }
//    return device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::write_sensor_metadata(core::query_metadata_interface *metadata,
//                                                                               uint32_t sensor_index, core::query_metadata_interface *device)
//{
//    if(metadata == nullptr)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//
//    rs::utils::ref_count_ptr<rs::sensors::camera::camera_intrinsics_extension> camera_intrinsics;
//    rs::utils::ref_count_ptr<rs::sensors::camera::streaming_extension> streaming_info;
//    rs::utils::ref_count_ptr<rs::extensions::common::extrinsics_extension> extrinsics_extension;
//
//    for(int j = 0; ; ++j)
//    {
//        rs::utils::ref_count_ptr<rs::core::data_object> obj;
//        if(device->query_metadata(j, &obj) == false)
//        {
//            break;
//        }
//
//        if (obj->extend_to(rs::extensions::common::extrinsics_extension::ID(), (void**)&extrinsics_extension) == true)
//        {
//            break;
//        }
//    }
//
//    for(int j = 0; ; ++j)
//    {
//        rs::utils::ref_count_ptr<rs::core::data_object> obj;
//        if(metadata->query_metadata(j, &obj) == false)
//        {
//            break;
//        }
//
//        rs::utils::ref_count_ptr<rs::extensions::common::info_extension> info_extension;
//        if (obj->extend_to(rs::extensions::common::info_extension::ID(), (void**)&info_extension) == true)
//        {
//            auto retval = write_vendor_info(info_extension, sensor_index);
//            if(retval != device_data_serializer::error_code::no_error)
//            {
//                return retval;
//            }
//        }
//        rs::utils::ref_count_ptr<rs::extensions::common::properties_extension> property_extension;
//        if (obj->extend_to(rs::extensions::common::properties_extension::ID(), (void**)&property_extension) == true)
//        {
//			auto timestamp_ns = rs::file_format::file_types::nanoseconds(FIRST_FRAME_TIMESTAMP);
//            auto retval = write_property(property_extension, sensor_index, timestamp_ns);
//            if(retval != device_data_serializer::error_code::no_error)
//            {
//                return retval;
//            }
//        }
//        rs::utils::ref_count_ptr<rs::sensors::camera::camera_intrinsics_extension> camera_intrinsics_extension;
//        if (obj->extend_to(rs::sensors::camera::camera_intrinsics_extension::ID(), (void**)&camera_intrinsics_extension) == true)
//        {
//            camera_intrinsics = camera_intrinsics_extension;
//        }
//        rs::utils::ref_count_ptr<rs::sensors::camera::streaming_extension> streaming_extension;
//        if (obj->extend_to(rs::sensors::camera::streaming_extension::ID(), (void**)&streaming_extension) == true)
//        {
//            streaming_info = streaming_extension;
//        }
//        rs::utils::ref_count_ptr<rs::sensors::motion::motion_extension> motion_extension;
//        if (obj->extend_to(rs::sensors::motion::motion_extension::ID(), (void**)&motion_extension) == true)
//        {
//            auto retval = write_motion_info(motion_extension, extrinsics_extension, sensor_index);
//            if(retval != device_data_serializer::error_code::no_error)
//            {
//                return retval;
//            }
//        }
//    }
//    if(streaming_info != nullptr)
//    {
//        write_stream_info(streaming_info, camera_intrinsics, extrinsics_extension, sensor_index);
//    }
//    return  device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::write_device_metadata(core::query_metadata_interface *metadata)
//{
//    if(metadata == nullptr)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//    for(int j = 0; ; ++j)
//    {
//        rs::utils::ref_count_ptr<rs::core::data_object> obj;
//        if(metadata->query_metadata(j, &obj) == false)
//        {
//            break;
//        }
//
//        rs::utils::ref_count_ptr<rs::extensions::common::info_extension> info_extension;
//        if (obj->extend_to(rs::extensions::common::info_extension::ID(), (void**)&info_extension) == true)
//        {
//            auto retval = write_vendor_info(info_extension, DEVICE_INDEX);
//            if(retval != device_data_serializer::error_code::no_error)
//            {
//                return retval;
//            }
//        }
//        rs::utils::ref_count_ptr<rs::extensions::common::properties_extension> property_extension;
//        if (obj->extend_to(rs::extensions::common::properties_extension::ID(), (void**)&property_extension) == true)
//        {
//			auto timestamp_ns = rs::file_format::file_types::nanoseconds(FIRST_FRAME_TIMESTAMP);
//            auto retval = write_property(property_extension, DEVICE_INDEX, timestamp_ns);
//            if(retval != device_data_serializer::error_code::no_error)
//            {
//                return retval;
//            }
//        }
//    }
//    return  device_data_serializer::error_code::no_error;
//}
//
//realsense_serializer::realsense_serializer(const char *file_path, ros_serializer_interface::access_mode mode) :
//    m_file_path(file_path),
//    m_sensor_count(0)
//{
//    if(mode == ros_serializer_interface::write)
//    {
//		m_writer.reset();
//		if (rs::file_format::stream_recorder::create(file_path, m_writer) == false)
//		{
//			throw std::runtime_error("failed to create seriliazer for writing");
//		}
//        m_first_frame_time = FIRST_FRAME_TIMESTAMP;
//    }
//    else if(mode == ros_serializer_interface::read)
//    {
//		m_reader.reset();
//        if(rs::file_format::stream_playback::create(file_path, m_reader) == false)
//        {
//            throw std::runtime_error("failed to create seriliazer for reading");
//        }
//        m_first_frame_time = FIRST_FRAME_TIMESTAMP;
//        if(read_metadata(&m_device_description) != device_data_serializer::error_code::no_error)
//        {
//            throw std::runtime_error("Failed to read header");
//        }
//    }
//    else
//    {
//        throw std::runtime_error("unsupported access mode");
//    }
//}
//
//device_data_serializer::error_code realsense_serializer::reset()
//{
//    if(m_reader != nullptr)
//    {
//        auto retval = seek_to_time(FIRST_FRAME_TIMESTAMP.count());
//        if(retval != device_data_serializer::error_code::no_error)
//        {
//            return retval;
//        }
//    }
//
//    if(m_writer != nullptr)
//    {
//		m_writer.reset();
//		if (rs::file_format::stream_recorder::create(m_file_path, m_writer) == false)
//		{
//            return device_data_serializer::write_failed;
//		}
//    }
//
//    return device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::query_device_description(core::device_description_interface **description)
//{
//    if(m_reader == nullptr)
//    {
//        return device_data_serializer::read_failed;
//    }
//    if (description == nullptr)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//
//    m_device_description->add_ref();
//    *description = m_device_description;
//
//    return device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::write_device_description(core::device_description_interface *description)
//{
//    if(m_writer == nullptr)
//    {
//        return device_data_serializer::write_failed;
//    }
//    if (description == nullptr)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//
//    auto retval = write_device_metadata(description);
//    if(retval != device_data_serializer::error_code::no_error)
//    {
//        return retval;
//    }
//
//    uint32_t sensor_description_count = description->sensor_description_count();
//    for (uint32_t sensor_index = 0; sensor_index<sensor_description_count; sensor_index++)
//    {
//        rs::utils::ref_count_ptr<rs::core::sensor_description_interface> sensor;
//        if(description->query_sensor_description(sensor_index, &sensor) == false)
//        {
//            return device_data_serializer::write_failed;
//        }
//
//        auto retval = write_sensor_metadata(sensor, sensor_index, description);
//        if(retval != device_data_serializer::error_code::no_error)
//    {
//        return retval;
//    }
//    }
//
//    write_sensor_count(sensor_description_count);
//    m_device_description = description;
//
//    return device_data_serializer::error_code::no_error;
//}
//
//device_data_serializer::error_code realsense_serializer::read(uint32_t &sensor_index, uint64_t &timestamp, core::data_object **obj)
//{
//    std::unique_lock<std::mutex> locker(m_mutex);
//    if(m_reader == nullptr)
//    {
//        return device_data_serializer::read_failed;
//    }
//    if (obj == nullptr)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//
//    //read cached properties avaialble after seek
//    if(m_cached_properties.empty() == false)
//    {
//        cached_property properties_sensor_list = (*m_cached_properties.begin());
//        *obj = properties_sensor_list.property;
//        properties_sensor_list.property->add_ref();
//        sensor_index = properties_sensor_list.sensor_id;
//        timestamp = properties_sensor_list.timestamp;
//        m_cached_properties.pop_back();
//        return device_data_serializer::error_code::no_error;
//    }
//
//    return read_sample(sensor_index, timestamp, obj);
//
//}
//
//device_data_serializer::error_code realsense_serializer::read_sample(uint32_t &sensor_index, uint64_t &timestamp, core::data_object **obj)
//{
//
//    std::shared_ptr<rs::file_format::ros_data_objects::sample> sample;
//    auto reader_retval = m_reader->read_next_sample(sample);
//    if(sample == nullptr || reader_retval != rs::file_format::status_no_error)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//
//    if(sample->get_type() == rs::file_format::file_types::sample_type::st_image)
//    {
//        return read_image(std::static_pointer_cast<ros_data_objects::image>(sample),
//                          sensor_index, timestamp, obj);
//    }
//    else if(sample->get_type() == rs::file_format::file_types::sample_type::st_motion)
//    {
//        return read_motion(std::static_pointer_cast<ros_data_objects::motion_sample>(sample),
//                           sensor_index, timestamp, obj);
//    }
//    else if(sample->get_type() == rs::file_format::file_types::sample_type::st_property)
//    {
//        return read_property(std::static_pointer_cast<ros_data_objects::property>(sample),
//                                    sensor_index, timestamp, obj);
//    }
//	else if (sample->get_type() == rs::file_format::file_types::sample_type::st_pose)
//	{
//		return read_pose(std::static_pointer_cast<ros_data_objects::pose>(sample),
//			sensor_index, timestamp, obj);
//	}
//    return device_data_serializer::error_code::read_failed;
//}
//
//device_data_serializer::error_code realsense_serializer::write(const uint32_t &sensor_index, const uint64_t &timestamp, core::data_object *obj)
//{
//    if(m_writer == nullptr)
//    {
//        return device_data_serializer::write_failed;
//    }
//    if (obj == nullptr)
//    {
//        return device_data_serializer::error_code::invalid_handle;
//    }
//
//    std::chrono::duration<uint64_t, std::micro> timestamp_us(timestamp);
//    auto timestamp_ns = rs::file_format::file_types::nanoseconds(timestamp_us);
//
//    rs::utils::ref_count_ptr<rs::data_objects::camera::image_data_extension> image_data_extension;
//    if (obj->extend_to(rs::data_objects::camera::image_data_extension::ID(), (void**)&image_data_extension) == true)
//    {
//        return write_image(image_data_extension->image(), sensor_index, timestamp_ns);
//    }
//
//    rs::utils::ref_count_ptr<rs::data_objects::motion::motion_data_extension> motion_data_extension;
//    if (obj->extend_to(rs::data_objects::motion::motion_data_extension::ID(), (void**)&motion_data_extension) == true)
//    {
//        return write_motion(motion_data_extension->data(), sensor_index, timestamp_ns);
//    }
//
//    rs::utils::ref_count_ptr<rs::extensions::common::properties_extension> properties_data_extension;
//    if (obj->extend_to(rs::extensions::common::properties_extension::ID(), (void**)&properties_data_extension) == true)
//    {
//        return write_property(properties_data_extension, sensor_index, timestamp_ns);
//    }
//	rs::utils::ref_count_ptr<rs::data_objects::tracking::pose_data_extension> six_dof_data;
//	if (obj->extend_to(rs::data_objects::tracking::pose_data_extension::ID(), (void**)&six_dof_data))
//	{
//		return write_pose(six_dof_data->get_pose(), sensor_index, timestamp_ns);
//	}
//    return device_data_serializer::error_code::invalid_handle;
//}
//
//device_data_serializer::error_code realsense_serializer::seek_to_time(uint64_t time_microseconds)
//{
//    if(m_reader == nullptr)
//    {
//        return device_data_serializer::read_failed;
//    }
//    auto seek_time = m_first_frame_time + rs::file_format::file_types::microseconds(time_microseconds);
//    auto time_interval = rs::file_format::file_types::nanoseconds(seek_time);
//
//    std::unique_lock<std::mutex> locker(m_mutex);
//    auto retval = set_properties_state(seek_time.count());
//    if(retval != device_data_serializer::error_code::no_error)
//    {
//        LOG_ERROR("Failed to seek_to_time " << time_microseconds << ", set_properties_state returned " << retval);
//        return retval;
//    }
//    auto sts = m_reader->seek_to_time(time_interval);
//    if(sts != rs::file_format::status_no_error)
//    {
//        LOG_ERROR("Failed to seek_to_time " << time_microseconds << ", m_reader->seek_to_time(" <<  time_interval.count() << ") returned " << sts);
//        return device_data_serializer::read_failed;
//    }
//    return device_data_serializer::error_code::no_error;
//}
//bool realsense_serializer::is_empty_intrinsics(const rs::core::intrinsics& intrinsics)
//{
//    auto empty = rs::core::intrinsics{};
//    return empty == intrinsics;
//}
//device_data_serializer::error_code realsense_serializer::query_duration(uint64_t& duration) const
//{
//    file_types::nanoseconds time;
//    if(m_reader->get_file_duration(time) != rs::file_format::status::status_no_error)
//    {
//        return error_code::read_failed;
//    }
//    duration = std::chrono::duration_cast<std::chrono::duration<uint64_t, std::micro>>(time).count();
//    return error_code::no_error;
//}
//
//bool rs::storage::serializers::ros_serializer_interface::create(const char* file_path, access_mode mode, device_data_serializer** serializer)
//{
//    if (serializer == nullptr)
//        return false;
//
//    rs::utils::ref_count_ptr<realsense_serializer> instance;
//    try
//    {
//        instance = rs::utils::make_ref_count_ptr<realsense_serializer>(file_path, mode);
//    }
//    catch (std::exception& e)
//    {
//        LOG_ERROR("Failed to create ros_serializer: " << e.what());
//        return false;
//    }
//
//    instance->add_ref();
//    *serializer = instance;
//    return true;
//}
//
//
