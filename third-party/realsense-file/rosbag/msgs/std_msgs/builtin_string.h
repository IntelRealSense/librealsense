/*
 * Copyright (C) 2009, Willow Garage, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the names of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef STD_MSGS_BUILTIN_STRING_H
#define STD_MSGS_BUILTIN_STRING_H

#include "trait_macros.h"
#include <std_msgs/String.h>

namespace rs2rosinternal
{
namespace message_traits
{

template<class ContainerAllocator>
struct MD5Sum<std::basic_string<char, std::char_traits<char>, ContainerAllocator> >
{
  static const char* value()
  {
    ROS_STATIC_ASSERT(MD5Sum<std_msgs::String>::static_value1 == 0x992ce8a1687cec8cULL);
    ROS_STATIC_ASSERT(MD5Sum<std_msgs::String>::static_value2 == 0x8bd883ec73ca41d1ULL);
    return MD5Sum<std_msgs::String_<ContainerAllocator> >::value();
  }

  static const char* value(const std::basic_string<char, std::char_traits<char>, ContainerAllocator>&)
  {
    return value();
  }
};

template<class ContainerAllocator >
struct DataType<std::basic_string<char, std::char_traits<char>, ContainerAllocator > >
{
  static const char* value()
  {
    return DataType<std_msgs::String_<ContainerAllocator> >::value();
  }

  static const char* value(const std::basic_string<char, std::char_traits<char>, ContainerAllocator >&)
  {
    return value();
  }
};

template<class ContainerAllocator >
struct Definition<std::basic_string<char, std::char_traits<char>, ContainerAllocator > >
{
  static const char* value()
  {
    return Definition<std_msgs::String_<ContainerAllocator> >::value();
  }

  static const char* value(const std::basic_string<char, std::char_traits<char>, ContainerAllocator >&)
  {
    return value();
  }
};

}
}

#endif
