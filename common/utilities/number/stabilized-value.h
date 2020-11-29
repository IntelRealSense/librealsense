// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>

namespace utilities {
namespace number {

    // The stabilized value implement a history based stable value. 
    // It supply a way to define and get values with a required stabilization.
    // When the user adds a value, the value is inserted into the history
    // When the user call "get" the last stable value is returned
    // Inputs: Template <value_type>
    //         required history size
    //         required stabilization percentage
    template<typename T>
    class stabilized_value
    {
    public:
        stabilized_value( size_t history_size, float stabilize_percentage )
            : _history_size( history_size )
            , _last_stable_value( 0 )
            , _stabilize_percentage( stabilize_percentage )
        {
            if( ( stabilize_percentage <= 0.0f ) || ( stabilize_percentage > 1.0f ) )
                throw std::runtime_error( "Illegal value for stabilize_percentage: " + std::to_string(stabilize_percentage));
        }

        void add(T val)
        {
            if (_values.empty())
            {
                _values.push_back(val);
                _last_stable_value = val;
                return;
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

            auto new_value_percentage
                = values_count_map[val] / static_cast< float >( _values.size() );
            if( new_value_percentage >= _stabilize_percentage )
            {
                _last_stable_value = val;
            }
        }

        T get()
        {
            if (_values.empty())
            {
                throw std::runtime_error( "No stable value exist, history is empty" );
            }

            return _last_stable_value;
        }

        void reset()
        {
            _values.clear();
            _last_stable_value = 0.0f;
        }
    private:
        std::deque<T> _values;
        size_t _history_size;
        T _last_stable_value;
        float _stabilize_percentage;
    };


}  // namespace value_stabilizer
}  // namespace utilities
