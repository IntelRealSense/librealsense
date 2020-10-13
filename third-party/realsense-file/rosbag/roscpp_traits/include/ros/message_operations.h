/*
 * Copyright (C) 2010, Willow Garage, Inc.
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

#ifndef ROSLIB_MESSAGE_OPERATIONS_H
#define ROSLIB_MESSAGE_OPERATIONS_H

#include <ostream>

namespace rs2rosinternal
{
namespace message_operations
{

template<typename M>
struct Printer
{
  template<typename Stream>
  static void stream(Stream& s, const std::string& indent, const M& value)
  {
    (void)indent;
    s << value << "\n";
  }
};

// Explicitly specialize for uint8_t/int8_t because otherwise it thinks it's a char, and treats
// the value as a character code
template<>
struct Printer<int8_t>
{
  template<typename Stream>
  static void stream(Stream& s, const std::string& indent, int8_t value)
  {
    (void)indent;
    s << static_cast<int32_t>(value) << "\n";
  }
};

template<>
struct Printer<uint8_t>
{
  template<typename Stream>
  static void stream(Stream& s, const std::string& indent, uint8_t value)
  {
    (void)indent;
    s << static_cast<uint32_t>(value) << "\n";
  }
};

} // namespace message_operations
} // namespace rs2rosinternal

#endif // ROSLIB_MESSAGE_OPERATIONS_H

