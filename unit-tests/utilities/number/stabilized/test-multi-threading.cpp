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
//       * Verify that the stabilized_value class is thread safe,
//         We fill the history with the first value and then let 2 thread run.
//         first thread add 66% of first value and the rest of second value.
//         second thread sample the 60% stability value each time and verify all equal to first value

TEST_CASE( "multi-threading", "[stabilized value]" )
{
    std::atomic< float > inserted_val_1 = { 20.0f };
    std::atomic< float > inserted_val_2 = { 55.0f };
    std::vector< float > values_vec;
    stabilized_value< float > stab_value( 10 );

    // Fill history with values (> 60% is 'inserted_val_1')
    std::thread first( [&]() {
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        for( int i = 0; i < 1000; i++ )
        {
            if( i <= 10 )
            {
                stab_value.add( inserted_val_1 );
                continue;
            }
            if( i % 3 )
                stab_value.add( inserted_val_1 );
            else
                stab_value.add( inserted_val_2 );
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
    } );

    // Sample stabilized value
    std::thread second( [&]() {
        while( stab_value.empty() )
        {
        }

        for( int i = 0; i < 1000; i++ )
        {
            values_vec.push_back( stab_value.get( 0.6f ) );
            if( i % 10 )
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
    } );

    if( first.joinable() )
        first.join();
    if( second.joinable() )
        second.join();

    // Verify that all samples show the > 60% number inserted 'inserted_val_1'
    int count = 0;
    for( auto val : values_vec )
        if( val == inserted_val_1 )
            count++;

    CHECK( count == 1000 );
}
