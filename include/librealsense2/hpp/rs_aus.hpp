// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_AUS_HPP
#define LIBREALSENSE_RS2_AUS_HPP

#include "rs_types.hpp"
#include <vector>

namespace rs2
{
 
    inline void aus_set(const char* counter, int value)
    {
        rs2_error* e = nullptr;
        rs2_aus_set(counter, value, &e);
        error::handle(e);
    }

    inline void aus_increment(const char* counter)
    {
        rs2_error* e = nullptr;
        rs2_aus_increment(counter, &e);
        error::handle(e);
    }

    inline void aus_decrement( const char * counter )
    {
        rs2_error * e = nullptr;
        rs2_aus_decrement( counter, &e );
        error::handle( e );
    }

    inline long aus_get(const char* counter)
    {
        rs2_error* e = nullptr;
        long result = rs2_aus_get(counter, &e);
        error::handle(e);
        return result;
    }

    inline void aus_start(const char* timer)
    {
        rs2_error* e = nullptr;
        rs2_aus_start(timer, &e);
        error::handle(e);
    }

    inline void aus_stop(const char* timer)
    {
        rs2_error* e = nullptr;
        rs2_aus_stop(timer, &e);
        error::handle(e);
    }


    inline std::vector<std::string> aus_get_counters_list()
    {
        rs2_error* e = nullptr;
        auto buffer = rs2_aus_get_counters_list(&e);
        std::shared_ptr<const rs2_strings_list> list(buffer, rs2_aus_delete_counters_list);
        error::handle(e);

        auto size = rs2_aus_get_counters_list_size(list.get(), &e);
        error::handle(e);

        std::vector<std::string> results;
        for (size_t i = 0; i < size; ++i) {
            const char* item_cstr = rs2_aus_get_counter_data(list.get(), i, &e);
            error::handle(e);
            results.push_back(item_cstr);
        }

        return results;
    }

}

#endif // LIBREALSENSE_RS2_AUS_HPP
