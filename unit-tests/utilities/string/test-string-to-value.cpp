// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../common/utilities/string/string-utilities.h

#include "common.h"
#include "../../../common/utilities/string/string-utilities.h"

using namespace utilities::string;

template<typename T>
struct string_checker
{
    std::string str;
    T value;
    bool expected_res;
};

template<typename T>
void check_tests(std::vector<string_checker<T>>& tests)
{
    T value;
    for (const auto& test : tests)
    {
        CHECK(string_to_value<T>(test.str, value) == test.expected_res);
        if (test.expected_res)
        {
            CHECK(value == test.value);
        }
    }
}

TEST_CASE("string to value", "[string]")
{
    // int
    std::vector<string_checker<int>> int_tests;
    int_tests.push_back(string_checker<int>{ "123", 123, true });
    int_tests.push_back(string_checker<int>{ "-123", -123, true });
    int_tests.push_back(string_checker<int>{ "-123456789", -123456789, true });
    int_tests.push_back(string_checker<int>{ "123456789123456789123456789", 0, false });
    int_tests.push_back(string_checker<int>{ "-123456789123456789123456789", 0, false });
    int_tests.push_back(string_checker<int>{ "abc", 0, false });

    check_tests<int>(int_tests);
    
    // unsigned int 
    std::vector<string_checker<unsigned int>> uint_tests;
    uint_tests.push_back(string_checker<unsigned int>{ "123", 123, true });
    uint_tests.push_back(string_checker<unsigned int>{ "123456789", 123456789u, true });
    uint_tests.push_back(string_checker<unsigned int>{ "-123456789", 0, false });
    uint_tests.push_back(string_checker<unsigned int>{ "123456789123456789123456789", 0, false });
    uint_tests.push_back(string_checker<unsigned int>{ "abc", 0, false });

    check_tests<unsigned int>(uint_tests);

    // short
    std::vector<string_checker<short>> s_tests;
    s_tests.push_back(string_checker<short>{ "123", 123, true });
    s_tests.push_back(string_checker<short>{ "-123", -123, true });
    s_tests.push_back(string_checker<short>{ "123456789123456789123456789", 0, false });
    s_tests.push_back(string_checker<short>{ "-123456789123456789123456789", 0, false });
    s_tests.push_back(string_checker<short>{ "abc", 0, false });

    check_tests<short>(s_tests);

    // unsigned short
    std::vector<string_checker<unsigned short>> us_tests;
    us_tests.push_back(string_checker<unsigned short>{ "123", 123, true });
    us_tests.push_back(string_checker<unsigned short>{ "-123", 0, false });
    us_tests.push_back(string_checker<unsigned short>{ "123456789123456789123456789", 0, false });
    us_tests.push_back(string_checker<unsigned short>{ "-123456789123456789123456789", 0, false });
    us_tests.push_back(string_checker<unsigned short>{ "abc", 0, false });

    check_tests<unsigned short>(us_tests);

    // long
    std::vector<string_checker<long>> l_tests;
    l_tests.push_back(string_checker<long>{ "123", 123, true });
    l_tests.push_back(string_checker<long>{ "-123", -123, true });
    l_tests.push_back(string_checker<long>{ "-123456789", -123456789, true });
    l_tests.push_back(string_checker<long>{ "123456789123456789123456789", 0, false });
    l_tests.push_back(string_checker<long>{ "-123456789123456789123456789", 0, false });
    l_tests.push_back(string_checker<long>{ "abc", 0, false });

    check_tests<long>(l_tests);


    // long long
    std::vector<string_checker<long long>> ll_tests;
    ll_tests.push_back(string_checker<long long>{ "123", 123, true });
    ll_tests.push_back(string_checker<long long>{ "-123", -123, true });
    ll_tests.push_back(string_checker<long long>{ "12345", 12345, true });
    ll_tests.push_back(string_checker<long long>{ "-12345", -12345, true });
    ll_tests.push_back(string_checker<long long>{ "123456789123456789123456789", 0, false });
    ll_tests.push_back(string_checker<long long>{ "-123456789123456789123456789", 0, false });
    ll_tests.push_back(string_checker<long long>{ "abc", 0, false });

    check_tests<long long>(ll_tests);

    // ungined long long
    std::vector<string_checker<unsigned long long>> ull_tests;
    ull_tests.push_back(string_checker<unsigned long long>{ "123", 123, true });
    ull_tests.push_back(string_checker<unsigned long long>{ "123456789", 123456789, true });
    ull_tests.push_back(string_checker<unsigned long long>{ "-123456789", 0, false });
    ull_tests.push_back(string_checker<unsigned long long>{ "123456789123456789123456789", 0, false });
    ull_tests.push_back(string_checker<unsigned long long>{ "abc", 0, false });

    check_tests<unsigned long long>(ull_tests);
    
    // float
    std::vector<string_checker<float>> f_tests;
    f_tests.push_back(string_checker<float>{ "1.23456789", 1.23456789, true });
    f_tests.push_back(string_checker<float>{ "2.12121212", 2.12121212, true });
    f_tests.push_back(string_checker<float>{ "-1.23456789", -1.23456789, true });
    f_tests.push_back(string_checker<float>{ "-2.12121212", -2.12121212, true });
    f_tests.push_back(string_checker<float>{ "INF", 0, false });
    f_tests.push_back(string_checker<float>{ "-INF", 0, false });
    f_tests.push_back(string_checker<float>{ "NaN", 0, false });
    f_tests.push_back(string_checker<float>{ "abc", 0, false });

    check_tests<float>(f_tests);
    
    // double
    std::vector<string_checker<double>> d_tests;
    d_tests.push_back(string_checker<double>{ "1.2345", 1.2345, true });
    d_tests.push_back(string_checker<double>{ "2.12121212", 2.12121212, true });
    d_tests.push_back(string_checker<double>{ "-1.2345", -1.2345, true });
    d_tests.push_back(string_checker<double>{ "-2.12121212", -2.12121212, true });
    d_tests.push_back(string_checker<double>{ "INF", 0, false });
    d_tests.push_back(string_checker<double>{ "-INF", 0, false });
    d_tests.push_back(string_checker<double>{ "NaN", 0, false });
    d_tests.push_back(string_checker<double>{ "abc", 0, false });

    check_tests<double>(d_tests);
    
    // long double
    std::vector<string_checker<long double>> ld_tests;
    ld_tests.push_back(string_checker<long double>{ "1.23456789", 1.23456789, true });
    ld_tests.push_back(string_checker<long double>{ "-1.23456789", -1.23456789, true });
    ld_tests.push_back(string_checker<long double>{ "2.12121212", 2.12121212, true });
    ld_tests.push_back(string_checker<long double>{ "-2.12121212", -2.12121212, true });
    ld_tests.push_back(string_checker<long double>{ "INF", 0, false });
    ld_tests.push_back(string_checker<long double>{ "-INF", 0, false });
    ld_tests.push_back(string_checker<long double>{ "NaN", 0, false });
    ld_tests.push_back(string_checker<long double>{ "abc", 0, false });

    check_tests<long double>(ld_tests);
    
    //assert static cimpilation error due to invalid given parameter T 
    /*
    std::vector<int> value_invalid;
    CHECK(!string_to_value<std::vector<int>>(std::string("abc"), value_invalid));
    */
}
