// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "ds5-safety.h"

#include <mutex>
#include <chrono>
#include <vector>
#include <map>
#include <iterator>
#include <cstddef>

#include "ds5-timestamp.h"
#include "ds5-options.h"
#include "stream.h"

namespace librealsense
{
    const std::map<uint32_t, rs2_format> safety_fourcc_to_rs2_format = {
        {rs_fourcc('R','A','W','8'), RS2_FORMAT_RAW8}
    };
    const std::map<uint32_t, rs2_stream> safety_fourcc_to_rs2_stream = {
        {rs_fourcc('R','A','W','8'), RS2_STREAM_SAFETY}
    };

    ds5_safety::ds5_safety(std::shared_ptr<context> ctx,
        const platform::backend_device_group& group)
        : device(ctx, group), ds5_device(ctx, group),
        _safety_stream(new stream(RS2_STREAM_SAFETY))
    {
        using namespace ds;

        auto safety_devs_info = filter_by_mi(group.uvc_devices, 8); // TODO REMI - assumption EP 8
        if (safety_devs_info.size() != 1)
            throw invalid_value_exception(to_string() << "RS4XX with Safety models are expected to include a single safety device! - "
                << safety_devs_info.size() << " found");

        auto safety_ep = create_safety_device(ctx, safety_devs_info);
    }

    std::shared_ptr<synthetic_sensor> ds5_safety::create_safety_device(std::shared_ptr<context> ctx,
        const std::vector<platform::uvc_device_info>& safety_devices_info)
    {
        using namespace ds;
        auto&& backend = ctx->get_backend();

        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_backup(new ds5_timestamp_reader(backend.create_time_service()));
        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_metadata(new ds5_timestamp_reader_from_metadata(std::move(ds5_timestamp_reader_backup)));

        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());
        auto raw_safety_ep = std::make_shared<uvc_sensor>("Raw Safety Device",
            backend.create_uvc_device(safety_devices_info.front()),
            std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(ds5_timestamp_reader_metadata), _tf_keeper, enable_global_time_option)),
            this);

        auto safety_ep = std::make_shared<ds5_safety_sensor>(this,
            raw_safety_ep,
            safety_fourcc_to_rs2_format,
            safety_fourcc_to_rs2_stream);

        safety_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);

        safety_ep->register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, safety_devices_info.front().device_path);

        // register options
        register_options(safety_ep);

        // register metadata
        register_metadata(safety_ep);

        // register processing blocks
        register_processing_blocks(safety_ep);
        
        return safety_ep;
    }


    void ds5_safety::register_options(std::shared_ptr<ds5_safety_sensor> safety_ep)
    {
        // empty
    }

    void ds5_safety::register_metadata(std::shared_ptr<ds5_safety_sensor> safety_ep)
    {
        safety_ep->get_raw_sensor()->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_uvc_header_parser(&platform::uvc_header::timestamp));
    }

    void ds5_safety::register_processing_blocks(std::shared_ptr<ds5_safety_sensor> safety_ep)
    {
        safety_ep->register_processing_block(processing_block_factory::create_id_pbf(RS2_FORMAT_RAW16, RS2_STREAM_SAFETY));
    }


    stream_profiles ds5_safety_sensor::init_stream_profiles()
    {
        auto lock = environment::get_instance().get_extrinsics_graph().lock();
        auto results = synthetic_sensor::init_stream_profiles();

        for (auto p : results)
        {
            // Register stream types
            if (p->get_stream_type() == RS2_STREAM_SAFETY)
                assign_stream(_owner->_safety_stream, p);
        }

        return results;
    }
}