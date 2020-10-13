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

#ifndef ROSCPP_SERIALIZATION_H
#define ROSCPP_SERIALIZATION_H

#include "roscpp_serialization_macros.h"

#include <ros/types.h>
#include <ros/time.h>

#include "serialized_message.h"
#include "ros/message_traits.h"
#include "ros/builtin_message_traits.h"
#include "ros/exception.h"
#include "ros/datatypes.h"

#include <vector>
#include <map>

#include <boost/call_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/not.hpp>

#include <cstring>

#define ROS_NEW_SERIALIZATION_API 1

/**
 * \brief Declare your serializer to use an allInOne member instead of requiring 3 different serialization
 * functions.
 *
 * The allinone method has the form:
\verbatim
template<typename Stream, typename T>
inline static void allInOne(Stream& stream, T t)
{
  stream.next(t.a);
  stream.next(t.b);
  ...
}
\endverbatim
 *
 * The only guarantee given is that Stream::next(T) is defined.
 */
#define ROS_DECLARE_ALLINONE_SERIALIZER \
  template<typename Stream, typename T> \
  inline static void write(Stream& stream, const T& t) \
  { \
    allInOne<Stream, const T&>(stream, t); \
  } \
  \
  template<typename Stream, typename T> \
  inline static void read(Stream& stream, T& t) \
  { \
    allInOne<Stream, T&>(stream, t); \
  } \
  \
  template<typename T> \
  inline static uint32_t serializedLength(const T& t) \
  { \
    LStream stream; \
    allInOne<LStream, const T&>(stream, t); \
    return stream.getLength(); \
  }

namespace rs2rosinternal
{
namespace serialization
{
namespace mt = message_traits;
namespace mpl = boost::mpl;

class ROSCPP_SERIALIZATION_DECL StreamOverrunException : public rs2rosinternal::Exception
{
public:
  StreamOverrunException(const std::string& what)
  : Exception(what)
  {}
};

ROSCPP_SERIALIZATION_DECL void throwStreamOverrun();

/**
 * \brief Templated serialization class.  Default implementation provides backwards compatibility with
 * old message types.
 *
 * Specializing the Serializer class is the only thing you need to do to get the ROS serialization system
 * to work with a type.
 */
template<typename T>
struct Serializer
{
  /**
   * \brief Write an object to the stream.  Normally the stream passed in here will be a rs2rosinternal::serialization::OStream
   */
  template<typename Stream>
  inline static void write(Stream& stream, typename boost::call_traits<T>::param_type t)
  {
    t.serialize(stream.getData(), 0);
  }

  /**
   * \brief Read an object from the stream.  Normally the stream passed in here will be a rs2rosinternal::serialization::IStream
   */
  template<typename Stream>
  inline static void read(Stream& stream, typename boost::call_traits<T>::reference t)
  {
    t.deserialize(stream.getData());
  }

  /**
   * \brief Determine the serialized length of an object.
   */
  inline static uint32_t serializedLength(typename boost::call_traits<T>::param_type t)
  {
    return t.serializationLength();
  }
};

/**
 * \brief Serialize an object.  Stream here should normally be a rs2rosinternal::serialization::OStream
 */
template<typename T, typename Stream>
inline void serialize(Stream& stream, const T& t)
{
  Serializer<T>::write(stream, t);
}

/**
 * \brief Deserialize an object.  Stream here should normally be a rs2rosinternal::serialization::IStream
 */
template<typename T, typename Stream>
inline void deserialize(Stream& stream, T& t)
{
  Serializer<T>::read(stream, t);
}

/**
 * \brief Determine the serialized length of an object
 */
template<typename T>
inline uint32_t serializationLength(const T& t)
{
  return Serializer<T>::serializedLength(t);
}

#define ROS_CREATE_SIMPLE_SERIALIZER(Type) \
  template<> struct Serializer<Type> \
  { \
    template<typename Stream> inline static void write(Stream& stream, const Type v) \
    { \
      *reinterpret_cast<Type*>(stream.advance(sizeof(v))) = v; \
    } \
    \
    template<typename Stream> inline static void read(Stream& stream, Type& v) \
    { \
      v = *reinterpret_cast<Type*>(stream.advance(sizeof(v))); \
    } \
    \
    inline static uint32_t serializedLength(const Type&) \
    { \
      return sizeof(Type); \
    } \
  };

#define ROS_CREATE_SIMPLE_SERIALIZER_ARM(Type) \
  template<> struct Serializer<Type> \
  { \
    template<typename Stream> inline static void write(Stream& stream, const Type v) \
    { \
      memcpy(stream.advance(sizeof(v)), &v, sizeof(v) ); \
    } \
    \
    template<typename Stream> inline static void read(Stream& stream, Type& v) \
    { \
      memcpy(&v, stream.advance(sizeof(v)), sizeof(v) ); \
    } \
    \
    inline static uint32_t serializedLength(const Type&) \
      { \
      return sizeof(Type); \
    } \
};

#if defined(__arm__) || defined(__arm)
    ROS_CREATE_SIMPLE_SERIALIZER_ARM(uint8_t)
    ROS_CREATE_SIMPLE_SERIALIZER_ARM(int8_t)
    ROS_CREATE_SIMPLE_SERIALIZER_ARM(uint16_t)
    ROS_CREATE_SIMPLE_SERIALIZER_ARM(int16_t)
    ROS_CREATE_SIMPLE_SERIALIZER_ARM(uint32_t)
    ROS_CREATE_SIMPLE_SERIALIZER_ARM(int32_t)
    ROS_CREATE_SIMPLE_SERIALIZER_ARM(uint64_t)
    ROS_CREATE_SIMPLE_SERIALIZER_ARM(int64_t)
    ROS_CREATE_SIMPLE_SERIALIZER_ARM(float)
    ROS_CREATE_SIMPLE_SERIALIZER_ARM(double)
#else
    ROS_CREATE_SIMPLE_SERIALIZER(uint8_t)
    ROS_CREATE_SIMPLE_SERIALIZER(int8_t)
    ROS_CREATE_SIMPLE_SERIALIZER(uint16_t)
    ROS_CREATE_SIMPLE_SERIALIZER(int16_t)
    ROS_CREATE_SIMPLE_SERIALIZER(uint32_t)
    ROS_CREATE_SIMPLE_SERIALIZER(int32_t)
    ROS_CREATE_SIMPLE_SERIALIZER(uint64_t)
    ROS_CREATE_SIMPLE_SERIALIZER(int64_t)
    ROS_CREATE_SIMPLE_SERIALIZER(float)
    ROS_CREATE_SIMPLE_SERIALIZER(double)
#endif

/**
 * \brief Serializer specialized for bool (serialized as uint8)
 */
template<> struct Serializer<bool>
{
  template<typename Stream> inline static void write(Stream& stream, const bool v)
  {
    uint8_t b = (uint8_t)v;
#if defined(__arm__) || defined(__arm)
    memcpy(stream.advance(1), &b, 1 );
#else
    *reinterpret_cast<uint8_t*>(stream.advance(1)) = b;
#endif
  }

  template<typename Stream> inline static void read(Stream& stream, bool& v)
  {
    uint8_t b;
#if defined(__arm__) || defined(__arm)
    memcpy(&b, stream.advance(1), 1 );
#else
    b = *reinterpret_cast<uint8_t*>(stream.advance(1));
#endif
    v = (bool)b;
  }

  inline static uint32_t serializedLength(bool)
  {
    return 1;
  }
};

/**
 * \brief  Serializer specialized for std::string
 */
template<class ContainerAllocator>
struct Serializer<std::basic_string<char, std::char_traits<char>, ContainerAllocator> >
{
  typedef std::basic_string<char, std::char_traits<char>, ContainerAllocator> StringType;

  template<typename Stream>
  inline static void write(Stream& stream, const StringType& str)
  {
    size_t len = str.size();
    stream.next((uint32_t)len);

    if (len > 0)
    {
      memcpy(stream.advance((uint32_t)len), str.data(), len);
    }
  }

  template<typename Stream>
  inline static void read(Stream& stream, StringType& str)
  {
    uint32_t len;
    stream.next(len);
    if (len > 0)
    {
      str = StringType((char*)stream.advance(len), len);
    }
    else
    {
      str.clear();
    }
  }

  inline static uint32_t serializedLength(const StringType& str)
  {
    return 4 + (uint32_t)str.size();
  }
};

/**
 * \brief Serializer specialized for rs2rosinternal::Time
 */
template<>
struct Serializer<rs2rosinternal::Time>
{
  template<typename Stream>
  inline static void write(Stream& stream, const rs2rosinternal::Time& v)
  {
    stream.next(v.sec);
    stream.next(v.nsec);
  }

  template<typename Stream>
  inline static void read(Stream& stream, rs2rosinternal::Time& v)
  {
    stream.next(v.sec);
    stream.next(v.nsec);
  }

  inline static uint32_t serializedLength(const rs2rosinternal::Time&)
  {
    return 8;
  }
};

/**
 * \brief Serializer specialized for rs2rosinternal::Duration
 */
template<>
struct Serializer<rs2rosinternal::Duration>
{
  template<typename Stream>
  inline static void write(Stream& stream, const rs2rosinternal::Duration& v)
  {
    stream.next(v.sec);
    stream.next(v.nsec);
  }

  template<typename Stream>
  inline static void read(Stream& stream, rs2rosinternal::Duration& v)
  {
    stream.next(v.sec);
    stream.next(v.nsec);
  }

  inline static uint32_t serializedLength(const rs2rosinternal::Duration&)
  {
    return 8;
  }
};

/**
 * \brief Vector serializer.  Default implementation does nothing
 */
template<typename T, class ContainerAllocator, class Enabled = void>
struct VectorSerializer
{};

/**
 * \brief Vector serializer, specialized for non-fixed-size, non-simple types
 */
template<typename T, class ContainerAllocator>
struct VectorSerializer<T, ContainerAllocator, typename boost::disable_if<mt::IsFixedSize<T> >::type >
{
  typedef std::vector<T, typename ContainerAllocator::template rebind<T>::other> VecType;
  typedef typename VecType::iterator IteratorType;
  typedef typename VecType::const_iterator ConstIteratorType;

  template<typename Stream>
  inline static void write(Stream& stream, const VecType& v)
  {
    stream.next((uint32_t)v.size());
    ConstIteratorType it = v.begin();
    ConstIteratorType end = v.end();
    for (; it != end; ++it)
    {
      stream.next(*it);
    }
  }

  template<typename Stream>
  inline static void read(Stream& stream, VecType& v)
  {
    uint32_t len;
    stream.next(len);
    v.resize(len);
    IteratorType it = v.begin();
    IteratorType end = v.end();
    for (; it != end; ++it)
    {
      stream.next(*it);
    }
  }

  inline static uint32_t serializedLength(const VecType& v)
  {
    uint32_t size = 4;
    ConstIteratorType it = v.begin();
    ConstIteratorType end = v.end();
    for (; it != end; ++it)
    {
      size += serializationLength(*it);
    }

    return size;
  }
};

/**
 * \brief Vector serializer, specialized for fixed-size simple types
 */
template<typename T, class ContainerAllocator>
struct VectorSerializer<T, ContainerAllocator, typename boost::enable_if<mt::IsSimple<T> >::type >
{
  typedef std::vector<T, typename ContainerAllocator::template rebind<T>::other> VecType;
  typedef typename VecType::iterator IteratorType;
  typedef typename VecType::const_iterator ConstIteratorType;

  template<typename Stream>
  inline static void write(Stream& stream, const VecType& v)
  {
    uint32_t len = (uint32_t)v.size();
    stream.next(len);
    if (!v.empty())
    {
      const uint32_t data_len = len * (uint32_t)sizeof(T);
      memcpy(stream.advance(data_len), &v.front(), data_len);
    }
  }

  template<typename Stream>
  inline static void read(Stream& stream, VecType& v)
  {
    uint32_t len;
    stream.next(len);
    v.resize(len);

    if (len > 0)
    {
      const uint32_t data_len = (uint32_t)sizeof(T) * len;
      memcpy(&v.front(), stream.advance(data_len), data_len);
    }
  }

  inline static uint32_t serializedLength(const VecType& v)
  {
    return 4 + (uint32_t)v.size() * (uint32_t)sizeof(T);
  }
};

/**
 * \brief Vector serializer, specialized for fixed-size non-simple types
 */
template<typename T, class ContainerAllocator>
struct VectorSerializer<T, ContainerAllocator, typename boost::enable_if<mpl::and_<mt::IsFixedSize<T>, mpl::not_<mt::IsSimple<T> > > >::type >
{
  typedef std::vector<T, typename ContainerAllocator::template rebind<T>::other> VecType;
  typedef typename VecType::iterator IteratorType;
  typedef typename VecType::const_iterator ConstIteratorType;

  template<typename Stream>
  inline static void write(Stream& stream, const VecType& v)
  {
    stream.next((uint32_t)v.size());
    ConstIteratorType it = v.begin();
    ConstIteratorType end = v.end();
    for (; it != end; ++it)
    {
      stream.next(*it);
    }
  }

  template<typename Stream>
  inline static void read(Stream& stream, VecType& v)
  {
    uint32_t len;
    stream.next(len);
    v.resize(len);
    IteratorType it = v.begin();
    IteratorType end = v.end();
    for (; it != end; ++it)
    {
      stream.next(*it);
    }
  }

  inline static uint32_t serializedLength(const VecType& v)
  {
    uint32_t size = 4;
    if (!v.empty())
    {
      uint32_t len_each = serializationLength(v.front());
      size += len_each * (uint32_t)v.size();
    }

    return size;
  }
};

/**
 * \brief serialize version for std::vector
 */
template<typename T, class ContainerAllocator, typename Stream>
inline void serialize(Stream& stream, const std::vector<T, ContainerAllocator>& t)
{
  VectorSerializer<T, ContainerAllocator>::write(stream, t);
}

/**
 * \brief deserialize version for std::vector
 */
template<typename T, class ContainerAllocator, typename Stream>
inline void deserialize(Stream& stream, std::vector<T, ContainerAllocator>& t)
{
  VectorSerializer<T, ContainerAllocator>::read(stream, t);
}

/**
 * \brief serializationLength version for std::vector
 */
template<typename T, class ContainerAllocator>
inline uint32_t serializationLength(const std::vector<T, ContainerAllocator>& t)
{
  return VectorSerializer<T, ContainerAllocator>::serializedLength(t);
}

/**
 * \brief Array serializer, default implementation does nothing
 */
template<typename T, size_t N, class Enabled = void>
struct ArraySerializer
{};

/**
 * \brief Array serializer, specialized for non-fixed-size, non-simple types
 */
template<typename T, size_t N>
struct ArraySerializer<T, N, typename boost::disable_if<mt::IsFixedSize<T> >::type>
{
  typedef std::array<T, N > ArrayType;
  typedef typename ArrayType::iterator IteratorType;
  typedef typename ArrayType::const_iterator ConstIteratorType;

  template<typename Stream>
  inline static void write(Stream& stream, const ArrayType& v)
  {
    ConstIteratorType it = v.begin();
    ConstIteratorType end = v.end();
    for (; it != end; ++it)
    {
      stream.next(*it);
    }
  }

  template<typename Stream>
  inline static void read(Stream& stream, ArrayType& v)
  {
    IteratorType it = v.begin();
    IteratorType end = v.end();
    for (; it != end; ++it)
    {
      stream.next(*it);
    }
  }

  inline static uint32_t serializedLength(const ArrayType& v)
  {
    uint32_t size = 0;
    ConstIteratorType it = v.begin();
    ConstIteratorType end = v.end();
    for (; it != end; ++it)
    {
      size += serializationLength(*it);
    }

    return size;
  }
};

/**
 * \brief Array serializer, specialized for fixed-size, simple types
 */
template<typename T, size_t N>
struct ArraySerializer<T, N, typename boost::enable_if<mt::IsSimple<T> >::type>
{
  typedef std::array<T, N > ArrayType;
  typedef typename ArrayType::iterator IteratorType;
  typedef typename ArrayType::const_iterator ConstIteratorType;

  template<typename Stream>
  inline static void write(Stream& stream, const ArrayType& v)
  {
    const uint32_t data_len = N * sizeof(T);
    memcpy(stream.advance(data_len), &v.front(), data_len);
  }

  template<typename Stream>
  inline static void read(Stream& stream, ArrayType& v)
  {
    const uint32_t data_len = N * sizeof(T);
    memcpy(&v.front(), stream.advance(data_len), data_len);
  }

  inline static uint32_t serializedLength(const ArrayType&)
  {
    return N * sizeof(T);
  }
};

/**
 * \brief Array serializer, specialized for fixed-size, non-simple types
 */
template<typename T, size_t N>
struct ArraySerializer<T, N, typename boost::enable_if<mpl::and_<mt::IsFixedSize<T>, mpl::not_<mt::IsSimple<T> > > >::type>
{
  typedef std::array<T, N > ArrayType;
  typedef typename ArrayType::iterator IteratorType;
  typedef typename ArrayType::const_iterator ConstIteratorType;

  template<typename Stream>
  inline static void write(Stream& stream, const ArrayType& v)
  {
    ConstIteratorType it = v.begin();
    ConstIteratorType end = v.end();
    for (; it != end; ++it)
    {
      stream.next(*it);
    }
  }

  template<typename Stream>
  inline static void read(Stream& stream, ArrayType& v)
  {
    IteratorType it = v.begin();
    IteratorType end = v.end();
    for (; it != end; ++it)
    {
      stream.next(*it);
    }
  }

  inline static uint32_t serializedLength(const ArrayType& v)
  {
    return serializationLength(v.front()) * N;
  }
};

/**
 * \brief serialize version for  std::array
 */
template<typename T, size_t N, typename Stream>
inline void serialize(Stream& stream, const std::array<T, N>& t)
{
  ArraySerializer<T, N>::write(stream, t);
}

/**
 * \brief deserialize version for  std::array
 */
template<typename T, size_t N, typename Stream>
inline void deserialize(Stream& stream, std::array<T, N>& t)
{
  ArraySerializer<T, N>::read(stream, t);
}

/**
 * \brief serializationLength version for  std::array
 */
template<typename T, size_t N>
inline uint32_t serializationLength(const std::array<T, N>& t)
{
  return ArraySerializer<T, N>::serializedLength(t);
}

/**
 * \brief Enum
 */
namespace stream_types
{
enum StreamType
{
  Input,
  Output,
  Length
};
}
typedef stream_types::StreamType StreamType;

/**
 * \brief Stream base-class, provides common functionality for IStream and OStream
 */
struct ROSCPP_SERIALIZATION_DECL Stream
{
  /*
   * \brief Returns a pointer to the current position of the stream
   */
  inline uint8_t* getData() { return data_; }
  /**
   * \brief Advances the stream, checking bounds, and returns a pointer to the position before it
   * was advanced.
   * \throws StreamOverrunException if len would take this stream past the end of its buffer
   */
  ROS_FORCE_INLINE uint8_t* advance(uint32_t len)
  {
    uint8_t* old_data = data_;
    data_ += len;
    if (data_ > end_)
    {
      // Throwing directly here causes a significant speed hit due to the extra code generated
      // for the throw statement
      throwStreamOverrun();
    }
    return old_data;
  }

  /**
   * \brief Returns the amount of space left in the stream
   */
  inline uint32_t getLength() { return (uint32_t)(end_ - data_); }

protected:
  Stream(uint8_t* _data, uint32_t _count)
  : data_(_data)
  , end_(_data + _count)
  {}

private:
  uint8_t* data_;
  uint8_t* end_;
};

/**
 * \brief Input stream
 */
struct ROSCPP_SERIALIZATION_DECL IStream : public Stream
{
  static const StreamType stream_type = stream_types::Input;

  IStream(uint8_t* data, uint32_t count)
  : Stream(data, count)
  {}

  /**
   * \brief Deserialize an item from this input stream
   */
  template<typename T>
  ROS_FORCE_INLINE void next(T& t)
  {
    deserialize(*this, t);
  }

  template<typename T>
  ROS_FORCE_INLINE IStream& operator>>(T& t)
  {
    deserialize(*this, t);
    return *this;
  }
};

/**
 * \brief Output stream
 */
struct ROSCPP_SERIALIZATION_DECL OStream : public Stream
{
  static const StreamType stream_type = stream_types::Output;

  OStream(uint8_t* data, uint32_t count)
  : Stream(data, count)
  {}

  /**
   * \brief Serialize an item to this output stream
   */
  template<typename T>
  ROS_FORCE_INLINE void next(const T& t)
  {
    serialize(*this, t);
  }

  template<typename T>
  ROS_FORCE_INLINE OStream& operator<<(const T& t)
  {
    serialize(*this, t);
    return *this;
  }
};


/**
 * \brief Length stream
 *
 * LStream is not what you would normally think of as a stream, but it is used in order to support
 * allinone serializers.
 */
struct ROSCPP_SERIALIZATION_DECL LStream
{
  static const StreamType stream_type = stream_types::Length;

  LStream()
  : count_(0)
  {}

  /**
   * \brief Add the length of an item to this length stream
   */
  template<typename T>
  ROS_FORCE_INLINE void next(const T& t)
  {
    count_ += serializationLength(t);
  }

  /**
   * \brief increment the length by len
   */
  ROS_FORCE_INLINE uint32_t advance(uint32_t len)
  {
    uint32_t old = count_;
    count_ += len;
    return old;
  }

  /**
   * \brief Get the total length of this tream
   */
  inline uint32_t getLength() { return count_; }

private:
  uint32_t count_;
};

/**
 * \brief Serialize a message
 */
template<typename M>
inline SerializedMessage serializeMessage(const M& message)
{
  SerializedMessage m;
  uint32_t len = serializationLength(message);
  m.num_bytes = len + 4;
  m.buf.resize(m.num_bytes);

  OStream s(m.buf.data(), (uint32_t)m.num_bytes);
  serialize(s, (uint32_t)m.num_bytes - 4);
  m.message_start = s.getData();
  serialize(s, message);

  return m;
}

/**
 * \brief Serialize a service response
 */
template<typename M>
inline SerializedMessage serializeServiceResponse(bool ok, const M& message)
{
  SerializedMessage m;

  if (ok)
  {
    uint32_t len = serializationLength(message);
    m.num_bytes = len + 5;
    m.buf.resize(m.num_bytes);

    OStream s(m.buf.data(), (uint32_t)m.num_bytes);
    serialize(s, (uint8_t)ok);
    serialize(s, (uint32_t)m.num_bytes - 5);
    serialize(s, message);
  }
  else
  {
    uint32_t len = serializationLength(message);
    m.num_bytes = len + 1;
    m.buf.resize(m.num_bytes);

    OStream s(m.buf.data(), (uint32_t)m.num_bytes);
    serialize(s, (uint8_t)ok);
    serialize(s, message);
  }

  return m;
}

/**
 * \brief Deserialize a message.  If includes_length is true, skips the first 4 bytes
 */
template<typename M>
inline void deserializeMessage(const SerializedMessage& m, M& message)
{
  IStream s(m.message_start, m.num_bytes - (m.message_start - m.buf.data()));
  deserialize(s, message);
}


// Additional serialization traits

template<typename M>
struct PreDeserializeParams
{
  std::shared_ptr<M> message;
  std::shared_ptr<std::map<std::string, std::string> > connection_header;
};

/**
 * \brief called by the SubscriptionCallbackHelper after a message is
 * instantiated but before that message is deserialized
 */
template<typename M>
struct PreDeserialize
{
  static void notify(const PreDeserializeParams<M>&) { }
};

} // namespace serialization

} // namespace rs2rosinternal

#endif // ROSCPP_SERIALIZATION_H
