// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "d500-motion.h"

#include <mutex>
#include <chrono>
#include <vector>
#include <map>
#include <iterator>
#include <cstddef>

#include <src/metadata.h>
#include "ds/ds-timestamp.h"
#include "ds/ds-options.h"
#include "d500-info.h"
#include "stream.h"
#include "proc/motion-transform.h"
#include "proc/auto-exposure-processor.h"
#include "backend.h"
#include <src/metadata-parser.h>

using namespace librealsense;
namespace librealsense
{
    

    rs2_motion_device_intrinsic d500_motion::get_motion_intrinsics(rs2_stream stream) const
    {
        return _ds_motion_common->get_motion_intrinsics(stream);
    }

    bool d500_motion::is_gyro_high_sensitivity() const
    {
        return false;
    }

    std::shared_ptr<synthetic_sensor> d500_motion::create_hid_device(std::shared_ptr<context> ctx,
                                                                const std::vector<platform::hid_device_info>& all_hid_infos,
                                                                const firmware_version& camera_fw_version)
    {
        return _ds_motion_common->create_hid_device(ctx, all_hid_infos, camera_fw_version, _tf_keeper);
    }

    d500_motion::d500_motion( std::shared_ptr< const d500_info > const & dev_info )
        : device( dev_info )
        , d500_device( dev_info )
    {
        using namespace ds;

        std::vector<platform::hid_device_info> hid_infos = dev_info->get_group().hid_devices;

        _ds_motion_common = std::make_shared<ds_motion_common>(this, _fw_version,
            _device_capabilities, _hw_monitor); 
        _ds_motion_common->init_motion(hid_infos.empty(), *_depth_stream);
                
        initialize_fisheye_sensor( dev_info->get_context(), dev_info->get_group() );

        // Try to add HID endpoint
        auto hid_ep = create_hid_device(dev_info->get_context(), dev_info->get_group().hid_devices, _fw_version);
        if (hid_ep)
        {
            _motion_module_device_idx = static_cast<uint8_t>(add_sensor(hid_ep));

            // HID metadata attributes
            hid_ep->get_raw_sensor()->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_hid_header_parser(&hid_header::timestamp));
        }
    }

    void d500_motion::initialize_fisheye_sensor(std::shared_ptr<context> ctx, const platform::backend_device_group& group)
    {
        using namespace ds;
        
        bool is_fisheye_avaialable = false;
        auto fisheye_infos = _ds_motion_common->init_fisheye(group, is_fisheye_avaialable);

        if (!is_fisheye_avaialable)
            return;

        std::unique_ptr< frame_timestamp_reader > ds_timestamp_reader_backup( new ds_timestamp_reader() );
        std::unique_ptr<frame_timestamp_reader> ds_timestamp_reader_metadata(new ds_timestamp_reader_from_metadata(std::move(ds_timestamp_reader_backup)));
        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());
        auto raw_fisheye_ep
            = std::make_shared< uvc_sensor >( "FishEye Sensor",
                                              get_backend()->create_uvc_device( fisheye_infos.front() ),
                                              std::unique_ptr< frame_timestamp_reader >( new global_timestamp_reader(
                                                  std::move( ds_timestamp_reader_metadata ),
                                                  _tf_keeper,
                                                  enable_global_time_option ) ),
                                              this );
        auto fisheye_ep = std::make_shared<ds_fisheye_sensor>(raw_fisheye_ep, this);

        _ds_motion_common->assign_fisheye_ep(raw_fisheye_ep, fisheye_ep, enable_global_time_option);

        register_fisheye_options();

        register_fisheye_metadata();

        // Add fisheye endpoint
        _fisheye_device_idx = add_sensor(fisheye_ep);
    }

    void d500_motion::register_fisheye_options()
    {
        _ds_motion_common->register_fisheye_options();
    }

    void d500_motion::register_fisheye_metadata()
    {
        _ds_motion_common->register_fisheye_metadata();
    }

    void d500_motion::register_stream_to_extrinsic_group(const stream_interface& stream, uint32_t group_index)
    {
        device::register_stream_to_extrinsic_group(stream, group_index);
    }
}
