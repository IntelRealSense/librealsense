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

	class sip
	{
	public:
		sip(const json& j)
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

	class temp_thresholds
	{
	public:
		temp_thresholds(const json& j)
		{
			std::vector<int8_t> ir_right_vec = j["ir_right"].get<std::vector<int8_t>>();
			std::memcpy(m_ir_right, ir_right_vec.data(), ir_right_vec.size());

			std::vector<int8_t> ir_left_vec = j["ir_left"].get<std::vector<int8_t>>();
			std::memcpy(m_ir_left, ir_left_vec.data(), ir_left_vec.size());

			std::vector<int8_t> apm_left_vec = j["apm_left"].get<std::vector<int8_t>>();
			std::memcpy(m_apm_left, apm_left_vec.data(), apm_left_vec.size());

			std::vector<int8_t> apm_right_vec = j["apm_right"].get<std::vector<int8_t>>();
			std::memcpy(m_apm_right, apm_right_vec.data(), apm_right_vec.size());

			std::vector<int8_t> reserved1_vec = j["reserved1"].get<std::vector<int8_t>>();
			std::memcpy(m_reserved1, reserved1_vec.data(), reserved1_vec.size());

			std::vector<int8_t> hkr_core_vec = j["hkr_core"].get<std::vector<int8_t>>();
			std::memcpy(m_hkr_core, hkr_core_vec.data(), hkr_core_vec.size());

			std::vector<int8_t> smcu_right_vec = j["smcu_right"].get<std::vector<int8_t>>();
			std::memcpy(m_smcu_right, smcu_right_vec.data(), smcu_right_vec.size());

			std::vector<int8_t> reserved2_vec = j["reserved2"].get<std::vector<int8_t>>();
			std::memcpy(m_reserved2, reserved2_vec.data(), reserved2_vec.size());

			std::vector<int8_t> sht4x_vec = j["sht4x"].get<std::vector<int8_t>>();
			std::memcpy(m_sht4x, sht4x_vec.data(), sht4x_vec.size());

			std::vector<int8_t> reserved3_vec = j["reserved3"].get<std::vector<int8_t>>();
			std::memcpy(m_reserved3, reserved3_vec.data(), reserved3_vec.size());

			std::vector<int8_t> imu_vec = j["imu"].get<std::vector<int8_t>>();
			std::memcpy(m_imu, imu_vec.data(), imu_vec.size());
		}

		json to_json() const {
			json j;
			j["ir_right"] = std::vector<int8_t>(std::begin(m_ir_right), std::end(m_ir_right));
			j["ir_left"] = native_arr_to_std_vector(m_ir_left);
			j["apm_left"] = native_arr_to_std_vector(m_apm_left);
			j["apm_right"] = native_arr_to_std_vector(m_apm_left);
			j["reserved1"] = native_arr_to_std_vector(m_reserved1);
			j["hkr_core"] = native_arr_to_std_vector(m_hkr_core);
			j["smcu_right"] = native_arr_to_std_vector(m_smcu_right);
			j["reserved2"] = native_arr_to_std_vector(m_reserved2);
			j["sht4x"] = native_arr_to_std_vector(m_sht4x);
			j["reserved3"] = native_arr_to_std_vector(m_reserved3);
			j["imu"] = native_arr_to_std_vector(m_imu);
			return j;
		}

	private:
		int8_t m_ir_right[4];
		int8_t m_ir_left[4];
		int8_t m_apm_left[4];
		int8_t m_apm_right[4];
		int8_t m_reserved1[4];
		int8_t m_hkr_core[4];
		int8_t m_smcu_right[4];
		int8_t m_reserved2[4];
		int8_t m_sht4x[4];
		int8_t m_reserved3[4];
		int8_t m_imu[4];
	};

	class humidity_thresholds
	{
	public:
		humidity_thresholds(const json& j)
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

	class voltage_thresholds
	{
	public:
		voltage_thresholds(const json& j)
		{
			m_vdd3v3 = j["vdd3v3"].get<uint8_t>();
			m_vdd1v8 = j["vdd1v8"].get<uint8_t>();
			m_vdd1v2 = j["vdd1v2"].get<uint8_t>();
			m_vdd1v1 = j["vdd1v1"].get<uint8_t>();
			m_vdd0v8 = j["vdd0v8"].get<uint8_t>();
			m_vdd0v6 = j["vdd0v6"].get<uint8_t>();
			m_vdd5vo_u = j["vdd5vo_u"].get<uint8_t>();
			m_vdd5vo_l = j["vdd5vo_l"].get<uint8_t>();
			m_vdd0v8_ddr = j["vdd0v8_ddr"].get<uint8_t>();

		}

		json to_json() const
		{
			json j;
			j["vdd3v3"] = m_vdd3v3;
			j["vdd1v8"] = m_vdd3v3;
			j["vdd1v2"] = m_vdd1v2;
			j["vdd1v1"] = m_vdd1v1;
			j["vdd0v8"] = m_vdd0v8;
			j["vdd0v6"] = m_vdd0v6;
			j["vdd5vo_l"] = m_vdd5vo_l;
			j["vdd0v8_ddr"] = m_vdd0v8_ddr;

			return j;
		}
	private:
		uint8_t m_vdd3v3;
		uint8_t m_vdd1v8;
		uint8_t m_vdd1v2;
		uint8_t m_vdd1v1;
		uint8_t m_vdd0v8;
		uint8_t m_vdd0v6;
		uint8_t m_vdd5vo_u;
		uint8_t m_vdd5vo_l;
		uint8_t m_vdd0v8_ddr;
	};

	class developer_mode
	{
	public:
		developer_mode(const json& j)
		{
			m_hkr = j["hkr"].get<uint8_t>();
			m_smcu = j["smcu"].get<uint8_t>();
			m_hkr_simulated_lock_state = j["hkr_simulated_lock_state"].get<uint8_t>();
			m_sc = j["sc"].get<uint8_t>();
		}

		json to_json() const
		{
			json j;
			j["hkr"] = m_hkr;
			j["smcu"] = m_smcu;
			j["hkr_simulated_lock_state"] = m_hkr_simulated_lock_state;
			j["sc"] = m_sc;
			return j;
		}

	private:
		uint8_t m_hkr;
		uint8_t m_smcu;
		uint8_t m_hkr_simulated_lock_state;
		uint8_t m_sc;
	};

	class application_config
	{
	public:

		application_config(const json& j) :
			m_sip(j["sip"]),
			m_dev_rules_selection(j["dev_rules_selection"]),
			m_depth_pipe_safety_checks_override(j["depth_pipe_safety_checks_override"]),
			m_triggered_calib_safety_checks_override(j["triggered_calib_safety_checks_override"]),
			m_smcu_bypass_directly_to_maintenance_mode(j["smcu_bypass_directly_to_maintenance_mode"]),
			m_smcu_skip_spi_error(j["smcu_skip_spi_error"]),
			m_temp_thresholds(j["temp_thresholds"]),
			m_humidity_thresholds(j["humidity_thresholds"]),
			m_voltage_thresholds(j["voltage_thresholds"]),
			m_reserved1(j["reserved1"]),
			m_reserved2(j["reserved2"]),
			m_developer_mode(j["developer_mode"]),
			m_depth_pipeline_config(j["depth_pipeline_config"]),
			m_depth_roi(j["depth_roi"]),
			m_ir_for_sip(j["ir_for_sip"])
		{
			std::vector<uint8_t> reserved3_vec = j["reserved3"].get<std::vector<uint8_t>>();
			std::memcpy(m_reserved3, reserved3_vec.data(), reserved3_vec.size());

			std::vector<uint8_t> digital_signature_vec = j["digital_signature"].get<std::vector<uint8_t>>();
			std::memcpy(m_digital_signature, digital_signature_vec.data(), digital_signature_vec.size());

			std::vector<uint8_t> reserved4_vec = j["reserved4"].get<std::vector<uint8_t>>();
			std::memcpy(m_reserved4, reserved4_vec.data(), reserved4_vec.size());
		}

		rsutils::json to_json()
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
			app_config_json["voltage_thresholds"] = m_voltage_thresholds.to_json();

			app_config_json["reserved1"] = m_reserved1;
			app_config_json["reserved2"] = m_reserved2;

			app_config_json["developer_mode"] = m_developer_mode.to_json();

			app_config_json["depth_pipeline_config"] = m_depth_pipeline_config;
			app_config_json["depth_roi"] = m_depth_roi;

			app_config_json["ir_for_sip"] = m_ir_for_sip;

			app_config_json["reserved3"] = native_arr_to_std_vector(m_reserved3);
			app_config_json["digital_signature"] = native_arr_to_std_vector(m_digital_signature);
			app_config_json["reserved4"] = native_arr_to_std_vector(m_reserved4);

			return j;
		}

	private:

		sip m_sip;
		uint64_t m_dev_rules_selection;
		uint8_t m_depth_pipe_safety_checks_override;
		uint8_t m_triggered_calib_safety_checks_override;
		uint8_t m_smcu_bypass_directly_to_maintenance_mode;
		uint8_t m_smcu_skip_spi_error;
		temp_thresholds m_temp_thresholds;
		humidity_thresholds m_humidity_thresholds;
		voltage_thresholds m_voltage_thresholds;
		uint8_t m_reserved1;
		uint8_t m_reserved2;
		developer_mode m_developer_mode;
		uint8_t m_depth_pipeline_config;
		uint8_t m_depth_roi;
		uint8_t m_ir_for_sip;
		uint8_t m_reserved3[39];
		uint8_t m_digital_signature[32];
		uint8_t m_reserved4[228];
	};

	class application_config_with_header
	{
	public:
		application_config_with_header(table_header header, application_config app_config):
			m_header(header), m_app_config(app_config)
		{
		}
		
		application_config get_application_config() const
		{
			return m_app_config;
		}

		table_header get_table_header() const
		{
			return m_header;
		}

	private:
		table_header m_header;
		application_config m_app_config;
	};

#pragma pack(pop)

}
