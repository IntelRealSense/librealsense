// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 SLAMcore Ltd. All Rights Reserved.

#include "catch.h"
#include <global_timestamp_reader.h>
#include <vector>

#define PERIOD_USEC 20000 // 50 Hz
#define TIMESTAMP_USEC_TO_MSEC 0.001

using namespace librealsense;

// When feeding inputs with zero latency to the linear regression
// we don't expect the linear regression to have any effect
static void testLinearCoefficients(const uint64_t ts_offset)
{
    CLinearCoefficients coeffs = CLinearCoefficients(15);

    // Create a small set of timestamps at a regular interval
    std::vector<uint64_t> timestamps;
    for (size_t i = 0; i < 10; ++i)
    {
        timestamps.push_back(ts_offset + i * PERIOD_USEC);
    }

    // Feed zero-latency "ping" samples
    for (const auto ts : timestamps)
    {
        // Create matching hardware and system timestamps
        const double hardware_ts = (ts % UINT32_MAX) * TIMESTAMP_USEC_TO_MSEC;
        const double system_ts = ts * TIMESTAMP_USEC_TO_MSEC;


        // Feed sample for linear regression
        coeffs.update_samples_base(hardware_ts);

        CSample sample(hardware_ts, system_ts);
        coeffs.add_value(sample);
    }

    // It should be possible to query timestamps in the past
    // This is to account for a delay before a frame or packet is received
    for (const auto ts : timestamps)
    {
        // Create matching hardware and system timestamps
        const double hardware_ts = (ts % UINT32_MAX) * TIMESTAMP_USEC_TO_MSEC;
        const double system_ts = ts * TIMESTAMP_USEC_TO_MSEC;

        // Query system time at the point of the sample
        coeffs.update_samples_base(hardware_ts);

        coeffs.update_last_sample_time(hardware_ts);
        const double queried_ts = coeffs.calc_value(hardware_ts);

        // We fed matching timestamps so we expect the output to be equal
        REQUIRE(queried_ts == system_ts);
    }
}

TEST_CASE("linear_coefficients_simple", "")
{
    testLinearCoefficients(0);
}

TEST_CASE("linear_coefficients_timewrap", "")
{
    // Start a little bit before the hardware wrap-around time
    const uint64_t ts_offset =
      ((UINT32_MAX - 5 * PERIOD_USEC) / PERIOD_USEC) * PERIOD_USEC;

    testLinearCoefficients(ts_offset);
}
