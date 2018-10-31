// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include <memory>

#include "core/extension.h"
#include "types.h"              // exception

namespace librealsense
{
    struct region_of_interest
    {
        int min_x;
        int min_y;
        int max_x;
        int max_y;
    };

    class region_of_interest_method
    {
    public:
        virtual void set(const region_of_interest& roi) = 0;
        virtual region_of_interest get() const = 0;

        virtual ~region_of_interest_method() = default;
    };

    class roi_sensor_interface
    {
    public:
        virtual region_of_interest_method& get_roi_method() const = 0;
        virtual void set_roi_method(std::shared_ptr<region_of_interest_method> roi_method) = 0;
    };

    class roi_sensor_base : public roi_sensor_interface
    {
    public:
        region_of_interest_method& get_roi_method() const override
        {
            if (!_roi_method.get())
                throw librealsense::not_implemented_exception("Region-of-interest is not implemented for this device!");
            return *_roi_method;
        }

        void set_roi_method(std::shared_ptr<region_of_interest_method> roi_method) override
        {
            _roi_method = roi_method;
        }

    private:
        std::shared_ptr<region_of_interest_method> _roi_method = nullptr;
    };

    MAP_EXTENSION(RS2_EXTENSION_ROI, librealsense::roi_sensor_interface);
}
