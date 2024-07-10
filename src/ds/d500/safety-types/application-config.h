// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <stdint.h>
#include "common.h"
#include <rsutils/json.h>

namespace librealsense
{

	using rsutils::json;

#pragma pack(push, 1)

	class SIP
	{
	public:

		SIP(uint8_t immediate_mode_safety_features_selection = 0,
			uint8_t temporal_safety_features_selection = 0,
			std::vector<uint8_t> mechanisms_thresholds = {},
			std::vector<uint8_t> mechanisms_sampling_interval = {},
			std::vector<uint8_t> reserved = {}) :
			m_immediate_mode_safety_features_selection(immediate_mode_safety_features_selection),
			m_temporal_safety_features_selection(temporal_safety_features_selection)
		{
			std::copy(mechanisms_thresholds.begin(), mechanisms_thresholds.end(), m_mechanisms_thresholds);
			std::copy(mechanisms_sampling_interval.begin(), mechanisms_sampling_interval.end(), m_mechanisms_sampling_interval);
			std::copy(reserved.begin(), reserved.end(), m_reserved);
		}

		SIP(const json& j)
		{
			m_immediate_mode_safety_features_selection = j["immediate_mode_safety_features_selection"].get<uint8_t>();
			m_temporal_safety_features_selection = j["temporal_safety_features_selection"].get<uint8_t>();
			m_immediate_mode_safety_features_selection = j["immediate_mode_safety_features_selection"].get<uint8_t>();

			std::vector<uint8_t> mechanisms_thresholds_vec = j["mechanisms_thresholds"].get<std::vector<uint8_t>>();
			std::memcpy(m_mechanisms_thresholds, mechanisms_thresholds_vec.data(), mechanisms_thresholds_vec.size());

			std::vector<uint8_t> mechanisms_sampling_interval_vec = j["mechanisms_sampling_interval"].get<std::vector<uint8_t>>();
			std::memcpy(m_mechanisms_sampling_interval, mechanisms_sampling_interval_vec.data(), mechanisms_sampling_interval_vec.size());

			std::vector<uint8_t> reserved_vec = j["reserved"].get<std::vector<uint8_t>>();
			std::memcpy(m_reserved, reserved_vec.data(), reserved_vec.size());
		}

		//// Method to deserialize from JSON
		//static SIP from_json(const json& j) {

		//	SIP sip;
		//	sip.m_immediate_mode_safety_features_selection = j["immediate_mode_safety_features_selection"].get<uint8_t>();
		//	sip.m_temporal_safety_features_selection = j["temporal_safety_features_selection"].get<uint8_t>();
		//	sip.m_immediate_mode_safety_features_selection = j["immediate_mode_safety_features_selection"].get<uint8_t>();

		//	std::vector<uint8_t> mechanisms_thresholds_vec = j["mechanisms_thresholds"].get<std::vector<uint8_t>>();
		//	std::memcpy(sip.m_mechanisms_thresholds, mechanisms_thresholds_vec.data(), mechanisms_thresholds_vec.size());

		//	std::vector<uint8_t> mechanisms_sampling_interval_vec = j["mechanisms_sampling_interval"].get<std::vector<uint8_t>>();
		//	std::memcpy(sip.m_mechanisms_sampling_interval, mechanisms_sampling_interval_vec.data(), mechanisms_sampling_interval_vec.size());

		//	std::vector<uint8_t> reserved_vec = j["reserved"].get<std::vector<uint8_t>>();
		//	std::memcpy(sip.m_reserved, reserved_vec.data(), reserved_vec.size());

		//	return sip;
		//}

		// Method to serialize to JSON
		json to_json() const {
			json j;

			j["immediate_mode_safety_features_selection"] = m_immediate_mode_safety_features_selection;
			j["temporal_safety_features_selection"] = m_temporal_safety_features_selection;

			size_t number_of_elements = sizeof(m_mechanisms_thresholds) / sizeof(m_mechanisms_thresholds[0]);
			std::vector<uint8_t> mechanisms_thresholds_vec(number_of_elements);
			memcpy(mechanisms_thresholds_vec.data(), m_mechanisms_thresholds, sizeof(m_mechanisms_thresholds));
			j["mechanisms_thresholds"] = mechanisms_thresholds_vec;

			number_of_elements = sizeof(m_mechanisms_sampling_interval) / sizeof(m_mechanisms_sampling_interval[0]);
			std::vector<uint8_t> mechanisms_sampling_interval_vec(number_of_elements);
			memcpy(mechanisms_sampling_interval_vec.data(), m_mechanisms_sampling_interval, sizeof(m_mechanisms_sampling_interval));
			j["mechanisms_sampling_interval_vec"] = mechanisms_sampling_interval_vec;

			number_of_elements = sizeof(m_reserved) / sizeof(m_reserved[0]);
			std::vector<uint8_t> reserved_vec(number_of_elements);
			memcpy(reserved_vec.data(), m_reserved, sizeof(m_reserved));
			j["reserved"] = reserved_vec;

			return j;
		}

	private:
		uint8_t m_immediate_mode_safety_features_selection;
		uint8_t m_temporal_safety_features_selection;
		uint8_t m_mechanisms_thresholds[16];
		uint8_t m_mechanisms_sampling_interval[8];
		uint8_t m_reserved[64];
	};

	class TempThresholds
	{
	public:
		TempThresholds(
			std::vector<uint8_t> ir_right = {},
			std::vector<uint8_t> ir_left = {},
			std::vector<uint8_t> apm_left = {},
			std::vector<uint8_t> apm_right = {},
			std::vector<uint8_t> reserved1 = {},
			std::vector<uint8_t> hkr_core = {},
			std::vector<uint8_t> smcu_right = {},
			std::vector<uint8_t> reserved2 = {},
			std::vector<uint8_t> sht4x = {},
			std::vector<uint8_t> reserved3 = {},
			std::vector<uint8_t> imu = {})
		{
			std::copy(ir_right.begin(), ir_right.end(), m_ir_right);
			std::copy(ir_left.begin(), ir_left.end(), m_ir_left);
			std::copy(apm_left.begin(), apm_left.end(), m_apm_left);
			std::copy(apm_right.begin(), apm_right.end(), m_apm_right);
			std::copy(reserved1.begin(), reserved1.end(), m_reserved1);
			std::copy(hkr_core.begin(), hkr_core.end(), m_hkr_core);
			std::copy(smcu_right.begin(), smcu_right.end(), m_smcu_right);
			std::copy(reserved2.begin(), reserved2.end(), m_reserved2);
			std::copy(sht4x.begin(), sht4x.end(), m_sht4x);
			std::copy(reserved3.begin(), reserved3.end(), m_reserved3);
			std::copy(imu.begin(), imu.end(), m_imu);
		}

		TempThresholds(const json& j)
		{
			std::vector<uint8_t> ir_right_vec = j["ir_right"].get<std::vector<uint8_t>>();
			std::memcpy(m_ir_right, ir_right_vec.data(), ir_right_vec.size());

			std::vector<uint8_t> ir_left_vec = j["ir_left"].get<std::vector<uint8_t>>();
			std::memcpy(m_ir_left, ir_left_vec.data(), ir_left_vec.size());

			std::vector<uint8_t> apm_left_vec = j["apm_left"].get<std::vector<uint8_t>>();
			std::memcpy(m_apm_left, apm_left_vec.data(), apm_left_vec.size());

			std::vector<uint8_t> apm_right_vec = j["apm_right"].get<std::vector<uint8_t>>();
			std::memcpy(m_apm_right, apm_right_vec.data(), apm_right_vec.size());

			std::vector<uint8_t> reserved1_vec = j["reserved1"].get<std::vector<uint8_t>>();
			std::memcpy(m_reserved1, reserved1_vec.data(), reserved1_vec.size());

			std::vector<uint8_t> hkr_core_vec = j["hkr_core"].get<std::vector<uint8_t>>();
			std::memcpy(m_hkr_core, hkr_core_vec.data(), hkr_core_vec.size());

			std::vector<uint8_t> smcu_right_vec = j["smcu_right"].get<std::vector<uint8_t>>();
			std::memcpy(m_smcu_right, smcu_right_vec.data(), smcu_right_vec.size());

			std::vector<uint8_t> reserved2_vec = j["reserved2"].get<std::vector<uint8_t>>();
			std::memcpy(m_reserved2, reserved2_vec.data(), reserved2_vec.size());

			std::vector<uint8_t> sht4x_vec = j["sht4x"].get<std::vector<uint8_t>>();
			std::memcpy(m_sht4x, sht4x_vec.data(), sht4x_vec.size());

			std::vector<uint8_t> reserved3_vec = j["reserved3"].get<std::vector<uint8_t>>();
			std::memcpy(m_reserved3, reserved3_vec.data(), reserved3_vec.size());

			std::vector<uint8_t> imu_vec = j["imu"].get<std::vector<uint8_t>>();
			std::memcpy(m_imu, imu_vec.data(), imu_vec.size());
		}

		// Method to serialize to JSON
		json to_json() const {
			json j;

			size_t number_of_elements = sizeof(m_ir_right) / sizeof(m_ir_right[0]);
			std::vector<uint8_t> ir_right_vec(number_of_elements);
			memcpy(ir_right_vec.data(), m_ir_right, sizeof(m_ir_right));
			j["ir_right"] = ir_right_vec;

			number_of_elements = sizeof(m_ir_right) / sizeof(m_ir_right[0]);
			//std::vector<uint8_t> ir_right_vec(number_of_elements);
			memcpy(ir_right_vec.data(), m_ir_right, sizeof(m_ir_right));
			j["ir_left"] = ir_right_vec;

			number_of_elements = sizeof(m_ir_right) / sizeof(m_ir_right[0]);
			//std::vector<uint8_t> ir_right_vec(number_of_elements);
			memcpy(ir_right_vec.data(), m_ir_right, sizeof(m_ir_right));
			j["apm_left"] = ir_right_vec;

			number_of_elements = sizeof(m_ir_right) / sizeof(m_ir_right[0]);
			//std::vector<uint8_t> ir_right_vec(number_of_elements);
			memcpy(ir_right_vec.data(), m_ir_right, sizeof(m_ir_right));
			j["apm_right"] = ir_right_vec;

			number_of_elements = sizeof(m_ir_right) / sizeof(m_ir_right[0]);
			//std::vector<uint8_t> ir_right_vec(number_of_elements);
			memcpy(ir_right_vec.data(), m_ir_right, sizeof(m_ir_right));
			j["reserved1"] = ir_right_vec;

			number_of_elements = sizeof(m_ir_right) / sizeof(m_ir_right[0]);
			//std::vector<uint8_t> ir_right_vec(number_of_elements);
			memcpy(ir_right_vec.data(), m_ir_right, sizeof(m_ir_right));
			j["hkr_core"] = ir_right_vec;

			number_of_elements = sizeof(m_ir_right) / sizeof(m_ir_right[0]);
			//std::vector<uint8_t> ir_right_vec(number_of_elements);
			memcpy(ir_right_vec.data(), m_ir_right, sizeof(m_ir_right));
			j["smcu_right"] = ir_right_vec;

			number_of_elements = sizeof(m_ir_right) / sizeof(m_ir_right[0]);
			//std::vector<uint8_t> ir_right_vec(number_of_elements);
			memcpy(ir_right_vec.data(), m_ir_right, sizeof(m_ir_right));
			j["reserved2"] = ir_right_vec;

			number_of_elements = sizeof(m_ir_right) / sizeof(m_ir_right[0]);
			//std::vector<uint8_t> ir_right_vec(number_of_elements);
			memcpy(ir_right_vec.data(), m_ir_right, sizeof(m_ir_right));
			j["m_sht4x"] = ir_right_vec;

			number_of_elements = sizeof(m_ir_right) / sizeof(m_ir_right[0]);
			//std::vector<uint8_t> ir_right_vec(number_of_elements);
			memcpy(ir_right_vec.data(), m_ir_right, sizeof(m_ir_right));
			j["reserved3"] = ir_right_vec;

			number_of_elements = sizeof(m_ir_right) / sizeof(m_ir_right[0]);
			//std::vector<uint8_t> ir_right_vec(number_of_elements);
			memcpy(ir_right_vec.data(), m_ir_right, sizeof(m_ir_right));
			j["imu"] = ir_right_vec;

			//TODO all fields

			return j;
		}

	private:
		int8_t m_ir_right[4];
		int8_t m_ir_left[4];
		int8_t m_apm_left[4];
		int8_t m_apm_right[4];
		uint8_t m_reserved1[4];
		int8_t m_hkr_core[4];
		int8_t m_smcu_right[4];
		uint8_t m_reserved2[4];
		int8_t m_sht4x[4];
		uint8_t m_reserved3[4];
		int8_t m_imu[4];
	};

	class HumidityThresholds
	{
	public:
		HumidityThresholds(uint8_t sht4x = 0) : m_sht4x(sht4x)
		{

		}

		HumidityThresholds(const json& j)
		{
			m_sht4x = j["sht4x"].get<uint8_t>();
		}

		json to_json() const
		{
			json j;
			j["sht4x"] = m_sht4x;
			return j;
		}
	private:
		uint8_t m_sht4x;
	};

	class VoltageThresholds
	{
		uint8_t vdd3v3;
		uint8_t vdd1v8;
		uint8_t vdd1v2;
		uint8_t vdd1v1;
		uint8_t vdd0v8;
		uint8_t vdd0v6;
		uint8_t vdd5vo_u;
		uint8_t vdd5vo_l;
		uint8_t vdd0v8_ddr;
	};

	class DeveloperMode
	{
		uint8_t hkr;
		uint8_t smcu;
		uint8_t hkr_simulated_lock_state;
		uint8_t sc;
	};

	class ApplicationConfig
	{
	public:
		ApplicationConfig()
		{

		}

		ApplicationConfig(const json& j)
		{
			m_sip = SIP(j["sip"]);

			m_dev_rules_selection = j["dev_rules_selection"];
			m_depth_pipe_safety_checks_override = j["depth_pipe_safety_checks_override"];
			m_triggered_calib_safety_checks_override = j["triggered_calib_safety_checks_override"]; 
			m_smcu_bypass_directly_to_maintenance_mode = j["smcu_bypass_directly_to_maintenance_mode"]; 
			m_smcu_skip_spi_error = j["smcu_skip_spi_error"];

			m_temp_thresholds = TempThresholds(j["temp_thresholds"]);
			m_humidity_thresholds = HumidityThresholds(j["humidity_thresholds"]);

			m_reserved1 = j["reserved1"];
			m_reserved2 = j["reserved2"];

		}

		rsutils::json toJson()
		{
			rsutils::json j;
			auto& app_config_json = j["application_config"];
			app_config_json["sip"] = m_sip.to_json();

			app_config_json["dev_rules_selection"] = m_dev_rules_selection;
			app_config_json["depth_pipe_safety_checks_override"] = m_depth_pipe_safety_checks_override;
			app_config_json["triggered_calib_safety_checks_override"] = m_triggered_calib_safety_checks_override;
			app_config_json["smcu_bypass_directly_to_maintenance_mode"] = m_smcu_bypass_directly_to_maintenance_mode;
			app_config_json["smcu_skip_spi_error"] = m_smcu_skip_spi_error;


			app_config_json["temp_thresholds"] = m_temp_thresholds.to_json();
			app_config_json["humidity_thresholds"] = m_humidity_thresholds.to_json();


			app_config_json["reserved1"] = m_reserved1;
			app_config_json["reserved2"] = m_reserved2;

			return j;
		}

	private:

		SIP m_sip;

		uint64_t m_dev_rules_selection;
		uint8_t m_depth_pipe_safety_checks_override;
		uint8_t m_triggered_calib_safety_checks_override;
		uint8_t m_smcu_bypass_directly_to_maintenance_mode;
		uint8_t m_smcu_skip_spi_error;

		TempThresholds m_temp_thresholds;
		HumidityThresholds m_humidity_thresholds;
		//VoltageThresholds voltage_thresholds;

		uint8_t m_reserved1;
		uint8_t m_reserved2;

		//DeveloperMode developer_mode;

		//uint8_t depth_pipeline_config;
		//uint8_t depth_roi;
		//uint8_t ir_for_sip;
		//uint8_t reserved3;
		//int8_t digital_signature[32];
		//int8_t reserved4[228];
	};

	class ApplicationConfigWithHeader
	{
	public:
		ApplicationConfigWithHeader()
		{
		}
		table_header header;
		ApplicationConfig app_config;
	};

#pragma pack(pop)

}
