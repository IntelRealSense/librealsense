#pragma once

#include <string>

namespace rs2
{
    // Helper class that is responsible for enabling per-frame metadata on different platforms
    // Currently implemented for Windows
    class metadata_helper
    {
    public:
        static metadata_helper& instance();

        // Check if metadata is enabled using Physical Port ID
        // (can be retrieved with device::get_info(RS2_CAMERA_INFO_PHYSICAL_PORT))
        // throws runtime_error in case of errors
        virtual bool is_enabled(std::string id) const { return true; }

        // Enable metadata for all connected devices
        // throws runtime_error in case of errors
        virtual void enable_metadata() { }

        static bool can_support_metadata(const std::string& product)
        {
            return product == "D400" || product == "SR300" || product == "L500";
        }

        // This is the command-line parameter that gets passed to another process (running as admin) in order to enable metadata -- see WinMain
        static std::string get_command_line_param() { return "--enable_metadata"; }
    };
}