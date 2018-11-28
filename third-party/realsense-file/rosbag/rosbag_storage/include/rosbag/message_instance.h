/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of Willow Garage, Inc. nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

#ifndef ROSBAG_MESSAGE_INSTANCE_H
#define ROSBAG_MESSAGE_INSTANCE_H

#include <ros/message_traits.h>
#include <ros/serialization.h>
//#include <ros/ros.h>
#include <ros/time.h>
#include <memory.h>
#include "rosbag/structures.h"
#include "rosbag/macros.h"

namespace rosbag {

class Bag;

//! A class pointing into a bag file
/*!
 *  The MessageInstance class itself is fairly light weight.  It
 *  simply contains a pointer to a bag-file and the index_entry
 *  necessary to get access to the corresponding data.
 *
 *  It adheres to the necessary rs2rosinternal::message_traits to be directly
 *  serializable.
 */
class ROSBAG_DECL MessageInstance
{
    friend class View;
  
public:
    rs2rosinternal::Time   const& getTime()              const;
    std::string const& getTopic()             const;
    std::string const& getDataType()          const;
    std::string const& getMD5Sum()            const;
    std::string const& getMessageDefinition() const;

    std::shared_ptr<rs2rosinternal::M_string> getConnectionHeader() const;

    std::string getCallerId() const;
    bool        isLatching()  const;

    //! Test whether the underlying message of the specified type.
    /*!
     * returns true iff the message is of the template type
     */
    template<class T>
    bool isType() const;

    //! Templated call to instantiate a message
    /*!
     * returns NULL pointer if incompatible
     */
    template<class T>
    std::shared_ptr<T> instantiate() const;
  
    //! Write serialized message contents out to a stream
    template<typename Stream>
    void write(Stream& stream) const;

    //! Size of serialized message
    uint32_t size() const;

private:
    MessageInstance(ConnectionInfo const* connection_info, IndexEntry const& index, Bag const& bag);

    ConnectionInfo const* connection_info_;
    IndexEntry const      index_entry_;
    Bag const*            bag_;
};


} // namespace rosbag

namespace rs2rosinternal {
namespace message_traits {

template<>
struct MD5Sum<rosbag::MessageInstance>
{
    static const char* value(const rosbag::MessageInstance& m) { return m.getMD5Sum().c_str(); }
};

template<>
struct DataType<rosbag::MessageInstance>
{
    static const char* value(const rosbag::MessageInstance& m) { return m.getDataType().c_str(); }
};

template<>
struct Definition<rosbag::MessageInstance>
{
    static const char* value(const rosbag::MessageInstance& m) { return m.getMessageDefinition().c_str(); }
};

} // namespace message_traits

namespace serialization
{

template<>
struct Serializer<rosbag::MessageInstance>
{
    template<typename Stream>
    inline static void write(Stream& stream, const rosbag::MessageInstance& m) {
        m.write(stream);
    }

    inline static uint32_t serializedLength(const rosbag::MessageInstance& m) {
        return m.size();
    }
};

} // namespace serialization

} // namespace rs2rosinternal

#include "rosbag/bag.h"

namespace rosbag {

template<class T>
bool MessageInstance::isType() const {
    char const* md5sum = rs2rosinternal::message_traits::MD5Sum<T>::value();
    return md5sum == std::string("*") || md5sum == getMD5Sum();
}

template<class T>
std::shared_ptr<T> MessageInstance::instantiate() const {
    if (!isType<T>())
        return std::shared_ptr<T>();

    return bag_->instantiateBuffer<T>(index_entry_);
}

template<typename Stream>
void MessageInstance::write(Stream& stream) const {
    bag_->readMessageDataIntoStream(index_entry_, stream);
}

} // namespace rosbag

#endif
