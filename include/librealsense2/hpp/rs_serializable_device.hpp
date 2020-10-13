// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs_types.hpp"
#include "rs_device.hpp"
#include <array>
#include  <string>

namespace rs2
{
    class serializable_device : public rs2::device
    {
    public:
        serializable_device(rs2::device d)
            : rs2::device(d.get())
        {
            rs2_error* e = nullptr;
            if (rs2_is_device_extendable_to(_dev.get(), RS2_EXTENSION_SERIALIZABLE, &e) == 0 && !e)
            {
                _dev = nullptr;
            }
            rs2::error::handle(e);
        }

        std::string serialize_json() const
        {
            std::string results;

            rs2_error* e = nullptr;
            std::shared_ptr<rs2_raw_data_buffer> json_data(
                rs2_serialize_json(_dev.get(), &e),
                rs2_delete_raw_data);
            rs2::error::handle(e);

            auto size = rs2_get_raw_data_size(json_data.get(), &e);
            rs2::error::handle(e);

            auto start = rs2_get_raw_data(json_data.get(), &e);
            rs2::error::handle(e);

            results.insert(results.begin(), start, start + size);

            return results;
        }

        void load_json(const std::string& json_content) const
        {
            rs2_error* e = nullptr;
            rs2_load_json(_dev.get(),
                json_content.data(),
                (unsigned int)json_content.size(),
                &e);
            rs2::error::handle(e);
        }
    };
}
