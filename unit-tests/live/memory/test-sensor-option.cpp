#include "../../test.h"
#include "../live-common.h"
#include <iostream>

//#test:device D400*
//#test:tag D400

TEST_CASE( "sensor get_option memory leak", "[live]" )
{
    // running this test with the following command:
    // `valgrind --leak-check=yes --show-leak-kinds=all --track-origins=yes ./unit-tests/build/live/memory/test-sensor-option/test-live-memory-sensor-option 2>&1 | grep 'depends\|No device'`
    // should print nothing!

    auto devices = find_devices_by_product_line_or_exit(RS2_PRODUCT_LINE_D400);
    auto dev = devices[0];

    auto depth_sens = dev.first< rs2::depth_sensor >();
    // float option_value(depth_sens.get_option(RS2_OPTION_EXPOSURE));
    if (!depth_sens.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
        return;
    float option_value(depth_sens.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE));

    REQUIRE( (option_value) ); // Using the value of option_value, if memory error exist, causes valgrind to output: "Conditional jump or move depends on uninitialised value(s)"
}

