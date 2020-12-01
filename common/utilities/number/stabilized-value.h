// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <atomic>

namespace utilities {
namespace number {

    // The stabilized value implement a history based stable value. 
    // It supply a way to define and get values with a required stabilization.
    // When the user adds a value, the value is inserted into the history
    // When the user call "get" the last stable value is returned
    // Inputs: Template <value_type>
    //         required history size
    //         required stabilization percentage
    template<typename T> class stabilized_value
{
public:
    stabilized_value( size_t history_size, float stabilize_percentage )
        : _history_size( history_size )
        , _last_stable_value( 0 )
        , _stabilize_percentage( stabilize_percentage )
    {
        if( ( stabilize_percentage <= 0.0f ) || ( stabilize_percentage > 1.0f ) )
            throw std::runtime_error( "Illegal value for stabilize_percentage: "
                                      + std::to_string( stabilize_percentage ) );
    }

    void add( T val )
    {
        _recalc_stable_val = true;

        if( _values.empty() )
        {
            _values.push_back( val );
            _last_stable_value = val;
            return;
        }

        _values.push_back( val );
        if( _values.size() > _history_size )
            _values.pop_front();
    }

    T get()
    {
        if( _values.empty() )
        {
            throw std::runtime_error( "No stable value exist, history is empty" );
        }

        if( _recalc_stable_val )
        {
            std::unordered_map< T, int > values_count_map;
            std::pair< T, int > most_stable_value = { 0, 0 };
            for( T val : _values )
            {
                if( values_count_map.find( val ) != values_count_map.end() )
                    values_count_map[val]++;
                else
                    values_count_map[val] = 1;

                if( most_stable_value.second < values_count_map[val] )
                {
                    most_stable_value.first = val;
                    most_stable_value.second = values_count_map[val];
                }
            }

            auto new_value_percentage
                = most_stable_value.second / static_cast< float >( _values.size() );
            if( new_value_percentage >= _stabilize_percentage )
            {
                _last_stable_value = most_stable_value.first;
            }

            _recalc_stable_val = false;
        }

        return _last_stable_value;
    }

    void reset()
    {
        _values.clear();
        _last_stable_value = 0.0f;
        _recalc_stable_val = false;
    }

private:
    std::deque< T > _values;
    const size_t _history_size;
    T _last_stable_value;
    const float _stabilize_percentage;
    std::atomic_bool _recalc_stable_val = { true };
};


}  // namespace value_stabilizer
}  // namespace utilities
