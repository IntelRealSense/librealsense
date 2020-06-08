// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/rs.hpp>   // Include RealSense Cross Platform API

#include <easylogging++.h>
#ifdef BUILD_SHARED_LIBS
// With static linkage, ELPP is initialized by librealsense, so doing it here will
// create errors. When we're using the shared .so/.dll, the two are separate and we have
// to initialize ours if we want to use the APIs!
INITIALIZE_EASYLOGGINGPP
#endif

// Catch also defines CHECK(), and so we have to undefine it or we get compilation errors!
#undef CHECK

// Let Catch define its own main() function
#define CATCH_CONFIG_MAIN
#include "../catch/catch.hpp"
#include "example.hpp"

// Define our own logging macro for debugging to stdout
// Can possibly turn it on automatically based on the Catch options supplied
// on the command-line, with a custom main():
//     Catch::Session catch_session;
//     int main (int argc, char * const argv[]) {
//         return catch_session.run( argc, argv );
//     }
//     #define TRACE(X) if( catch_session.configData().verbosity == ... ) {}
// With Catch2, we can turn this into SCOPED_INFO.
#define TRACE(X) do { \
    std::cout << X << std::endl; \
    } while(0)


//------------------------------  HELPERS  ---------------------------------//

//----------- TYPES --------------//
typedef std::function<void(float&)> value_change_callback;
typedef std::function<bool(float valueAfterChange, float valueRequested)> value_validation_callback;
typedef std::function<bool(float value, float limit)> loop_condition_callback;

//----------- STREAMING ----------//
void checkOption_streaming(window& app, rs2::rates_printer& printer, rs2::colorizer& color_map, 
    const rs2::pipeline& pipe, const rs2::sensor& sensor, rs2_option option, 
    float initialValue, float limitValue, int number_of_iterations, float tolerance,
    loop_condition_callback lc_cb, value_change_callback vc_cb, value_validation_callback vv_cb)
{
    float valueAfterChange = initialValue;
    float valueToSet = initialValue;
    while (/*app &&*/ lc_cb(valueAfterChange, limitValue))
    {
        //rs2::frameset data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
        //app.show(data);
        vc_cb(valueToSet);
        if (!lc_cb(valueToSet, limitValue))
            valueToSet = limitValue;
        REQUIRE(sensor.get_option(RS2_OPTION_AUTO_EXPOSURE_MODE) == 0);
        sensor.set_option(option, valueToSet);
        int iterations = number_of_iterations;
        while (iterations-- > 0 && !vv_cb(valueAfterChange, valueToSet))
        {
            valueAfterChange = sensor.get_option(option);
        }
        bool result = vv_cb(valueAfterChange, valueToSet);
        if (!result)
        {
            std::cout << "valueToSet = " << valueToSet << ", valueAfterChange = " << valueAfterChange << std::endl;
        }
        REQUIRE(result);
    }
}

void checkOptionForBothSensors_streaming(window& app, rs2::rates_printer& printer, rs2::colorizer& color_map,
    const rs2::pipeline& pipe, const rs2::sensor& sensor, rs2_option option, rs2_option option_second, float initialValue, float limitValue, int number_of_iterations, float tolerance,
    loop_condition_callback lc_cb, value_change_callback vc_cb, value_validation_callback vv_cb)
{
    float valueAfterChange = initialValue;
    float valueAfterChange_second = initialValue;
    float valueToSet = initialValue;
    while (/*app &&*/ lc_cb(valueAfterChange, limitValue) && lc_cb(valueAfterChange_second, limitValue))
    {
        //rs2::frameset data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
        //app.show(data); 
        vc_cb(valueToSet);
        if (!lc_cb(valueToSet, limitValue))
            valueToSet = limitValue;
        REQUIRE(sensor.get_option(RS2_OPTION_AUTO_EXPOSURE_MODE) == 0);

        //data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
        //app.show(data);
        //setting actions
        sensor.set_option(option, valueToSet);
        sensor.set_option(option_second, valueToSet);

        //getting actions
        int iterations = number_of_iterations;
        while (iterations-- > 0 && (!vv_cb(valueAfterChange, valueToSet) || !vv_cb(valueAfterChange_second, valueToSet)))
        {
            if (valueAfterChange != valueToSet)
                valueAfterChange = sensor.get_option(option);
            if (valueAfterChange_second != valueToSet)
                valueAfterChange_second = sensor.get_option(option_second);

            //rs2::frameset data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
            //app.show(data);
        }

        //checkings actions
        bool result = vv_cb(valueAfterChange, valueToSet);
        if (!result)
        {
            std::cout << "option: " << rs2_option_to_string(option) << "valueToSet = " << valueToSet << ", valueAfterChange = " << valueAfterChange << std::endl;
        }
        REQUIRE(result);
        bool result_second = vv_cb(valueAfterChange_second, valueToSet);
        if (!result_second)
        {
            std::cout << "option: " << rs2_option_to_string(option_second) << "valueToSet = " << valueToSet << ", valueAfterChange = " << valueAfterChange_second << std::endl;
        }
        REQUIRE(result);
        REQUIRE(result_second);
    }
}

void checkOption_streaming2(const rs2::sensor& sensor, rs2_option option,
    float initialValue, float limitValue, int number_of_iterations, float tolerance,
    loop_condition_callback lc_cb, value_change_callback vc_cb, value_validation_callback vv_cb)
{
    float valueAfterChange = initialValue;
    float valueToSet = initialValue;
    while (lc_cb(valueToSet, limitValue))
    {
        vc_cb(valueToSet);
        if (!lc_cb(valueToSet, limitValue))
            valueToSet = limitValue;
        REQUIRE(sensor.get_option(RS2_OPTION_AUTO_EXPOSURE_MODE) == 0);
        sensor.set_option(option, valueToSet);
        int iterations = number_of_iterations;
        while (iterations-- > 0 && !vv_cb(valueAfterChange, valueToSet))
        {
            valueAfterChange = sensor.get_option(option);
        }
        bool result = vv_cb(valueAfterChange, valueToSet);
        if (!result)
        {
            std::cout << "valueToSet = " << valueToSet << ", valueAfterChange = " << valueAfterChange << std::endl;
        }
        REQUIRE(result);
    }
}

void checkOptionForBothSensors_streaming2(const rs2::sensor& sensor, rs2_option option, rs2_option option_second, float initialValue, float limitValue, int number_of_iterations, float tolerance,
    loop_condition_callback lc_cb, value_change_callback vc_cb, value_validation_callback vv_cb)
{
    float valueAfterChange = initialValue;
    float valueAfterChange_second = initialValue;
    float valueToSet = initialValue;
    while (lc_cb(valueToSet, limitValue))
    {
        vc_cb(valueToSet);
        if (!lc_cb(valueToSet, limitValue))
            valueToSet = limitValue;
        REQUIRE(sensor.get_option(RS2_OPTION_AUTO_EXPOSURE_MODE) == 0);

        //setting actions
        sensor.set_option(option, valueToSet);
        sensor.set_option(option_second, valueToSet);

        //getting actions
        int iterations = number_of_iterations;
        while (iterations-- > 0 && (!vv_cb(valueAfterChange, valueToSet) || !vv_cb(valueAfterChange_second, valueToSet)))
        {
            if (valueAfterChange != valueToSet)
                valueAfterChange = sensor.get_option(option);
            if (valueAfterChange_second != valueToSet)
                valueAfterChange_second = sensor.get_option(option_second);
            
        }

        //checkings actions
        bool result = vv_cb(valueAfterChange, valueToSet);
        if (!result)
        {
            std::cout << "option: " << rs2_option_to_string(option) << "valueToSet = " << valueToSet << ", valueAfterChange = " << valueAfterChange << std::endl;
        }
        REQUIRE(result);
        bool result_second = vv_cb(valueAfterChange_second, valueToSet);
        if (!result_second)
        {
            std::cout << "option: " << rs2_option_to_string(option_second) << "valueToSet = " << valueToSet << ", valueAfterChange = " << valueAfterChange_second << std::endl;
        }
        REQUIRE(result);
        REQUIRE(result_second);
    }
}
