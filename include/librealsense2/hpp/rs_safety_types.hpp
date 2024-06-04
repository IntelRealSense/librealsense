// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_SAFETY_TYPES_HPP
#define LIBREALSENSE_RS2_SAFETY_TYPES_HPP

#include "../h/rs_safety_types.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <iomanip>

inline std::string sc_reserved_arr_to_string(const uint8_t* data, size_t len)
{
    std::stringstream ss;
    for (auto i = 0; i < len; i++) {
        std::stringstream hex_represntation;
        hex_represntation << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(data[i]);
        ss << " [" << i << "]=0x" << hex_represntation.str() << "\t";
    }
    return ss.str();
}

inline std::ostream& operator<<(std::ostream& out, rs2_safety_zone const& sz)
{
    out << "\n\t\t" << "Zone Polygon - Point #0: { x:" << sz.zone_polygon[0].x << ", y:" << sz.zone_polygon[0].y << " }";
    out << "\n\t\t" << "Zone Polygon - Point #1: { x:" << sz.zone_polygon[1].x << ", y:" << sz.zone_polygon[1].y << " }";
    out << "\n\t\t" << "Zone Polygon - Point #2: { x:" << sz.zone_polygon[2].x << ", y:" << sz.zone_polygon[2].y << " }";
    out << "\n\t\t" << "Zone Polygon - Point #3: { x:" << sz.zone_polygon[3].x << ", y:" << sz.zone_polygon[3].y << " }";
    out << "\n\t\t" << "Safety Trigger Confidence: " << (int)(sz.safety_trigger_confidence);
    out << "\n\t\t" << "Reserved[7]: " << sc_reserved_arr_to_string(sz.reserved, 7);
    return out;
}

inline std::ostream& operator<<(std::ostream& out, rs2_safety_2d_masking_zone const& mz)
{
    out << "\n\t\t" << "Masking Zone Attributes: " << mz.attributes;
    out << "\n\t\t" << "Masking Zone Minimal Range: " << mz.minimal_range;
    for (int pixel = 0 ; pixel < 4 ; pixel++)
    {
        out << "\n\t\t" << "Mask pixel #" << pixel << ": (" << (int)(mz.region_of_interests[pixel].i) << "," << (int)(mz.region_of_interests[pixel].j) << ")";
    }
    return out;
}

inline std::ostream& operator<<(std::ostream& out, rs2_safety_environment const& se)
{
    out << "\t\t" << "Safety Trigger Duration: " << se.safety_trigger_duration << " [sec]";
    out << "\n\t\t" << "Linear Velocity: " << se.linear_velocity << " [m/sec]";
    out << "\n\t\t" << "Angular Velocity: " << se.angular_velocity << " [rad/sec]";
    out << "\n\t\t" << "Payload Weight: " << se.payload_weight << " [kg]";
    out << "\n\t\t" << "Surface Inclination: " << se.surface_inclination << " [deg]";
    out << "\n\t\t" << "Surface Height: " << se.surface_height << " [m]";
    out << "\n\t\t" << "Diagnostic Zone fill rate threshold: " << (int)(se.diagnostic_zone_fill_rate_threshold) << " [%]";
    out << "\n\t\t" << "Floor fill threshold: " << (int)(se.floor_fill_threshold) << " [%]";
    out << "\n\t\t" << "Depth fill threshold: " << (int)(se.depth_fill_threshold) << " [%]";
    out << "\n\t\t" << "Dignostic Zone height median  threshold: " << (int)(se.diagnostic_zone_height_median_threshold);
    out << "\n\t\t" << "Vision HaRa persistency: " << (int)(se.vision_hara_persistency);
    out << "\n\t\t" << "Crypto Signature[32]: " << sc_reserved_arr_to_string(se.crypto_signature, 32);
    out << "\n\t\t" << "Reserved[3]: " << sc_reserved_arr_to_string(se.reserved, 3);
    return out;
}

inline std::ostream& operator<<(std::ostream& out, rs2_safety_extrinsics_table const& ext)
{
    out << "\t\t\t" << "Rotation:";
    out << "\n\t\t\t\t" << "{ x:" << ext.rotation.x.x << ", y:" << ext.rotation.x.y << ", z:" << ext.rotation.x.z << " }";
    out << "\n\t\t\t\t" << "{ x:" << ext.rotation.y.x << ", y:" << ext.rotation.y.y << ", z:" << ext.rotation.y.z << " }";
    out << "\n\t\t\t\t" << "{ x:" << ext.rotation.z.x << ", y:" << ext.rotation.z.y << ", z:" << ext.rotation.z.z << " }";
    out << "\n\t\t\t" << "Translation: { x:" << ext.translation.x << ", y : " << ext.translation.y << ", z : " << ext.translation.z << " }";
    return out;
}

inline std::ostream& operator<<(std::ostream& out, rs2_safety_occupancy_grid_params const& occ_grid_params)
{
    out << "\n\t\t\t\t" << "Grid Cell Seed" << occ_grid_params.grid_cell_seed ;
    out << "\n\t\t\t\t" << "Close Range Quorum" << occ_grid_params.close_range_quorum;
    out << "\n\t\t\t\t" << "Mid Range Quorum" << occ_grid_params.mid_range_quorum;
    out << "\n\t\t\t\t" << "Long Range Quorum" << occ_grid_params.long_range_quorum;
    return out;
}

inline std::ostream& operator<<(std::ostream& out, rs2_safety_smcu_arbitration_params const& smcu_arbitration_params)
{
    out << "\n\t\t\t\t" << "l_0_total_threshold" << smcu_arbitration_params.l_0_total_threshold;
    out << "\n\t\t\t\t" << "l_0_sustained_rate_threshold" << smcu_arbitration_params.l_0_sustained_rate_threshold;
    out << "\n\t\t\t\t" << "l_1_total_threshold" << smcu_arbitration_params.l_1_total_threshold;
    out << "\n\t\t\t\t" << "l_1_sustained_rate_threshold" << smcu_arbitration_params.l_1_sustained_rate_threshold;
    out << "\n\t\t\t\t" << "l_4_total_threshold" << smcu_arbitration_params.l_4_total_threshold;
    out << "\n\t\t\t\t" << "hkr_stl_timeout" << smcu_arbitration_params.hkr_stl_timeout;
    out << "\n\t\t\t\t" << "mcu_stl_timeout" << smcu_arbitration_params.mcu_stl_timeout;
    out << "\n\t\t\t\t" << "sustained_aicv_frame_drops" << smcu_arbitration_params.sustained_aicv_frame_drops;
    out << "\n\t\t\t\t" << "generic_threshold_1" << smcu_arbitration_params.ossd_self_test_pulse_width;
    return out;
}



inline std::ostream& operator<<(std::ostream& out, rs2_safety_preset const& sp)
{
    out << "Safety Preset:";
    out << "\n\t" << "Platform Config:";
    out << "\n\t\t" << "Transformation Link:" << "\n" << sp.platform_config.transformation_link;
    out << "\n\t\t" << "Robot /w Payload Height: " << sp.platform_config.robot_height << " [m]";
    out << "\n\t\t" << "Resereved[20]: " << sc_reserved_arr_to_string(sp.platform_config.reserved, 20);
    out << "\n\t" << "Safety Zone #0 (Danger):" << "\n" << sp.safety_zones[0];
    out << "\n\t" << "Safety Zone #1 (Warning):" << "\n" << sp.safety_zones[1];
    for (int i = 0; i < 8; i++)
    {
        out << "\n\t" << "2D Masking Zone #" << i << ":" << "\n" << sp.masking_zones[i];
    }
    out << "\n\t" << "Reserved[16]: " << sc_reserved_arr_to_string(sp.reserved, 16);
    out << "\n\t" << "Environment:" << "\n" << sp.environment;
    return out;
}

inline bool operator==(rs2_safety_preset const& self, rs2_safety_preset const& other)
{
    return !std::memcmp(&self, &other, sizeof(rs2_safety_preset));
}

inline std::ostream& operator<<(std::ostream& out, rs2_safety_interface_config_with_header const& sic)
{
    out << "Safety Interface Config:\n";
    out << "\t" << "Pins:";
    out << "\t\t" << "direction:"             << "\t\t" << "|" << "\t\t" << "functionality \n";
    out << "\t\t" << sic.payload.power.direction      << "\t\t" << "|" << "\t\t" << sic.payload.power.functionality << "\n";
    out << "\t\t" << sic.payload.ossd1_b.direction    << "\t\t" << "|" << "\t\t" << sic.payload.ossd1_b.functionality << "\n";
    out << "\t\t" << sic.payload.ossd1_a.direction    << "\t\t" << "|" << "\t\t" << sic.payload.ossd1_a.functionality << "\n";
    out << "\t\t" << sic.payload.preset3_a.direction  << "\t\t" << "|" << "\t\t" << sic.payload.preset3_a.functionality << "\n";
    out << "\t\t" << sic.payload.preset3_b.direction  << "\t\t" << "|" << "\t\t" << sic.payload.preset3_b.functionality << "\n";
    out << "\t\t" << sic.payload.preset4_a.direction  << "\t\t" << "|" << "\t\t" << sic.payload.preset4_a.functionality << "\n";
    out << "\t\t" << sic.payload.preset1_b.direction  << "\t\t" << "|" << "\t\t" << sic.payload.preset1_b.functionality << "\n";
    out << "\t\t" << sic.payload.preset1_a.direction  << "\t\t" << "|" << "\t\t" << sic.payload.preset1_a.functionality << "\n";
    out << "\t\t" << sic.payload.gpio_0.direction     << "\t\t" << "|" << "\t\t" << sic.payload.gpio_0.functionality << "\n";
    out << "\t\t" << sic.payload.gpio_1.direction     << "\t\t" << "|" << "\t\t" << sic.payload.gpio_1.functionality << "\n";
    out << "\t\t" << sic.payload.gpio_3.direction     << "\t\t" << "|" << "\t\t" << sic.payload.gpio_3.functionality << "\n";
    out << "\t\t" << sic.payload.gpio_2.direction     << "\t\t" << "|" << "\t\t" << sic.payload.gpio_2.functionality << "\n";
    out << "\t\t" << sic.payload.preset2_b.direction  << "\t\t" << "|" << "\t\t" << sic.payload.preset2_b.functionality << "\n";
    out << "\t\t" << sic.payload.gpio_4.direction     << "\t\t" << "|" << "\t\t" << sic.payload.gpio_4.functionality << "\n";
    out << "\t\t" << sic.payload.preset2_a.direction  << "\t\t" << "|" << "\t\t" << sic.payload.preset2_a.functionality << "\n";
    out << "\t\t" << sic.payload.preset4_b.direction  << "\t\t" << "|" << "\t\t" << sic.payload.preset4_b.functionality << "\n";
    out << "\t\t" << sic.payload.ground.direction     << "\t\t" << "|" << "\t\t" << sic.payload.ground.functionality << "\n";
    out << "\n";
    out << "\t\t" << "gpio stabilization interval: "  << "\t\t" << sic.payload.gpio_stabilization_interval << "\n";
    out << "\t\t" << "Camera position: " << "\t\t" << sic.payload.camera_position << "\n";
    out << "\t\t" << "Occupancy grid params: " << "\t\t" << sic.payload.occupancy_grid_params << "\n";
    out << "\t\t" << "SMCU arbitration params: " << "\t\t" << sic.payload.smcu_arbitration_params << "\n";
    out << "\t\t" << "Crypto signature[32]: " << "\t\t" << sc_reserved_arr_to_string(sic.payload.crypto_signature, 32) << "\n";
    out << "\t\t" << "Reserved[17]: " << "\t\t" << sc_reserved_arr_to_string(sic.payload.reserved, 9) << "\n";

    return out;
}

inline bool operator==(rs2_safety_interface_config_with_header const& self, rs2_safety_interface_config_with_header const& other)
{
    return !std::memcmp(&self, &other, sizeof(rs2_safety_interface_config_with_header));
}

#endif // LIBREALSENSE_RS2_SAFETY_TYPES_HPP
