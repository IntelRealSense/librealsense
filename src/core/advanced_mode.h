// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include "ds5/ds5-private.h"
#include "hw-monitor.h"
#include "streaming.h"
#define RS4XX_ADVANCED_MODE_HPP
#include "../rs4xx/rs4xx_advanced_mode/include/advanced_mode_command.h"
#undef RS4XX_ADVANCED_MODE_HPP


namespace librealsense
{
    template<class T>
    struct advanced_mode_traits;

    #define MAP_ADVANCED_MODE(T, E) template<> struct advanced_mode_traits<T> { static const EtAdvancedModeRegGroup group = E; }

    MAP_ADVANCED_MODE(STDepthControlGroup, etDepthControl);
    MAP_ADVANCED_MODE(STRsm, etRsm);
    MAP_ADVANCED_MODE(STRauSupportVectorControl, etRauSupportVectorControl);
    MAP_ADVANCED_MODE(STColorControl, etColorControl);
    MAP_ADVANCED_MODE(STRauColorThresholdsControl, etRauColorThresholdsControl);
    MAP_ADVANCED_MODE(STSloColorThresholdsControl, etSloColorThresholdsControl);
    MAP_ADVANCED_MODE(STSloPenaltyControl, etSloPenaltyControl);
    MAP_ADVANCED_MODE(STHdad, etHdad);
    MAP_ADVANCED_MODE(STColorCorrection, etColorCorrection);
    MAP_ADVANCED_MODE(STDepthTableControl, etDepthTableControl);
    MAP_ADVANCED_MODE(STAEControl, etAEControl);
    MAP_ADVANCED_MODE(STCensusRadius, etCencusRadius9);


    class ds5_advanced_mode_interface : public virtual device_interface
    {
    public:
        virtual bool is_enabled() const = 0;

        virtual void go_to_advanced_mode() const = 0;

        virtual void get_depth_control_group(STDepthControlGroup* ptr, int mode = 0) const = 0;
        virtual void get_rsm(STRsm* ptr, int mode = 0) const = 0;
        virtual void get_rau_support_vector_control(STRauSupportVectorControl* ptr, int mode = 0) const = 0;
        virtual void get_color_control(STColorControl* ptr, int mode = 0) const = 0;
        virtual void get_rau_color_thresholds_control(STRauColorThresholdsControl* ptr, int mode = 0) const = 0;
        virtual void get_slo_color_thresholds_control(STSloColorThresholdsControl* ptr, int mode = 0) const = 0;
        virtual void get_slo_penalty_control(STSloPenaltyControl* ptr, int mode = 0) const = 0;
        virtual void get_hdad(STHdad* ptr, int mode = 0) const = 0;
        virtual void get_color_correction(STColorCorrection* ptr, int mode = 0) const = 0;
        virtual void get_depth_table_control(STDepthTableControl* ptr, int mode = 0) const = 0;
        virtual void get_ae_control(STAEControl* ptr, int mode = 0) const = 0;
        virtual void get_census_radius(STCensusRadius* ptr, int mode = 0) const = 0;

        virtual void set_depth_control_group(const STDepthControlGroup& val) = 0;
        virtual void set_rsm(const STRsm& val) = 0;
        virtual void set_rau_support_vector_control(const STRauSupportVectorControl& val) = 0;
        virtual void set_color_control(const STColorControl& val) = 0;
        virtual void set_rau_color_thresholds_control(const STRauColorThresholdsControl& val) = 0;
        virtual void set_slo_color_thresholds_control(const STSloColorThresholdsControl& val) = 0;
        virtual void set_slo_penalty_control(const STSloPenaltyControl& val) = 0;
        virtual void set_hdad(const STHdad& val) = 0;
        virtual void set_color_correction(const STColorCorrection& val) = 0;
        virtual void set_depth_table_control(const STDepthTableControl& val) = 0;
        virtual void set_ae_control(const STAEControl& val) = 0;
        virtual void set_census_radius(const STCensusRadius& val) = 0;

        virtual ~ds5_advanced_mode_interface() = default;
    };

    class ds5_advanced_mode_base : public ds5_advanced_mode_interface
    {
    public:
        explicit ds5_advanced_mode_base(std::shared_ptr<hw_monitor> hwm)
            : _hw_monitor(hwm) {}

        bool is_enabled() const
        {
            auto results = send_recieve(encode_command(ds::fw_cmd::advanced_mode_enabled));
            assert_no_error(ds::fw_cmd::advanced_mode_enabled, results);
            return *(reinterpret_cast<uint32_t*>(results.data()) + 1) > 0;
        }

        void go_to_advanced_mode() const
        {
            send_recieve(encode_command(ds::fw_cmd::enable_advanced_mode, 1));
            send_recieve(encode_command(ds::fw_cmd::reset));
        }

        void get_depth_control_group(STDepthControlGroup* ptr, int mode = 0) const
        {
            *ptr = get<STDepthControlGroup>(advanced_mode_traits<STDepthControlGroup>::group, nullptr, mode);
        }

        void get_rsm(STRsm* ptr, int mode = 0) const
        {
            *ptr = get<STRsm>(advanced_mode_traits<STRsm>::group, nullptr, mode);
        }

        void get_rau_support_vector_control(STRauSupportVectorControl* ptr, int mode = 0) const
        {
            *ptr = get<STRauSupportVectorControl>(advanced_mode_traits<STRauSupportVectorControl>::group, nullptr, mode);
        }

        void get_color_control(STColorControl* ptr, int mode = 0) const
        {
            *ptr = get<STColorControl>(advanced_mode_traits<STColorControl>::group, nullptr, mode);
        }

        void get_rau_color_thresholds_control(STRauColorThresholdsControl* ptr, int mode = 0) const
        {
            *ptr = get<STRauColorThresholdsControl>(advanced_mode_traits<STRauColorThresholdsControl>::group, nullptr, mode);
        }

        void get_slo_color_thresholds_control(STSloColorThresholdsControl* ptr, int mode = 0) const
        {
            *ptr = get<STSloColorThresholdsControl>(advanced_mode_traits<STSloColorThresholdsControl>::group, nullptr, mode);
        }

        void get_slo_penalty_control(STSloPenaltyControl* ptr, int mode = 0) const
        {
            *ptr = get<STSloPenaltyControl>(advanced_mode_traits<STSloPenaltyControl>::group, nullptr, mode);
        }

        void get_hdad(STHdad* ptr, int mode = 0) const
        {
            *ptr = get<STHdad>(advanced_mode_traits<STHdad>::group, nullptr, mode);
        }

        void get_color_correction(STColorCorrection* ptr, int mode = 0) const
        {
            *ptr = get<STColorCorrection>(advanced_mode_traits<STColorCorrection>::group, nullptr, mode);
        }

        void get_depth_table_control(STDepthTableControl* ptr, int mode = 0) const
        {
            *ptr = get<STDepthTableControl>(advanced_mode_traits<STDepthTableControl>::group, nullptr, mode);
        }

        void get_ae_control(STAEControl* ptr, int mode = 0) const
        {
            *ptr = get<STAEControl>(advanced_mode_traits<STAEControl>::group, nullptr, mode);
        }

        void get_census_radius(STCensusRadius* ptr, int mode = 0) const
        {
            *ptr = get<STCensusRadius>(advanced_mode_traits<STCensusRadius>::group, nullptr, mode);
        }

        void set_depth_control_group(const STDepthControlGroup& val)
        {
            set(val, advanced_mode_traits<STDepthControlGroup>::group);
        }

        void set_rsm(const STRsm& val)
        {
            set(val, advanced_mode_traits<STRsm>::group);
        }

        void set_rau_support_vector_control(const STRauSupportVectorControl& val)
        {
            set(val, advanced_mode_traits<STRauSupportVectorControl>::group);
        }

        void set_color_control(const STColorControl& val)
        {
            set(val, advanced_mode_traits<STColorControl>::group);
        }

        void set_rau_color_thresholds_control(const STRauColorThresholdsControl& val)
        {
            set(val, advanced_mode_traits<STRauColorThresholdsControl>::group);
        }

        void set_slo_color_thresholds_control(const STSloColorThresholdsControl& val)
        {
            set(val, advanced_mode_traits<STSloColorThresholdsControl>::group);
        }

        void set_slo_penalty_control(const STSloPenaltyControl& val)
        {
            set(val, advanced_mode_traits<STSloPenaltyControl>::group);
        }

        void set_hdad(const STHdad& val)
        {
            set(val, advanced_mode_traits<STHdad>::group);
        }

        void set_color_correction(const STColorCorrection& val)
        {
            set(val, advanced_mode_traits<STColorCorrection>::group);
        }

        void set_depth_table_control(const STDepthTableControl& val)
        {
            set(val, advanced_mode_traits<STDepthTableControl>::group);
        }

        void set_ae_control(const STAEControl& val)
        {
            set(val, advanced_mode_traits<STAEControl>::group);
        }

        void set_census_radius(const STCensusRadius& val)
        {
            set(val, advanced_mode_traits<STCensusRadius>::group);
        }


    private:
        std::shared_ptr<hw_monitor> _hw_monitor;

        static const uint16_t HW_MONITOR_COMMAND_SIZE = 1000;
        static const uint16_t HW_MONITOR_BUFFER_SIZE = 1024;

        std::vector<uint8_t> send_recieve(const std::vector<uint8_t>& input) const
        {
            auto res = _hw_monitor->send(input);
            if (res.empty())
            {
                throw std::runtime_error("Advanced mode write failed!");
            }
            return res;
        }

        template<class T>
        void set(const T& strct, EtAdvancedModeRegGroup cmd) const
        {
            auto ptr = (uint8_t*)(&strct);
            std::vector<uint8_t> data(ptr, ptr + sizeof(T));

            assert_no_error(ds::fw_cmd::set_advanced,
                send_recieve(encode_command(ds::fw_cmd::set_advanced, static_cast<uint32_t>(cmd), 0, 0, 0, data)));
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        template<class T>
        T get(EtAdvancedModeRegGroup cmd, T* ptr = static_cast<T*>(nullptr), int mode = 0) const
        {
            T res;
            auto data = assert_no_error(ds::fw_cmd::get_advanced,
                send_recieve(encode_command(ds::fw_cmd::get_advanced,
                static_cast<uint32_t>(cmd), mode)));
            if (data.size() < sizeof(T))
            {
                throw std::runtime_error("The camera returned invalid sized result!");
            }
            res = *reinterpret_cast<T*>(data.data());
            return res;
        }

        static uint32_t pack(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3)
        {
            return (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
        }

        static std::vector<uint8_t> assert_no_error(ds::fw_cmd opcode, const std::vector<uint8_t>& results)
        {
            if (results.size() < sizeof(uint32_t)) throw std::runtime_error("Incomplete operation result!");
            auto opCodeAsUint32 = pack(results[3], results[2], results[1], results[0]);
            if (opCodeAsUint32 != static_cast<uint32_t>(opcode))
            {
                std::stringstream ss;
                ss << "Operation failed with error code=" << static_cast<int>(opCodeAsUint32);
                throw std::runtime_error(ss.str());
            }
            std::vector<uint8_t> result;
            result.resize(results.size() - sizeof(uint32_t));
            std::copy(results.data() + sizeof(uint32_t),
                      results.data() + results.size(), result.data());
            return result;
        }

        std::vector<uint8_t> encode_command(ds::fw_cmd opcode,
            uint32_t p1 = 0,
            uint32_t p2 = 0,
            uint32_t p3 = 0,
            uint32_t p4 = 0,
            std::vector<uint8_t> data = std::vector<uint8_t>()) const
        {
            std::vector<uint8_t> raw_data;
            auto cmd_op_code = static_cast<uint32_t>(opcode);

            const uint16_t pre_header_data = 0xcdab;
            raw_data.resize(HW_MONITOR_BUFFER_SIZE);
            auto write_ptr = raw_data.data();
            auto header_size = 4;

            auto cur_index = 2;
            *reinterpret_cast<uint16_t *>(write_ptr + cur_index) = pre_header_data;
            cur_index += sizeof(uint16_t);
            *reinterpret_cast<unsigned int *>(write_ptr + cur_index) = cmd_op_code;
            cur_index += sizeof(unsigned int);

            // Parameters
            *reinterpret_cast<unsigned*>(write_ptr + cur_index) = p1;
            cur_index += sizeof(unsigned);
            *reinterpret_cast<unsigned*>(write_ptr + cur_index) = p2;
            cur_index += sizeof(unsigned);
            *reinterpret_cast<unsigned*>(write_ptr + cur_index) = p3;
            cur_index += sizeof(unsigned);
            *reinterpret_cast<unsigned*>(write_ptr + cur_index) = p4;
            cur_index += sizeof(unsigned);

            // Data
            std::copy(data.begin(), data.end(), reinterpret_cast<uint8_t*>(write_ptr + cur_index));
            cur_index += data.size();

            *reinterpret_cast<uint16_t*>(raw_data.data()) = static_cast<uint16_t>(cur_index - header_size);// Length doesn't include hdr.
            raw_data.resize(cur_index);
            return raw_data;
        }
    };
}
