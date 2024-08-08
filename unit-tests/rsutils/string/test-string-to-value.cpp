// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:dependencies rsutils

#include "common.h"
#include <rsutils/string/string-utilities.h>
#include <ostream>
using namespace rsutils::string;

template <typename T, typename std::enable_if <std::is_arithmetic<T>::value>::type* = nullptr> // check if T is arithmetic during compile
std::string to_str(const T& t)
{ 
    /// The std::to_string will make the min float value equal zero, so another function is 
    /// params: a value type T 
    /// returns: a string of the given number
    std::ostringstream os;
    os << t;
    return os.str();
}

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
    for (const auto test : tests)
    {
        CAPTURE(test.str);
        CAPTURE(test.expected_res);
        CHECK(string_to_value<T>(test.str, value) == test.expected_res);
        if (test.expected_res)
        {
            CAPTURE(value);
            CAPTURE(test.value);
            if (std::is_integral<T>::value)
                CHECK(value == test.value);
            else // floating-point
            {
                CHECK(value == Catch::Approx(test.value));
            }
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
    int_tests.push_back(string_checker<int>{ std::to_string(std::numeric_limits<int>::max())+"0", 0, false });
    int_tests.push_back(string_checker<int>{ std::to_string(std::numeric_limits<int>::min())+"0", 0, false });
    int_tests.push_back(string_checker<int>{ "abc", 0, false });

    check_tests<int>(int_tests);
    
    // unsigned int 
    std::vector<string_checker<unsigned int>> uint_tests;
    uint_tests.push_back(string_checker<unsigned int>{ "123", 123, true });
    uint_tests.push_back(string_checker<unsigned int>{ "123456789", 123456789, true });
    uint_tests.push_back(string_checker<unsigned int>{ "-123456789", 0, false });
    uint_tests.push_back(string_checker<unsigned int>{ std::to_string(std::numeric_limits<unsigned int>::max())+"0", 0, false });
    uint_tests.push_back(string_checker<unsigned int>{ "abc", 0, false });

    check_tests<unsigned int>(uint_tests);

    // short
    std::vector<string_checker<short>> s_tests;
    s_tests.push_back(string_checker<short>{ "123", 123, true });
    s_tests.push_back(string_checker<short>{ "-123", -123, true });
    s_tests.push_back(string_checker<short>{ std::to_string(std::numeric_limits<short>::max())+"0", 0, false });
    s_tests.push_back(string_checker<short>{ std::to_string(std::numeric_limits<short>::min())+"0", 0, false });
    s_tests.push_back(string_checker<short>{ "abc", 0, false });

    check_tests<short>(s_tests);

    // unsigned short
    std::vector<string_checker<unsigned short>> us_tests;
    us_tests.push_back(string_checker<unsigned short>{ "123", 123, true });
    us_tests.push_back(string_checker<unsigned short>{ "-123", 0, false });
    us_tests.push_back(string_checker<unsigned short>{std::to_string(std::numeric_limits<unsigned short>::max())+"0", 0, false });
    us_tests.push_back(string_checker<unsigned short>{ "abc", 0, false });

    check_tests<unsigned short>(us_tests);

    // long
    std::vector<string_checker<long>> l_tests;
    l_tests.push_back(string_checker<long>{ "123", 123, true });
    l_tests.push_back(string_checker<long>{ "-123", -123, true });
    l_tests.push_back(string_checker<long>{ "-123456789", -123456789, true });
    l_tests.push_back(string_checker<long>{ std::to_string(std::numeric_limits<long>::max())+"0", 0, false });
    l_tests.push_back(string_checker<long>{ std::to_string(std::numeric_limits<long>::min())+"0", 0, false });
    l_tests.push_back(string_checker<long>{ "abc", 0, false });

    check_tests<long>(l_tests);

    // long long
    std::vector<string_checker<long long>> ll_tests;
    ll_tests.push_back(string_checker<long long>{ "123", 123, true });
    ll_tests.push_back(string_checker<long long>{ "-123", -123, true });
    ll_tests.push_back(string_checker<long long>{ "12345", 12345, true });
    ll_tests.push_back(string_checker<long long>{ "-12345", -12345, true });
    ll_tests.push_back(string_checker<long long>{ std::to_string(std::numeric_limits<long long>::max())+"0", 0, false });
    ll_tests.push_back(string_checker<long long>{ std::to_string(std::numeric_limits<long long>::min()) + "0", 0, false });
    ll_tests.push_back(string_checker<long long>{ "abc", 0, false });

    check_tests<long long>(ll_tests);

    // ungined long long
    std::vector<string_checker<unsigned long long>> ull_tests;
    ull_tests.push_back(string_checker<unsigned long long>{ "123", 123, true });
    ull_tests.push_back(string_checker<unsigned long long>{ "123456789", 123456789, true });
    ull_tests.push_back(string_checker<unsigned long long>{ "-123456789", 0, false });
    ull_tests.push_back(string_checker<unsigned long long>{ std::to_string(std::numeric_limits<unsigned long long>::max())+"0", 0, false });
    ull_tests.push_back(string_checker<unsigned long long>{ "abc", 0, false });

    check_tests<unsigned long long>(ull_tests);

    // float
    std::vector<string_checker<float>> f_tests;
    f_tests.push_back(string_checker<float>{ "1.23456789", 1.23456789f, true });
    f_tests.push_back(string_checker<float>{ "2.12121212", 2.12121212f, true });
    f_tests.push_back(string_checker<float>{ "-1.23456789", -1.23456789f, true });
    f_tests.push_back(string_checker<float>{ "-2.12121212", -2.12121212f, true });
    f_tests.push_back(string_checker<float>{ "INF", 0.f, false });
    f_tests.push_back(string_checker<float>{ "-INF", 0.f, false });
    f_tests.push_back(string_checker<float>{ to_str(std::numeric_limits<float>::max()) + "0", 0.f, false });
    f_tests.push_back(string_checker<float>{ to_str(std::numeric_limits<float>::min())+"0", 0.f, false });
    f_tests.push_back(string_checker<float>{ to_str(std::numeric_limits<float>::lowest())+"0", 0.f, false });
    f_tests.push_back(string_checker<float>{ "NaN", 0.f, false });
    f_tests.push_back(string_checker<float>{ "abc", 0.f, false });

    check_tests<float>(f_tests);

    // double
    std::vector<string_checker<double>> d_tests;
    d_tests.push_back(string_checker<double>{ "9.876543", 9.876543, true });
    d_tests.push_back(string_checker<double>{ "2.12121212", 2.12121212, true });
    d_tests.push_back(string_checker<double>{ "-1.234598765", -1.234598765, true });
    d_tests.push_back(string_checker<double>{ "-2.12121212", -2.12121212, true });
    d_tests.push_back(string_checker<double>{ "INF", 0., false });
    d_tests.push_back(string_checker<double>{ "-INF", 0., false });
    d_tests.push_back(string_checker<double>{ to_str(std::numeric_limits<double>::max())+"0", 0., false });
    d_tests.push_back(string_checker<double>{ to_str(std::numeric_limits<double>::min())+"0", 0., false });
    d_tests.push_back(string_checker<double>{ to_str(std::numeric_limits<double>::lowest())+"0", 0., false });
    d_tests.push_back(string_checker<double>{ "NaN", 0., false });
    d_tests.push_back(string_checker<double>{ "abc", 0., false });

    check_tests<double>(d_tests);

    // long double
    std::vector<string_checker<long double>> ld_tests;
    ld_tests.push_back(string_checker<long double>{ "12345.6789123456789", 12345.6789123456789, true });
    ld_tests.push_back(string_checker<long double>{ "5432.123456789", 5432.123456789, true });
    ld_tests.push_back(string_checker<long double>{ "-12345678.23456789", -12345678.23456789, true });
    ld_tests.push_back(string_checker<long double>{ "INF", 0., false });
    ld_tests.push_back(string_checker<long double>{ "-INF", 0., false });
    ld_tests.push_back(string_checker<long double>{ to_str(std::numeric_limits<long double>::max())+"0", 0., false });
    ld_tests.push_back(string_checker<long double>{ to_str(std::numeric_limits<long double>::min())+"0", 0., false });
    ld_tests.push_back(string_checker<long double>{ to_str(std::numeric_limits<long double>::lowest())+"0", 0., false });
    ld_tests.push_back(string_checker<long double>{ "NaN", 0., false });
    ld_tests.push_back(string_checker<long double>{ "abc", 0., false });

    check_tests<long double>(ld_tests);
}
