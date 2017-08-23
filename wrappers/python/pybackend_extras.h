#include "../src/types.h"

using namespace librealsense;

namespace pybackend2 {
    enum class command
    {
        enable_advanced_mode = 0x2d,
        advanced_mode_enabled = 0x30,
        reset = 0x20,
        set_advanced = 0x2B,
        get_advanced = 0x2C,
    };

#define HW_MONITOR_COMMAND_SIZE 1000
#define HW_MONITOR_BUFFER_SIZE 1024

    platform::guid stoguid(std::string);

    std::vector<uint8_t> encode_command(command, uint32_t, uint32_t, uint32_t, uint32_t, std::vector<uint8_t>);
}
