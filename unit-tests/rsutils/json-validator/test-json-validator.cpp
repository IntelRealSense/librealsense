// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 RealSense, Inc. All Rights Reserved.

//#cmake:dependencies rsutils

#include <unit-tests/test.h>
#include <rsutils/json.h>
#include <rsutils/json-validator.h>

using json = nlohmann::json;
using rsutils::json_validator::validate_json_field;

namespace {

TEST_CASE( "test missing field in JSON" )
{
    std::string json_str = "{ \"key1\" : 1, \"key2\": 2 }";
    json j = json::parse(json_str);
    try
    {
        validate_json_field<int>(j, "key3");
        CHECK(false);
    }
    catch(std::runtime_error& e)
    {
        CHECK(std::string(e.what()) == "Missing field 'key3' in JSON.");
    }
    catch(...)
    {
        CHECK(false);
    }
}

TEST_CASE( "test field value of wrong type JSON" )
{
    std::string json_str = "{ \"key1\" : 1.0 }";
    json j = json::parse(json_str);
    try
    {
        validate_json_field<int>(j, "key1");
        CHECK(false);
    }
    catch(std::runtime_error& e)
    {
        CHECK(std::string(e.what()) == "Invalid type for field 'key1'");
    }
    catch(...)
    {
        CHECK(false);
    }
}

TEST_CASE( "test wrong array size" )
{
    std::string json_str = "{ \"arr\": [1, 2, 3] }";
    json j = json::parse(json_str);
    try
    {
        validate_json_field<std::vector<int>>(j, "arr", 4);
        CHECK(false);
    }
    catch(std::runtime_error& e)
    {
        CHECK(std::string(e.what()) == "Invalid size for array field 'arr'");
    }
    catch(...)
    {
        CHECK(false);
    }
}

TEST_CASE( "test wrong array element type" )
{
    std::string json_str = "{ \"arr\": [1, 2.0, 3] }";
    json j = json::parse(json_str);
    try
    {
        validate_json_field<std::vector<int>>(j, "arr", 3);
        CHECK(false);
    }
    catch(std::runtime_error& e)
    {
        CHECK(std::string(e.what()) == "Invalid element type in array field 'arr'");
    }
    catch(...)
    {
        CHECK(false);
    }
}

}