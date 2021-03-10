//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "l500-serializable.h"
#include "../../../third-party/json.hpp"

namespace librealsense
{
    using json = nlohmann::json;

    l500_serializable::l500_serializable(std::shared_ptr<hw_monitor> hw_monitor, synthetic_sensor & depth_sensor)
        :_hw_monitor_ptr(hw_monitor),
         _depth_sensor(depth_sensor)
    {
    }

    std::vector<uint8_t> l500_serializable::serialize_json() const
    {
        json j;
        auto options = _depth_sensor.get_supported_options();

        for (auto o : options)
        {
            auto&& opt = _depth_sensor.get_option(o);
            auto val = opt.query();
            j[get_string(o)] = val;
        }

        auto str = j.dump(4);
        return std::vector<uint8_t>(str.begin(), str.end());
    }

    void l500_serializable::load_json(const std::string& json_content)
    {
        json j = json::parse(json_content);

        auto opts = _depth_sensor.get_supported_options();
        for (auto o: opts)
        {
            auto& opt = _depth_sensor.get_option(o);
            if (opt.is_read_only()) continue;

            auto key = get_string(o);
            auto it = j.find(key);
            if (it != j.end())
            {
                float val = it.value();
                opt.set(val);
            }

        }
    }
} // namespace librealsense
