// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>

namespace utilities {
    namespace string {

        // Converts string to value via a given argument with desire type
        // Input: 
        //      - string of a number
        //      - argument with T type
        // returns true if conversion succeeded

        // NOTE for unsigned types:
        // if the minus sign was part of the input sequence, 
        // the numeric value calculated from the sequence of digits is negated as if 
        // by unary minus in the result type, which applies unsigned integer wraparound rules.
        // for example - when stoul gets'-4', it will return -(unsigned long)4

        template <typename T, typename std::enable_if <std::is_arithmetic<T>::value>::type* = nullptr> // check if T is arithmetic during compile
        inline bool string_to_value(const std::string& str, T& result)
        {
            try
            {
                size_t last_char_idx;
                if (std::is_integral<T>::value)
                {
                        if (std::is_same<T, unsigned long>::value)
                    {
                        if (str[0] == '-') return false; //for explanation see 'NOTE for unsigned types'
                        result = std::stoul(str, &last_char_idx);
                    }
                    else if (std::is_same<T, unsigned long long>::value)
                    {
                            if (str[0] == '-') return false;
                        result = std::stoull(str, &last_char_idx);
                    }
                    else if (std::is_same<T, long>::value)
                    {
                        result = std::stol(str, &last_char_idx);
                    }
                    else if (std::is_same<T, long long>::value)
                    {
                        result = std::stoll(str, &last_char_idx);
                    }
                    else if (std::is_same<T, int>::value || std::is_same<T, short>::value)
                    {// no dedicated function fot short in std
                        result = std::stoi(str, &last_char_idx);
                    }
                    else if (std::is_same<T, unsigned short>::value || std::is_same<T, unsigned int>::value)
                        { // no dedicated function in std - unsgined corresponds to 16 bit, and unsigned long corresponds to 32 bit
                            if (str[0] == '-') return false;
                            result = std::stoul(str, &last_char_idx);
                        }
                    else
                    {
                            return false;
                    }
                }
                else if (std::is_floating_point<T>::value)
                {/*== !std::isfinite((long double)result)*/ 
                    if (std::is_same<T, float>::value)
                    {
                        result = std::stof(str, &last_char_idx);
                        if (std::isnan((float)result) || std::isinf((float)result) || !std::isfinite((float)result)) return false; // if value is NaN or inf (or -inf), return false
                    }
                    else if (std::is_same<T, double>::value)
                    {
                        result = std::stod(str, &last_char_idx); 
                        int res = std::isfinite((double)result);
                        if (std::isnan((double)result) || std::isinf((double)result) || !std::isfinite((double)result)) 
                            return false; // if value is NaN or inf (or -inf) or ( 0 < value < min_float/double), return false
                    }
                    else if (std::is_same<T, long double>::value)
                    {
                        result = std::stold(str, &last_char_idx);
                        if (std::isnan((long double)result) || std::isinf((long double)result) || !std::isfinite((long double)result)) return false;
                    }
                    else
                    {
                        return false;
                    }
                }
                return str.size() == last_char_idx; // all the chars in the string are converted to the value (thus there are no other chars other than numbers)
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
}  // namespace utilities
