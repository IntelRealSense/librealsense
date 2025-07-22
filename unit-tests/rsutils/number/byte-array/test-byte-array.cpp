// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

//#cmake:dependencies rsutils

#include <unit-tests/test.h>
#include <rsutils/number/byte-manipulation.h>

using namespace rsutils::number;

namespace {


TEST_CASE( "test reversing single byte - case 1" )
{
	uint8_t byte = 0x55; // b'01010101
	uint8_t reversed_byte = 0xAA; // b'10101010
	CHECK(reverse_bits(byte) == reversed_byte);  // little-endian
}

TEST_CASE("test reversing single byte - case 2")
{
	uint8_t byte = 0x1; // b'00000001
	uint8_t reversed_byte = 0x80; // b'10000000
	CHECK(reverse_bits(byte) == reversed_byte);
}

TEST_CASE("test reversing single byte - case 3")
{
	uint8_t byte = 0x0; // b'00000000
	uint8_t reversed_byte = 0x0; // b'00000000
	CHECK(reverse_bits(byte) == reversed_byte);
}

TEST_CASE("test reversing single byte - case 4")
{
	uint8_t byte = 0xFF; // b'11111111
	uint8_t reversed_byte = 0xFF; // b'11111111
	CHECK(reverse_bits(byte) == reversed_byte);
}

TEST_CASE("test reversing array of bytes of size 1")
{
	std::vector<uint8_t> bytes_arr = { 0x55 };  // { b'01010101 }
	uint8_t reversed_byte_at_0 = 0xAA; // b'10101010
	reverse_byte_array_bits(bytes_arr.data(), 1);
	CHECK(bytes_arr[0] == reversed_byte_at_0);  // little-endian
}

TEST_CASE("test reversing array of bytes of size 4")
{
	std::vector<uint8_t> bytes_arr = { 0x55, 0x01, 0x80, 0xF0 };  
	// bytes_arr = { b'01010101, b'00000001, b'10000000, b'11110000 }
	std::vector<uint8_t> expected_reversed_bytes = { 0xAA, 0x80, 0x01, 0xF };
	// expected_reversed_bytes = { b'10101010, b'10000000, b'00000001, b'00001111}

	reverse_byte_array_bits(bytes_arr.data(), static_cast<int>(bytes_arr.size()));
	for (int i = 0; i < bytes_arr.size(); ++i)
	{
		CHECK(bytes_arr[i] == expected_reversed_bytes[i]);
	}
}


}
