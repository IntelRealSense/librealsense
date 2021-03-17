// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../common/utilities/string/string-utilities.h

#include "common.h"
#include "../../../common/utilities/string/string-utilities.h"

using namespace utilities::string;

TEST_CASE("string to value", "[string]")
{
    int value_i1;
    string_to_value<int>(std::string("123"), value_i1);
    CHECK(value_i1 == 123);

    int value_i2;
    string_to_value<int>(std::string("-123"), value_i2);
    CHECK(value_i2 == -123);

    unsigned int value_u;
    string_to_value<unsigned int>(std::string("123"), value_u);
    CHECK(value_u == 123u);

    long value_l;
    string_to_value<long>(std::string("123"), value_l);
    CHECK(value_l == 123);

    long long value_ll;
    string_to_value<long long> (std::string("123456789"), value_ll);
    CHECK(value_ll == 123456789);

    unsigned long value_ul;
    string_to_value<unsigned long>(std::string("123"), value_ul);
    CHECK(value_ul == 123);

    unsigned long long value_ull;
    string_to_value<unsigned long long>(std::string("123456789"), value_ull);
    CHECK(value_ull == 123456789);

    float value_f;
    string_to_value<float>(std::string("1.23456789"), value_f);
    CHECK(value_f == 1.23456789f);

    double value_d;
    string_to_value<double>(std::string("1.2345"), value_d);
    CHECK(value_d == 1.2345);

    long double value_ld;
    string_to_value<long double>(std::string("1.23456789"), value_ld);
    CHECK(value_ld == 1.23456789);

    char value_c;
    CHECK(!string_to_value<char>(std::string("abc"), value_c));

    int value;
    CHECK(!string_to_value<int>(std::string("123abc"), value));

    int value_big;
    CHECK(!string_to_value<int>(std::string("9999999999999999999999999999999999999999999999999999999999999999999"), value_big));

}
