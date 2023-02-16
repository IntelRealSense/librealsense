// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "d500-occupancy.h"

#include <mutex>
#include <chrono>
#include <vector>
#include <map>
#include <iterator>
#include <cstddef>

#include "ds/ds-timestamp.h"
#include "ds/ds-options.h"
#include "stream.h"

namespace librealsense
{
    const std::map<uint32_t, rs2_format> occupancy_fourcc_to_rs2_format = {
        {rs_fourcc('G','R','E','Y'), RS2_FORMAT_RAW8}
    };
    const std::map<uint32_t, rs2_stream> occupancy_fourcc_to_rs2_stream = {
        {rs_fourcc('G','R','E','Y'), RS2_STREAM_OCCUPANCY}
    };

    d500_occupancy::d500_occupancy(std::shared_ptr<context> ctx,
        const platform::backend_device_group& group)
        : device(ctx, group), d500_device(ctx, group),
        _occupancy_stream(new stream(RS2_STREAM_OCCUPANCY))
    {
        using namespace ds;

        const uint32_t occupancy_stream_mi = 13;
        auto occupancy_devs_info = filter_by_mi(group.uvc_devices, occupancy_stream_mi);
        
        if (occupancy_devs_info.size() != 1)
            throw invalid_value_exception(rsutils::string::from() << "RS5XX with Safety models are expected to include a single occupancy device! - "
                << occupancy_devs_info.size() << " found");

        auto occupancy_ep = create_occupancy_device(ctx, occupancy_devs_info);
        _occupancy_device_idx = add_sensor(occupancy_ep);
    }

    std::shared_ptr<synthetic_sensor> d500_occupancy::create_occupancy_device(std::shared_ptr<context> ctx,
        const std::vector<platform::uvc_device_info>& occupancy_devices_info)
    {
        using namespace ds;
        auto&& backend = ctx->get_backend();

        register_stream_to_extrinsic_group(*_occupancy_stream, 0);
        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_depth_stream, *_occupancy_stream);

        std::unique_ptr<frame_timestamp_reader> ds_timestamp_reader_backup(new ds_timestamp_reader(backend.create_time_service()));
        std::unique_ptr<frame_timestamp_reader> ds_timestamp_reader_metadata(new ds_timestamp_reader_from_metadata(std::move(ds_timestamp_reader_backup)));

        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());

        auto raw_occupancy_ep = std::make_shared<uvc_sensor>("Raw Safety Device",
            backend.create_uvc_device(occupancy_devices_info.front()),
            std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(ds_timestamp_reader_metadata), _tf_keeper, enable_global_time_option)),
            this);

        //raw_occupancy_ep->register_xu(occupancy_xu); // making sure the XU is initialized every time we power the camera

        auto occupancy_ep = std::make_shared<d500_occupancy_sensor>(this,
            raw_occupancy_ep,
            occupancy_fourcc_to_rs2_format,
            occupancy_fourcc_to_rs2_stream);

        occupancy_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);

        occupancy_ep->register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, occupancy_devices_info.front().device_path);

        // register options
        register_options(occupancy_ep, raw_occupancy_ep);

        // register metadata
        register_metadata(raw_occupancy_ep);

        // register processing blocks
        register_processing_blocks(occupancy_ep);
        
        return occupancy_ep;
    }

    void d500_occupancy::register_options(std::shared_ptr<d500_occupancy_sensor> occupancy_ep, std::shared_ptr<uvc_sensor> raw_occupancy_sensor)
    {

    }

    void d500_occupancy::register_metadata(std::shared_ptr<uvc_sensor> raw_occupancy_ep)
    {
        raw_occupancy_ep->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, 
            make_uvc_header_parser(&platform::uvc_header::timestamp));

        // attributes of md_occupancy_mode
        auto md_prop_offset = offsetof(metadata_raw, mode) +
            offsetof(md_occupancy_mode, intel_occupancy_info);

        raw_occupancy_ep->register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER, 
            make_attribute_parser(&md_occupancy_info::frame_counter, 
                md_occupancy_info_attributes::frame_counter_attribute, md_prop_offset));

        raw_occupancy_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_DEPTH_FRAME_COUNTER,
            make_attribute_parser(&md_occupancy_info::depth_frame_counter, 
                md_occupancy_info_attributes::depth_frame_counter_attribute, md_prop_offset));
        
        raw_occupancy_ep->register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP,
            make_attribute_parser(&md_occupancy_info::frame_timestamp, 
                md_occupancy_info_attributes::frame_timestamp_attribute, md_prop_offset));

        raw_occupancy_ep->register_metadata(RS2_FRAME_METADATA_OCCUPANCY_FLOOR_PLANE_EQUATION_A,
            make_attribute_parser(&md_occupancy_info::floor_plane_equation_a,
                md_occupancy_info_attributes::floor_plane_equation_a_attribute, md_prop_offset));

        raw_occupancy_ep->register_metadata(RS2_FRAME_METADATA_OCCUPANCY_FLOOR_PLANE_EQUATION_B,
            make_attribute_parser(&md_occupancy_info::floor_plane_equation_b,
                md_occupancy_info_attributes::floor_plane_equation_b_attribute, md_prop_offset));

        raw_occupancy_ep->register_metadata(RS2_FRAME_METADATA_OCCUPANCY_FLOOR_PLANE_EQUATION_C,
            make_attribute_parser(&md_occupancy_info::floor_plane_equation_c,
                md_occupancy_info_attributes::floor_plane_equation_c_attribute, md_prop_offset));

        raw_occupancy_ep->register_metadata(RS2_FRAME_METADATA_OCCUPANCY_FLOOR_PLANE_EQUATION_D,
            make_attribute_parser(&md_occupancy_info::floor_plane_equation_d,
                md_occupancy_info_attributes::floor_plane_equation_d_attribute, md_prop_offset));

        raw_occupancy_ep->register_metadata(RS2_FRAME_METADATA_OCCUPANCY_SAFETY_PRESET_ID,
            make_attribute_parser(&md_occupancy_info::safety_preset_id,
                md_occupancy_info_attributes::safety_preset_id_attribute, md_prop_offset));

        raw_occupancy_ep->register_metadata(RS2_FRAME_METADATA_OCCUPANCY_GRID_ROWS,
            make_attribute_parser(&md_occupancy_info::grid_rows,
                md_occupancy_info_attributes::grid_rows_attribute, md_prop_offset));

        raw_occupancy_ep->register_metadata(RS2_FRAME_METADATA_OCCUPANCY_GRID_COLUMNS,
            make_attribute_parser(&md_occupancy_info::grid_columns,
                md_occupancy_info_attributes::grid_columns_attribute, md_prop_offset));

        raw_occupancy_ep->register_metadata(RS2_FRAME_METADATA_OCCUPANCY_CELL_SIZE,
            make_attribute_parser(&md_occupancy_info::cell_size,
                md_occupancy_info_attributes::cell_size_attribute, md_prop_offset));

        raw_occupancy_ep->register_metadata(RS2_FRAME_METADATA_CRC,
            make_attribute_parser_with_crc(&md_occupancy_info::payload_crc32,
                md_occupancy_info_attributes::payload_crc32_attribute, md_prop_offset));
    }

    void d500_occupancy::register_processing_blocks(std::shared_ptr<d500_occupancy_sensor> occupancy_ep)
    {
        occupancy_ep->register_processing_block(processing_block_factory::create_id_pbf(RS2_FORMAT_RAW8, RS2_STREAM_OCCUPANCY));
    }


    stream_profiles d500_occupancy_sensor::init_stream_profiles()
    {
        auto lock = environment::get_instance().get_extrinsics_graph().lock();
        auto results = synthetic_sensor::init_stream_profiles();

        for (auto p : results)
        {
            // Register stream types
            if (p->get_stream_type() == RS2_STREAM_OCCUPANCY)
                assign_stream(_owner->_occupancy_stream, p);

            auto&& video = dynamic_cast<video_stream_profile_interface*>(p.get());
            const auto&& profile = to_profile(p.get());

            std::weak_ptr<d500_occupancy_sensor> wp =
                std::dynamic_pointer_cast<d500_occupancy_sensor>(this->shared_from_this());
            video->set_intrinsics([profile, wp]()
                {
                    auto sp = wp.lock();
                    if (sp)
                        return sp->get_intrinsics(profile);
                    else
                        return rs2_intrinsics{};
                });
        }

        return results;
    }

    rs2_intrinsics d500_occupancy_sensor::get_intrinsics(const stream_profile& profile) const
    {
        return rs2_intrinsics();
    }
}
