// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022-2024 Intel Corporation. All Rights Reserved.

#include "d500-safety.h"

#include <vector>
#include <map>
#include <cstddef>

#include "ds/ds-timestamp.h"
#include "ds/ds-options.h"
#include "d500-options.h"
#include "d500-info.h"
#include "d500-md.h"
#include "stream.h"
#include "backend.h"
#include "platform/platform-utils.h"
#include <src/fourcc.h>
#include <src/metadata-parser.h>
#include <src/core/advanced_mode.h>

namespace librealsense
{
    const std::map<uint32_t, rs2_format> safety_fourcc_to_rs2_format = {
        {rs_fourcc('G','R','E','Y'), RS2_FORMAT_RAW8}
    };
    const std::map<uint32_t, rs2_stream> safety_fourcc_to_rs2_stream = {
        {rs_fourcc('G','R','E','Y'), RS2_STREAM_SAFETY}
    };

    d500_safety::d500_safety(std::shared_ptr< const d500_info > const & dev_info  )
        : device( dev_info ), d500_device( dev_info ),
        _safety_stream(new stream(RS2_STREAM_SAFETY))
    {
        using namespace ds;

        const uint32_t safety_stream_mi = 11;
        auto safety_devs_info = filter_by_mi( dev_info->get_group().uvc_devices, safety_stream_mi);
        
        if (safety_devs_info.size() != 1)
            throw invalid_value_exception(rsutils::string::from() << "RS5XX models with Safety are expected to include a single safety device! - "
                << safety_devs_info.size() << " found");

        auto safety_ep = create_safety_device( dev_info->get_context(), safety_devs_info );
        _safety_device_idx = add_sensor( safety_ep );
    }

    std::shared_ptr<synthetic_sensor> d500_safety::create_safety_device(std::shared_ptr<context> ctx,
        const std::vector<platform::uvc_device_info>& safety_devices_info)
    {
        using namespace ds;

        register_stream_to_extrinsic_group(*_safety_stream, 0);
        environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_depth_stream, *_safety_stream);

        std::unique_ptr< frame_timestamp_reader > ds_timestamp_reader_backup( new ds_timestamp_reader() );
        std::unique_ptr<frame_timestamp_reader> ds_timestamp_reader_metadata( new ds_timestamp_reader_from_metadata_safety(std::move(ds_timestamp_reader_backup)));

        auto enable_global_time_option = std::shared_ptr<global_time_option>(new global_time_option());

        auto raw_safety_ep = std::make_shared<uvc_sensor>("Raw Safety Device",
            get_backend()->create_uvc_device(safety_devices_info.front()),
            std::unique_ptr<frame_timestamp_reader>(new global_timestamp_reader(std::move(ds_timestamp_reader_metadata), _tf_keeper, enable_global_time_option)),
            this);

        raw_safety_ep->register_xu(safety_xu); // making sure the XU is initialized every time we power the camera

        auto safety_ep = std::make_shared<d500_safety_sensor>(this,
            raw_safety_ep,
            safety_fourcc_to_rs2_format,
            safety_fourcc_to_rs2_stream);

        safety_ep->register_option(RS2_OPTION_GLOBAL_TIME_ENABLED, enable_global_time_option);

        safety_ep->register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, safety_devices_info.front().device_path);

        // register options
        register_options(safety_ep, raw_safety_ep);

        // register metadata
        register_metadata(raw_safety_ep);

        // register processing blocks
        register_processing_blocks(safety_ep);
        
        return safety_ep;
    }

    void d500_safety::set_advanced_mode_device( ds_advanced_mode_base * advanced_mode )
    {
        _advanced_mode = advanced_mode;
        block_advanced_mode_if_needed( _safety_camera_oper_mode->query() );
    }

    void d500_safety::block_advanced_mode_if_needed( float val )
    {
        // Note - requirement to block advanced mode was removed to allow validation checks. Gated by FW only.
        //if( ! _advanced_mode )
        //    throw std::runtime_error( "Advanced mode device not set" );
        //
        //if( val == static_cast< float >( RS2_SAFETY_MODE_SERVICE ) )
        //    _advanced_mode->unblock();
        //else
        //    _advanced_mode->block( std::string( "Option can be set only in safety service mode" ) );
    }

    void d500_safety::register_options(std::shared_ptr<d500_safety_sensor> safety_ep, std::shared_ptr<uvc_sensor> raw_safety_sensor)
    {
        // Register safety preset active index option
        static const std::chrono::milliseconds safety_preset_change_timeout( 1000 );
        auto active_safety_preset = std::make_shared<ensure_set_xu_option<uint16_t>>(
            raw_safety_sensor,
            safety_xu,
            xu_id::SAFETY_PRESET_ACTIVE_INDEX,
            "Safety Preset Active Index",
            safety_preset_change_timeout);

        safety_ep->register_option(RS2_OPTION_SAFETY_PRESET_ACTIVE_INDEX, active_safety_preset);
        
        // Register operational mode option
        static const std::chrono::milliseconds safety_mode_change_timeout( 2000 );
        auto safety_camera_oper_mode = std::make_shared< cascade_option< ensure_set_xu_option< uint16_t > > >(
            raw_safety_sensor,
            safety_xu,
            xu_id::SAFETY_CAMERA_OPER_MODE,
            "Safety camera operational mode",
            std::map< float, std::string >{ { float( RS2_SAFETY_MODE_RUN ),     "Run" },
                                            { float( RS2_SAFETY_MODE_STANDBY ), "Standby" },
                                            { float( RS2_SAFETY_MODE_SERVICE ), "Service" } },
            safety_mode_change_timeout );

        safety_camera_oper_mode->add_observer( [this]( float val ) { block_advanced_mode_if_needed( val ); } );
        _safety_camera_oper_mode = safety_camera_oper_mode;

        safety_ep->register_option( RS2_OPTION_SAFETY_MODE, safety_camera_oper_mode );

        safety_ep->register_option(RS2_OPTION_SAFETY_MCU_TEMPERATURE,
            std::make_shared<temperature_option>(_hw_monitor, temperature_option::temperature_component::SMCU, "Temperature reading for Safety MCU"));

    }

    void d500_safety::gate_depth_option( rs2_option opt,
                                         synthetic_sensor & depth_sensor,
                                         const std::vector < std::tuple< std::shared_ptr< option >, float, std::string > > & options_and_reasons )
    {
        // Replcaes the option registered in depth sensor with a `gated_by_value_option` that proxies the original
        // options and can block setting it if conditions are not met ( safety not in service mode ).
        depth_sensor.register_option( opt,
                                      std::make_shared< gated_by_value_option >( depth_sensor.get_option_handler( opt ),
                                                                                 options_and_reasons ) );
    }

    void d500_safety::gate_depth_options()
    {
        auto& ds = get_depth_sensor();
        std::vector< std::tuple< std::shared_ptr< option >, float, std::string > > options_and_reasons
            = { std::make_tuple( _safety_camera_oper_mode,
                                 static_cast<float>(RS2_SAFETY_MODE_SERVICE),
                                 std::string("Option can be set only in safety service mode")) };

        gate_depth_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, ds, options_and_reasons);
        gate_depth_option(RS2_OPTION_EXPOSURE, ds, options_and_reasons);
        gate_depth_option(RS2_OPTION_GAIN, ds, options_and_reasons);
        gate_depth_option(RS2_OPTION_EMITTER_ALWAYS_ON, ds, options_and_reasons);
        gate_depth_option(RS2_OPTION_INTER_CAM_SYNC_MODE, ds, options_and_reasons);
        gate_depth_option(RS2_OPTION_DEPTH_UNITS, ds, options_and_reasons);
        gate_depth_option(RS2_OPTION_EMITTER_ENABLED, ds, options_and_reasons);
        gate_depth_option(RS2_OPTION_LASER_POWER, ds, options_and_reasons);
        gate_depth_option(RS2_OPTION_VISUAL_PRESET, ds, options_and_reasons);
    }

    void d500_safety::register_metadata(std::shared_ptr<uvc_sensor> raw_safety_ep)
    {
        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, 
            make_uvc_header_parser(&platform::uvc_header::timestamp));

        // attributes of md_safety_mode
        auto md_prop_offset = metadata_raw_mode_offset +
            offsetof(md_safety_mode, intel_safety_info);

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER, 
            make_attribute_parser(&md_safety_info::frame_counter, 
                md_safety_info_attributes::frame_counter_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_DEPTH_FRAME_COUNTER,
            make_attribute_parser(&md_safety_info::depth_frame_counter, 
                md_safety_info_attributes::depth_frame_counter_attribute, md_prop_offset));
        
        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP,
            make_attribute_parser(&md_safety_info::frame_timestamp, 
                md_safety_info_attributes::frame_timestamp_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_LEVEL1,
            make_attribute_parser(&md_safety_info::level1_signal, 
                md_safety_info_attributes::level1_signal_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_LEVEL1_ORIGIN,
            make_attribute_parser(&md_safety_info::level1_frame_counter_origin, 
                md_safety_info_attributes::level1_frame_counter_origin_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_LEVEL2,
            make_attribute_parser(&md_safety_info::level2_signal, 
                md_safety_info_attributes::level2_signal_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_LEVEL2_ORIGIN,
            make_attribute_parser(&md_safety_info::level2_frame_counter_origin, 
                md_safety_info_attributes::level2_frame_counter_origin_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_LEVEL1_VERDICT,
            make_attribute_parser(&md_safety_info::level1_verdict, 
                md_safety_info_attributes::level1_verdict_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_LEVEL2_VERDICT,
            make_attribute_parser(&md_safety_info::level2_verdict, 
                md_safety_info_attributes::level2_verdict_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_OPERATIONAL_MODE,
            make_attribute_parser(&md_safety_info::operational_mode,
                md_safety_info_attributes::operational_mode_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_VISION_VERDICT,
            make_attribute_parser(&md_safety_info::vision_safety_verdict,
                md_safety_info_attributes::vision_safety_verdict_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_HARA_EVENTS,
            make_attribute_parser( &md_safety_info::vision_hara_status,
                md_safety_info_attributes::vision_hara_status_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_PRESET_INTEGRITY,
            make_attribute_parser(&md_safety_info::safety_preset_integrity,
                md_safety_info_attributes::safety_preset_integrity_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_PRESET_ID_SELECTED,
            make_attribute_parser(&md_safety_info::safety_preset_id_selected,
                md_safety_info_attributes::safety_preset_id_selected_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_PRESET_ID_USED,
            make_attribute_parser(&md_safety_info::safety_preset_id_used,
                md_safety_info_attributes::safety_preset_id_used_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_SOC_FUSA_EVENTS,
            make_attribute_parser(&md_safety_info::soc_fusa_events, 
                md_safety_info_attributes::soc_fusa_events_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_SOC_FUSA_ACTION,
            make_attribute_parser(&md_safety_info::soc_fusa_action, 
                md_safety_info_attributes::soc_fusa_action_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_SOC_L0_COUNTER,
            make_attribute_parser(&md_safety_info::soc_l_0_counter,
                md_safety_info_attributes::soc_status_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_SOC_L0_RATE,
            make_attribute_parser(&md_safety_info::soc_l_0_rate,
                md_safety_info_attributes::soc_status_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_SOC_L1_COUNTER,
            make_attribute_parser(&md_safety_info::soc_l_1_counter,
                md_safety_info_attributes::soc_status_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_SOC_L1_RATE,
            make_attribute_parser(&md_safety_info::soc_l_1_rate,
                md_safety_info_attributes::soc_status_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_SOC_GMT_STATUS,
            make_attribute_parser(&md_safety_info::soc_gmt_status,
                md_safety_info_attributes::soc_status_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_SOC_HKR_CRITICAL_ERROR_GPIO,
            make_attribute_parser(&md_safety_info::soc_hkr_critical_error_gpio,
                md_safety_info_attributes::soc_status_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_SOC_MONITOR_L2_ERROR_TYPE,
            make_attribute_parser(&md_safety_info::soc_monitor_l2_error_type,
                md_safety_info_attributes::soc_status_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_SOC_MONITOR_L3_ERROR_TYPE,
            make_attribute_parser(&md_safety_info::soc_monitor_l3_error_type,
                md_safety_info_attributes::soc_status_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_MB_FUSA_EVENT,
            make_attribute_parser(&md_safety_info::mb_fusa_event, 
                md_safety_info_attributes::mb_fusa_event_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_MB_FUSA_ACTION,
            make_attribute_parser(&md_safety_info::mb_fusa_action, 
                md_safety_info_attributes::mb_fusa_action_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_MB_STATUS,
            make_attribute_parser(&md_safety_info::mb_status,
                md_safety_info_attributes::mb_status_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_SMCU_LIVELINESS,
            make_attribute_parser(&md_safety_info::smcu_liveliness,
                md_safety_info_attributes::smcu_liveliness_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_SMCU_STATE,
            make_attribute_parser(&md_safety_info::smcu_state,
                md_safety_info_attributes::smcu_state_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_NON_FUSA_GPIO,
            make_attribute_parser(&md_safety_info::non_fusa_gpio,
                md_safety_info_attributes::non_fusa_gpio_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_SMCU_DEBUG_STATUS_BITMASK,
            make_attribute_parser(&md_safety_info::smcu_status_bitmask,
                md_safety_info_attributes::smcu_debug_info_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_SMCU_DEBUG_INFO_INTERNAL_STATE,
            make_attribute_parser(&md_safety_info::smcu_internal_state,
                md_safety_info_attributes::smcu_debug_info_attribute, md_prop_offset));

        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_SAFETY_SMCU_DEBUG_INFO_BIST_STATUS,
            make_attribute_parser(&md_safety_info::smcu_bist_status,
                md_safety_info_attributes::smcu_debug_info_attribute, md_prop_offset));

        // calc CRC for safety MD payload, starting from "version" field (not including the uvc and md headers),
        // and without the crc field itself.
        raw_safety_ep->register_metadata(RS2_FRAME_METADATA_CRC,
            make_attribute_parser_with_crc(&md_safety_info::crc32,
                md_safety_info_attributes::crc32_attribute, md_prop_offset, offsetof(md_safety_info, version), offsetof(md_safety_info,crc32)));
    }

    void d500_safety::register_processing_blocks(std::shared_ptr<d500_safety_sensor> safety_ep)
    {
        safety_ep->register_processing_block(processing_block_factory::create_id_pbf(RS2_FORMAT_RAW8, RS2_STREAM_SAFETY));
    }


    stream_profiles d500_safety_sensor::init_stream_profiles()
    {
        auto lock = environment::get_instance().get_extrinsics_graph().lock();
        auto results = synthetic_sensor::init_stream_profiles();

        for (auto p : results)
        {
            // Register stream types
            if (p->get_stream_type() == RS2_STREAM_SAFETY)
                assign_stream(_owner->_safety_stream, p);

            auto&& video = dynamic_cast<video_stream_profile_interface*>(p.get());
            const auto&& profile = to_profile(p.get());

            std::weak_ptr<d500_safety_sensor> wp =
                std::dynamic_pointer_cast<d500_safety_sensor>(this->shared_from_this());
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

    rs2_intrinsics d500_safety_sensor::get_intrinsics(const stream_profile& profile) const
    {
        return rs2_intrinsics();
    }

    void d500_safety_sensor::set_safety_preset(int index, const rs2_safety_preset& sp) const
    {
        //calculate CRC
        auto computed_crc32 = rsutils::number::calc_crc32(reinterpret_cast<const uint8_t*>(&sp), sizeof(rs2_safety_preset));

        // prepare vector of data to be sent (header + sp)
        rs2_safety_preset_with_header data;
        uint16_t version = ((uint16_t)0x03 << 8) | 0x00;  // major=0x03, minor=0x00 --> ver = major.minor
        data.header = { version, static_cast<uint16_t>(ds::d500_calibration_table_id::safety_preset_id), 
            sizeof(rs2_safety_preset), computed_crc32 };
        data.safety_preset = sp;
        auto data_as_ptr = reinterpret_cast<const uint8_t*>(&data);

        // prepare command
        command cmd(ds::SAFETY_PRESET_WRITE);
        cmd.param1 = index;
        cmd.data.insert(cmd.data.end(), data_as_ptr, data_as_ptr + sizeof(data));
        cmd.require_response = false;

        // send command 
        _owner->_hw_monitor->send(cmd);
    }

    rs2_safety_preset d500_safety_sensor::get_safety_preset(int index) const
    {
        rs2_safety_preset_with_header* result;

        // prepare command
        command cmd(ds::SAFETY_PRESET_READ);
        cmd.require_response = true;
        cmd.param1 = index;

        // send command to device and get response (safety_preset entry + header)
        std::vector< uint8_t > response = _owner->_hw_monitor->send(cmd);
        if (response.size() < sizeof(rs2_safety_preset_with_header))
        {
            throw io_exception(rsutils::string::from() << "Safety preset read at index=" << index << " failed");
        }

        // cast response to safety_preset_with_header struct
        result = reinterpret_cast<rs2_safety_preset_with_header*>(response.data());

        // check CRC before returning result       
        auto computed_crc32 = rsutils::number::calc_crc32(response.data() + sizeof(rs2_safety_preset_header), sizeof(rs2_safety_preset));
        if (computed_crc32 != result->header.crc32)
        {
            throw invalid_value_exception(rsutils::string::from() << "invalid CRC value for index=" << index);
        }

        return result->safety_preset;
    }

    std::string d500_safety_sensor::safety_preset_to_json_string(rs2_safety_preset const& sp) const
    {
        rsutils::json json_data;
        auto&& rotation = json_data["safety_preset"]["platform_config"]["transformation_link"]["rotation"];
        rotation[0][0] = sp.platform_config.transformation_link.rotation.x.x;
        rotation[0][1] = sp.platform_config.transformation_link.rotation.x.y;
        rotation[0][2] = sp.platform_config.transformation_link.rotation.x.z;
        rotation[1][0] = sp.platform_config.transformation_link.rotation.y.x;
        rotation[1][1] = sp.platform_config.transformation_link.rotation.y.y;
        rotation[1][2] = sp.platform_config.transformation_link.rotation.y.z;
        rotation[2][0] = sp.platform_config.transformation_link.rotation.z.x;
        rotation[2][1] = sp.platform_config.transformation_link.rotation.z.y;
        rotation[2][2] = sp.platform_config.transformation_link.rotation.z.z;

        auto&& translation = json_data["safety_preset"]["platform_config"]["transformation_link"]["translation"];
        translation[0] = sp.platform_config.transformation_link.translation.x;
        translation[1] = sp.platform_config.transformation_link.translation.y;
        translation[2] = sp.platform_config.transformation_link.translation.z;

        json_data["safety_preset"]["platform_config"]["robot_height"] = sp.platform_config.robot_height;

        auto&& safety_zones = json_data["safety_preset"]["safety_zones"];
        for (int safety_zone_index = 0; safety_zone_index < 2; safety_zone_index++)
        {
            std::string zone_type = (safety_zone_index == 0) ? "danger_zone" : "warning_zone";
            for (int zone_polygon_index = 0; zone_polygon_index < 4; zone_polygon_index++)
            {
                std::string p = "p" + std::to_string(zone_polygon_index);
                safety_zones[zone_type]["zone_polygon"][p]["x"] = sp.safety_zones[safety_zone_index].zone_polygon[zone_polygon_index].x;
                safety_zones[zone_type]["zone_polygon"][p]["y"] = sp.safety_zones[safety_zone_index].zone_polygon[zone_polygon_index].y;
            }
            safety_zones[zone_type]["safety_trigger_confidence"] = sp.safety_zones[safety_zone_index].safety_trigger_confidence;
        }

        auto&& masking_zones = json_data["safety_preset"]["masking_zones"];
        for (int masking_zone_index = 0; masking_zone_index < 8; masking_zone_index++)
        {
            std::string masking_zone_index_str = std::to_string(masking_zone_index);
            for (int region_of_interests_index = 0; region_of_interests_index < 4; region_of_interests_index++)
            {
                std::string p = "p" + std::to_string(region_of_interests_index);
                masking_zones[masking_zone_index_str]["region_of_interests"][p]["i"] = sp.masking_zones[masking_zone_index].region_of_interests[region_of_interests_index].i;
                masking_zones[masking_zone_index_str]["region_of_interests"][p]["j"] = sp.masking_zones[masking_zone_index].region_of_interests[region_of_interests_index].j;
            }
            masking_zones[masking_zone_index_str]["attributes"] = sp.masking_zones[masking_zone_index].attributes;
        }

        auto&& environment = json_data["safety_preset"]["environment"];
        environment["safety_trigger_duration"] = sp.environment.safety_trigger_duration;
        environment["linear_velocity"] = sp.environment.linear_velocity;
        environment["angular_velocity"] = sp.environment.angular_velocity;
        environment["payload_weight"] = sp.environment.payload_weight;
        environment["surface_inclination"] = sp.environment.surface_inclination;
        environment["surface_height"] = sp.environment.surface_height;
        environment["diagnostic_zone_fill_rate_threshold"] = sp.environment.diagnostic_zone_fill_rate_threshold;
        environment["floor_fill_threshold"] = sp.environment.floor_fill_threshold;
        environment["depth_fill_threshold"] = sp.environment.depth_fill_threshold;
        environment["diagnostic_zone_height_median_threshold"] = sp.environment.diagnostic_zone_height_median_threshold;
        environment["vision_hara_persistency"] = sp.environment.vision_hara_persistency;
        
        size_t number_of_elements = sizeof(sp.environment.crypto_signature) / sizeof(sp.environment.crypto_signature[0]);
        std::vector<uint8_t> crypto_signature_byte_array(number_of_elements);
        memcpy(crypto_signature_byte_array.data(), sp.environment.crypto_signature, sizeof(sp.environment.crypto_signature));
        environment["crypto_signature"] = crypto_signature_byte_array;

        return json_data.dump();
    }


    rs2_safety_preset d500_safety_sensor::json_string_to_safety_preset(const std::string& json_str) const
    {
        rsutils::json json_data = rsutils::json::parse(json_str);

        rs2_safety_preset sp;
        auto&& rotation = json_data["safety_preset"]["platform_config"]["transformation_link"]["rotation"];
        sp.platform_config.transformation_link.rotation.x.x = rotation[0][0];
        sp.platform_config.transformation_link.rotation.x.y = rotation[0][1];
        sp.platform_config.transformation_link.rotation.x.z = rotation[0][2];
        sp.platform_config.transformation_link.rotation.y.x = rotation[1][0];
        sp.platform_config.transformation_link.rotation.y.y = rotation[1][1];
        sp.platform_config.transformation_link.rotation.y.z = rotation[1][2];
        sp.platform_config.transformation_link.rotation.z.x = rotation[2][0];
        sp.platform_config.transformation_link.rotation.z.y = rotation[2][1];
        sp.platform_config.transformation_link.rotation.z.z = rotation[2][2];

        auto&& translation = json_data["safety_preset"]["platform_config"]["transformation_link"]["translation"];
        sp.platform_config.transformation_link.translation.x = translation[0];
        sp.platform_config.transformation_link.translation.y = translation[1];
        sp.platform_config.transformation_link.translation.z = translation[2];

        sp.platform_config.robot_height = json_data["safety_preset"]["platform_config"]["robot_height"];

        auto&& safety_zones = json_data["safety_preset"]["safety_zones"];
        for (int safety_zone_index = 0; safety_zone_index < 2; safety_zone_index++)
        {
            std::string zone_type = (safety_zone_index == 0) ? "danger_zone" : "warning_zone";
            for (int zone_polygon_index = 0; zone_polygon_index < 4; zone_polygon_index++)
            {
                std::string p = "p" + std::to_string(zone_polygon_index);
                sp.safety_zones[safety_zone_index].zone_polygon[zone_polygon_index].x = safety_zones[zone_type]["zone_polygon"][p]["x"];
                sp.safety_zones[safety_zone_index].zone_polygon[zone_polygon_index].y = safety_zones[zone_type]["zone_polygon"][p]["y"];
            }
            sp.safety_zones[safety_zone_index].safety_trigger_confidence = safety_zones[zone_type]["safety_trigger_confidence"];
        }

        auto&& masking_zones = json_data["safety_preset"]["masking_zones"];
        for (int masking_zone_index = 0; masking_zone_index < 8; masking_zone_index++)
        {
            std::string masking_zone_index_str = std::to_string(masking_zone_index);
            for (int region_of_interests_index = 0; region_of_interests_index < 4; region_of_interests_index++)
            {
                std::string p = "p" + std::to_string(region_of_interests_index);
                sp.masking_zones[masking_zone_index].region_of_interests[region_of_interests_index].i = masking_zones[masking_zone_index_str]["region_of_interests"][p]["i"];
                sp.masking_zones[masking_zone_index].region_of_interests[region_of_interests_index].j = masking_zones[masking_zone_index_str]["region_of_interests"][p]["j"];
            }
            sp.masking_zones[masking_zone_index].attributes = masking_zones[masking_zone_index_str]["attributes"];
        }

        auto&& environment = json_data["safety_preset"]["environment"];
        sp.environment.safety_trigger_duration = environment["safety_trigger_duration"];
        sp.environment.linear_velocity = environment["linear_velocity"];
        sp.environment.angular_velocity = environment["angular_velocity"];
        sp.environment.payload_weight = environment["payload_weight"];
        sp.environment.surface_inclination = environment["surface_inclination"];
        sp.environment.surface_height = environment["surface_height"];
        sp.environment.diagnostic_zone_fill_rate_threshold = environment["diagnostic_zone_fill_rate_threshold"];
        sp.environment.floor_fill_threshold = environment["floor_fill_threshold"];
        sp.environment.depth_fill_threshold = environment["depth_fill_threshold"];
        sp.environment.diagnostic_zone_height_median_threshold = environment["diagnostic_zone_height_median_threshold"];
        sp.environment.vision_hara_persistency = environment["vision_hara_persistency"];
        
        std::vector<uint8_t> crypto_signature_vector = environment["crypto_signature"].get<std::vector<uint8_t>>();
        std::memcpy(sp.environment.crypto_signature, crypto_signature_vector.data(), crypto_signature_vector.size() * sizeof(uint8_t));

        return sp;
    }

    void d500_safety_sensor::set_safety_interface_config(const rs2_safety_interface_config& sic) const
    {
        // calculate CRC
        uint32_t computed_crc32 = rsutils::number::calc_crc32(reinterpret_cast<const uint8_t*>(&sic), sizeof(rs2_safety_interface_config));

        // prepare vector of data to be sent (header + sp)
        rs2_safety_interface_config_with_header sic_with_header;
        uint16_t version = ((uint16_t)0x03 << 8) | 0x00;  // major=0x03, minor=0x00 --> ver = major.minor
        uint32_t calib_version = 0;  // ignoring this field, as requested by sw architect
        sic_with_header.header = { version, static_cast<uint16_t>(ds::d500_calibration_table_id::safety_interface_cfg_id),
            sizeof(rs2_safety_interface_config), calib_version, computed_crc32 };
        sic_with_header.payload = sic;
        auto data_as_ptr = reinterpret_cast<const uint8_t*>(&sic_with_header);

        // prepare command
        command cmd(ds::SET_HKR_CONFIG_TABLE,
            static_cast<int>(ds::d500_calib_location::d500_calib_flash_memory),
            static_cast<int>(ds::d500_calibration_table_id::safety_interface_cfg_id),
            static_cast<int>(ds::d500_calib_type::d500_calib_gold));
        cmd.data.insert(cmd.data.end(), data_as_ptr, data_as_ptr + sizeof(rs2_safety_interface_config_with_header));
        cmd.require_response = false;

        // send command 
        _owner->_hw_monitor->send(cmd);
    }

    rs2_safety_interface_config d500_safety_sensor::get_safety_interface_config(rs2_calib_location loc) const
    {
        if (loc != RS2_CALIB_LOCATION_FLASH && loc != RS2_CALIB_LOCATION_RAM)
            throw io_exception(rsutils::string::from() << "Safety Interface Config can be read only from Flash or RAM");
        ds::d500_calib_location d500_loc = (loc == RS2_CALIB_LOCATION_RAM) ? ds::d500_calib_location::d500_calib_ram_memory :
            ds::d500_calib_location::d500_calib_flash_memory;
        rs2_safety_interface_config_with_header* result;

        // prepare command
        command cmd(ds::GET_HKR_CONFIG_TABLE,
            static_cast<int>(d500_loc),
            static_cast<int>(ds::d500_calibration_table_id::safety_interface_cfg_id),
            static_cast<int>(ds::d500_calib_type::d500_calib_gold));
        cmd.require_response = true;

        // send command to device and get response (safety_interface_config entry + header)
        std::vector< uint8_t > response = _owner->_hw_monitor->send(cmd);
        if (response.size() < sizeof(rs2_safety_interface_config_with_header))
        {
            throw io_exception(rsutils::string::from() << "Safety Interface Config failed");
        }
        // check CRC before returning result       
        auto computed_crc32 = rsutils::number::calc_crc32(response.data() + sizeof(rs2_safety_interface_config_header),
            sizeof(rs2_safety_interface_config));
        result = reinterpret_cast<rs2_safety_interface_config_with_header*>(response.data());
        if (computed_crc32 != result->header.crc32)
        {
            throw invalid_value_exception(rsutils::string::from() << "Safety Interface Config invalid CRC value");
        }
        
        return result->payload;
    }
}
