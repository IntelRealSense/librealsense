// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "ds5-motion.h"

#include <mutex>
#include <chrono>
#include <vector>
#include <map>
#include <iterator>
#include <cstddef>

#include "device.h"
#include "context.h"
#include "image.h"
#include "metadata-parser.h"

#include "ds5-timestamp.h"
#include "ds5-options.h"
#include "ds5-private.h"
#include "core/motion.h"
#include "stream.h"
#include "environment.h"
#include "proc/motion-transform.h"
#include "proc/auto-exposure-processor.h"

namespace librealsense
{
    const std::map<uint32_t, rs2_format> fisheye_fourcc_to_rs2_format = {
        {rs_fourcc('R','A','W','8'), RS2_FORMAT_RAW8},
        {rs_fourcc('G','R','E','Y'), RS2_FORMAT_RAW8},
    };
    const std::map<uint32_t, rs2_stream> fisheye_fourcc_to_rs2_stream = {
        {rs_fourcc('R','A','W','8'), RS2_STREAM_FISHEYE},
        {rs_fourcc('G','R','E','Y'), RS2_STREAM_FISHEYE},
    };

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

    class ds5_hid_sensor : public synthetic_sensor,
                           public motion_sensor
    {
    public:
        explicit ds5_hid_sensor(std::string name,
            std::shared_ptr<sensor_base> sensor,
            device* device,
            ds5_motion* owner)
            : synthetic_sensor(name, sensor, device),
            _owner(owner)
        {}

        rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream stream) const
        {
            return _owner->get_motion_intrinsics(stream);
        }

        stream_profiles init_stream_profiles() override
        {
            auto lock = environment::get_instance().get_extrinsics_graph().lock();
            auto results = synthetic_sensor::init_stream_profiles();

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
                    assert(motion);
                    auto st = p->get_stream_type();
                    motion->set_intrinsics([this, st]() { return get_motion_intrinsics(st); });
                }
            }

            return results;
        }

    private:
        const ds5_motion* _owner;
    };

    class ds5_fisheye_sensor : public synthetic_sensor, public video_sensor_interface, public roi_sensor_base, public fisheye_sensor
    {
    public:
        explicit ds5_fisheye_sensor(std::shared_ptr<sensor_base> sensor,
            device* device,
            ds5_motion* owner)
            : synthetic_sensor("Wide FOV Camera", sensor, device, fisheye_fourcc_to_rs2_format, fisheye_fourcc_to_rs2_stream),
            _owner(owner)
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

            auto results = synthetic_sensor::init_stream_profiles();
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

        std::shared_ptr<uvc_sensor> get_raw_sensor()
        {
            auto uvc_raw_sensor = As<uvc_sensor, sensor_base>(get_raw_sensor());
            return uvc_raw_sensor;
        }
    private:
        const ds5_motion* _owner;
    };

    rs2_motion_device_intrinsic ds5_motion::get_motion_intrinsics(rs2_stream stream) const
    {
        if (stream == RS2_STREAM_ACCEL)
            return create_motion_intrinsics(**_accel_intrinsic);

        if (stream == RS2_STREAM_GYRO)
            return create_motion_intrinsics(**_gyro_intrinsic);

        throw std::runtime_error(to_string() << "Motion Intrinsics unknown for stream " << rs2_stream_to_string(stream) << "!");
    }

    std::shared_ptr<synthetic_sensor> ds5_motion::create_hid_device(std::shared_ptr<context> ctx,
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

        // Dynamically populate the supported HID profiles according to the selected IMU module
        std::vector<odr> accel_fps_rates;
        std::map<unsigned, unsigned> fps_and_frequency_map;
        if (ds::d400_caps::CAP_BMI_085 && _device_capabilities)
            accel_fps_rates = { odr::IMU_FPS_100,odr::IMU_FPS_200 };
        else // Applies to BMI_055 and unrecognized sensors
            accel_fps_rates = { odr::IMU_FPS_63,odr::IMU_FPS_250 };

        for (auto&& elem : accel_fps_rates)
        {
            sensor_name_and_hid_profiles.push_back({ accel_sensor_name, { RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL, 0, 1, 1, static_cast<uint16_t>(elem)} });
            fps_and_frequency_map.emplace(unsigned(elem), hid_fps_translation.at(elem));
        }
        fps_and_sampling_frequency_per_rs2_stream[RS2_STREAM_ACCEL] = fps_and_frequency_map;

        auto raw_hid_ep = std::make_shared<hid_sensor>(ctx->get_backend().create_hid_device(all_hid_infos.front()),
            std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(iio_hid_ts_reader), _tf_keeper, enable_global_time_option)),
            std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(custom_hid_ts_reader), _tf_keeper, enable_global_time_option)),
            fps_and_sampling_frequency_per_rs2_stream,
            sensor_name_and_hid_profiles,
            this);

        auto hid_ep = std::make_shared<ds5_hid_sensor>("Motion Module", raw_hid_ep, this, this);

        hid_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);

        // register pre-processing
        bool enable_imu_correction = false;
        std::shared_ptr<enable_motion_correction> mm_correct_opt = nullptr;

        //  Motion intrinsic calibration presents is a prerequisite for motion correction.
        try
        {
            // Writing to log to dereference underlying structure
            LOG_INFO("Accel Sensitivity:" << (**_accel_intrinsic).sensitivity);
            LOG_INFO("Gyro Sensitivity:" << (**_gyro_intrinsic).sensitivity);

            mm_correct_opt = std::make_shared<enable_motion_correction>(hid_ep.get(),
                option_range{ 0, 1, 1, 1 });
            hid_ep->register_option(RS2_OPTION_ENABLE_MOTION_CORRECTION, mm_correct_opt);
        }
        catch (...) {}

        hid_ep->register_processing_block(
            { {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL} },
            { {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL} },
            [&, mm_correct_opt]() { return std::make_shared<acceleration_transform>(_mm_calib, mm_correct_opt);
        });

        hid_ep->register_processing_block(
            { {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO} },
            { {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO} },
            [&, mm_correct_opt]() { return std::make_shared<gyroscope_transform>(_mm_calib, mm_correct_opt);
        });

        uint16_t pid = static_cast<uint16_t>(strtoul(all_hid_infos.front().pid.data(), nullptr, 16));

        if ((camera_fw_version >= firmware_version(custom_sensor_fw_ver)) &&
                (!val_in_range(pid, { ds::RS400_IMU_PID, ds::RS435I_PID, ds::RS430I_PID, ds::RS465_PID, ds::RS405_PID, ds::RS455_PID })))
        {
            hid_ep->register_option(RS2_OPTION_MOTION_MODULE_TEMPERATURE,
                                    std::make_shared<motion_module_temperature_option>(*raw_hid_ep));
        }

        return hid_ep;
    }

    std::shared_ptr<auto_exposure_mechanism> ds5_motion::register_auto_exposure_options(synthetic_sensor* ep, const platform::extension_unit* fisheye_xu)
    {
        auto uvc_raw_sensor = As<uvc_sensor, sensor_base>(ep->get_raw_sensor());
        auto gain_option =  std::make_shared<uvc_pu_option>(*uvc_raw_sensor, RS2_OPTION_GAIN);

        auto exposure_option =  std::make_shared<uvc_xu_option<uint16_t>>(*uvc_raw_sensor,
                *fisheye_xu,
                librealsense::ds::FISHEYE_EXPOSURE, "Exposure time of Fisheye camera");

        auto ae_state = std::make_shared<auto_exposure_state>();
        auto auto_exposure = std::make_shared<auto_exposure_mechanism>(*gain_option, *exposure_option, *ae_state);

        auto auto_exposure_option = std::make_shared<enable_auto_exposure_option>(ep,
                                                                                  auto_exposure,
                                                                                  ae_state,
                                                                                  option_range{0, 1, 1, 1});

        ep->register_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE,auto_exposure_option);

        ep->register_option(RS2_OPTION_AUTO_EXPOSURE_MODE,
                                std::make_shared<auto_exposure_mode_option>(auto_exposure,
                                                                            ae_state,
                                                                            option_range{0, 2, 1, 0},
                                                                            std::map<float, std::string>{{0.f, "Static"},
                                                                                                         {1.f, "Anti-Flicker"},
                                                                                                         {2.f, "Hybrid"}}));
        ep->register_option(RS2_OPTION_AUTO_EXPOSURE_CONVERGE_STEP,
                                std::make_shared<auto_exposure_step_option>(auto_exposure,
                                                                            ae_state,
                                                                            option_range{ 0.1f, 1.0f, 0.1f, ae_step_default_value }));
        ep->register_option(RS2_OPTION_POWER_LINE_FREQUENCY,
                                std::make_shared<auto_exposure_antiflicker_rate_option>(auto_exposure,
                                                                                        ae_state,
                                                                                        option_range{50, 60, 10, 60},
                                                                                        std::map<float, std::string>{{50.f, "50Hz"},
                                                                                                                     {60.f, "60Hz"}}));

        ep->register_option(RS2_OPTION_GAIN,
                                    std::make_shared<auto_disabling_control>(
                                    gain_option,
                                    auto_exposure_option));

        ep->register_option(RS2_OPTION_EXPOSURE,
                                    std::make_shared<auto_disabling_control>(
                                    exposure_option,
                                    auto_exposure_option));

        ep->register_processing_block(
            { {RS2_FORMAT_RAW8}},
            { {RS2_FORMAT_RAW8, RS2_STREAM_FISHEYE} },
            [auto_exposure_option]() {
                return std::make_shared<auto_exposure_processor>(RS2_STREAM_FISHEYE, *auto_exposure_option);
            }
        );
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

        _mm_calib = std::make_shared<mm_calib_handler>(_hw_monitor,_device_capabilities);

        _accel_intrinsic = std::make_shared<lazy<ds::imu_intrinsic>>([this]() { return _mm_calib->get_intrinsic(RS2_STREAM_ACCEL); });
        _gyro_intrinsic = std::make_shared<lazy<ds::imu_intrinsic>>([this]() { return _mm_calib->get_intrinsic(RS2_STREAM_GYRO); });
        // D435i to use predefined values extrinsics
        _depth_to_imu = std::make_shared<lazy<rs2_extrinsics>>([this]() { return _mm_calib->get_extrinsic(RS2_STREAM_ACCEL); });

        initialize_fisheye_sensor(ctx,group);

        // Make sure all MM streams are positioned with the same extrinsics
        environment::get_instance().get_extrinsics_graph().register_extrinsics(*_depth_stream, *_accel_stream, _depth_to_imu);
        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_accel_stream, *_gyro_stream);
        register_stream_to_extrinsic_group(*_gyro_stream, 0);
        register_stream_to_extrinsic_group(*_accel_stream, 0);

        // Try to add HID endpoint
        auto hid_ep = create_hid_device(ctx, group.hid_devices, _fw_version);
        if (hid_ep)
        {
            _motion_module_device_idx = static_cast<uint8_t>(add_sensor(hid_ep));

            // HID metadata attributes
            hid_ep->get_raw_sensor()->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_hid_header_parser(&platform::hid_header::timestamp));
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
        auto raw_fisheye_ep = std::make_shared<uvc_sensor>("FishEye Sensor", backend.create_uvc_device(fisheye_infos.front()),
                                std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(ds5_timestamp_reader_metadata), _tf_keeper, enable_global_time_option)), this);
        auto fisheye_ep = std::make_shared<ds5_fisheye_sensor>(raw_fisheye_ep, this, this);

        fisheye_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);
        raw_fisheye_ep->register_xu(fisheye_xu); // make sure the XU is initialized everytime we power the camera

        if (_fw_version >= firmware_version("5.6.3.0")) // Create Auto Exposure controls from FW version 5.6.3.0
        {
            auto fisheye_auto_exposure = register_auto_exposure_options(fisheye_ep.get(), &fisheye_xu);
            fisheye_ep->set_roi_method(std::make_shared<fisheye_auto_exposure_roi_method>(fisheye_auto_exposure));
        }
        else
        {
            fisheye_ep->register_option(RS2_OPTION_GAIN,
                                        std::make_shared<uvc_pu_option>(*raw_fisheye_ep.get(),
                                                                        RS2_OPTION_GAIN));
            fisheye_ep->register_option(RS2_OPTION_EXPOSURE,
                                        std::make_shared<uvc_xu_option<uint16_t>>(*raw_fisheye_ep.get(),
                                                                                  fisheye_xu,
                                                                                  librealsense::ds::FISHEYE_EXPOSURE,
                                                                                  "Exposure time of Fisheye camera"));
        }

        // Metadata registration
        raw_fisheye_ep->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP,   make_uvc_header_parser(&platform::uvc_header::timestamp));
        raw_fisheye_ep->register_metadata(RS2_FRAME_METADATA_AUTO_EXPOSURE,     make_additional_data_parser(&frame_additional_data::fisheye_ae_mode));

        // attributes of md_capture_timing
        auto md_prop_offset = offsetof(metadata_raw, mode) +
                   offsetof(md_fisheye_mode, fisheye_mode) +
                   offsetof(md_fisheye_normal_mode, intel_capture_timing);

        raw_fisheye_ep->register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER,     make_attribute_parser(&md_capture_timing::frame_counter, md_capture_timing_attributes::frame_counter_attribute,md_prop_offset));
        raw_fisheye_ep->register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP, make_rs400_sensor_ts_parser(make_uvc_header_parser(&platform::uvc_header::timestamp),
                make_attribute_parser(&md_capture_timing::sensor_timestamp, md_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset)));

        // attributes of md_capture_stats
        md_prop_offset = offsetof(metadata_raw, mode) +
                offsetof(md_fisheye_mode, fisheye_mode) +
                offsetof(md_fisheye_normal_mode, intel_capture_stats);

        // attributes of md_capture_stats
        md_prop_offset = offsetof(metadata_raw, mode) +
                offsetof(md_fisheye_mode, fisheye_mode) +
                offsetof(md_fisheye_normal_mode, intel_configuration);

        raw_fisheye_ep->register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_HW_TYPE,   make_attribute_parser(&md_configuration::hw_type,    md_configuration_attributes::hw_type_attribute, md_prop_offset));
        raw_fisheye_ep->register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_SKU_ID,    make_attribute_parser(&md_configuration::sku_id,     md_configuration_attributes::sku_id_attribute, md_prop_offset));
        raw_fisheye_ep->register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_FORMAT,    make_attribute_parser(&md_configuration::format,     md_configuration_attributes::format_attribute, md_prop_offset));
        raw_fisheye_ep->register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_WIDTH,     make_attribute_parser(&md_configuration::width,      md_configuration_attributes::width_attribute, md_prop_offset));
        raw_fisheye_ep->register_metadata((rs2_frame_metadata_value)RS2_FRAME_METADATA_HEIGHT,    make_attribute_parser(&md_configuration::height,     md_configuration_attributes::height_attribute, md_prop_offset));

        // attributes of md_fisheye_control
        md_prop_offset = offsetof(metadata_raw, mode) +
                offsetof(md_fisheye_mode, fisheye_mode) +
                offsetof(md_fisheye_normal_mode, intel_fisheye_control);

        raw_fisheye_ep->register_metadata(RS2_FRAME_METADATA_GAIN_LEVEL,        make_attribute_parser(&md_fisheye_control::manual_gain, md_depth_control_attributes::gain_attribute, md_prop_offset));
        raw_fisheye_ep->register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE,   make_attribute_parser(&md_fisheye_control::manual_exposure, md_depth_control_attributes::exposure_attribute, md_prop_offset));

        // Add fisheye endpoint
        _fisheye_device_idx = add_sensor(fisheye_ep);
    }

    mm_calib_handler::mm_calib_handler(std::shared_ptr<hw_monitor> hw_monitor, ds::d400_caps dev_cap) :
        _hw_monitor(hw_monitor), _dev_cap(dev_cap)
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
            catch(const std::exception&)
            {
                LOG_WARNING("IMU Calibration is not available, see the previous message");
            }

            std::shared_ptr<mm_calib_parser> prs = nullptr;
            switch (calib_id)
            {
                case ds::dm_v2_eeprom_id: // DM V2 id
                    prs = std::make_shared<dm_v2_imu_calib_parser>(raw, _dev_cap, valid); break;
                case ds::tm1_eeprom_id: // TM1 id
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
