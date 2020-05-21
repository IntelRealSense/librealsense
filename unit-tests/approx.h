#pragma once

#include "catch.h"
#include <limits>
#include <sstream>
#include <iomanip>


/*

We need to compare floating point values, therefore we need an approximation
function, which Catch provides for us:
    REQUIRE( performComputation() == Approx( 2.1 ));
(see https://github.com/catchorg/Catch2/blob/master/docs/assertions.md)
For example (with the default epsilon):
    2.61007666588 ~= 2.61007662723
This may not be good enough for us...

Three controls exist for the comparison:
  - margin (absolute difference)
    |a-b| <= margin
  - scale - ignored for now; see below
  - epsilon (relative difference)
    |a-b| <= epsilon * |b|


Catch v1 vs v2
----------------

In v1, the formula for approx was:
    |a-b| <= epsilon * (scale + max( |a|, |b| ))
With the default for scale being 1.
With v2, this changed to:
    |a-b| <= margin  ||  |a-b| <= epsilon * (scale + b )
(it's really slightly different, but the gist is the above)
The scale has changed to 0!
Note that it's now only relative to the "golden" number to which we're comparing!


Absolute vs relative comparisons
----------------------------------

Absolute and relative tolerances are tested as:
    |a-b| <= MARGIN
and:
    |a-b| <= EPSILON * max(|a|, |b|)

The absolute tolerance test fails when x and y become large, and the relative
tolerance test fails when they become small. It is therefore best to combine
the two tests together in a single test.

But this is always subject to context: generalizing here is convenient, that's all...


Approx to 0
-------------

Because the scale is 0 in v2, and the margin defaults to 0, there is essentially no
approximate comparison to 0! We must use a margin if we want to do this.

Which value to choose is a good question, though. Because most of our math is in
floats, we choose to use the float epsilon: any two numbers are deemed equal if their
difference is less than the smallest float number representable:
*/
#if ! defined( __APPROX_MARGIN )
#define __APPROX_MARGIN std::numeric_limits<float>::epsilon()
#endif
template< typename F > struct __approx_margin {};
template<> struct __approx_margin< double > { static constexpr double value() { return __APPROX_MARGIN; } };
template<> struct __approx_margin< float  > { static constexpr float  value() { return __APPROX_MARGIN * 4; } };
template< typename F > F approx_margin( F ) { return __approx_margin< F >::value(); }

/*
But note that for floats, this number is scaled up!


Epsilon
---------
Approx sets its epsilon to:
    std::numeric_limits<float>::epsilon()*100
This might be too big.

Instead, we set the epsilon to the same as the margin, by default:
*/
#if ! defined( __APPROX_EPSILON )
#define __APPROX_EPSILON __APPROX_MARGIN
#endif
template< typename F > struct __approx_epsilon {};
template<> struct __approx_epsilon< double > { static constexpr double value() { return __APPROX_EPSILON; } };
template<> struct __approx_epsilon< float  > { static constexpr float  value() { return __APPROX_EPSILON * 4; } };
template< typename F > F approx_epsilon( F ) { return __approx_epsilon< F >::value(); }
/*
Note that this is still way smaller than the default!


How?
------

We provide our own functions to do approximate comparison:
    REQUIRE( performComputation() == approx( 2.1 ));
*/

// Custom version of Approx, ==> better replaced by matchers <== for more control,
// but provides LRS defaults that should closely (but not exactly) match them
template< typename F >
inline Approx approx( F f )
{
    return Approx( f )
        .margin( __approx_margin< F >::value() )
        .epsilon( __approx_epsilon< F >::value() );
}

/*

Literals
----------

Note that Catch has literals that make the syntax nice:
    using namespace Catch::literals;
    REQUIRE( performComputation() == 2.1_a );
Because we have our own implementatin (and because it's more verbose) we do NOT want
to use the literal that Catch supplies.


Matchers
----------

The above are good, but if you want more control, matchers provide a customizable
comparison:
    REQUIRE_THAT( performComputation(), approx_equals( 2.1 ));
Or, for more control:
    REQUIRE_THAT( performComputation(), approx_abs( 2.1 ));
    REQUIRE_THAT( performComputation(), approx_rel( 2.1 ));
Or, with the Catch matchers, even more:
    REQUIRE_THAT( performComputation(), WithinAbs( 2.1, 0.1 ));   // 2.0  ->  2.2
    REQUIRE_THAT( performComputation(), WithinRel( 2.1, 0.05 ));  // 5% from 2.1
    REQUIRE_THAT( performComputation(), WithinUlps( 2.1, 2 ));    // two epsilons from 2.1
These matchers are type-sensitive (float vs. double).
*/
#define approx_abs(D) \
    Catch::WithinAbs( (D), approx_margin((D)) )
#define approx_rel(D) \
    Catch::WithinRel( (D), approx_epsilon((D)) )
#define approx_equals(D) \
    ( approx_abs(D)  ||  approx_rel(D) )


// Utility function to help debug precision errors:
//         INFO( full_precision( d ) );
//         REQUIRE( 0.0 == d );
template< class T >
std::string full_precision( T const d )
{
    std::ostringstream s;
    s << std::setprecision( std::numeric_limits< T >::max_digits10 ) << d;
    return s.str();
}

