// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <memory>
#include <string>
#include <media/device_snapshot.h>
#include "core/debug.h"
#include "core/serialization.h"
#include "archive.h"
#include "types.h"
#include "media/ros/file_types.h"
#include "media/ros/data_objects/vendor_data.h"
#include "media/ros/data_objects/image.h"
#include "media/ros/data_objects/stream_data.h"
#include "media/ros/data_objects/image_stream_info.h"

#include "status.h"
#include "file_types.h"
#include "ros/exception.h"
#include "rosbag/bag.h"
#include "std_msgs/UInt32.h"

namespace librealsense
{
    class ros_writer: public device_serializer::writer
    {
    public:
        ros_writer(const std::string& file) :
            m_data_object_writer(m_bag),
            m_file_path(file)
        {
            reset();
        }

        void write_device_description(const librealsense::device_snapshot& device_description) override
        {
            for (auto&& device_extension_snapshot : device_description.get_device_extensions_snapshots().get_snapshots())
            {
                write_extension_snapshot(file_format::get_device_index(), device_extension_snapshot.first, device_extension_snapshot.second);
            }
            uint32_t sensor_index = 0;
            for (auto&& sensors_snapshot : device_description.get_sensors_snapshots())
            {
                for (auto&& sensor_extension_snapshot : sensors_snapshot.get_sensor_extensions_snapshots().get_snapshots())
                {
                    write_extension_snapshot(std::to_string(sensor_index), sensor_extension_snapshot.first, sensor_extension_snapshot.second);
                }
                write_streaming_info(std::to_string(sensor_index), sensors_snapshot.get_streamig_profiles());
                sensor_index++;
            }
            write_sensor_count(device_description.get_sensors_snapshots().size());
        }

        void reset() override
        {
            try
            {
                m_bag.close();
                m_bag.open(m_file_path, rosbag::BagMode::Write);
                m_bag.setCompression(rosbag::CompressionType::LZ4);
                write_file_version();
            }
            catch(rosbag::BagIOException& e)
            {
                throw librealsense::invalid_value_exception(e.what());
            }
            catch(std::exception& e)
            {
                throw librealsense::invalid_value_exception(to_string() << "Failed to initialize writer. " << e.what());
            }
            m_first_frame_time = file_format::get_first_frame_timestamp();
        }

        void write(std::chrono::nanoseconds timestamp, uint32_t sensor_index, librealsense::frame_holder&& frame) override
        {
            auto timestamp_ns =file_format::file_types::nanoseconds(timestamp);


            if (Is<video_frame>(frame.frame))
            {
                auto image_obj = std::make_shared<file_format::ros_data_objects::image>(timestamp, sensor_index, std::move(frame));
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

        void write_file_version()
        {
            std_msgs::UInt32 msg;
            msg.data =file_format::get_file_version();
            m_data_object_writer.write(file_format::get_file_version_topic(),file_format::file_types::nanoseconds::min(), msg);
        }

    private:

        void write_streaming_info(const std::string& sensor_index, const std::vector<stream_profile>& profiles)
        {
            for (auto profile : profiles)
            {
               file_format::ros_data_objects::image_stream_data image_stream_info{};
                image_stream_info.device_id = sensor_index;
                image_stream_info.fps = profile.fps;
                image_stream_info.height = profile.height;
                image_stream_info.width = profile.width;
                image_stream_info.type = profile.stream;
                image_stream_info.format = profile.format;
                
                //TODO: fill extrinsics
                //image_stream_info.stream_extrinsics.extrinsics_data = extrinsics
                //image_stream_info.stream_extrinsics.reference_point_id = ref_point;

                //TODO: fill intrinsics
                //set model to None instead of empty string to allow empty intrinsics
                image_stream_info.intrinsics.model = rs2_distortion::RS2_DISTORTION_NONE;
                //image_stream_info.intrinsics = {};
                auto msg = std::make_shared<file_format::ros_data_objects::image_stream_info>(image_stream_info);
                record(msg);
            }
        }

        void write_extension_snapshot(const std::string& id, rs2_extension type, std::shared_ptr<librealsense::extension_snapshot> snapshot)
        {
            switch (type)
            {
            case RS2_EXTENSION_UNKNOWN:
                throw invalid_value_exception("Unknown extension");
            case RS2_EXTENSION_DEBUG :
                break;
            case RS2_EXTENSION_INFO :
                write_vendor_info({ std::chrono::nanoseconds(0), id, type, snapshot });
                break;
            case RS2_EXTENSION_MOTION :
                break;
            case RS2_EXTENSION_OPTIONS :
                break;
            case RS2_EXTENSION_VIDEO :
                break;
            case RS2_EXTENSION_ROI :
                break;
            case RS2_EXTENSION_VIDEO_FRAME :
                break;
            case RS2_EXTENSION_MOTION_FRAME :
                break;
            case RS2_EXTENSION_COUNT :
                break;
            default:
                throw invalid_value_exception("Unsupported extension");

            }
        }
        void write_debug_info(const device_serializer::snapshot_box& box)
        {

        }
            
        void write_vendor_info(const device_serializer::snapshot_box& box)
        {
            auto info = std::dynamic_pointer_cast<info_interface>(box.snapshot);
            assert(info != nullptr);
            for (uint32_t i = 0; i < static_cast<uint32_t>(RS2_CAMERA_INFO_COUNT); i++)
            {
                auto camera_info = static_cast<rs2_camera_info>(i);
                if(!info->supports_info(camera_info))
                {
                    continue;
                }

               file_format::ros_data_objects::vendor_info vendor_info = {};
                vendor_info.device_id = box.sensor_index;
                vendor_info.name =  rs2_camera_info_to_string(camera_info);
                vendor_info.value = info->get_info(camera_info);

                auto msg = std::make_shared<file_format::ros_data_objects::vendor_data>(vendor_info);
                record(msg);
            }
        }

        void write_sensor_count(size_t sensor_count)
        {
           file_format::ros_data_objects::vendor_info vendor_info = {};
            vendor_info.device_id = file_format::get_device_index();
            vendor_info.name = file_format::get_sensor_count_topic();
            vendor_info.value = std::to_string(sensor_count);

            auto msg = std::make_shared<file_format::ros_data_objects::vendor_data>(vendor_info);
            record(msg);
        }

        void record(std::shared_ptr<file_format::ros_data_objects::stream_data> data)
        {
            data->write_data(m_data_object_writer);
        }

        data_object_writer m_data_object_writer;
        file_format::file_types::nanoseconds m_first_frame_time;
        std::string m_file_path;
        rosbag::Bag m_bag;
    };
}
