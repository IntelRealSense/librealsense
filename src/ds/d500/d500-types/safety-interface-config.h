// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include "common.h"

namespace librealsense
{

#pragma pack(push, 1)

    class pin
    {
    public:
        pin() = default;

        pin(const json &j)
        {
            validate_json(j);
            m_direction = j.at("direction");
            m_functionality = j.at("functionality");
        }

        json to_json() const
        {
            return {
                {"direction", m_direction},
                {"functionality", m_functionality}};
        }

    private:
        void validate_json(const json &j) const
        {
            if (!j.is_object())
            {
                throw std::invalid_argument("Invalid pin format");
            }
            for (const auto &field : {"direction", "functionality"})
            {
                if (!j.contains(field))
                {
                    throw std::invalid_argument(std::string("Invalid pin format: missing field: ") + field);
                }
            }
        }

        uint8_t m_direction;
        uint8_t m_functionality;
    };

    class m12_safety_pins_configuration
    {
    public:
        m12_safety_pins_configuration() = default;

        m12_safety_pins_configuration(const json &j)
        {
            validate_json(j);
            m_power = pin(j.at("power"));
            m_ossd1_b = pin(j.at("ossd1_b"));
            m_ossd1_a = pin(j.at("ossd1_a"));
            m_gpio_0 = pin(j.at("gpio_0"));
            m_gpio_1 = pin(j.at("gpio_1"));
            m_gpio_2 = pin(j.at("gpio_2"));
            m_gpio_3 = pin(j.at("gpio_3"));
            m_preset3_a = pin(j.at("preset3_a"));
            m_preset3_b = pin(j.at("preset3_b"));
            m_preset4_a = pin(j.at("preset4_a"));
            m_preset1_b = pin(j.at("preset1_b"));
            m_preset1_a = pin(j.at("preset1_a"));
            m_preset2_b = pin(j.at("preset2_b"));
            m_gpio_4 = pin(j.at("gpio_4"));
            m_preset2_a = pin(j.at("preset2_a"));
            m_preset4_b = pin(j.at("preset4_b"));
            m_ground = pin(j.at("ground"));
        }

        json to_json() const
        {
            json j;
            j["power"] = m_power.to_json();
            j["ossd1_b"] = m_ossd1_b.to_json();
            j["ossd1_a"] = m_ossd1_a.to_json();
            j["gpio_0"] = m_gpio_0.to_json();
            j["gpio_1"] = m_gpio_1.to_json();
            j["gpio_2"] = m_gpio_2.to_json();
            j["gpio_3"] = m_gpio_3.to_json();
            j["preset3_a"] = m_preset3_a.to_json();
            j["preset3_b"] = m_preset3_b.to_json();
            j["preset4_a"] = m_preset4_a.to_json();
            j["preset1_b"] = m_preset1_b.to_json();
            j["preset1_a"] = m_preset1_a.to_json();
            j["preset2_b"] = m_preset2_b.to_json();
            j["gpio_4"] = m_gpio_4.to_json();
            j["preset2_a"] = m_preset2_a.to_json();
            j["preset4_b"] = m_preset4_b.to_json();
            j["ground"] = m_ground.to_json();
            return j;
        }

    private:
        void validate_json(const json &j) const
        {
            if (!j.is_object())
            {
                throw std::invalid_argument("Invalid m12_safety_pins_configuration format");
            }
            for (const auto &field : {"power", "ossd1_b", "ossd1_a", "gpio_0", "gpio_1", "gpio_2", "gpio_3", "preset3_a", "preset3_b", "preset4_a", "preset1_b", "preset1_a", "preset2_b", "gpio_4", "preset2_a", "preset4_b", "ground"})
            {
                if (!j.contains(field))
                {
                    throw std::invalid_argument(std::string("Invalid m12_safety_pins_configuration format: missing field: ") + field);
                }
            }
        }

        pin m_power;
        pin m_ossd1_b;
        pin m_ossd1_a;
        pin m_gpio_0;
        pin m_gpio_1;
        pin m_gpio_2;
        pin m_gpio_3;
        pin m_preset3_a;
        pin m_preset3_b;
        pin m_preset4_a;
        pin m_preset1_b;
        pin m_preset1_a;
        pin m_preset2_b;
        pin m_gpio_4;
        pin m_preset2_a;
        pin m_preset4_b;
        pin m_ground;
    };

    class occupancy_grid_params
    {
    public:
        occupancy_grid_params() = default;

        occupancy_grid_params(const json &j)
        {
            validate_json(j);
            m_grid_cell_seed = j.at("grid_cell_seed");
            m_close_range_quorum = j.at("close_range_quorum");
            m_mid_range_quorum = j.at("mid_range_quorum");
            m_long_range_quorum = j.at("long_range_quorum");
        }

        json to_json() const
        {
            json j;
            j["grid_cell_seed"] = m_grid_cell_seed;
            j["close_range_quorum"] = m_close_range_quorum;
            j["mid_range_quorum"] = m_mid_range_quorum;
            j["long_range_quorum"] = m_long_range_quorum;
            return j;
        }

    private:
        void validate_json(const json &j) const
        {
            if (!j.is_object())
            {
                throw std::invalid_argument("Invalid occupancy_grid_params format");
            }
            for (const auto &field : {"grid_cell_seed", "close_range_quorum", "mid_range_quorum", "long_range_quorum"})
            {
                if (!j.contains(field))
                {
                    throw std::invalid_argument(std::string("Invalid occupancy_grid_params format: missing field: ") + field);
                }
            }
        }

        uint16_t m_grid_cell_seed;
        uint8_t m_close_range_quorum;
        uint8_t m_mid_range_quorum;
        uint8_t m_long_range_quorum;
    };

    class smcu_arbitration_params
    {
    public:
        smcu_arbitration_params() = default;

        smcu_arbitration_params(const json &j)
        {
            validate_json(j);
            m_l_0_total_threshold = j.at("l_0_total_threshold").get<uint16_t>();
            m_l_0_sustained_rate_threshold = j.at("l_0_sustained_rate_threshold").get<uint8_t>();
            m_l_1_total_threshold = j.at("l_1_total_threshold").get<uint8_t>();
            m_l_1_sustained_rate_threshold = j.at("l_1_sustained_rate_threshold").get<uint8_t>();
            m_l_2_total_threshold = j.at("l_2_total_threshold").get<uint8_t>();
            m_hkr_stl_timeout = j.at("hkr_stl_timeout").get<uint8_t>();
            m_mcu_stl_timeout = j.at("mcu_stl_timeout").get<uint8_t>();
            m_sustained_aicv_frame_drops = j.at("sustained_aicv_frame_drops").get<uint8_t>();
            m_ossd_self_test_pulse_width = j.at("ossd_self_test_pulse_width").get<uint8_t>();
        }

        json to_json() const
        {
            json j;
            j["l_0_total_threshold"] = m_l_0_total_threshold;
            j["l_0_sustained_rate_threshold"] = m_l_0_sustained_rate_threshold;
            j["l_1_total_threshold"] = m_l_1_total_threshold;
            j["l_1_sustained_rate_threshold"] = m_l_1_sustained_rate_threshold;
            j["l_2_total_threshold"] = m_l_2_total_threshold;
            j["hkr_stl_timeout"] = m_hkr_stl_timeout;
            j["mcu_stl_timeout"] = m_mcu_stl_timeout;
            j["sustained_aicv_frame_drops"] = m_sustained_aicv_frame_drops;
            j["ossd_self_test_pulse_width"] = m_ossd_self_test_pulse_width;
            return j;
        }

    private:
        void validate_json(const json &j) const
        {
            if (!j.is_object())
            {
                throw std::invalid_argument("Invalid smcu_arbitration_params format");
            }
            for (const auto &field : {"l_0_total_threshold", "l_0_sustained_rate_threshold", "l_1_total_threshold", "l_1_sustained_rate_threshold", "l_2_total_threshold", "hkr_stl_timeout", "mcu_stl_timeout", "sustained_aicv_frame_drops", "ossd_self_test_pulse_width"})
            {
                if (!j.contains(field))
                {
                    throw std::invalid_argument(std::string("Invalid smcu_arbitration_params format: missing field: ") + field);
                }
            }
        }

        uint16_t m_l_0_total_threshold;
        uint8_t m_l_0_sustained_rate_threshold;
        uint16_t m_l_1_total_threshold;
        uint8_t m_l_1_sustained_rate_threshold;
        uint8_t m_l_2_total_threshold;
        uint8_t m_hkr_stl_timeout;
        uint8_t m_mcu_stl_timeout;
        uint8_t m_sustained_aicv_frame_drops;
        uint8_t m_ossd_self_test_pulse_width;
    };

    class safety_interface_config
    {
    public:
        safety_interface_config() = default;

        safety_interface_config(const json &j)
        {
            validate_json(j);
            m_pins_configs = m12_safety_pins_configuration(j.at("m12_safety_pins_configuration"));
            m_gpio_stabilization_interval = j.at("gpio_stabilization_interval").get<uint8_t>();
            m_camera_position = camera_position(j.at("camera_position"));
            m_occupancy_grid_params = occupancy_grid_params(j.at("occupancy_grid_params"));
            m_smcu_arbitration_params = smcu_arbitration_params(j.at("smcu_arbitration_params"));
            for (size_t i = 0; i < 32; ++i)
            {
                m_crypto_signature[i] = j.at("crypto_signature")[i].get<uint8_t>();
            }
        }

        json to_json()
        {
            json j;
            auto &sic = j["safety_interface_config"];
            sic["m12_safety_pins_configuration"] = m_pins_configs.to_json();
            sic["gpio_stabilization_interval"] = m_gpio_stabilization_interval;
            sic["camera_position"] = m_camera_position.to_json();
            sic["occupancy_grid_params"] = m_occupancy_grid_params.to_json();
            sic["smcu_arbitration_params"] = m_smcu_arbitration_params.to_json();
            sic["crypto_signature"] = m_crypto_signature;
            return j;
        }

    private:
        void validate_json(const json &j) const
        {
            if (!j.is_object())
            {
                throw std::invalid_argument("Invalid safety_interface_config format: not a JSON object");
            }

            // Check for all required fields
            const std::vector<std::string> required_fields = {
                "m12_safety_pins_configuration",
                "gpio_stabilization_interval",
                "camera_position",
                "occupancy_grid_params",
                "smcu_arbitration_params",
                "crypto_signature"};

            for (const auto &field : required_fields)
            {
                if (!j.contains(field))
                {
                    throw std::invalid_argument("Invalid safety_interface_config format: missing field: " + field);
                }
            }

            // Validate m12_safety_pins_configuration
            if (!j["m12_safety_pins_configuration"].is_object())
            {
                throw std::invalid_argument("Invalid format: m12_safety_pins_configuration must be a JSON object");
            }

            // Validate gpio_stabilization_interval
            if (!j["gpio_stabilization_interval"].is_number_unsigned())
            {
                throw std::invalid_argument("Invalid format: gpio_stabilization_interval must be an unsigned number");
            }

            // Validate camera_position
            if (!j["camera_position"].is_object()) // Assuming camera_position is a complex object
            {
                throw std::invalid_argument("Invalid format: camera_position must be a JSON object");
            }

            // Validate occupancy_grid_params
            if (!j["occupancy_grid_params"].is_object())
            {
                throw std::invalid_argument("Invalid format: occupancy_grid_params must be a JSON object");
            }

            // Validate smcu_arbitration_params
            if (!j["smcu_arbitration_params"].is_object())
            {
                throw std::invalid_argument("Invalid format: smcu_arbitration_params must be a JSON object");
            }

            // Validate crypto_signature (should be an array of 32 elements)
            if (!j["crypto_signature"].is_array() || j["crypto_signature"].size() != 32)
            {
                throw std::invalid_argument("Invalid format: crypto_signature must be an array of 32 unsigned integers");
            }
            for (const auto &val : j["crypto_signature"])
            {
                if (!val.is_number_unsigned())
                {
                    throw std::invalid_argument("Invalid format: crypto_signature contains non-unsigned values");
                }
            }
        }

        m12_safety_pins_configuration m_pins_configs;
        uint8_t m_gpio_stabilization_interval;
        camera_position m_camera_position;
        occupancy_grid_params m_occupancy_grid_params;
        smcu_arbitration_params m_smcu_arbitration_params;
        std::array<uint8_t, 32> m_crypto_signature; // SHA2 or similar
        std::array<uint8_t, 17> m_reserved2 = {0};  // Can be modified by changing the table minor version, without breaking back-compat
    };

    /***
     *  safety_interface_config_with_header class
     *  Consists of safety interface config table and a table header
     */
    class safety_interface_config_with_header
    {
    public:
        safety_interface_config_with_header(table_header header, safety_interface_config sic) : m_header(header), m_safety_interface_config(sic)
        {
        }

        safety_interface_config get_safety_interface_config() const
        {
            return m_safety_interface_config;
        }

        table_header get_table_header() const
        {
            return m_header;
        }

    private:
        table_header m_header;
        safety_interface_config m_safety_interface_config;
    };

#pragma pack(pop)
}
