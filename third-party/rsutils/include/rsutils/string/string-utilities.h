// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <algorithm>
#include <string>
#include <sstream>
#include <cmath> // std::isfinite

namespace rsutils {
namespace string {
    inline std::string hexify(unsigned char n)
    {
        std::string res;

        do
        {
            res += "0123456789ABCDEF"[n % 16];
            n >>= 4;
        } while (n);

        std::reverse(res.begin(), res.end());

        if (res.size() == 1)
        {
            res.insert(0, "0");
        }

        return res;
    }

    inline std::string to_lower(std::string x)
    {
        std::transform(x.begin(), x.end(), x.begin(), tolower);
        return x;
    }

    inline std::string to_upper(std::string x)
    {
        std::transform(x.begin(), x.end(), x.begin(), toupper);
        return x;
    }

    inline bool string_to_bool(const std::string& x)
    {
        return (to_lower(x) == "true");
    }

    // Pass by-value to perform in-place modifications
    inline unsigned int ascii_hex_string_to_uint(std::string str)
    {
        std::string delimiter = "x";

        auto pos = str.find(delimiter);
        str.erase(0, pos + delimiter.length());

        std::stringstream ss;
        unsigned int value;
        ss << str;
        ss >> std::hex >> value;

        return value;
    }

    template < typename T,
        typename std::enable_if< std::is_arithmetic< T >::value >::
        type* = nullptr >  // check if T is arithmetic during compile
        inline bool string_to_value(const std::string& str, T& result)
    {
        // Converts string to value via a given argument with desire type
        // Input:
        //      - string of a number
        //      - argument with T type
        // returns:true if conversion succeeded
        // NOTE for unsigned types:
        // if T is unsinged and the given string represents a negative number (meaning it starts with
        // '-'), then string_to_value returns false (rather than send the string to the std conversion
        // function). The reason for that is, stoul will return return a number in that case rather than
        // throwing an exception.

        try
        {
            size_t last_char_idx = 0;
            if (std::is_integral< T >::value)
            {
                if (std::is_same< T, unsigned long >::value)
                {
                    if (str[0] == '-')
                        return false;  // for explanation see 'NOTE for unsigned types'
                    result = static_cast<T>(std::stoul(str, &last_char_idx));
                }
                else if (std::is_same< T, unsigned long long >::value)
                {
                    if (str[0] == '-')
                        return false;
                    result = static_cast<T>(std::stoull(str, &last_char_idx));
                }
                else if (std::is_same< T, long >::value)
                {
                    result = static_cast<T>(std::stol(str, &last_char_idx));
                }
                else if (std::is_same< T, long long >::value)
                {
                    result = static_cast<T>(std::stoll(str, &last_char_idx));
                }
                else if (std::is_same< T, int >::value)
                {
                    result = static_cast<T>(std::stoi(str, &last_char_idx));
                }
                else if (std::is_same< T, short >::value)
                {  // no dedicated function fot short in std

                    int check_value = std::stoi(str, &last_char_idx);

                    if (check_value > std::numeric_limits< short >::max()
                        || check_value < std::numeric_limits< short >::min())
                        throw std::out_of_range("short");

                    result = static_cast<T>(check_value);
                }
                else if (std::is_same< T, unsigned int >::value)
                {  // no dedicated function in std - unsgined corresponds to 16 bit, and unsigned long
                    // corresponds to 32 bit
                    if (str[0] == '-')
                        return false;

                    unsigned long check_value = std::stoul(str, &last_char_idx);

                    if (check_value > std::numeric_limits< unsigned int >::max())
                        throw std::out_of_range("unsigned int");

                    result = static_cast<T>(check_value);
                }
                else if (std::is_same< T, unsigned short >::value
                    || std::is_same< T, unsigned short >::value)
                {  // no dedicated function in std - unsgined corresponds to 16 bit, and unsigned long
                    // corresponds to 32 bit
                    if (str[0] == '-')
                        return false;

                    unsigned long check_value = std::stoul(str, &last_char_idx);

                    if (check_value > std::numeric_limits< unsigned short >::max())
                        throw std::out_of_range("unsigned short");

                    result = static_cast<T>(check_value);
                }
                else
                {
                    return false;
                }
            }
            else if (std::is_floating_point< T >::value)
            {
                if (std::is_same< T, float >::value)
                {
                    result = static_cast<T>(std::stof(str, &last_char_idx));
                    if (!std::isfinite((float)result))
                        return false;
                }
                else if (std::is_same< T, double >::value)
                {
                    result = static_cast<T>(std::stod(str, &last_char_idx));
                    if (!std::isfinite((double)result))
                        return false;
                }
                else if (std::is_same< T, long double >::value)
                {
                    result = static_cast<T>(std::stold(str, &last_char_idx));
                    if (!std::isfinite((long double)result))
                        return false;
                }
                else
                {
                    return false;
                }
            }
            return str.size()
                == last_char_idx;  // all the chars in the string are converted to the value (thus there
                                    // are no other chars other than numbers)
        }
        catch (const std::invalid_argument&)
        {
            return false;
        }
        catch (const std::out_of_range&)
        {
            return false;
        }
    }

}  // namespace string
}  // namespace rsutils
