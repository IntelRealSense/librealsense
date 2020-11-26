// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>

namespace utilities {
namespace number {

// TODO add description and UT
template<typename T>
    class stabilized_value
    {
    public:
        stabilized_value( size_t history_size, float stabilize_percentage )
            : _history_size( history_size )
            , _last_stable_value( 0 )
            , _stabilize_percentage( stabilize_percentage )
        {
            if( ( stabilize_percentage <= 0.0f ) || ( stabilize_percentage >= 1.0f ) )
                throw std::runtime_error( "Illegal value for stabilize_percentage" );
        }

        T get(T val)
        {
            if (_values.empty())
            {
                _values.push_back(val);
                _last_stable_value = val;
                return _last_stable_value;
            }

            _values.push_back(val);
            if (_values.size() > _history_size)
                _values.pop_front();

            std::unordered_map<T, int> values_count_map;
            for (T val : _values)
                if (values_count_map.find(val) != values_count_map.end()) 
                    values_count_map[val]++;
                else
                    values_count_map[val] = 1;


            if ((values_count_map[val] / _values.size()) >= _stabilize_percentage)
            {
                _last_stable_value = val;
            }

            return _last_stable_value;
        }
    private:
        std::deque<T> _values;
        size_t _history_size;
        T _last_stable_value;
        float _stabilize_percentage;
    };


}  // namespace value_stabilizer
}  // namespace utilities
