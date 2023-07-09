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

inline std::string sc_mos_target_to_string(rs2_safety_mos_type const& mos) {
    std::string result;
    switch (mos)
    {
    case rs2_safety_mos_type_value::mos_hand:
        result = "HAND";
        break;
    case rs2_safety_mos_type_value::mos_leg:
        result = "LEG";
        break;
    case rs2_safety_mos_type_value::mos_body:
        result = "BODY";
        break;
    default:
        result = "UNKNOWN";
        break;
    }
    return result;
}

inline std::ostream& operator<<(std::ostream& out, rs2_safety_zone const& sz)
{
    out << "\n\t\t" << "Zone Polygon - Point #0: { x:" << sz.zone_polygon[0].x << ", y:" << sz.zone_polygon[0].y << " }";
    out << "\n\t\t" << "Zone Polygon - Point #1: { x:" << sz.zone_polygon[1].x << ", y:" << sz.zone_polygon[1].y << " }";
    out << "\n\t\t" << "Zone Polygon - Point #2: { x:" << sz.zone_polygon[2].x << ", y:" << sz.zone_polygon[2].y << " }";
    out << "\n\t\t" << "Zone Polygon - Point #3: { x:" << sz.zone_polygon[3].x << ", y:" << sz.zone_polygon[3].y << " }";
    out << "\n\t\t" << "Safety Trigger Confidence: " << (int)(sz.safety_trigger_confidence);
    out << "\n\t\t" << "Minimum Object Size - Diameter: " << sz.minimum_object_size.x << " [mm]";
    out << "\n\t\t" << "Minimum Object Size - Length: " << sz.minimum_object_size.y << " [mm]";
    out << "\n\t\t" << "Minimum Object Size - Target Type: " << sc_mos_target_to_string(sz.mos_target_type);
    out << "\n\t\t" << "Reserved[8]: " << sc_reserved_arr_to_string(sz.reserved, 8);
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
    out << "\t\t" << "Grid Cell Size: " << se.grid_cell_size << " [m]";
    out << "\n\t\t" << "Safety Trigger Duration: " << se.safety_trigger_duration << " [sec]";
    out << "\n\t\t" << "Linear Velocity: " << se.linear_velocity << " [m/sec]";
    out << "\n\t\t" << "Angular Velocity: " << se.angular_velocity << " [rad/sec]";
    out << "\n\t\t" << "Payload Weight: " << se.payload_weight << " [kg]";
    out << "\n\t\t" << "Surface Inclination: " << se.surface_inclination << " [deg]";
    out << "\n\t\t" << "Surface Height: " << se.surface_height << " [m]";
    out << "\n\t\t" << "Surface Confidence: " << (int)(se.surface_confidence) << " [%]";
    out << "\n\t\t" << "Reserved[15]: " << sc_reserved_arr_to_string(se.reserved, 15);
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

inline std::ostream& operator<<(std::ostream& out, rs2_safety_preset const& sp)
{
    out << "Safety Preset:";
    out << "\n\t" << "Platform Config:";
    out << "\n\t\t" << "Transformation Link:" << "\n" << sp.platform_config.transformation_link;
    out << "\n\t\t" << "Robot Height: " << sp.platform_config.robot_height << " [m]";
    out << "\n\t\t" << "Robot Mass: " << sp.platform_config.robot_mass << " [kg]";
    out << "\n\t\t" << "Resereved[16]: " << sc_reserved_arr_to_string(sp.platform_config.reserved, 16);
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
    out << "\t\t" << "gpio stabilization interval: " << "\t\t" << sic.payload.gpio_stabilization_interval << "\n";
    out << "\t\t" << "safety zone selection overlap time period: " << "\t\t" << sic.payload.safety_zone_selection_overlap_time_period << "\n";
    return out;
}

inline bool operator==(rs2_safety_interface_config_with_header const& self, rs2_safety_interface_config_with_header const& other)
{
    return !std::memcmp(&self, &other, sizeof(rs2_safety_interface_config_with_header));
}

#endif // LIBREALSENSE_RS2_SAFETY_TYPES_HPP
