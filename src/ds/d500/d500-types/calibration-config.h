// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include "common.h"

namespace librealsense
{

#pragma pack(push, 1)

    /*
    vertex class
    handles JSON representing 2D Vertex (x,y)
    JSON schema: [ float, float ]
    */
    class vertex
    {
    public:
        vertex() : m_x(0), m_y(0) {}

        vertex(const json &j)
        {
            validate_json(j);
            m_x = j[0].get<uint16_t>();
            m_y = j[1].get<uint16_t>();
        }

        json to_json() const
        {
            return json::array({m_x, m_y});
        }

    private:
        void validate_json(const json &j) const
        {
            if (!j.is_array() || j.size() != 2)
            {
                throw std::invalid_argument("Invalid Vertex format: each vertex should be an array of size=2");
            }
            if (!j[0].is_number_unsigned() || !j[1].is_number_unsigned())
            {
                throw std::invalid_argument("Invalid Vertex type: Each vertex must include only unsigned integers");
            }
        }

    private:
        uint16_t m_x;
        uint16_t m_y;
    };

    /*
    roi class
    handles JSON of 4 vertices representing region of interest
    JSON schema:
    {
        "vertex_0": [0, 0],
        "vertex_1": [0, 0],
        "vertex_2": [0, 0],
        "vertex_3": [0, 0]
    }
    */
    class roi
    {
    public:
        roi() = default;

        roi(const json &j)
        {
            validate_json(j);
            m_vertex_0 = vertex(j.at("vertex_0"));
            m_vertex_1 = vertex(j.at("vertex_1"));
            m_vertex_2 = vertex(j.at("vertex_2"));
            m_vertex_3 = vertex(j.at("vertex_3"));
        }

        json to_json() const
        {
            return {
                {"vertex_0", m_vertex_0.to_json()},
                {"vertex_1", m_vertex_1.to_json()},
                {"vertex_2", m_vertex_2.to_json()},
                {"vertex_3", m_vertex_3.to_json()}};
        }

    private:
        void validate_json(const json &j) const
        {
            if (!j.is_object())
            {
                throw std::invalid_argument("Invalid ROI format");
            }
            for (const auto &field : {"vertex_0", "vertex_1", "vertex_2", "vertex_3"})
            {
                if (!j.contains(field))
                {
                    throw std::invalid_argument(std::string("Invalid ROI format: missing field: ") + field);
                }
            }
        }

    private:
        vertex m_vertex_0;
        vertex m_vertex_1;
        vertex m_vertex_2;
        vertex m_vertex_3;
    };

    /*
    camera_position class
    handles JSON of representing the camera positon: rotation row major 3x3 matrix and translation vector
    all values are floats.
    JSON schema:
    {
        "rotation":
            [
                [a, b, c],
                [d, e, f],
                [g, h, i]
            ],
        "translation": {x, y, z}
    }
    */
    class camera_position
    {

    public:
        camera_position() = default;

        camera_position(const json &j)
        {
            validate_json(j);
            for (size_t i = 0; i < 3; ++i)
            {
                for (size_t k = 0; k < 3; ++k)
                {
                    m_rotation[i][k] = j.at("rotation")[i][k].get<float>();
                }
            }
            for (size_t i = 0; i < 3; ++i)
            {
                m_translation[i] = j.at("translation")[i].get<float>();
            }
        }

        json to_json() const
        {
            json j;
            j["rotation"] = json::array();
            for (size_t i = 0; i < 3; ++i)
            {
                j["rotation"].push_back(json::array());
                for (size_t k = 0; k < 3; ++k)
                {
                    j["rotation"][i].push_back(m_rotation[i][k]);
                }
            }
            j["translation"] = m_translation;
            return j;
        }

    private:
        void validate_json(const json &j) const
        {
            if (!j.is_object() || j.size() != 2 || !j.contains("rotation") || !j.contains("translation"))
            {
                throw std::invalid_argument("Invalid camera_position format: camera_position must include rotation and translation fields");
            }
            if (!j.at("rotation").is_array() || j.at("rotation").size() != 3)
            {
                throw std::invalid_argument("Invalid rotation format: rotation must be a 3x3 matrix");
            }
            for (size_t i = 0; i < 3; ++i)
            {
                if (!j.at("rotation")[i].is_array() || j.at("rotation")[i].size() != 3)
                {
                    throw std::invalid_argument("Invalid rotation row format: rotation must be a 3x3 matrix");
                }
                for (size_t k = 0; k < 3; ++k)
                {
                    if (!j.at("rotation")[i][k].is_number_float() && !j.at("rotation")[i][k].is_number_integer())
                    {
                        throw std::invalid_argument("Invalid rotation type: all rotation values must be floats.");
                    }
                }
            }
            if (!j.at("translation").is_array() || j.at("translation").size() != 3)
            {
                throw std::invalid_argument("Invalid translation format: translation vector should be an array of size=3");
            }
            for (size_t i = 0; i < 3; ++i)
            {
                if (!j.at("translation")[i].is_number_float())
                {
                    throw std::invalid_argument("Invalid translation type: translation vector values should be floats");
                }
            }
        }

    private:
        std::array<std::array<float, 3>, 3> m_rotation; // row major order
        std::array<float, 3> m_translation;
    };

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
