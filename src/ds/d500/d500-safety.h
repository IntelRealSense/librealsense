// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "d500-device.h"
#include "safety-sensor.h"
#include "core/video.h"

namespace librealsense
{
#pragma pack(push, 1)
    struct safety_interface_config_pin
    {
        uint8_t _direction;
        uint8_t _functionality;

        safety_interface_config_pin(uint8_t direction, uint8_t functionality) :
            _direction(direction), _functionality(functionality) {}

        safety_interface_config_pin(rs2_safety_interface_config_pin direction,
            rs2_safety_interface_config_pin functionality) :
            _direction(*reinterpret_cast<uint8_t*>(&direction)),
            _functionality(*reinterpret_cast<uint8_t*>(&functionality)) {}
    };
    struct safety_interface_config
    {
        safety_interface_config_pin _power;
        safety_interface_config_pin _ossd1_b;
        safety_interface_config_pin _ossd1_a;
        safety_interface_config_pin _preset3_a;
        safety_interface_config_pin _preset3_b;
        safety_interface_config_pin _preset4_a;
        safety_interface_config_pin _preset1_b;
        safety_interface_config_pin _preset1_a;
        safety_interface_config_pin _gpio_0;
        safety_interface_config_pin _gpio_1;
        safety_interface_config_pin _gpio_3;
        safety_interface_config_pin _gpio_2;
        safety_interface_config_pin _preset2_b;
        safety_interface_config_pin _gpio_4;
        safety_interface_config_pin _preset2_a;
        safety_interface_config_pin _preset4_b;
        safety_interface_config_pin _ground;
        uint8_t _gpio_stabilization_interval;
        uint8_t _safety_zone_selection_overlap_time_period;
        uint8_t _reserved[20];

        safety_interface_config(const rs2_safety_interface_config& sic)
            : _power(sic.power.direction, sic.power.functionality),
            _ossd1_b(sic.ossd1_b.direction, sic.ossd1_b.functionality),
            _ossd1_a(sic.ossd1_a.direction, sic.ossd1_a.functionality),
            _preset3_a(sic.preset3_a.direction, sic.preset3_a.functionality),
            _preset3_b(sic.preset3_b.direction, sic.preset3_b.functionality),
            _preset4_a(sic.preset4_a.direction, sic.preset4_a.functionality),
            _preset1_b(sic.preset1_b.direction, sic.preset1_b.functionality),
            _preset1_a(sic.preset1_a.direction, sic.preset1_a.functionality),
            _gpio_0(sic.gpio_0.direction, sic.gpio_0.functionality),
            _gpio_1(sic.gpio_1.direction, sic.gpio_1.functionality),
            _gpio_3(sic.gpio_3.direction, sic.gpio_3.functionality),
            _gpio_2(sic.gpio_2.direction, sic.gpio_2.functionality),
            _preset2_b(sic.preset2_b.direction, sic.preset2_b.functionality),
            _gpio_4(sic.gpio_4.direction, sic.gpio_4.functionality),
            _preset2_a(sic.preset2_a.direction, sic.preset2_a.functionality),
            _preset4_b(sic.preset4_b.direction, sic.preset4_b.functionality),
            _ground(sic.ground.direction, sic.ground.functionality),
            _gpio_stabilization_interval(sic.gpio_stabilization_interval),
            _safety_zone_selection_overlap_time_period(sic.safety_zone_selection_overlap_time_period)
        {
            std::memset(_reserved, 0, 20);
        }

    };

    struct safety_interface_config_header
    {
        uint16_t version;       // major.minor. Big-endian
        uint16_t table_type;    // type
        uint32_t table_size;    // full size including: header footer
        uint32_t calib_version; // major.minor.index
        uint32_t crc32;         // crc of all the data in table excluding this header/CRC
    };

    struct safety_interface_config_with_header
    {
        safety_interface_config_header header;
        safety_interface_config config;

        safety_interface_config_with_header(const safety_interface_config& cfg) :
            header(),
            config(cfg) {}
    };
#pragma pack(pop)

    class d500_safety_sensor;

    class d500_safety : public virtual d500_device
    {
    public:
        std::shared_ptr<synthetic_sensor> create_safety_device(std::shared_ptr<context> ctx,
            const std::vector<platform::uvc_device_info>& safety_devices_info);

        d500_safety( std::shared_ptr< const d500_info > const & dev_info );

        synthetic_sensor& get_safety_sensor()
        {
            return dynamic_cast<synthetic_sensor&>(get_sensor(_safety_device_idx));
        }

    private:

        friend class d500_safety_sensor;

        void register_options(std::shared_ptr<d500_safety_sensor> safety_ep, std::shared_ptr<uvc_sensor> raw_safety_sensor);
        void register_metadata(std::shared_ptr<uvc_sensor> safety_ep);
        void register_processing_blocks(std::shared_ptr<d500_safety_sensor> safety_ep);

        void gate_depth_option( rs2_option opt,
                                synthetic_sensor & depth_sensor,
                                const std::vector< std::tuple< std::shared_ptr< option >, float, std::string > > & options_and_reasons );

    protected:
        std::shared_ptr<stream_interface> _safety_stream;
        uint8_t _safety_device_idx;
    };

    class d500_safety_sensor : public synthetic_sensor,
                              public video_sensor_interface,
                              public safety_sensor
    {
    public:
        explicit d500_safety_sensor(d500_safety* owner,
            std::shared_ptr<uvc_sensor> uvc_sensor,
            std::map<uint32_t, rs2_format> safety_fourcc_to_rs2_format,
            std::map<uint32_t, rs2_stream> safety_fourcc_to_rs2_stream)
            : synthetic_sensor("Safety Camera", uvc_sensor, owner, safety_fourcc_to_rs2_format, safety_fourcc_to_rs2_stream),
            _owner(owner)
        {}

        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override;
        stream_profiles init_stream_profiles() override;

        void set_safety_preset(int index, const rs2_safety_preset& sp) const override;
        rs2_safety_preset get_safety_preset(int index) const override;

        void set_safety_interface_config(const rs2_safety_interface_config& sic) const override;
        rs2_safety_interface_config get_safety_interface_config(rs2_calib_location loc = RS2_CALIB_LOCATION_RAM) const override;
    protected:
        rs2_safety_interface_config generate_from_squeezed_structure(const safety_interface_config& cfg) const;
        rs2_safety_interface_config_pin generate_from_safety_interface_config_pin(const safety_interface_config_pin& pin) const;

        const d500_safety* _owner;
    };

    // subdevice[h] unit[fw], node[h] guid[fw]
    const platform::extension_unit safety_xu = { 4, 0xC, 2,
    { 0xf6c3c3d1, 0x5cde, 0x4477,{ 0xad, 0xf0, 0x41, 0x33, 0xf5, 0x8d, 0xa6, 0xf4 } } };

    namespace xu_id
    {
        const uint8_t SAFETY_CAMERA_OPER_MODE = 0x1;
        const uint8_t SAFETY_PRESET_ACTIVE_INDEX = 0x2;
    }
}
