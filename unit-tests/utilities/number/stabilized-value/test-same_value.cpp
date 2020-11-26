// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../../common/utilities/number/stabilized-value.hpp


#include "../../../test.h"
#include "../common/utilities/number/stabilized-value.hpp"

using namespace utilities::number;

TEST_CASE( "Same value", "[stabilized value]" )
{
    stabilized_value< float > stab_value( 5, 0.4f );
    CHECK(1.0f == stab_value.get(1.0f));
    CHECK(1.0f == stab_value.get(1.0f));
    CHECK(1.0f == stab_value.get(1.0f));
    CHECK(1.0f == stab_value.get(1.0f));
    CHECK(1.0f == stab_value.get(1.0f));

    CHECK(1.0f == stab_value.get(1.0f));

}
