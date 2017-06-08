// See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifndef R400_ADVANCED_MODE_HPP
#define R400_ADVANCED_MODE_HPP

#include <vector>
#include <sstream>
#include <thread>
#include <chrono>

namespace rs400
{
    enum class command
    {
        enable_advanced_mode = 0x2d,
        advanced_mode_enabled = 0x30,
        reset = 0x20,
        set_advanced = 0x2B,
        get_advanced = 0x2C,
    };

#pragma pack(push, 1)
#include "AdvancedModeCommand.h"
#pragma pack(pop)

    typedef std::function<std::vector<uint8_t>(std::vector<uint8_t>)> send_receive;

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

    class advanced_mode
    {
    public:
        explicit advanced_mode(send_receive action)
            : _perform(std::move(action)) {}

        bool is_enabled() const
        {
            auto results = _perform(encode_command(command::advanced_mode_enabled));
            assert_no_error(command::advanced_mode_enabled, results);
            return *(reinterpret_cast<uint32_t*>(results.data()) + 1) > 0;
        }

        void go_to_advanced_mode() const
        {
            _perform(encode_command(command::enable_advanced_mode, 1));
            _perform(encode_command(command::reset));
        }

        template<class T>
        void get(T* ptr, int mode = 0) const
        {
            *ptr = get<T>(advanced_mode_traits<T>::group, nullptr, mode);
        }

        template<class T>
        void set(const T& val) const
        {
            set(val, advanced_mode_traits<T>::group);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

    private:
        send_receive _perform;

        const uint16_t HW_MONITOR_COMMAND_SIZE = 1000;
        const uint16_t HW_MONITOR_BUFFER_SIZE = 1024;

        template<class T>
        void set(const T& strct, EtAdvancedModeRegGroup cmd) const
        {
            auto ptr = (uint8_t*)(&strct);
            std::vector<uint8_t> data(ptr, ptr + sizeof(T));

            assert_no_error(command::set_advanced,
                _perform(encode_command(command::set_advanced, static_cast<uint32_t>(cmd), 0, 0, 0, data)));
        }

        template<class T>
        T get(EtAdvancedModeRegGroup cmd, T* ptr = static_cast<T*>(nullptr), int mode = 0) const
        {
            T res;
            auto data = assert_no_error(command::get_advanced,
                _perform(encode_command(command::get_advanced,
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

        static std::vector<uint8_t> assert_no_error(command opcode, const std::vector<uint8_t>& results)
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

        std::vector<uint8_t> encode_command(command opcode,
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

#endif // R400_ADVANCED_MODE_HPP
