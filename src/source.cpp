// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "source.h"
#include "option.h"

namespace librealsense
{
    class frame_queue_size : public option_base
    {
    public:
        frame_queue_size(std::atomic<uint32_t>* ptr, const option_range& opt_range)
            : option_base(opt_range),
              _ptr(ptr)
        {}

        void set(float value) override
        {
            if (!is_valid(value))
                throw invalid_value_exception(to_string() << "set(frame_queue_size) failed! Given value " << value << " is out of range.");

            *_ptr = static_cast<uint32_t>(value);
        }

        float query() const override { return static_cast<float>(_ptr->load()); }

        bool is_enabled() const override { return true; }

        const char* get_description() const override
        {
            return "Max number of frames you can hold at a given time. Increasing this number will reduce frame drops but increase lattency, and vice versa";
        }
    private:
        std::atomic<uint32_t>* _ptr;
    };

    std::shared_ptr<option> callback_source::get_published_size_option()
    {
        return std::make_shared<frame_queue_size>(&_max_publish_list_size, option_range{ 0, 32, 1, 16 });
    }
}

