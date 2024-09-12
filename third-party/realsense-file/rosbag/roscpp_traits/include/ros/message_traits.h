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

#ifndef ROSLIB_MESSAGE_TRAITS_H
#define ROSLIB_MESSAGE_TRAITS_H

#include "message_forward.h"

#include "ros/time.h"

#include <string>
#include <type_traits>
#include <memory.h>

namespace std_msgs
{
  ROS_DECLARE_MESSAGE(Header)
}

#define ROS_IMPLEMENT_SIMPLE_TOPIC_TRAITS(msg, md5sum, datatype, definition) \
  namespace rs2rosinternal \
  { \
  namespace message_traits \
  { \
  template<> struct MD5Sum<msg> \
  { \
    static const char* value() { return md5sum; } \
    static const char* value(const msg&) { return value(); } \
  }; \
  template<> struct DataType<msg> \
  { \
    static const char* value() { return datatype; } \
    static const char* value(const msg&) { return value(); } \
  }; \
  template<> struct Definition<msg> \
  { \
    static const char* value() { return definition; } \
    static const char* value(const msg&) { return value(); } \
  }; \
  } \
  }


namespace rs2rosinternal
{
namespace message_traits
{

/**
 * \brief A simple datatype is one that can be memcpy'd directly in array form, i.e. it's a POD, fixed-size type and
 * sizeof(M) == sum(serializationLength(M:a...))
 */
template<typename M> struct IsSimple : public std::false_type {};
/**
 * \brief A fixed-size datatype is one whose size is constant, i.e. it has no variable-length arrays or strings
 */
template<typename M> struct IsFixedSize : public std::false_type {};
/**
 * \brief HasHeader informs whether or not there is a header that gets serialized as the first thing in the message
 */
template<typename M> struct HasHeader : public std::false_type {};

/**
 * \brief Am I message or not
 */
template<typename M> struct IsMessage : public std::false_type {};

/**
 * \brief Specialize to provide the md5sum for a message
 */
template<typename M>
struct MD5Sum
{
  static const char* value()
  {
    return M::__s_getMD5Sum();
  }

  static const char* value(const M& m)
  {
    return m.__getMD5Sum();
  }
};

/**
 * \brief Specialize to provide the datatype for a message
 */
template<typename M>
struct DataType
{
  static const char* value()
  {
    return M::__s_getDataType();
  }

  static const char* value(const M& m)
  {
    return m.__getDataType();
  }
};

/**
 * \brief Specialize to provide the definition for a message
 */
template<typename M>
struct Definition
{
  static const char* value()
  {
    return M::__s_getMessageDefinition();
  }

  static const char* value(const M& m)
  {
    return m.__getMessageDefinition();
  }
};

/**
 * \brief Header trait.  In the default implementation pointer()
 * returns &m.header if HasHeader<M>::value is true, otherwise returns NULL
 */
template<typename M, typename Enable = void>
struct Header
{
  static std_msgs::Header* pointer(M& m) { (void)m; return 0; }
  static std_msgs::Header const* pointer(const M& m) { (void)m; return 0; }
};

template<typename M>
struct Header< M, typename std::enable_if< HasHeader< M >::value >::type >
{
  static std_msgs::Header* pointer(M& m) { return &m.header; }
  static std_msgs::Header const* pointer(const M& m) { return &m.header; }
};

/**
 * \brief FrameId trait.  In the default implementation pointer()
 * returns &m.header.frame_id if HasHeader<M>::value is true, otherwise returns NULL.  value()
 * does not exist, and causes a compile error
 */
template<typename M, typename Enable = void>
struct FrameId
{
  static std::string* pointer(M& m) { (void)m; return 0; }
  static std::string const* pointer(const M& m) { (void)m; return 0; }
};

template<typename M>
struct FrameId< M, typename std::enable_if< HasHeader< M >::value >::type >
{
  static std::string* pointer(M& m) { return &m.header.frame_id; }
  static std::string const* pointer(const M& m) { return &m.header.frame_id; }
  static std::string value(const M& m) { return m.header.frame_id; }
};

/**
 * \brief TimeStamp trait.  In the default implementation pointer()
 * returns &m.header.stamp if HasHeader<M>::value is true, otherwise returns NULL.  value()
 * does not exist, and causes a compile error
 */
template<typename M, typename Enable = void>
struct TimeStamp
{
  static rs2rosinternal::Time* pointer(M& m) { (void)m; return 0; }
  static rs2rosinternal::Time const* pointer(const M& m) { (void)m; return 0; }
};

template<typename M>
struct TimeStamp< M, typename std::enable_if< HasHeader< M >::value >::type >
{
  static rs2rosinternal::Time* pointer(typename std::remove_const<M>::type &m) { return &m.header.stamp; }
  static rs2rosinternal::Time const* pointer(const M& m) { return &m.header.stamp; }
  static rs2rosinternal::Time value(const M& m) { return m.header.stamp; }
};

/**
 * \brief returns MD5Sum<M>::value();
 */
template<typename M>
inline const char* md5sum()
{
  return MD5Sum<typename std::remove_reference<typename std::remove_const<M>::type>::type>::value();
}

/**
 * \brief returns DataType<M>::value();
 */
template<typename M>
inline const char* datatype()
{
  return DataType<typename std::remove_reference<typename std::remove_const<M>::type>::type>::value();
}

/**
 * \brief returns Definition<M>::value();
 */
template<typename M>
inline const char* definition()
{
  return Definition<typename std::remove_reference<typename std::remove_const<M>::type>::type>::value();
}

/**
 * \brief returns MD5Sum<M>::value(m);
 */
template<typename M>
inline const char* md5sum(const M& m)
{
  return MD5Sum<typename std::remove_reference<typename std::remove_const<M>::type>::type>::value(m);
}

/**
 * \brief returns DataType<M>::value(m);
 */
template<typename M>
inline const char* datatype(const M& m)
{
  return DataType<typename std::remove_reference<typename std::remove_const<M>::type>::type>::value(m);
}

/**
 * \brief returns Definition<M>::value(m);
 */
template<typename M>
inline const char* definition(const M& m)
{
  return Definition<typename std::remove_reference<typename std::remove_const<M>::type>::type>::value(m);
}

/**
 * \brief returns Header<M>::pointer(m);
 */
template<typename M>
inline std_msgs::Header* header(M& m)
{
  return Header<typename std::remove_reference<typename std::remove_const<M>::type>::type>::pointer(m);
}

/**
 * \brief returns Header<M>::pointer(m);
 */
template<typename M>
inline std_msgs::Header const* header(const M& m)
{
  return Header<typename std::remove_reference<typename std::remove_const<M>::type>::type>::pointer(m);
}

/**
 * \brief returns FrameId<M>::pointer(m);
 */
template<typename M>
inline std::string* frameId(M& m)
{
  return FrameId<typename std::remove_reference<typename std::remove_const<M>::type>::type>::pointer(m);
}

/**
 * \brief returns FrameId<M>::pointer(m);
 */
template<typename M>
inline std::string const* frameId(const M& m)
{
  return FrameId<typename std::remove_reference<typename std::remove_const<M>::type>::type>::pointer(m);
}

/**
 * \brief returns TimeStamp<M>::pointer(m);
 */
template<typename M>
inline rs2rosinternal::Time* timeStamp(M& m)
{
  return TimeStamp<typename std::remove_reference<typename std::remove_const<M>::type>::type>::pointer(m);
}

/**
 * \brief returns TimeStamp<M>::pointer(m);
 */
template<typename M>
inline rs2rosinternal::Time const* timeStamp(const M& m)
{
  return TimeStamp<typename std::remove_reference<typename std::remove_const<M>::type>::type>::pointer(m);
}

/**
 * \brief returns IsSimple<M>::value;
 */
template<typename M>
inline bool isSimple()
{
  return IsSimple<typename std::remove_reference<typename std::remove_const<M>::type>::type>::value;
}

/**
 * \brief returns IsFixedSize<M>::value;
 */
template<typename M>
inline bool isFixedSize()
{
  return IsFixedSize<typename std::remove_reference<typename std::remove_const<M>::type>::type>::value;
}

/**
 * \brief returns HasHeader<M>::value;
 */
template<typename M>
inline bool hasHeader()
{
  return HasHeader<typename std::remove_reference<typename std::remove_const<M>::type>::type>::value;
}

} // namespace message_traits
} // namespace rs2rosinternal

#endif // ROSLIB_MESSAGE_TRAITS_H
