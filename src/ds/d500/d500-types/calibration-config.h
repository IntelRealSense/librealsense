// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include "common.h"

namespace librealsense
{

#pragma pack(push, 1)

    /*
    calibration_config class
    Handles JSONS represnting calibration config table
    JSON schema:
    {
        "roi_num_of_segments": 0,
        "roi_0":
        {
            "vertex_0": [0, 0],
            "vertex_1": [0, 0],
            "vertex_2": [0, 0],
            "vertex_3": [0, 0]
        },
        "roi_1":
        {
            "vertex_0": [0, 0],
            "vertex_1": [0, 0],
            "vertex_2": [0, 0],
            "vertex_3": [0, 0]
        },
        "roi_2":
        {
            "vertex_0": [0, 0],
            "vertex_1": [0, 0],
            "vertex_2": [0, 0],
            "vertex_3": [0, 0]
        },
        "roi_3":
        {
            "vertex_0": [0, 0],
            "vertex_1": [0, 0],
            "vertex_2": [0, 0],
            "vertex_3": [0, 0]
        },
        "camera_position":
        {
            "rotation":
            [
                [ 0.0,  0.0,  1.0],
                [-1.0,  0.0,  0.0],
                [ 0.0, -1.0,  0.0]
            ],
            "translation": [0.0, 0.0, 0.4]
        },
        "crypto_signature": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    }
    */
    class calibration_config
    {
    public:
        calibration_config(const json &j)
        {
            validate_json(j);

            m_roi_num_of_segments = j.at("roi_num_of_segments").get<uint8_t>();

            for (size_t i = 0; i < 4; ++i)
            {
                std::string roi_field = "roi_" + std::to_string(i);
                m_rois[i] = roi(j.at(roi_field));
            }

            m_camera_position = camera_position(j.at("camera_position"));

            for (size_t i = 0; i < 32; ++i)
            {
                m_crypto_signature[i] = j.at("crypto_signature")[i].get<uint8_t>();
            }
        }

        json to_json() const
        {
            json j;
            auto &calib_config = j["calibration_config"];
            calib_config["roi_num_of_segments"] = m_roi_num_of_segments;
            for (size_t i = 0; i < 4; ++i)
            {
                std::string roi_field = "roi_" + std::to_string(i);
                calib_config[roi_field] = m_rois[i].to_json();
            }
            calib_config["camera_position"] = m_camera_position.to_json();
            calib_config["crypto_signature"] = m_crypto_signature;
            return j;
        }

    private:
        void validate_json(const json &j) const
        {
            if (!j.is_object() || j.size() != 7)
            {
                throw std::invalid_argument(
                    "Invalid calibration_config format: calibration_config must include 'roi_num_of_segments', "
                    "'roi_0', 'roi_1', 'roi_2', 'roi_3', 'camera_position', and 'crypto_signature'");
            }
            if (!j.at("roi_num_of_segments").is_number_unsigned())
            {
                throw std::invalid_argument("Invalid roi_num_of_segments type: roi_num_of_segments must be unsigned integer");
            }
            for (size_t i = 0; i < 4; ++i)
            {
                std::string roi_field = "roi_" + std::to_string(i);
                if (!j.at(roi_field).is_object())
                {
                    throw std::invalid_argument("Invalid " + roi_field + " format");
                }
            }
            if (!j.at("camera_position").is_object())
            {
                throw std::invalid_argument("Invalid camera_position format");
            }
            if (!j.at("crypto_signature").is_array() || j.at("crypto_signature").size() != 32)
            {
                throw std::invalid_argument("Invalid crypto_signature format: crypto_signature must be an array of size=32");
            }
            for (size_t i = 0; i < 32; ++i)
            {
                if (!j.at("crypto_signature")[i].is_number_unsigned())
                {
                    throw std::invalid_argument("Invalid crypto_signature type: all elements in crypto_signature array must be unsigned integers");
                }
            }
        }

    private:
        uint8_t m_roi_num_of_segments; // Calibration ROI number of segments
        std::array<roi, 4> m_rois;     // Calibration 4 ROIs
        std::array<uint8_t, 12> m_reserved1 = {0};

        camera_position m_camera_position; // Camera extrinsics: rotation 3x3 matrix (row major) + translation vecotr
        std::array<uint8_t, 300> m_reserved2 = {0};

        std::array<uint8_t, 32> m_crypto_signature; // SHA2 or similar
        std::array<uint8_t, 39> m_reserved3 = {0};
    };

    /***
     *  calibration_config_with_header class
     *  Consists of calibration config table and a table header
     *  According to flash 0.92:
     *      Version    major.minor: 0x01 0x01
     *      table type    ctCalibCFG(0xC0DD)
     *      table size    496
     */
    class calibration_config_with_header
    {
    public:
        calibration_config_with_header(table_header header, const calibration_config &calib_config) : m_header(header), m_calib_config(calib_config)
        {
        }

        calibration_config get_calibration_config() const
        {
            return m_calib_config;
        }

        table_header get_table_header() const
        {
            return m_header;
        }

    private:
        table_header m_header;
        calibration_config m_calib_config;
    };

#pragma pack(pop)
}
