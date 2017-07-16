// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>

#include "device.h"
#include "context.h"
#include "image.h"
#include "metadata-parser.h"

#include "ds5-timestamp.h"
#include "ds5-options.h"
#include "ds5-motion.h"
#include "core/motion.h"

namespace librealsense
{
    class fisheye_auto_exposure_roi_method : public region_of_interest_method
    {
    public:
        fisheye_auto_exposure_roi_method(std::shared_ptr<auto_exposure_mechanism> auto_exposure)
            : _auto_exposure(auto_exposure)
        {}

        void set(const region_of_interest& roi) override
        {
            _auto_exposure->update_auto_exposure_roi(roi);
            _roi = roi;
        }

        region_of_interest get() const override
        {
            return _roi;
        }

    private:
        std::shared_ptr<auto_exposure_mechanism> _auto_exposure;
        region_of_interest _roi{};
    };

    class ds5_hid_sensor : public hid_sensor, public motion_sensor_interface
    {
    public:
        explicit ds5_hid_sensor(const ds5_motion* owner, std::shared_ptr<platform::hid_device> hid_device,
            std::unique_ptr<frame_timestamp_reader> hid_iio_timestamp_reader,
            std::unique_ptr<frame_timestamp_reader> custom_hid_timestamp_reader,
            std::map<rs2_stream, std::map<unsigned, unsigned>> fps_and_sampling_frequency_per_rs2_stream,
            std::vector<std::pair<std::string, stream_profile>> sensor_name_and_hid_profiles,
            std::shared_ptr<platform::time_service> ts)
            : hid_sensor(hid_device, move(hid_iio_timestamp_reader), move(custom_hid_timestamp_reader), 
                fps_and_sampling_frequency_per_rs2_stream, sensor_name_and_hid_profiles, ts, owner), _owner(owner)
        { 
        }

        rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream stream) const override
        {
            return _owner->get_motion_intrinsics(stream);
        }

    private:
        const ds5_motion* _owner;
    };

    class ds5_fisheye_sensor : public uvc_sensor, public video_sensor_interface
    {
    public:
        explicit ds5_fisheye_sensor(const ds5_motion* owner, std::shared_ptr<platform::uvc_device> uvc_device,
            std::unique_ptr<frame_timestamp_reader> timestamp_reader,
            std::shared_ptr<platform::time_service> ts)
            : uvc_sensor("Wide FOV Camera", uvc_device, move(timestamp_reader), ts, owner), _owner(owner)
        {}

        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override
        {
            return get_intrinsic_by_resolution(
                *_owner->_fisheye_intrinsics_raw,
                ds::calibration_table_id::fisheye_calibration_id,
                profile.width, profile.height);
        }
    private:
        const ds5_motion* _owner;
    };

    rs2_motion_device_intrinsic ds5_motion::get_motion_intrinsics(rs2_stream stream) const
    {
        if (stream == RS2_STREAM_ACCEL)
            return create_motion_intrinsics(*_accel_intrinsics);

        if (stream == RS2_STREAM_GYRO)
            return create_motion_intrinsics(*_gyro_intrinsics);

        throw std::runtime_error(to_string() << "Motion Intrinsics unknown for stream " << rs2_stream_to_string(stream) << "!");
    }

    pose ds5_motion::get_device_position(unsigned int subdevice) const
    {
        if (subdevice == _fisheye_device_idx)
        {
            auto extr = librealsense::ds::get_fisheye_extrinsics_data(*_fisheye_extrinsics_raw);
            return inverse(extr);
        }

        if (subdevice == _motion_module_device_idx)
        {
            // Fist, get Fish-eye pose
            auto fe_pose = get_device_position(_fisheye_device_idx);

            auto motion_extr = *_motion_module_extrinsics_raw;

            auto rot = motion_extr.rotation;
            auto trans = motion_extr.translation;

            pose ex = {{rot(0,0), rot(1,0),rot(2,0),rot(1,0), rot(1,1),rot(2,1),rot(0,2), rot(1,2),rot(2,2)},
                       {trans[0], trans[1], trans[2]}};

            return fe_pose * ex;
        }

        return ds5_device::get_device_position(subdevice);
    }

    std::vector<uint8_t> ds5_motion::get_raw_fisheye_intrinsics_table() const
    {
        const int offset = 0x84;
        const int size = 0x98;
        command cmd(ds::MMER, offset, size);
        return _hw_monitor->send(cmd);
    }

    ds::imu_calibration_table ds5_motion::get_motion_module_calibration_table() const
    {
        const int offset = 0x134;
        const int size = sizeof(ds::imu_calibration_table);
        command cmd(ds::MMER, offset, size);
        auto result = _hw_monitor->send(cmd);
        if (result.size() < sizeof(ds::imu_calibration_table))
            throw std::runtime_error("Not enough data returned from the device!");

        auto table = ds::check_calib<ds::imu_calibration_table>(result);

        return *table;
    }

    std::vector<uint8_t> ds5_motion::get_raw_fisheye_extrinsics_table() const
    {
        command cmd(ds::GET_EXTRINSICS);
        return _hw_monitor->send(cmd);
    }

    std::shared_ptr<hid_sensor> ds5_motion::create_hid_device(const platform::backend& backend,
                                                                const std::vector<platform::hid_device_info>& all_hid_infos,
                                                                const firmware_version& camera_fw_version)
    {
        if (all_hid_infos.empty())
        {
            throw std::runtime_error("HID device is missing!");
        }

        static const char* custom_sensor_fw_ver = "5.6.0.0";
        if (camera_fw_version >= firmware_version(custom_sensor_fw_ver))
        {
            static const std::vector<std::pair<std::string, stream_profile>> custom_sensor_profiles =
                {{std::string("custom"), {RS2_STREAM_GPIO1, 1, 1, 1, RS2_FORMAT_GPIO_RAW}},
                 {std::string("custom"), {RS2_STREAM_GPIO2, 1, 1, 1, RS2_FORMAT_GPIO_RAW}},
                 {std::string("custom"), {RS2_STREAM_GPIO3, 1, 1, 1, RS2_FORMAT_GPIO_RAW}},
                 {std::string("custom"), {RS2_STREAM_GPIO4, 1, 1, 1, RS2_FORMAT_GPIO_RAW}}};
            std::copy(custom_sensor_profiles.begin(), custom_sensor_profiles.end(), std::back_inserter(sensor_name_and_hid_profiles));
        }

        auto hid_ep = std::make_shared<ds5_hid_sensor>(this, backend.create_hid_device(all_hid_infos.front()),
                                                        std::unique_ptr<frame_timestamp_reader>(new ds5_iio_hid_timestamp_reader()),
                                                        std::unique_ptr<frame_timestamp_reader>(new ds5_custom_hid_timestamp_reader()),
                                                        fps_and_sampling_frequency_per_rs2_stream,
                                                        sensor_name_and_hid_profiles,
                                                        backend.create_time_service());
        hid_ep->register_pixel_format(pf_accel_axes);
        hid_ep->register_pixel_format(pf_gyro_axes);


        hid_ep->set_pose(lazy<pose>([](){pose p = {{ { 1,0,0 },{ 0,1,0 },{ 0,0,1 } },{ 0,0,0 }}; return p; }));

        if (camera_fw_version >= firmware_version(custom_sensor_fw_ver))
        {
            hid_ep->register_option(RS2_OPTION_MOTION_MODULE_TEMPERATURE,
                                    std::make_shared<motion_module_temperature_option>(*hid_ep));
            hid_ep->register_pixel_format(pf_gpio_timestamp);
        }

        return hid_ep;
    }

    std::shared_ptr<auto_exposure_mechanism> ds5_motion::register_auto_exposure_options(uvc_sensor* uvc_ep, const platform::extension_unit* fisheye_xu)
    {
        auto gain_option =  std::make_shared<uvc_pu_option>(*uvc_ep, RS2_OPTION_GAIN);

        auto exposure_option =  std::make_shared<uvc_xu_option<uint16_t>>(*uvc_ep,
                *fisheye_xu,
                librealsense::ds::FISHEYE_EXPOSURE, "Exposure time of Fisheye camera");

        auto ae_state = std::make_shared<auto_exposure_state>();
        auto auto_exposure = std::make_shared<auto_exposure_mechanism>(*gain_option, *exposure_option, *ae_state);

        auto auto_exposure_option = std::make_shared<enable_auto_exposure_option>(uvc_ep,
                                                                                  auto_exposure,
                                                                                  ae_state,
                                                                                  option_range{0, 1, 1, 1});

        uvc_ep->register_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE,auto_exposure_option);

        uvc_ep->register_option(RS2_OPTION_AUTO_EXPOSURE_MODE,
                                std::make_shared<auto_exposure_mode_option>(auto_exposure,
                                                                            ae_state,
                                                                            option_range{0, 2, 1, 0},
                                                                            std::map<float, std::string>{{0.f, "Static"},
                                                                                                         {1.f, "Anti-Flicker"},
                                                                                                         {2.f, "Hybrid"}}));
        uvc_ep->register_option(RS2_OPTION_AUTO_EXPOSURE_ANTIFLICKER_RATE,
                                std::make_shared<auto_exposure_antiflicker_rate_option>(auto_exposure,
                                                                                        ae_state,
                                                                                        option_range{50, 60, 10, 60},
                                                                                        std::map<float, std::string>{{50.f, "50Hz"},
                                                                                                                     {60.f, "60Hz"}}));


        uvc_ep->register_option(RS2_OPTION_GAIN,
                                    std::make_shared<auto_disabling_control>(
                                    gain_option,
                                    auto_exposure_option));

        uvc_ep->register_option(RS2_OPTION_EXPOSURE,
                                    std::make_shared<auto_disabling_control>(
                                    exposure_option,
                                    auto_exposure_option));

        return auto_exposure;
    }

    ds5_motion::ds5_motion(const platform::backend& backend,
                           const platform::backend_device_group& group)
        : ds5_device(backend, group)
    {
        using namespace ds;

        _fisheye_intrinsics_raw = [this]() { return get_raw_fisheye_intrinsics_table(); };
        _fisheye_extrinsics_raw = [this]() { return get_raw_fisheye_extrinsics_table(); };
        _motion_module_extrinsics_raw = [this]() { return get_motion_module_calibration_table().imu_to_fisheye; };
        _accel_intrinsics = [this](){ return get_motion_module_calibration_table().accel_intrinsics; };
        _gyro_intrinsics = [this](){ return get_motion_module_calibration_table().gyro_intrinsics; };

        std::string motion_module_fw_version = "";
        if (_fw_version >= firmware_version("5.5.8.0"))
        {
            motion_module_fw_version = _hw_monitor->get_firmware_version_string(GVD, motion_module_fw_version_offset);
        }

        auto fisheye_infos = filter_by_mi(group.uvc_devices, 3);
        if (fisheye_infos.size() != 1)
            throw invalid_value_exception("RS450 model is expected to include a single fish-eye device!");

        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_backup(new ds5_timestamp_reader(backend.create_time_service()));

        auto fisheye_ep = std::make_shared<ds5_fisheye_sensor>(this, backend.create_uvc_device(fisheye_infos.front()),
                                                    std::unique_ptr<frame_timestamp_reader>(new ds5_timestamp_reader_from_metadata(std::move(ds5_timestamp_reader_backup))),
                                                    backend.create_time_service());

        fisheye_ep->register_xu(fisheye_xu); // make sure the XU is initialized everytime we power the camera
        fisheye_ep->register_pixel_format(pf_raw8);
        fisheye_ep->register_pixel_format(pf_fe_raw8_unpatched_kernel); // W/O for unpatched kernel

        if (_fw_version >= firmware_version("5.6.3.0")) // Create Auto Exposure controls from FW version 5.6.3.0
        {
            auto fisheye_auto_exposure = register_auto_exposure_options(fisheye_ep.get(), &fisheye_xu);
            fisheye_ep->set_roi_method(std::make_shared<fisheye_auto_exposure_roi_method>(fisheye_auto_exposure));
        }
        else
        {
            fisheye_ep->register_option(RS2_OPTION_GAIN,
                                        std::make_shared<uvc_pu_option>(*fisheye_ep.get(),
                                                                        RS2_OPTION_GAIN));
            fisheye_ep->register_option(RS2_OPTION_EXPOSURE,
                                        std::make_shared<uvc_xu_option<uint16_t>>(*fisheye_ep.get(),
                                                                                  fisheye_xu,
                                                                                  librealsense::ds::FISHEYE_EXPOSURE,
                                                                                  "Exposure time of Fisheye camera"));
        }

        // Metadata registration
        fisheye_ep->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP,   make_uvc_header_parser(&platform::uvc_header::timestamp));
        fisheye_ep->register_metadata(RS2_FRAME_METADATA_AUTO_EXPOSURE,     make_additional_data_parser(&frame_additional_data::fisheye_ae_mode));

        // attributes of md_capture_timing
        auto md_prop_offset = offsetof(metadata_raw, mode) +
                   offsetof(md_fisheye_mode, fisheye_mode) +
                   offsetof(md_fisheye_normal_mode, intel_capture_timing);

        fisheye_ep->register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER,     make_attribute_parser(&md_capture_timing::frame_counter, md_capture_timing_attributes::frame_counter_attribute,md_prop_offset));
        fisheye_ep->register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP, make_rs400_sensor_ts_parser(make_uvc_header_parser(&platform::uvc_header::timestamp),
                make_attribute_parser(&md_capture_timing::sensor_timestamp, md_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset)));

        // attributes of md_capture_stats
        md_prop_offset = offsetof(metadata_raw, mode) +
                offsetof(md_fisheye_mode, fisheye_mode) +
                offsetof(md_fisheye_normal_mode, intel_capture_stats);

        // attributes of md_capture_stats
        md_prop_offset = offsetof(metadata_raw, mode) +
                offsetof(md_fisheye_mode, fisheye_mode) +
                offsetof(md_fisheye_normal_mode, intel_configuration);

        fisheye_ep->register_metadata((rs2_frame_metadata)RS2_FRAME_METADATA_HW_TYPE,   make_attribute_parser(&md_configuration::hw_type,    md_configuration_attributes::hw_type_attribute, md_prop_offset));
        fisheye_ep->register_metadata((rs2_frame_metadata)RS2_FRAME_METADATA_SKU_ID,    make_attribute_parser(&md_configuration::sku_id,     md_configuration_attributes::sku_id_attribute, md_prop_offset));
        fisheye_ep->register_metadata((rs2_frame_metadata)RS2_FRAME_METADATA_FORMAT,    make_attribute_parser(&md_configuration::format,     md_configuration_attributes::format_attribute, md_prop_offset));
        fisheye_ep->register_metadata((rs2_frame_metadata)RS2_FRAME_METADATA_WIDTH,     make_attribute_parser(&md_configuration::width,      md_configuration_attributes::width_attribute, md_prop_offset));
        fisheye_ep->register_metadata((rs2_frame_metadata)RS2_FRAME_METADATA_HEIGHT,    make_attribute_parser(&md_configuration::height,     md_configuration_attributes::height_attribute, md_prop_offset));

        // attributes of md_fisheye_control
        md_prop_offset = offsetof(metadata_raw, mode) +
                offsetof(md_fisheye_mode, fisheye_mode) +
                offsetof(md_fisheye_normal_mode, intel_fisheye_control);

        fisheye_ep->register_metadata(RS2_FRAME_METADATA_GAIN_LEVEL,        make_attribute_parser(&md_fisheye_control::manual_gain, md_depth_control_attributes::gain_attribute, md_prop_offset));
        fisheye_ep->register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE,   make_attribute_parser(&md_fisheye_control::manual_exposure, md_depth_control_attributes::exposure_attribute, md_prop_offset));

        // Add fisheye endpoint
        _fisheye_device_idx = add_sensor(fisheye_ep);

        fisheye_ep->set_pose(lazy<pose>([this](){return get_device_position(_fisheye_device_idx);}));

        // Add hid endpoint
        auto hid_ep = create_hid_device(backend, group.hid_devices, _fw_version);
        _motion_module_device_idx = add_sensor(hid_ep);

        try
        {
            hid_ep->register_option(RS2_OPTION_ENABLE_MOTION_CORRECTION,
                                    std::make_shared<enable_motion_correction>(hid_ep.get(),
                                                                               *_accel_intrinsics,
                                                                               *_gyro_intrinsics,
                                                                               option_range{0, 1, 1, 1}));
        }
        catch (const std::exception& ex)
        {
            LOG_ERROR("Motion Device is not calibrated! Motion Data Correction will not be available! Error: " << ex.what());
        }

        hid_ep->set_pose(lazy<pose>([this](){return get_device_position(_motion_module_device_idx); }));

        if (!motion_module_fw_version.empty())
            register_info(RS2_CAMERA_INFO_FIRMWARE_VERSION, motion_module_fw_version);

    }
}
