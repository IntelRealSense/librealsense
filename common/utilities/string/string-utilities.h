// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>

namespace utilities {
    namespace string {

        // Converts string to number
        // returns true if conversion succeeded, false otherwise
        // Input: 
        //      - string of a number
        //      - parameter T - the number's type

        template<class T>
        inline bool string_to_value(const std::string& str, T& result)
        {
            try
            {
                size_t last_char_idx;
                if (std::is_arithmetic<T>::value)
                {
                    if (std::is_integral<T>::value)
                    {
                         if (std::is_same<T, unsigned long>::value)
                        {
                            result = std::stoul(str, &last_char_idx);
                        }
                        else if (std::is_same<T, unsigned long long>::value)
                        {
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
                        else if (std::is_same<T, int>::value)
                        {
                            result = std::stoi(str, &last_char_idx);
                        }
                        else if (std::is_same<T, short>::value)
                         { // no dedicated function in std
                             result = std::stoi(str, &last_char_idx);
                         }
                        else if (std::is_same<T, unsigned short>::value)
                         { // no dedicated function in std
                             result = std::stoul(str, &last_char_idx);
                         }
                        else if (std::is_same<T, unsigned int>::value)
                         { // no dedicated function in std - unsgined in corresponds to 16 bit, and unsigned long  corresponds to 32 bit
                             result = std::stoul(str, &last_char_idx);
                         }
                        else
                        {
                            return false;
                        }
                    }
                    else if (std::is_floating_point<T>::value)
                    {
                        if (std::is_same<T, double>::value)
                        {
                            result = std::stod(str, &last_char_idx);
                        }
                        else if (std::is_same<T, long double>::value)
                        {
                            result = std::stold(str, &last_char_idx);
                        }
                        else if (std::is_same<T, float>::value)
                        {
                            result = std::stof(str, &last_char_idx);
                        }
                        else
                        {
                            return false;
                        }
                    }

                    return str.size() == last_char_idx; // all the chars in the string are converted to the value (thus there are no other chars other than numbers)

                }
                else // T is not arithmetic  
                {
                    return false;
                }
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