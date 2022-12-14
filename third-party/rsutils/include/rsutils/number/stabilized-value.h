// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <deque>
#include <unordered_map>

namespace rsutils {
namespace number {

// Given a rapidly changing history of a value, we do not always want to show every minor
// change right away. Rather, a "stable" value can be deduced from the history and changed
// only when the history clearly reflects a new "stabilization percentage".
//
// The stabilized value is, in order of importance:
//   - the highest-occurring value in history that's over the required percentage
//   - the previous stabilized value, if it's still in history
//   - the value which occurs the most in history
//
// If the history is empty, no stable value exists and an exception will be thrown.
//
// Inputs: Type of value <T>
//         Required history size in the ctor
//
// The stabilized_value class is thread safe.
//
template < typename T >
class stabilized_value
{
public:
    stabilized_value( size_t history_size )
        : _history_size( history_size )
        , _last_stable_value( 0 )
    {
        if( ! history_size )
            throw std::runtime_error( "history size must be > 0" );
    }

    stabilized_value() = delete;
    stabilized_value( const stabilized_value & ) = delete;


    void add( T val )
    {
        const std::lock_guard< std::mutex > lock( _mutex );
        if( _values.empty() )
        {
            _last_stable_value = val;
            _last_stable_percentage = 100.f;
            _values.push_back( val );
        }
        else
        {
            if( _values.size() >= _history_size )
                _values.pop_front();
            _last_stable_percentage = 0.f;
            _values.push_back( val );
        }
    }

    T get( float stabilization_percent = 0.75f ) const
    {
        if( stabilization_percent <= 0.f  ||  stabilization_percent > 1.f )
            throw std::runtime_error( "illegal stabilization percentage "
                + std::to_string( stabilization_percent ));

        std::lock_guard< std::mutex > lock( _mutex );

        if( _values.empty() )
            throw std::runtime_error( "history is empty; no stable value" );

        if( _last_stable_percentage != stabilization_percent )
        {
            _last_stable_percentage = stabilization_percent;
            std::unordered_map< T, int > values_count_map;
            std::pair< T, int > most_stable_value = { 0.f, 0 };
            for( T val : _values )
            {
                auto current_val = ++values_count_map[val];

                if( most_stable_value.second < current_val )
                {
                    most_stable_value.first = val;
                    most_stable_value.second = current_val;
                }
            }

            auto new_value_percentage
                = most_stable_value.second / static_cast< float >( _values.size() );
            
            // The stabilized value is, in order of importance:
            //   - the highest-occurring value in history that's over the required percentage
            //   - the previous stabilized value, if it's still in history
            //   - the value which occurs the most in history
            if( new_value_percentage >= _last_stable_percentage
                ||  values_count_map.find( _last_stable_value ) == values_count_map.end() )
            {
                _last_stable_value = most_stable_value.first;
            }
        }

        return _last_stable_value;
    }

    void clear()
    {
        const std::lock_guard< std::mutex > lock( _mutex );
        _values.clear();
    }

    bool empty() const
    {
        const std::lock_guard< std::mutex > lock( _mutex );
        return _values.empty();
    }

private:
    std::deque< T > _values;
    const size_t _history_size;
    mutable T _last_stable_value;
    mutable float _last_stable_percentage;
    mutable std::mutex _mutex;
};

}  // namespace number
}  // namespace rsutils
