// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <type_traits>
#include <limits>
#include <stdlib.h>  // size_t


namespace rsutils {
namespace number {


// Compute the average of a set of numbers, one at a time, without overflow!
// We adapt this to a signed integral T type, where we must start counting leftovers.
// We also add some rounding.
// 
template< class T, typename Enable = void >
class running_average;


// Compute the average of a set of numbers, one at a time.
// This is the basic implementation, using doubles. See:
//     https://www.heikohoffmann.de/htmlthesis/node134.html
// The basic code:
//     double average( double[] ary )
//     {
//         double avg = 0;
//         int n = 0;
//         for( double x : ary )
//             avg += (x - avg) / ++n;
//         return avg;
//     }
//
template<>
class running_average< double >
{
    double _avg = 0.;
    size_t _n = 0;

public:
    running_average() = default;

    size_t size() const { return _n; }
    double get() const { return _avg; }

    void add( double x ) { _avg += (x - _avg) / ++_n; }
};


// Compute the average of a set of numbers, one at a time.
// 
// Adapted to a signed integral T type, where we must start counting leftovers.
// We also add some rounding.
// And we must do all that WITHOUT OVERFLOW!
// 
template< class T >
class running_average< T, typename std::enable_if< std::is_integral< T >::value >::type >
{
    T _avg = 0;
    size_t _n = 0;
    T _leftover = 0;

public:
    running_average() = default;

    size_t size() const { return _n; }
    T get() const { return _avg; }
    T leftover() const { return _leftover; }

    double fraction() const { return _n ? double( _leftover ) / double( _n ) : 0.;  }
    double get_double() const { return _avg + fraction(); }

    void add( T x ) { _avg += int_div_mod( x - _avg, ++_n, _leftover ); }

private:
    static T add_no_overflow( T a, T b )
    {
        if( a > 0 )
        {
            if( b > std::numeric_limits< T >::max() - a )
                return a;  // discard b
        }
        else if( a < 0 )
        {
            if( b < std::numeric_limits< T >::min() - a )
                return a;  // discard b
        }
        return a + b;
    }
    static T int_div_mod( int64_t dividend_, size_t n, T & remainder )
    {
        // We need the modulo sign to be the same as the dividend!
        // And, more importantly, modulo can be implemented differently based on the compiler, so we cannot use it!
        T dividend = add_no_overflow( dividend_, remainder );
        // We want 6.5 to be rounded to 7, but have to be careful with the sign:
        T rounding = n / 2;
        T rounded = add_no_overflow( dividend, dividend < 0 ? -rounding : rounding );
        T result = rounded / (T)n;
        remainder = dividend - n * result;
        return result;
    }
};


}  // namespace number
}  // namespace rsutils
