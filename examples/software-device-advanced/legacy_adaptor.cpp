#include <librealsense2/rs.hpp> // Librealsense Cross-Platform API
#include <librealsense2/hpp/rs_internal.hpp> // Librealsense Software Device header

#include "api.h"
#include "legacy_adaptor.h"
#include "active_obj.hpp"

// TODO: actual JSON parsing
int parse_json(std::string str) {
    return std::stoi(str);
}

rs2_device * rs2_create_legacy_adaptor_device(const char * str, rs2_error** error) BEGIN_API_CALL {
    VALIDATE_NOT_NULL(str);

    int idx = parse_json(std::string(str));
    
    // Create software device such that it doesn't delete the 
    // underlying C pointer so we can pass it upward to the application
    rs2::software_device sd([](rs2_device*) {});
    auto ao = std::make_shared<legacy_active_obj>(idx, sd);
    ao->finalize_binding(sd);

    // extract the raw pointer to rs2_device from the software_device
    return sd.get().get();
} HANDLE_EXCEPTIONS_AND_RETURN(nullptr, str)

const char * rs2_legacy_adaptor_get_json_format() BEGIN_API_CALL {
    return "int";
} catch (...) { return nullptr; }