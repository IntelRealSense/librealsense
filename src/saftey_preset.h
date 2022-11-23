#pragma once

#include "types.h"
#include "core/extension.h"

namespace librealsense
{
	// Convenience blocks
	//struct float2 { float x, y; };
	//struct float3 { float x, y, z; };
	//struct float3x3 { float3 x, y, z; };  // column-major

	struct safety_extrinsics_table
	{
		float3x3            rotation;           // Rotation matrix Sensor->Robot CS (""Leveled World"" assumed)
		float3              translation;        // Metric units
	};

	enum class safety_zone_type : uint8_t
	{
		e_zone_danger = 0,
		e_zone_warning = 1,
		e_zone_mask = 2,
		e_zone_max = 3
	};

	enum class safety_mos_type : uint8_t
	{
		e_mos_hand = 0,
		e_mos_leg = 1,
		e_mos_body = 2,
		e_mos_max = 3,
	};

	enum class safety_zone_flags : uint16_t
	{
		is_mandatory = (1u << 0),
		is_valid = (1u << 1)
	};

	struct safety_preset_header
	{
		big_endian<uint16_t>    version;        // major.minor. Big-endian
		uint16_t                table_type;     // Safety Preset type
		uint32_t                table_size;     // full size including: header footer
		uint32_t                crc32;          // crc of all the data in table excluding this header/CRC
	};

	struct safety_platform
	{
		safety_extrinsics_table    transformation_link;    // Sensor->System Rigid-body Transformation (System CS is ""Leveled World"" assumed)
		float               robot_height;           // meters. Used to calculate the max height for collision scanning
		float               robot_mass;             // mass of SRSS (kg)
		uint8_t             reserved[16];
	};

	struct safety_zone
	{
		uint16_t            flags;                  // see safety_zone_flags enumeration
		safety_zone_type           zone_type;              // see zone_type enumeration
		float2              zone_polygon[4];        // The zone polygon (area) is defined by its four corners, ordered
													// Internal requirements: 
													// - Trinagular or simple quadrilateral shape
													// - Concave or convex, self-intersections not allowed
													// - Area must be greater than zero, thus at least 3 out of 4 vertices must be different
		float2              masking_zone_v_boundary;// For Mask Zone type, denotes the min and max height of the masking region
													// masking_zone_height.x = min, masking_zone_height.y = max

		// Safety Zone Actuator properties
		uint8_t             safety_trigger_confidence;  // number of consecutive frames to raise safety signal

		float2              miminum_object_size;        // MOS is two-dimentional: {diameter (mm), length (mm) }
		safety_mos_type            mos_target_type;            // based on guidance from  IEC 62998-2
		uint8_t             reserved[16];
	};

	struct safety_environment
	{
		float               grid_cell_size;             // Denotes the square tile size for occupancy grid
		float               safety_trigger_duration;    // duration in seconds to keep safety signal high after safety MCU is back to normal
		// Platform dynamics properties
		float               max_linear_velocity;        // m/sec
		float               max_angular_velocity;       // rad/sec
		float               payload_weight;             // a typical mass of the carriage payload in kg
		// Environmetal properties
		float               surface_inclination;        // expected floor min/max inclination angle, degrees
		float               surface_height;             // min height above surface to be used for obstacle avoidance (meter)
		uint8_t             surface_confidence;         // min fill rate required for safe floor detection

		uint8_t             reserved[16];
	};

	struct safety_preset
	{
		safety_preset_header   header;
		safety_platform        platform_config;        //  left camera intrinsic data, normilized
		safety_zone            safety_zones[4];        // Zones: 0 - Danger; 1- Warning; (2,3) - Mask (optiononal)
		safety_environment     environment;            // Provides input for Zone planning and safety algo execution
	};


	class safety_preset_interface : public recordable<safety_preset_interface>
	{
	public:
		virtual std::shared_ptr<safety_preset> get_safety_preset_at_index(int index) = 0;
		virtual void set_safety_preset_at_index(int index, std::shared_ptr<safety_preset> sp) = 0;
	};
	MAP_EXTENSION(RS2_EXTENSION_SAFETY_PRESET, librealsense::safety_preset_interface);

}