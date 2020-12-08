// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <atomic>
#include <mutex>

namespace utilities {
namespace number {

// The stabilized value implement a history based stable value.
// It supply a way to define and get values with a required stabilization.
// When the user adds a value, the value is inserted into the history
// When the user call "get" the last stable value is returned
// Inputs: Template <value_type>
//         required history size
//         required stabilization percentage as a fraction , range [0-1]
// The stabilized_value class is thread safe.
template < typename T > class stabilized_value
{
public:
    stabilized_value( size_t history_size )
        : _history_size( history_size )
        , _last_stable_value( 0 )
    {

    }

    stabilized_value() = delete;
    stabilized_value(const stabilized_value &) = delete;


    void add( T val )
    {
        const std::lock_guard< std::mutex > lock( _mutex );
        if( _values.empty() )
        {
            _values.push_back( val );
            _last_stable_value = val;
            return;
        }

        _recalc_stable_val = true;

        _values.push_back( val );
        if( _values.size() > _history_size )
            _values.pop_front();
    }

    T get( float stabilization_percent = 0.75f) const
    {
        if ((stabilization_percent <= 0.0f) || (stabilization_percent > 1.0f))
            throw std::runtime_error("Illegal value for stabilize_percentage: "
                + std::to_string(stabilization_percent));

        std::lock_guard< std::mutex > lock( _mutex );

        if( _values.empty() )
        {
            throw std::runtime_error( "history is empty; no stable value" );
        }

        if (_recalc_stable_val || (_stabilize_percentage != stabilization_percent))
        {
            _stabilize_percentage = stabilization_percent;
            std::unordered_map< T, int > values_count_map;
            std::pair< T, int > most_stable_value = { 0, 0 };
            for( T val : _values )
            {
                ++values_count_map[val];

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

    void clear()
    {
        const std::lock_guard< std::mutex > lock( _mutex );

        _values.clear();
        _last_stable_value = 0.0f;
        _recalc_stable_val = false;
    }

    bool empty()
    {
        const std::lock_guard< std::mutex > lock( _mutex );
        return _values.empty();
    }

private:
    std::deque< T > _values;
    const size_t _history_size;
    mutable T _last_stable_value;
    mutable float _stabilize_percentage;
    mutable std::atomic_bool _recalc_stable_val = { true };
    mutable std::mutex _mutex;
};

}  // namespace number
}  // namespace utilities
