#include "../../test.h"
#include "../../func/func-common.h"
#include <iostream>


TEST_CASE( "sensor get_option memory leak", "[live]" )
{
    // running this test with the following command:
    // `valgrind --leak-check=yes --show-leak-kinds=all --track-origins=yes ./unit-tests/build/utilities/memory/test-sensor-option/test-utilities-memory-sensor-option 2>&1 | grep depends`
    // should print nothing!

    auto devices = find_devices_by_product_line_or_exit(RS2_PRODUCT_LINE_DEPTH);
    if (devices.size() == 0) return;
    auto dev = devices[0];

    auto depth_sens = dev.first< rs2::depth_sensor >();
    // float option_value(depth_sens.get_option(RS2_OPTION_EXPOSURE));
    float option_value(depth_sens.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE));

    std::cout << "Declare: BOOL::" << option_value << std::endl;
}