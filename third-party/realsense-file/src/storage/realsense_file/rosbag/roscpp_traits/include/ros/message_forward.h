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

#ifndef ROSLIB_MESSAGE_FORWARD_H
#define ROSLIB_MESSAGE_FORWARD_H

// Make sure that either __GLIBCXX__ or _LIBCPP_VERSION is defined.
#include <cstddef>

// C++ standard section 17.4.3.1/1 states that forward declarations of STL types
// that aren't specializations involving user defined types results in undefined
// behavior. Apparently only libc++ has a problem with this and won't compile it.
#ifndef _LIBCPP_VERSION
namespace std
{
template<typename T> class allocator;
}
#else
#include <memory>
#endif

namespace boost
{
template<typename T> class shared_ptr;
}

/**
 * \brief Forward-declare a message, including Ptr and ConstPtr types, with an allocator
 *
 * \param ns The namespace the message should be declared inside
 * \param m The "base" message type, i.e. the name of the .msg file
 * \param new_name The name you'd like the message to have
 * \param alloc The allocator to use, e.g. std::allocator
 */
#define ROS_DECLARE_MESSAGE_WITH_ALLOCATOR(msg, new_name, alloc) \
  template<class Allocator> struct msg##_; \
  typedef msg##_<alloc<void> > new_name; \
  typedef boost::shared_ptr<new_name> new_name##Ptr; \
  typedef boost::shared_ptr<new_name const> new_name##ConstPtr;

/**
 * \brief Forward-declare a message, including Ptr and ConstPtr types, using std::allocator
 * \param msg The "base" message type, i.e. the name of the .msg file
 */
#define ROS_DECLARE_MESSAGE(msg) ROS_DECLARE_MESSAGE_WITH_ALLOCATOR(msg, msg, std::allocator)

#endif // ROSLIB_MESSAGE_FORWARD_H
