// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "d500-depth-mapping.h"

#include "d500-safety.h"
#include "d500-info.h"
#include "d500-md.h"


#include <vector>
#include <map>
#include <cstddef>

#include "ds/ds-timestamp.h"
#include "ds/ds-options.h"
#include <src/backend.h>
#include <src/fourcc.h>
#include "stream.h"

#include "platform/platform-utils.h"

#include <thread>

namespace librealsense
{
    const std::map<uint32_t, rs2_format> mapping_fourcc_to_rs2_format = {
        {rs_fourcc('G','R','E','Y'), RS2_FORMAT_RAW8},
        // point cloud - w/a done in backend in order to distinguish between occupancy
        // and labeled point cloud streams - PAL8 instead of GREY 
        // because both are received as GREY 
        {rs_fourcc('P','A','L','8'), RS2_FORMAT_RAW8}
    };
    const std::map<uint32_t, rs2_stream> mapping_fourcc_to_rs2_stream = {
        {rs_fourcc('G','R','E','Y'), RS2_STREAM_OCCUPANCY},
        {rs_fourcc('P','A','L','8'), RS2_STREAM_LABELED_POINT_CLOUD}
    };

    d500_depth_mapping::d500_depth_mapping( std::shared_ptr< const d500_info > const & dev_info)
        : device( dev_info ), d500_device( dev_info ),
        _occupancy_stream(new stream(RS2_STREAM_OCCUPANCY)),
        _point_cloud_stream(new stream(RS2_STREAM_LABELED_POINT_CLOUD))
    {
        using namespace ds;
        const uint32_t mapping_stream_mi = 13;
        auto mapping_devs_info = filter_by_mi( dev_info->get_group().uvc_devices, mapping_stream_mi);
        
        if (mapping_devs_info.size() != 1)
            throw invalid_value_exception(rsutils::string::from() << "RS5XX models with Safety are expected to include a single depth mapping device! - "
                << mapping_devs_info.size() << " found");

        auto mapping_ep = create_depth_mapping_device( dev_info->get_context(), mapping_devs_info );
        _depth_mapping_device_idx = add_sensor(mapping_ep);
    }

    std::shared_ptr<synthetic_sensor> d500_depth_mapping::create_depth_mapping_device(std::shared_ptr<context> ctx,
        const std::vector<platform::uvc_device_info>& occupancy_devices_info)
    {
        using namespace ds;

        std::unique_ptr<frame_timestamp_reader> ds_timestamp_reader_backup(new ds_timestamp_reader());
        std::unique_ptr<frame_timestamp_reader> ds_timestamp_reader_metadata(new ds_timestamp_reader_from_metadata_depth_mapping(std::move(ds_timestamp_reader_backup)));

        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());

        auto raw_mapping_ep = std::make_shared<uvc_sensor>("Raw Depth Mapping Device",
            get_backend()->create_uvc_device(occupancy_devices_info.front()),
            std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(ds_timestamp_reader_metadata), _tf_keeper, enable_global_time_option)),
            this);

        auto mapping_ep = std::make_shared<d500_depth_mapping_sensor>(this,
            raw_mapping_ep,
            mapping_fourcc_to_rs2_format,
            mapping_fourcc_to_rs2_stream);

        mapping_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);

        mapping_ep->register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, occupancy_devices_info.front().device_path);

        // register_extrinsics
        register_extrinsics();

        // register options
        register_options(mapping_ep, raw_mapping_ep);

        // register metadata
        register_metadata(raw_mapping_ep);

        // register processing blocks
        register_processing_blocks(mapping_ep);
        
        return mapping_ep;
    }

    void d500_depth_mapping::register_extrinsics()
    {
        // extrinsics to depth lazy, becasue safety sensor's api is used and it may be constructed later
        // than the depth mapping device (though it may not be the case in the device contructor's order, in ds500-factory)
        _depth_to_depth_mapping_extrinsics = std::make_shared< rsutils::lazy< rs2_extrinsics > >( [this]()
            {
                // getting access to safety sensor api
                auto safety_device = dynamic_cast<d500_safety*>(this);
                auto& safety_sensor = dynamic_cast<d500_safety_sensor&>(safety_device->get_safety_sensor());
                
                // Pull extrinsic from safety preset number 1 for setting labeled point cloud extrinsic
                // According to SRS ID 3.3.1.7.a: reading/modify safety presets
                // TODO: Remove this w/a after ES2
                int safety_preset_index = 1;
                rs2_extrinsics res;
                rs2_safety_preset safety_preset;
                try 
                {
                    safety_preset = safety_sensor.get_safety_preset(safety_preset_index);
                }
                catch (...)
                {
                    throw std::runtime_error("Could not read safety preset at index 1");
                }
                auto extrinsics_from_preset = safety_preset.platform_config.transformation_link;
                auto rot = extrinsics_from_preset.rotation;

                // converting row-major matrix to column-major
                float rotation_matrix[9] = { rot.x.x, rot.y.x, rot.z.x,
                                             rot.x.y, rot.y.y, rot.z.y,
                                             rot.x.z, rot.y.z, rot.z.z };

                std::memcpy(res.rotation, &rotation_matrix, sizeof rotation_matrix);
                std::memcpy(res.translation, &extrinsics_from_preset.translation, sizeof extrinsics_from_preset.translation);
                return res;
            });

        register_stream_to_extrinsic_group(*_occupancy_stream, 0);
        environment::get_instance().get_extrinsics_graph().register_extrinsics(*_depth_stream, *_occupancy_stream, _depth_to_depth_mapping_extrinsics);

        register_stream_to_extrinsic_group(*_point_cloud_stream, 0);
        environment::get_instance().get_extrinsics_graph().register_extrinsics(*_depth_stream, *_point_cloud_stream, _depth_to_depth_mapping_extrinsics);
    }

    void d500_depth_mapping::register_options(std::shared_ptr<d500_depth_mapping_sensor> occupancy_ep, std::shared_ptr<uvc_sensor> raw_mapping_sensor)
    {

    }

    void d500_depth_mapping::register_metadata(std::shared_ptr<uvc_sensor> raw_mapping_ep)
    {
        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, 
            make_uvc_header_parser(&platform::uvc_header::timestamp));

        register_occupancy_metadata(raw_mapping_ep);
        register_point_cloud_metadata(raw_mapping_ep);
    }

    void d500_depth_mapping::register_occupancy_metadata(std::shared_ptr<uvc_sensor> raw_mapping_ep)
    {
        // attributes of md_occupancy
        auto md_prop_offset = metadata_raw_mode_offset +
            offsetof(md_mapping_mode, intel_occupancy);

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER,
            make_attribute_parser(&md_occupancy::frame_counter,
                md_occupancy_attributes::frame_counter_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_DEPTH_FRAME_COUNTER,
            make_attribute_parser(&md_occupancy::depth_frame_counter,
                md_occupancy_attributes::depth_frame_counter_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP,
            make_attribute_parser(&md_occupancy::frame_timestamp,
                md_occupancy_attributes::frame_timestamp_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_FLOOR_DETECTION,
            make_attribute_parser(&md_occupancy::floor_detection,
                md_occupancy_attributes::floor_detection_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_CLIFF_DETECTION,
            make_attribute_parser(&md_occupancy::cliff_detection,
                md_occupancy_attributes::cliff_detection_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_DEPTH_FILL_RATE,
            make_attribute_parser(&md_occupancy::depth_fill_rate,
                md_occupancy_attributes::depth_fill_rate_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_SENSOR_ANGLE_ROLL,
            make_attribute_parser(&md_occupancy::sensor_roll_angle,
                md_occupancy_attributes::sensor_roll_angle_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_SENSOR_ANGLE_PITCH,
            make_attribute_parser(&md_occupancy::sensor_pitch_angle,
                md_occupancy_attributes::sensor_pitch_angle_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_FLOOR_MEDIAN_HEIGHT,
            make_attribute_parser(&md_occupancy::floor_median_height,
                md_occupancy_attributes::floor_median_height_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_DEPTH_STDEV,
            make_attribute_parser(&md_occupancy::depth_stdev,
                md_occupancy_attributes::depth_stdev_mm_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_PRESET_ID,
            make_attribute_parser(&md_occupancy::safety_preset_id,
                md_occupancy_attributes::safety_preset_id_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_OCCUPANCY_GRID_ROWS,
            make_attribute_parser(&md_occupancy::grid_rows,
                md_occupancy_attributes::grid_rows_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_OCCUPANCY_GRID_COLUMNS,
            make_attribute_parser(&md_occupancy::grid_columns,
                md_occupancy_attributes::grid_columns_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_OCCUPANCY_CELL_SIZE,
            make_attribute_parser(&md_occupancy::cell_size,
                md_occupancy_attributes::cell_size_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_CRC,
            make_attribute_parser(&md_occupancy::payload_crc32,
                md_occupancy_attributes::payload_crc32_attribute, md_prop_offset));
    }

    void d500_depth_mapping::register_point_cloud_metadata(std::shared_ptr<uvc_sensor> raw_mapping_ep)
    {
        // attributes of md_point_cloud
        auto md_prop_offset = metadata_raw_mode_offset +
            offsetof(md_mapping_mode, intel_point_cloud);

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER,
            make_attribute_parser(&md_point_cloud::frame_counter,
                md_point_cloud_attributes::frame_counter_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_DEPTH_FRAME_COUNTER,
            make_attribute_parser(&md_point_cloud::depth_frame_counter,
                md_point_cloud_attributes::depth_frame_counter_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP,
            make_attribute_parser(&md_point_cloud::frame_timestamp,
                md_point_cloud_attributes::frame_timestamp_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_FLOOR_DETECTION,
            make_attribute_parser(&md_point_cloud::floor_detection,
                md_point_cloud_attributes::floor_detection_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_CLIFF_DETECTION,
            make_attribute_parser(&md_point_cloud::cliff_detection,
                md_point_cloud_attributes::cliff_detection_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_DEPTH_FILL_RATE,
            make_attribute_parser(&md_point_cloud::depth_fill_rate,
                md_point_cloud_attributes::depth_fill_rate_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_SENSOR_ANGLE_ROLL,
            make_attribute_parser(&md_point_cloud::sensor_roll_angle,
                md_point_cloud_attributes::sensor_roll_angle_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_SENSOR_ANGLE_PITCH,
            make_attribute_parser(&md_point_cloud::sensor_pitch_angle,
                md_point_cloud_attributes::sensor_pitch_angle_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_FLOOR_MEDIAN_HEIGHT,
            make_attribute_parser(&md_point_cloud::floor_median_height,
                md_point_cloud_attributes::floor_median_height_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_DEPTH_STDEV,
            make_attribute_parser(&md_point_cloud::depth_stdev,
                md_point_cloud_attributes::depth_stdev_mm_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_PRESET_ID,
            make_attribute_parser(&md_point_cloud::safety_preset_id,
                md_point_cloud_attributes::safety_preset_id_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_NUMBER_OF_3D_VERTICES,
            make_attribute_parser(&md_point_cloud::number_of_3d_vertices,
                md_point_cloud_attributes::number_of_3d_vertices_attribute, md_prop_offset));

        raw_mapping_ep->register_metadata(RS2_FRAME_METADATA_CRC,
            make_attribute_parser(&md_point_cloud::payload_crc32,
                md_point_cloud_attributes::payload_crc32_attribute, md_prop_offset));
    }

    void d500_depth_mapping::register_processing_blocks(std::shared_ptr<d500_depth_mapping_sensor> mapping_ep)
    {
        mapping_ep->register_processing_block(processing_block_factory::create_id_pbf(RS2_FORMAT_RAW8, RS2_STREAM_OCCUPANCY));
        mapping_ep->register_processing_block(processing_block_factory::create_id_pbf(RS2_FORMAT_RAW8, RS2_STREAM_LABELED_POINT_CLOUD));
    }

    stream_profiles d500_depth_mapping_sensor::init_stream_profiles()
    {
        auto lock = environment::get_instance().get_extrinsics_graph().lock();
        auto results = synthetic_sensor::init_stream_profiles();
        stream_profiles relevant_results;
        for (auto p : results)
        {
            if (p->get_stream_type() == RS2_STREAM_OCCUPANCY)
            {
                auto&& video = dynamic_cast<video_stream_profile_interface*>(p.get());
                const auto&& profile = to_profile(p.get());
                if (profile.width == 2880)
                    continue;
                relevant_results.push_back(std::move(p));
            }
            else if (p->get_stream_type() == RS2_STREAM_LABELED_POINT_CLOUD)
            {
                auto&& video = dynamic_cast<video_stream_profile_interface*>(p.get());
                const auto&& profile = to_profile(p.get());
                if (profile.width == 256)
                    continue;
                relevant_results.push_back(std::move(p));
            }
        }

        for (auto p : relevant_results)
        {
            // Register stream types
            if (p->get_stream_type() == RS2_STREAM_OCCUPANCY)
                assign_stream(_owner->_occupancy_stream, p);
            else if (p->get_stream_type() == RS2_STREAM_LABELED_POINT_CLOUD)
                assign_stream(_owner->_point_cloud_stream, p);

            auto&& video = dynamic_cast<video_stream_profile_interface*>(p.get());
            const auto&& profile = to_profile(p.get());

            std::weak_ptr<d500_depth_mapping_sensor> wp =
                std::dynamic_pointer_cast<d500_depth_mapping_sensor>(this->shared_from_this());
            video->set_intrinsics([profile, wp]()
                {
                    auto sp = wp.lock();
                    if (sp)
                        return sp->get_intrinsics(profile);
                    else
                        return rs2_intrinsics{};
                });
        }

        return relevant_results;
    }

    rs2_intrinsics d500_depth_mapping_sensor::get_intrinsics(const stream_profile& profile) const
    {
        return rs2_intrinsics();
    }
}
