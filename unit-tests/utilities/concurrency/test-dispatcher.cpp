// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../src/dispatcher.cpp

#include <unit-tests/test.h>
#include <common/utilities/time/timer.h>
#include <src/concurrency.h>

#include <algorithm>
#include <vector>

using namespace utilities::time;

// We use this function as a CPU stress test function
int fibo( int num )
{
    if( num < 2 )
        return 1;
    return fibo( num - 1 ) + fibo( num - 2 );
}

TEST_CASE( "dispatcher main flow" )
{
    dispatcher d(3);
    std::atomic_bool run = { false };
    auto func = [&](dispatcher::cancellable_timer c) 
    {
        c.try_sleep(std::chrono::seconds(1));
        run = true;
    };

    d.start();
    REQUIRE(d.empty());
    // We want to make sure that if we invoke some functions, the dispatcher is not empty.
    // We add 2 functions that take some time so that even if the first one pop,
    // the second will still be in the queue and it will not be empty
    d.invoke(func);
    d.invoke(func);
    REQUIRE_FALSE(d.empty());
    REQUIRE(d.flush());
    REQUIRE(run);
    d.stop();   
}

TEST_CASE( "invoke and wait" )
{
    dispatcher d(2);

    std::atomic_bool run = { false };
    auto func = [&](dispatcher::cancellable_timer c)
    {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        run = true;
    };

    d.start();
    stopwatch sw;
    d.invoke_and_wait(func, []() {return false; }, true);
    REQUIRE(sw.get_elapsed_ms() > 3000); // verify we get here only after the function call ended
    d.stop();
}

TEST_CASE("verify stop() not consuming high CPU usage")
{
    // using shared_ptr because no copy constructor is allowed for a dispatcher.
    std::vector<std::shared_ptr<dispatcher>> dispatchers;

    for (int i = 0 ; i < 32; ++i)
    {
        dispatchers.push_back(std::make_shared<dispatcher>(10));
    }

    for (auto &&dispatcher : dispatchers)
    {
        dispatcher->start();
    }

    for (auto&& dispatcher : dispatchers)
    {
        dispatcher->stop();
    }

  
    // Allow some time for all threads to do some work
    std::this_thread::sleep_for(std::chrono::seconds(5));

    stopwatch sw;

    // Do some stress work
    REQUIRE(fibo(40) == 165580141);
    // Verify the stress test did not take too long.
    // We had an issue that stop() call cause a high CPU usage and therefore other operations stall, 
    // This test took > 9 seconds on an 8 cores PC, after the fix it took ~1.5 sec on 1 core run (on release configuration).
    // We allow 9 seconds to support debug configuration as well
    REQUIRE(sw.get_elapsed_ms() < 9000);
}
