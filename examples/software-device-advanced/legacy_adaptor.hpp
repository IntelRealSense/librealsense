#ifndef LIBREALSENSE_RS2_LEGACY_ADAPTOR_HPP
#define LIBREALSENSE_RS2_LEGACY_ADAPTOR_HPP

#include "librealsense2/rs.hpp"
#include "legacy_adaptor.h"

namespace rs2 {
    namespace legacy {
        class legacy_device : public rs2::software_device {
            std::shared_ptr<rs2_device> create_device_ptr(std::string json)
            {
                rs2_error* e = nullptr;
                std::shared_ptr<rs2_device> dev(
                    rs2_create_legacy_adaptor_device(json.c_str(), &e),
                    rs2_delete_device);
                rs2::error::handle(e);
                return dev;
            }
        public:
            legacy_device(std::string json)
                : rs2::software_device(create_device_ptr(json))
            {}

            static std::string get_json_format() {
                return rs2_legacy_adaptor_get_json_format();
            }
        };
    }
}

#endif