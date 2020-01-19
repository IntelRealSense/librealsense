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
        virtual bool is_enabled(std::string id) const { return true; }

        // Enable metadata for all connected devices
        virtual void enable_metadata() { }

        static std::string get_command_line_param() { return "--enable_metadata"; }
    };
}