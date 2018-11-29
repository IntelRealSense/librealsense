
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

#ifndef STD_MSGS_TRAIT_MACROS_H
#define STD_MSGS_TRAIT_MACROS_H

#define STD_MSGS_DEFINE_BUILTIN_TRAITS(builtin, msg, static_md5sum1, static_md5sum2) \
  namespace rs2rosinternal \
  { \
  namespace message_traits \
  { \
    \
    template<> struct MD5Sum<builtin> \
    { \
      static const char* value() \
      { \
        return MD5Sum<std_msgs::msg>::value(); \
      } \
      \
      static const char* value(const builtin&) \
      { \
        return value(); \
      } \
    }; \
    \
    template<> struct DataType<builtin> \
    { \
      static const char* value() \
      { \
        return DataType<std_msgs::msg>::value(); \
      } \
     \
      static const char* value(const builtin&) \
      { \
        return value(); \
      } \
    }; \
    \
    template<> struct Definition<builtin> \
    { \
      static const char* value() \
      { \
        return Definition<std_msgs::msg>::value(); \
      } \
      \
      static const char* value(const builtin&) \
      { \
        return value(); \
      } \
    }; \
    \
  } \
  }

#endif // STD_MSGS_TRAIT_MACROS_H
