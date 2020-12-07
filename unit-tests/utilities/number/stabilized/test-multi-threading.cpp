// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../../common/utilities/number/stabilized-value.h

#include <thread>
#include <chrono>
#include "../../../test.h"
#include <../common/utilities/number/stabilized-value.h>

using namespace utilities::number;
// Test group description:
//       * This tests group verifies stabilized_value class.
//
// Current test description:
//       * Verify that the stabilized_value class is thread safe

TEST_CASE( "multi-threading", "[stabilized value]" )
{
    try
    {
        std::atomic<float> inserted_val_1 = { 20.0f };
        std::atomic<float> inserted_val_2 = { 55.0f };
        std::vector<float> values_vec;
        stabilized_value< float > stab_value( 10, 0.6f );
        
        // Fill history with values (> 60% is 'inserted_val_1')
        std::thread first( [&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            for (int i = 0; i < 1000; i++)
            {
                if (i%3)
                    stab_value.add(inserted_val_1);
                else
                    stab_value.add(inserted_val_2);
            }
        } );

        // Sample stabilized value
        std::thread second( [&]() {
            while (stab_value.empty());
            for (int i = 0; i < 1000; i++)
                values_vec.push_back(stab_value.get());
        } );

        if (first.joinable()) first.join();
        if (second.joinable()) second.join();

        // Verify that all samples show the > 60% number inserted 'inserted_val_1'
        for (auto val : values_vec)
            CHECK(val == inserted_val_1);
    }
    catch( const std::exception & e )
    {
        FAIL( "Exception caught: " << e.what() );
    }
}
