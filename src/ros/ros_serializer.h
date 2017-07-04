//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
//
//#pragma once
//#include <map>
//#include <mutex>
//#include "include/stream_playback.h"
//#include "include/stream_recorder.h"
//#include "include/data_objects/image.h"
//#include "include/data_objects/property.h"
//#include "include/data_objects/motion_sample.h"
//#include "include/data_objects/vendor_data.h"
//#include "include/data_objects/pose.h"
//
////#include "rs/utils/ref_count_base.h"
////#include "rs/utils/ref_count_ptr.h"
//
////#include "rs/data_objects/camera/camera_data_objects.h"
////#include "rs/data_objects/motion/motion_data_objects.h"
////#include "rs/data_objects/common/properties_data_object.h"
////#include "rs/data_objects/common/info_data_object.h"
////#include "rs/data_objects/common/extrinsics_data_object.h"
////#include "rs/data_objects/tracking/pose_data_objects.h"
//
////#include "rs/storage/serializers/ros_serializer_interface.h"
//
//namespace rs
//{
//    namespace storage
//    {
//        namespace serializers
//        {
//            class realsense_serializer :
//                    public rs::utils::ref_count_base<rs::storage::serializers::ros_serializer_interface>
//            {
//            private:
//                struct cached_property
//                {
//                    uint32_t sensor_id;
//                    uint64_t timestamp;
//                    rs::utils::ref_count_ptr<rs::core::data_object> property;
//                };
//
//                std::unique_ptr<rs::file_format::stream_playback>                   m_reader;
//                std::unique_ptr<rs::file_format::stream_recorder>                   m_writer;
//                rs::utils::ref_count_ptr<rs::core::device_description_interface>    m_device_description;
//				rs::file_format::file_types::microseconds                           m_first_frame_time;
//                std::string                                                         m_file_path;
//                std::vector<cached_property>                                        m_cached_properties;
//                std::map<uint32_t, std::vector<rs::core::guid> >                    m_propertiesper_sensor;
//                std::mutex                                                          m_mutex;
//                uint32_t                                                            m_sensor_count;
//
//                core::device_data_serializer::error_code read_sample(uint32_t &sensor_index, uint64_t &timestamp, core::data_object **obj);
//
//                core::device_data_serializer::error_code set_properties_state(uint64_t seek_time);
//
//                rs::core::device_data_serializer::error_code read_metadata(core::device_description_interface** description);
//
//                rs::core::device_data_serializer::error_code write_sensor_count(uint32_t sensor_count);
//
//                rs::core::device_data_serializer::error_code write_vendor_info(rs::extensions::common::info_extension* info_api,
//                                                                        uint32_t sensor_index);
//
//                rs::core::device_data_serializer::error_code read_vendor_info(std::map<uint32_t, rs::utils::ref_count_ptr<rs::core::data_object>>& info,
//                                                                              uint32_t& sensor_count);
//
//                rs::core::device_data_serializer::error_code write_property(rs::extensions::common::properties_extension* info_api,
//                                                                        uint32_t sensor_index,
//																		rs::file_format::file_types::nanoseconds timestamp);
//
//                rs::core::device_data_serializer::error_code read_property(std::shared_ptr<rs::file_format::ros_data_objects::property> sample,
//                                                                         uint32_t& sensor_index,
//                                                                         uint64_t& timestamp,
//                                                                         core::data_object** obj);
//
//                rs::core::device_data_serializer::error_code write_stream_info(sensors::camera::streaming_extension* stream_info,
//                                                                               sensors::camera::camera_intrinsics_extension* intrinsics_info,
//                                                                               rs::extensions::common::extrinsics_extension* extrinsics_info,
//                                                                               uint32_t sensor_index);
//
//                rs::core::device_data_serializer::error_code read_stream_info(std::map<uint32_t, std::vector<rs::utils::ref_count_ptr<rs::core::data_object>>>& infos,
//                                                                              std::map<uint32_t, std::pair<rs::core::extrinsics, uint64_t>>& extrinsics);
//
//                rs::core::device_data_serializer::error_code write_motion_info(rs::sensors::motion::motion_extension* motion_info,
//                                                                               rs::extensions::common::extrinsics_extension* extrinsics_info,
//                                                                               uint32_t sensor_index);
//
//                rs::core::device_data_serializer::error_code read_motion_info(std::map<uint32_t, rs::utils::ref_count_ptr<rs::core::data_object>>& infos,
//                                                                              std::map<uint32_t, std::pair<rs::core::extrinsics, uint64_t>>& extrinsics);
//
//                rs::core::device_data_serializer::error_code write_motion(const rs::core::motion_sample& sample,
//                                                                        int32_t sensor_index,
//																		rs::file_format::file_types::nanoseconds timestamp);
//
//                rs::core::device_data_serializer::error_code read_motion(std::shared_ptr<rs::file_format::ros_data_objects::motion_sample> sample,
//                                                                         uint32_t& sensor_index,
//                                                                         uint64_t& timestamp,
//                                                                         core::data_object** obj);
//
//                rs::core::device_data_serializer::error_code write_image(rs::core::image_interface* img,
//                                                                        int32_t sensor_index,
//																		rs::file_format::file_types::nanoseconds timestamp);
//
//                rs::core::device_data_serializer::error_code read_image(std::shared_ptr<rs::file_format::ros_data_objects::image> sample,
//                                                                        uint32_t& sensor_index,
//                                                                        uint64_t& timestamp,
//                                                                        core::data_object** obj);
//
//                rs::core::device_data_serializer::error_code read_properties_metadata(std::map<uint32_t,
//                                                                                      std::vector<rs::utils::ref_count_ptr<rs::core::data_object>>>& properties);
//
//                rs::core::device_data_serializer::error_code write_sensor_metadata(rs::core::query_metadata_interface *metadata,
//                                                                            uint32_t sensor_index, core::query_metadata_interface *device = nullptr);
//
//                rs::core::device_data_serializer::error_code write_device_metadata(core::query_metadata_interface *metadata);
//
//                bool copy_image_metadata(const rs::core::metadata_interface* source,
//                                         std::map<rs::file_format::file_types::metadata_type, std::vector<uint8_t>>& target);
//
//                bool copy_image_metadata(const std::map<rs::file_format::file_types::metadata_type, std::vector<uint8_t>> source,
//                                         rs::core::metadata_interface* target);
//
//
//				rs::core::device_data_serializer::error_code write_pose(const rs::core::pose &sample,
//																		uint32_t sensor_index,
//																		rs::file_format::file_types::nanoseconds timestamp);
//
//				rs::core::device_data_serializer::error_code read_pose(std::shared_ptr<rs::file_format::ros_data_objects::pose> sample,
//																uint32_t &sensor_index, uint64_t &timestamp,
//																core::data_object **obj);
//				static bool is_empty_extrinsics(const core::extrinsics& e);
//				static bool is_empty_intrinsics(const core::intrinsics& intrinsics);
//				static bool create_extrinsics_object(std::map<uint32_t, std::pair<core::extrinsics, uint64_t>>& extrinsics,core::data_object** extrinsics_object);
//				static error_code create_vendor_info_object(std::vector<std::shared_ptr<file_format::ros_data_objects::vendor_data>>& vendor_data,core::data_object** obj);
//				static bool get_sensor_count(std::vector<std::shared_ptr<file_format::ros_data_objects::vendor_data>>& vendor_data,uint32_t& sensor_count);
//            public:
//
//                realsense_serializer(const char* file_path, ros_serializer_interface::access_mode mode);
//
//                virtual rs::core::device_data_serializer::error_code reset() override;
//
//                virtual rs::core::device_data_serializer::error_code query_device_description(rs::core::device_description_interface** description) override;
//
//                virtual rs::core::device_data_serializer::error_code write_device_description(rs::core::device_description_interface* description) override;
//
//                virtual rs::core::device_data_serializer::error_code read(uint32_t& sensor_index, uint64_t& timestamp, core::data_object** obj) override;
//
//                virtual rs::core::device_data_serializer::error_code write(const uint32_t& sensor_index, const uint64_t& timestamp, core::data_object* obj) override;
//
//                virtual rs::core::device_data_serializer::error_code seek_to_time(uint64_t time_microseconds) override;
//
//				virtual rs::core::device_data_serializer::error_code query_duration(uint64_t& duration) const override;
//			};
//        }
//    }
//}
//
//
//
