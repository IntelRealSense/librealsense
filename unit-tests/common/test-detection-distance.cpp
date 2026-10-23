// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#include <unit-tests/catch.h>
#include <common/detection-distance.h>

using rs2::detection_distance_mode;
using rs2::detection_distance_fw_value;
using rs2::needs_host_detection_distance;
using rs2::pick_detection_distance;


TEST_CASE( "detection-distance/off shows no distance", "[detection-distance]" )
{
    // The reported bug: with the control off the FW returns 0, and the viewer must not fill in its own estimate
    CHECK_FALSE( needs_host_detection_distance( detection_distance_mode::off, 0.f ) );
    CHECK( pick_detection_distance( detection_distance_mode::off, 0.f, 1.7f ) == 0.f );
    CHECK( pick_detection_distance( detection_distance_mode::off, 2.f, 1.7f ) == 0.f );
    CHECK( detection_distance_fw_value( detection_distance_mode::off ) == 0.f );
}

TEST_CASE( "detection-distance/host uses the viewer estimate only", "[detection-distance]" )
{
    CHECK( needs_host_detection_distance( detection_distance_mode::host, 0.f ) );
    CHECK( needs_host_detection_distance( detection_distance_mode::host, 2.f ) );
    CHECK( pick_detection_distance( detection_distance_mode::host, 0.f, 1.7f ) == 1.7f );
    CHECK( pick_detection_distance( detection_distance_mode::host, 2.f, 1.7f ) == 1.7f );
    CHECK( pick_detection_distance( detection_distance_mode::host, 0.f, 0.f ) == 0.f );  // no depth at the box
    CHECK( detection_distance_fw_value( detection_distance_mode::host ) == 0.f );
}

TEST_CASE( "detection-distance/on-camera uses the FW distance only", "[detection-distance]" )
{
    CHECK_FALSE( needs_host_detection_distance( detection_distance_mode::on_camera, 0.f ) );
    CHECK_FALSE( needs_host_detection_distance( detection_distance_mode::on_camera, 2.f ) );
    CHECK( pick_detection_distance( detection_distance_mode::on_camera, 2.f, 1.7f ) == 2.f );
    CHECK( pick_detection_distance( detection_distance_mode::on_camera, 0.f, 1.7f ) == 0.f );  // FW out of range
    CHECK( detection_distance_fw_value( detection_distance_mode::on_camera ) == 1.f );
}

TEST_CASE( "detection-distance/fallback keeps the behavior of cameras without the control", "[detection-distance]" )
{
    // e.g. D555 over DDS, or D585 FW before the control: FW distance when present, else the viewer estimate
    CHECK_FALSE( needs_host_detection_distance( detection_distance_mode::fallback, 2.f ) );
    CHECK( needs_host_detection_distance( detection_distance_mode::fallback, 0.f ) );
    CHECK( pick_detection_distance( detection_distance_mode::fallback, 2.f, 1.7f ) == 2.f );
    CHECK( pick_detection_distance( detection_distance_mode::fallback, 0.f, 1.7f ) == 1.7f );
}
