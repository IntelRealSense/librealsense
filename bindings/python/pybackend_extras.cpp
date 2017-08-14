#include "pybackend_extras.h"
#include <cinttypes>

using namespace librealsense;

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../third-party/stb_image_write.h"

// 123e4567-e89b-12d3-a456-426655440000
namespace pybackend2 {

    platform::guid stoguid(std::string str)
    {
        platform::guid g;
        sscanf(str.c_str(), "%8" SCNx32"-%4" SCNx16"-%4" SCNx16"-%2" SCNx8"%2" SCNx8"-%2"
                            SCNx8"%2" SCNx8"%2" SCNx8"%2" SCNx8"%2" SCNx8"%2" SCNx8,
               &g.data1, &g.data2, &g.data3, g.data4, g.data4+1, g.data4+2,
               g.data4+3, g.data4+4, g.data4+5, g.data4+6, g.data4+7);

        return g;
    }

    std::vector<uint8_t> encode_command(command opcode,
        uint32_t p1 = 0,
        uint32_t p2 = 0,
        uint32_t p3 = 0,
        uint32_t p4 = 0,
        std::vector<uint8_t> data = std::vector<uint8_t>())
    {
        std::vector<uint8_t> raw_data;
        auto cmd_op_code = static_cast<uint32_t>(opcode);

        const uint16_t pre_header_data = 0xcdab;
        raw_data.resize(HW_MONITOR_BUFFER_SIZE);
        auto write_ptr = raw_data.data();
        size_t header_size = 4;

        size_t cur_index = 2;
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

}
