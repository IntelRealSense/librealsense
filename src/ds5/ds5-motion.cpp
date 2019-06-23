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
#include "ds5-private.h"
#include "ds5-motion.h"
#include "core/motion.h"
#include "stream.h"
#include "environment.h"

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

    class ds5_hid_sensor : public hid_sensor
    {
    public:
        explicit ds5_hid_sensor(ds5_motion* owner, std::shared_ptr<platform::hid_device> hid_device,
            std::unique_ptr<frame_timestamp_reader> hid_iio_timestamp_reader,
            std::unique_ptr<frame_timestamp_reader> custom_hid_timestamp_reader,
            std::map<rs2_stream, std::map<unsigned, unsigned>> fps_and_sampling_frequency_per_rs2_stream,
            std::vector<std::pair<std::string, stream_profile>> sensor_name_and_hid_profiles)
            : hid_sensor(hid_device, move(hid_iio_timestamp_reader), move(custom_hid_timestamp_reader),
                fps_and_sampling_frequency_per_rs2_stream, sensor_name_and_hid_profiles, owner), _owner(owner)
        {
        }

        rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream stream) const
        {
            return _owner->get_motion_intrinsics(stream);
        }

        stream_profiles init_stream_profiles() override
        {
            auto lock = environment::get_instance().get_extrinsics_graph().lock();
            auto results = hid_sensor::init_stream_profiles();

            for (auto p : results)
            {
                // Register stream types
                if (p->get_stream_type() == RS2_STREAM_ACCEL)
                    assign_stream(_owner->_accel_stream, p);
                if (p->get_stream_type() == RS2_STREAM_GYRO)
                    assign_stream(_owner->_gyro_stream, p);

                //set motion intrinsics
                if (p->get_stream_type() == RS2_STREAM_ACCEL || p->get_stream_type() == RS2_STREAM_GYRO)
                {
                    auto motion = dynamic_cast<motion_stream_profile_interface*>(p.get());
                    assert(motion); //Expecting to succeed for motion stream (since we are under the "if")
                    auto st = p->get_stream_type();
                    motion->set_intrinsics([this, st]() { return get_motion_intrinsics(st); });
                }
            }

            return results;
        }

    private:
        const ds5_motion* _owner;
    };

    class ds5_fisheye_sensor : public uvc_sensor, public video_sensor_interface, public roi_sensor_base
    {
    public:
        explicit ds5_fisheye_sensor(ds5_motion* owner, std::shared_ptr<platform::uvc_device> uvc_device,
            std::unique_ptr<frame_timestamp_reader> timestamp_reader)
            : uvc_sensor("Wide FOV Camera", uvc_device, move(timestamp_reader), owner), _owner(owner)
        {}

        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override
        {
            return get_intrinsic_by_resolution(
                *_owner->_fisheye_calibration_table_raw,
                ds::calibration_table_id::fisheye_calibration_id,
                profile.width, profile.height);
        }

        stream_profiles init_stream_profiles() override
        {
            auto lock = environment::get_instance().get_extrinsics_graph().lock();

            auto results = uvc_sensor::init_stream_profiles();
            for (auto p : results)
            {
                // Register stream types
                if (p->get_stream_type() == RS2_STREAM_FISHEYE)
                    assign_stream(_owner->_fisheye_stream, p);

                auto video = dynamic_cast<video_stream_profile_interface*>(p.get());

                auto profile = to_profile(p.get());
                std::weak_ptr<ds5_fisheye_sensor> wp =
                    std::dynamic_pointer_cast<ds5_fisheye_sensor>(this->shared_from_this());
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
    private:
        const ds5_motion* _owner;
    };

    rs2_motion_device_intrinsic ds5_motion::get_motion_intrinsics(rs2_stream stream) const
    {
        if (stream == RS2_STREAM_ACCEL)
            return create_motion_intrinsics(*_accel_intrinsic);

        if (stream == RS2_STREAM_GYRO)
            return create_motion_intrinsics(*_gyro_intrinsic);

        throw std::runtime_error(to_string() << "Motion Intrinsics unknown for stream " << rs2_stream_to_string(stream) << "!");
    }

    std::shared_ptr<hid_sensor> ds5_motion::create_hid_device(std::shared_ptr<context> ctx,
                                                                const std::vector<platform::hid_device_info>& all_hid_infos,
                                                                const firmware_version& camera_fw_version)
    {
        if (all_hid_infos.empty())
        {
            LOG_WARNING("No HID info provided, IMU is disabled");
            return nullptr;
        }

        static const char* custom_sensor_fw_ver = "5.6.0.0";

        std::unique_ptr<frame_timestamp_reader> iio_hid_ts_reader(new iio_hid_timestamp_reader());
        std::unique_ptr<frame_timestamp_reader> custom_hid_ts_reader(new ds5_custom_hid_timestamp_reader());
        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());
        auto hid_ep = std::make_shared<ds5_hid_sensor>(this, ctx->get_backend().create_hid_device(all_hid_infos.front()),
                                                        std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(iio_hid_ts_reader), _tf_keeper, enable_global_time_option)),
                                                        std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(custom_hid_ts_reader), _tf_keeper, enable_global_time_option)),
                                                        fps_and_sampling_frequency_per_rs2_stream,
                                                        sensor_name_and_hid_profiles);

        hid_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);
        hid_ep->register_pixel_format(pf_accel_axes);
        hid_ep->register_pixel_format(pf_gyro_axes);

        uint16_t pid = static_cast<uint16_t>(strtoul(all_hid_infos.front().pid.data(), nullptr, 16));

        if ((camera_fw_version >= firmware_version(custom_sensor_fw_ver)) && (!val_in_range(pid, { ds::RS400_IMU_PID, ds::RS435I_PID, ds::RS430I_PID })))
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
        uvc_ep->register_option(RS2_OPTION_AUTO_EXPOSURE_CONVERGE_STEP,
                                std::make_shared<auto_exposure_step_option>(auto_exposure,
                                                                            ae_state,
                                                                            option_range{ 0.1f, 1.0f, 0.1f, ae_step_default_value }));
        uvc_ep->register_option(RS2_OPTION_POWER_LINE_FREQUENCY,
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

    ds5_motion::ds5_motion(std::shared_ptr<context> ctx,
                           const platform::backend_device_group& group)
        : device(ctx, group), ds5_device(ctx, group),
          _fisheye_stream(new stream(RS2_STREAM_FISHEYE)),
          _accel_stream(new stream(RS2_STREAM_ACCEL)),
          _gyro_stream(new stream(RS2_STREAM_GYRO))
    {
        using namespace ds;

        _mm_calib = std::make_shared<mm_calib_handler>(_hw_monitor);

        _accel_intrinsic = [this]() { return _mm_calib->get_intrinsic(RS2_STREAM_ACCEL); };
        _gyro_intrinsic = [this]() { return _mm_calib->get_intrinsic(RS2_STREAM_GYRO); };

        std::string motion_module_fw_version = "";
        if (_fw_version >= firmware_version("5.5.8.0"))
        {
            std::vector<uint8_t> gvd_buff(HW_MONITOR_BUFFER_SIZE);
            _hw_monitor->get_gvd(gvd_buff.size(), gvd_buff.data(), GVD);
            motion_module_fw_version = _hw_monitor->get_firmware_version_string(gvd_buff, camera_fw_version_offset);
        }

        initialize_fisheye_sensor(ctx,group);

        // D435i to use predefined values extrinsics
        _depth_to_imu = std::make_shared<lazy<rs2_extrinsics>>([this]()
        {
            return _mm_calib->get_extrinsic(RS2_STREAM_ACCEL);
        });

        // Make sure all MM streams are positioned with the same extrinsics
        environment::get_instance().get_extrinsics_graph().register_extrinsics(*_depth_stream, *_accel_stream, _depth_to_imu);
        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_accel_stream, *_gyro_stream);
        register_stream_to_extrinsic_group(*_gyro_stream, 0);
        register_stream_to_extrinsic_group(*_accel_stream, 0);

        // Try to add hid endpoint
        auto hid_ep = create_hid_device(ctx, group.hid_devices, _fw_version);
        if (hid_ep)
        {
            _motion_module_device_idx = add_sensor(hid_ep);

            std::function<void(rs2_stream stream, frame_interface* fr, callback_invocation_holder callback)> align_imu_axes  = nullptr;

            // Perform basic IMU transformation to align orientation with Depth sensor CS.
            try
            {
                float3x3 imu_to_depth = _mm_calib->imu_to_depth_alignment();
                align_imu_axes = [imu_to_depth](rs2_stream stream, frame_interface* fr, callback_invocation_holder callback)
                {
                    if (fr->get_stream()->get_format() == RS2_FORMAT_MOTION_XYZ32F)
                    {
                        auto xyz = (float3*)(fr->get_frame_data());

                        // The IMU sensor orientation shall be aligned with depth sensor's coordinate system
                        *xyz = imu_to_depth * (*xyz);
                    }
                };
            }
            catch (const std::exception& ex){
                LOG_INFO("Motion Module - no extrinsic calibration, " << ex.what());
            }

            try
            {
                hid_ep->register_option(RS2_OPTION_ENABLE_MOTION_CORRECTION,
                    std::make_shared<enable_motion_correction>(hid_ep.get(),
                        *_accel_intrinsic,
                        *_gyro_intrinsic,
                        _depth_to_imu,
                        align_imu_axes, // The motion correction callback also includes the axes rotation routine
                        option_range{ 0, 1, 1, 1 }));
            }
            catch (const std::exception& ex)
            {
                LOG_INFO("Motion Module - no intrinsic calibration, " << ex.what());

                // transform IMU axes if supported
                hid_ep->register_on_before_frame_callback(align_imu_axes);
            }

            if ((!motion_module_fw_version.empty()) && ("255.255.255.255" != motion_module_fw_version))
                register_info(RS2_CAMERA_INFO_FIRMWARE_VERSION, motion_module_fw_version);

            // HID metadata attributes
            hid_ep->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_hid_header_parser(&platform::hid_header::timestamp));
        }
    }

    void ds5_motion::initialize_fisheye_sensor(std::shared_ptr<context> ctx, const platform::backend_device_group& group)
    {
        using namespace ds;

        auto fisheye_infos = filter_by_mi(group.uvc_devices, 3);
        fisheye_infos = filter_device_by_capability(fisheye_infos,d400_caps::CAP_FISHEYE_SENSOR);

        bool fe_dev_present = (fisheye_infos.size() == 1);
        bool fe_capability = (d400_caps::CAP_UNDEFINED == _device_capabilities) ?
            true :  !!(static_cast<uint32_t>(_device_capabilities&d400_caps::CAP_FISHEYE_SENSOR));

        // Motion module w/o FishEye sensor
        if (!(fe_dev_present | fe_capability)) return;

        // Inconsistent FW
        if (fe_dev_present ^ fe_capability)
            throw invalid_value_exception(to_string()
            << "Inconsistent HW/FW setup, FW FishEye capability = " << fe_capability
            << ", FishEye devices " <<  std::dec << fisheye_infos.size()
            << " while expecting " << fe_capability);

        _fisheye_calibration_table_raw = [this]()
        {
            return _mm_calib->get_fisheye_calib_raw();
        };

        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_backup(new ds5_timestamp_reader(environment::get_instance().get_time_service()));
        auto&& backend = ctx->get_backend();
        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_metadata(new ds5_timestamp_reader_from_metadata(std::move(ds5_timestamp_reader_backup)));
        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());
        auto fisheye_ep = std::make_shared<ds5_fisheye_sensor>(this, backend.create_uvc_device(fisheye_infos.front()),
                                std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(ds5_timestamp_reader_metadata), _tf_keeper, enable_global_time_option)));

        fisheye_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);
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

        fisheye_ep->register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_HW_TYPE,   make_attribute_parser(&md_configuration::hw_type,    md_configuration_attributes::hw_type_attribute, md_prop_offset));
        fisheye_ep->register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_SKU_ID,    make_attribute_parser(&md_configuration::sku_id,     md_configuration_attributes::sku_id_attribute, md_prop_offset));
        fisheye_ep->register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_FORMAT,    make_attribute_parser(&md_configuration::format,     md_configuration_attributes::format_attribute, md_prop_offset));
        fisheye_ep->register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_WIDTH,     make_attribute_parser(&md_configuration::width,      md_configuration_attributes::width_attribute, md_prop_offset));
        fisheye_ep->register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_HEIGHT,    make_attribute_parser(&md_configuration::height,     md_configuration_attributes::height_attribute, md_prop_offset));

        // attributes of md_fisheye_control
        md_prop_offset = offsetof(metadata_raw, mode) +
                offsetof(md_fisheye_mode, fisheye_mode) +
                offsetof(md_fisheye_normal_mode, intel_fisheye_control);

        fisheye_ep->register_metadata(RS2_FRAME_METADATA_GAIN_LEVEL,        make_attribute_parser(&md_fisheye_control::manual_gain, md_depth_control_attributes::gain_attribute, md_prop_offset));
        fisheye_ep->register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE,   make_attribute_parser(&md_fisheye_control::manual_exposure, md_depth_control_attributes::exposure_attribute, md_prop_offset));

        // Add fisheye endpoint
        _fisheye_device_idx = add_sensor(fisheye_ep);

        // Not applicable for TM1
        //_depth_to_fisheye = std::make_shared<lazy<rs2_extrinsics>>([this]()
        //{
        //    auto extr = get_fisheye_extrinsics_data(*_fisheye_extrinsics_raw);
        //    return from_pose(inverse(extr));
        //});

        //_fisheye_to_imu = std::make_shared<lazy<rs2_extrinsics>>([this]()
        //{
        //    auto fe_calib = (*_tm1_eeprom).calibration_table.calib_model.fe_calibration;

        //    auto rot = fe_calib.fisheye_to_imu.rotation;
        //    auto trans = fe_calib.fisheye_to_imu.translation;

        //    pose ex = { { rot(0,0), rot(1,0),rot(2,0),rot(0,1), rot(1,1),rot(2,1),rot(0,2), rot(1,2),rot(2,2) },
        //    { trans[0], trans[1], trans[2] } };

        //    return from_pose(ex);
        //});
    }

    mm_calib_handler::mm_calib_handler(std::shared_ptr<hw_monitor> hw_monitor) :  _hw_monitor(hw_monitor)
    {
        _imu_eeprom_raw = [this]() { return get_imu_eeprom_raw(); };

        _calib_parser = [this]() {

            std::vector<uint8_t> raw(ds::tm1_eeprom_size);
            uint16_t calib_id = ds::dm_v2_eeprom_id; //assume DM V2 IMU as default platform
            bool valid = false;

            try
            {
                raw = *_imu_eeprom_raw;
                calib_id = *reinterpret_cast<uint16_t*>(raw.data());
                valid = true;
            }
            catch(const std::exception& exc)
            {
                LOG_INFO("IMU EEPROM Read error: " << exc.what());
            }

            std::shared_ptr<mm_calib_parser> prs = nullptr;
            switch (calib_id)
            {
                case ds::dm_v2_eeprom_id: // DM V2 id
                    prs = std::make_shared<dm_v2_imu_calib_parser>(raw,valid); break;
                case ds::tm1_eeprom_id:// TM1 id
                    prs = std::make_shared<tm1_imu_calib_parser>(raw); break;
                default:
                    throw recoverable_exception(to_string() << "Motion Intrinsics unresolved - "
                                << ((valid)? "device is not calibrated" : "invalid calib type "),
                                RS2_EXCEPTION_TYPE_BACKEND);
            }
            return prs;
        };
    }

    std::vector<uint8_t> mm_calib_handler::get_imu_eeprom_raw() const
    {
        const int offset = 0;
        const int size = ds::eeprom_imu_table_size;
        command cmd(ds::MMER, offset, size);
        return _hw_monitor->send(cmd);
    }

    ds::imu_intrinsic mm_calib_handler::get_intrinsic(rs2_stream stream)
    {
        return (*_calib_parser)->get_intrinsic(stream);
    }

    rs2_extrinsics mm_calib_handler::get_extrinsic(rs2_stream stream)
    {
        return (*_calib_parser)->get_extrinsic_to(stream);
    }

    const std::vector<uint8_t> mm_calib_handler::get_fisheye_calib_raw()
    {
        auto fe_calib_table = (*(ds::check_calib<ds::tm1_eeprom>(*_imu_eeprom_raw))).calibration_table.calib_model.fe_calibration;
        uint8_t* fe_calib_ptr = reinterpret_cast<uint8_t*>(&fe_calib_table);
        return std::vector<uint8_t>(fe_calib_ptr, fe_calib_ptr+ ds::fisheye_calibration_table_size);
    }
}
