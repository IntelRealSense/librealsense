#pragma once

#include "catch.h"
#include <limits>


// We need to compare floating point values, therefore we need an approximation
// function, which Catch provides for us:
//     REQUIRE( performComputation() == Approx( 2.1 ));
// (see https://github.com/catchorg/Catch2/blob/master/docs/assertions.md)
// For example (with the default epsilon):
//     2.61007666588 ~= 2.61007662723
// This may not be good enough for us and want to control the epsilon. By default
// Approx sets its epsilon to:
//     std::numeric_limits<float>::epsilon()*100
// And to set a custom epsilon:
//     REQUIRE( <val> == Approx(2.1).epsilon(0.01) );  // allow up to 1% diff
// So we want a macro that does this automatically:
#if ! defined( __EPSILON )
#define __EPSILON (std::numeric_limits<float>::epsilon()*1000)
#endif
#define approx(X) Approx(X).epsilon(__EPSILON)
// Because we have our own macro (and because it's more verbose) we do NOT want
// to use the literal that Catch supplies:
//     using namespace Catch::literals;
//     REQUIRE( performComputation() == 2.1_a );
