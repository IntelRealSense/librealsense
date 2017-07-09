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

#ifndef ROSLIB_SERIALIZED_MESSAGE_H
#define ROSLIB_SERIALIZED_MESSAGE_H

#include "roscpp_serialization_macros.h"

#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>

namespace ros
{

class ROSCPP_SERIALIZATION_DECL SerializedMessage
{
public:
  boost::shared_array<uint8_t> buf;
  size_t num_bytes;
  uint8_t* message_start;

  boost::shared_ptr<void const> message;
  const std::type_info* type_info;

  SerializedMessage()
  : buf(boost::shared_array<uint8_t>())
  , num_bytes(0)
  , message_start(0)
  , type_info(0)
  {}

  SerializedMessage(boost::shared_array<uint8_t> buf, size_t num_bytes)
  : buf(buf)
  , num_bytes(num_bytes)
  , message_start(buf ? buf.get() : 0)
  , type_info(0)
  { }
};

} // namespace ros

#endif // ROSLIB_SERIALIZED_MESSAGE_H
