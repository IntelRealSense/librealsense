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
 
#ifndef STD_MSGS_INCLUDING_HEADER_DEPRECATED_DEF
#error("Do not include this file directly.  Instead, include std_msgs/Header.h")
#endif

namespace roslib
{
template <class ContainerAllocator>
struct Header_ : public std_msgs::Header_<ContainerAllocator>
{
  typedef Header_<ContainerAllocator> Type;

  ROS_DEPRECATED Header_()
  {
  }

  ROS_DEPRECATED Header_(const ContainerAllocator& _alloc)
  : std_msgs::Header_<ContainerAllocator>(_alloc)
  {
  }

  ROS_DEPRECATED Header_(const std_msgs::Header_<ContainerAllocator>& rhs)
  {
    *this = rhs;
  }

  ROS_DEPRECATED Type& operator=(const std_msgs::Header_<ContainerAllocator>& rhs)
  {
    if (this == &rhs)
      return *this;
    this->seq = rhs.seq;
    this->stamp = rhs.stamp;
    this->frame_id = rhs.frame_id;
    return *this;
  }

  ROS_DEPRECATED operator std_msgs::Header_<ContainerAllocator>()
  {
    std_msgs::Header_<ContainerAllocator> h;
    h.seq = this->seq;
    h.stamp = this->stamp;
    h.frame_id = this->frame_id;
    return h;
  }

private:
  static const char* __s_getDataType_() { return "roslib/Header"; }
public:
   static const std::string __s_getDataType() { return __s_getDataType_(); }

   const std::string __getDataType() const { return __s_getDataType_(); }

private:
  static const char* __s_getMD5Sum_() { return "2176decaecbce78abc3b96ef049fabed"; }
public:
   static const std::string __s_getMD5Sum() { return __s_getMD5Sum_(); }

   const std::string __getMD5Sum() const { return __s_getMD5Sum_(); }

private:
  static const char* __s_getMessageDefinition_() { return "# Standard metadata for higher-level stamped data types.\n\
# This is generally used to communicate timestamped data \n\
# in a particular coordinate frame.\n\
# \n\
# sequence ID: consecutively increasing ID \n\
uint32 seq\n\
#Two-integer timestamp that is expressed as:\n\
# * stamp.secs: seconds (stamp_secs) since epoch\n\
# * stamp.nsecs: nanoseconds since stamp_secs\n\
# time-handling sugar is provided by the client library\n\
time stamp\n\
#Frame this data is associated with\n\
# 0: no frame\n\
# 1: global frame\n\
string frame_id\n\
\n\
"; }
public:
   static const std::string __s_getMessageDefinition() { return __s_getMessageDefinition_(); }

   const std::string __getMessageDefinition() const { return __s_getMessageDefinition_(); }

   virtual uint8_t *serialize(uint8_t *write_ptr, uint32_t seq) const
  {
    ros::serialization::OStream stream(write_ptr, 1000000000);
    ros::serialization::serialize(stream, this->seq);
    ros::serialization::serialize(stream, this->stamp);
    ros::serialization::serialize(stream, this->frame_id);
    return stream.getData();
  }

   virtual uint8_t *deserialize(uint8_t *read_ptr)
  {
    ros::serialization::IStream stream(read_ptr, 1000000000);
    ros::serialization::deserialize(stream, this->seq);
    ros::serialization::deserialize(stream, this->stamp);
    ros::serialization::deserialize(stream, this->frame_id);
    return stream.getData();
  }

   virtual uint32_t serializationLength() const
  {
    uint32_t size = 0;
    size += ros::serialization::serializationLength(this->seq);
    size += ros::serialization::serializationLength(this->stamp);
    size += ros::serialization::serializationLength(this->frame_id);
    return size;
  }

  typedef boost::shared_ptr< ::roslib::Header_<ContainerAllocator> > Ptr;
  typedef boost::shared_ptr< ::roslib::Header_<ContainerAllocator>  const> ConstPtr;
}; // struct Header
typedef  ::roslib::Header_<std::allocator<void> > Header;

typedef boost::shared_ptr< ::roslib::Header> HeaderPtr;
typedef boost::shared_ptr< ::roslib::Header const> HeaderConstPtr;


template<typename ContainerAllocator>
std::ostream& operator<<(std::ostream& s, const  ::roslib::Header_<ContainerAllocator> & v)
{
  ros::message_operations::Printer< ::roslib::Header_<ContainerAllocator> >::stream(s, "", v);
  return s;}

} // namespace roslib

namespace ros
{
namespace message_traits
{
template<class ContainerAllocator>
struct MD5Sum< ::roslib::Header_<ContainerAllocator> > {
  static const char* value() 
  {
    return "2176decaecbce78abc3b96ef049fabed";
  }

  static const char* value(const  ::roslib::Header_<ContainerAllocator> &) { return value(); } 
  static const uint64_t static_value1 = 0x2176decaecbce78aULL;
  static const uint64_t static_value2 = 0xbc3b96ef049fabedULL;
};

template<class ContainerAllocator>
struct DataType< ::roslib::Header_<ContainerAllocator> > {
  static const char* value() 
  {
    return "roslib/Header";
  }

  static const char* value(const  ::roslib::Header_<ContainerAllocator> &) { return value(); } 
};

template<class ContainerAllocator>
struct Definition< ::roslib::Header_<ContainerAllocator> > {
  static const char* value() 
  {
    return "# Standard metadata for higher-level stamped data types.\n\
# This is generally used to communicate timestamped data \n\
# in a particular coordinate frame.\n\
# \n\
# sequence ID: consecutively increasing ID \n\
uint32 seq\n\
#Two-integer timestamp that is expressed as:\n\
# * stamp.secs: seconds (stamp_secs) since epoch\n\
# * stamp.nsecs: nanoseconds since stamp_secs\n\
# time-handling sugar is provided by the client library\n\
time stamp\n\
#Frame this data is associated with\n\
# 0: no frame\n\
# 1: global frame\n\
string frame_id\n\
\n\
";
  }

  static const char* value(const  ::roslib::Header_<ContainerAllocator> &) { return value(); } 
};

} // namespace message_traits
} // namespace ros

namespace ros
{
namespace serialization
{

template<class ContainerAllocator> struct Serializer< ::roslib::Header_<ContainerAllocator> >
{
  template<typename Stream, typename T> inline static void allInOne(Stream& stream, T m)
  {
    stream.next(m.seq);
    stream.next(m.stamp);
    stream.next(m.frame_id);
  }

  ROS_DECLARE_ALLINONE_SERIALIZER
}; // struct Header_
} // namespace serialization
} // namespace ros

namespace ros
{
namespace message_operations
{

template<class ContainerAllocator>
struct Printer< ::roslib::Header_<ContainerAllocator> >
{
  template<typename Stream> static void stream(Stream& s, const std::string& indent, const  ::roslib::Header_<ContainerAllocator> & v) 
  {
    s << indent << "seq: ";
    Printer<uint32_t>::stream(s, indent + "  ", v.seq);
    s << indent << "stamp: ";
    Printer<ros::Time>::stream(s, indent + "  ", v.stamp);
    s << indent << "frame_id: ";
    Printer<std::basic_string<char, std::char_traits<char>, typename ContainerAllocator::template rebind<char>::other > >::stream(s, indent + "  ", v.frame_id);
  }
};


} // namespace message_operations
} // namespace ros

