/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2013, Open Source Robotics Foundation
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
 *   * Neither the name of Open Source Robotics Foundation nor the names of its
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
 */

#ifndef SENSOR_MSGS_IMPL_POINT_CLOUD_ITERATOR_H
#define SENSOR_MSGS_IMPL_POINT_CLOUD_ITERATOR_H

#include <sensor_msgs/PointCloud2.h>
#include <cstdarg>
#include <string>
#include <vector>
#include <sstream>

/**
 * \brief Private implementation used by PointCloud2Iterator
 * \author Vincent Rabaud
 */

namespace
{
/** Return the size of a datatype (which is an enum of sensor_msgs::PointField::) in bytes
 * @param datatype one of the enums of sensor_msgs::PointField::
 */
inline int sizeOfPointField(int datatype)
{
  if ((datatype == sensor_msgs::PointField::INT8) || (datatype == sensor_msgs::PointField::UINT8))
    return 1;
  else if ((datatype == sensor_msgs::PointField::INT16) || (datatype == sensor_msgs::PointField::UINT16))
    return 2;
  else if ((datatype == sensor_msgs::PointField::INT32) || (datatype == sensor_msgs::PointField::UINT32) ||
      (datatype == sensor_msgs::PointField::FLOAT32))
    return 4;
  else if (datatype == sensor_msgs::PointField::FLOAT64)
    return 8;
  else
  {
    std::stringstream err;
    err << "PointField of type " << datatype << " does not exist";
    throw std::runtime_error(err.str());
  }
  return -1;
}

/** Private function that adds a PointField to the "fields" member of a PointCloud2
 * @param cloud_msg the PointCloud2 to add a field to
 * @param name the name of the field
 * @param count the number of elements in the PointField
 * @param datatype the datatype of the elements
 * @param offset the offset of that element
 * @return the offset of the next PointField that will be added to the PointCLoud2
 */
inline int addPointField(sensor_msgs::PointCloud2 &cloud_msg, const std::string &name, int count, int datatype,
    int offset)
{
  sensor_msgs::PointField point_field;
  point_field.name = name;
  point_field.count = count;
  point_field.datatype = datatype;
  point_field.offset = offset;
  cloud_msg.fields.push_back(point_field);

  // Update the offset
  return offset + point_field.count * sizeOfPointField(datatype);
}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace sensor_msgs
{

inline PointCloud2Modifier::PointCloud2Modifier(PointCloud2& cloud_msg) : cloud_msg_(cloud_msg)
{
}

inline size_t PointCloud2Modifier::size() const
{
  return cloud_msg_.data.size() / cloud_msg_.point_step;
}

inline void PointCloud2Modifier::reserve(size_t size)
{
  cloud_msg_.data.reserve(size * cloud_msg_.point_step);
}

inline void PointCloud2Modifier::resize(size_t size)
{
  cloud_msg_.data.resize(size * cloud_msg_.point_step);

  // Update height/width
  if (cloud_msg_.height == 1) {
    cloud_msg_.width = size;
    cloud_msg_.row_step = size * cloud_msg_.point_step;
  } else
    if (cloud_msg_.width == 1)
      cloud_msg_.height = size;
    else {
      cloud_msg_.height = 1;
      cloud_msg_.width = size;
      cloud_msg_.row_step = size * cloud_msg_.point_step;
    }
}

inline void PointCloud2Modifier::clear()
{
  cloud_msg_.data.clear();

  // Update height/width
  if (cloud_msg_.height == 1)
    cloud_msg_.row_step = cloud_msg_.width = 0;
  else
    if (cloud_msg_.width == 1)
      cloud_msg_.height = 0;
    else
      cloud_msg_.row_step = cloud_msg_.width = cloud_msg_.height = 0;
}


/**
 * @brief Function setting some fields in a PointCloud and adjusting the
 *        internals of the PointCloud2
 * @param n_fields the number of fields to add. The fields are given as
 *        triplets: name of the field as char*, number of elements in the
 *        field, the datatype of the elements in the field
 *
 * E.g, you create your PointCloud2 message with XYZ/RGB as follows:
 * <PRE>
 *   setPointCloud2FieldsByString(cloud_msg, 4, "x", 1, sensor_msgs::PointField::FLOAT32,
 *                                              "y", 1, sensor_msgs::PointField::FLOAT32,
 *                                              "z", 1, sensor_msgs::PointField::FLOAT32,
 *                                              "rgb", 1, sensor_msgs::PointField::FLOAT32);
 * </PRE>
 * WARNING: THIS DOES NOT TAKE INTO ACCOUNT ANY PADDING AS DONE UNTIL HYDRO
 * For simple usual cases, the overloaded setPointCloud2FieldsByString is what you want.
 */
inline void PointCloud2Modifier::setPointCloud2Fields(int n_fields, ...)
{
  cloud_msg_.fields.clear();
  cloud_msg_.fields.reserve(n_fields);
  va_list vl;
  va_start(vl, n_fields);
  int offset = 0;
  for (int i = 0; i < n_fields; ++i) {
    // Create the corresponding PointField
    std::string name(va_arg(vl, char*));
    int count(va_arg(vl, int));
    int datatype(va_arg(vl, int));
    offset = addPointField(cloud_msg_, name, count, datatype, offset);
  }
  va_end(vl);

  // Resize the point cloud accordingly
  cloud_msg_.point_step = offset;
  cloud_msg_.row_step = cloud_msg_.width * cloud_msg_.point_step;
  cloud_msg_.data.resize(cloud_msg_.height * cloud_msg_.row_step);
}

/**
 * @brief Function setting some fields in a PointCloud and adjusting the
 *        internals of the PointCloud2
 * @param n_fields the number of fields to add. The fields are given as
 *        strings: "xyz" (3 floats), "rgb" (3 uchar stacked in a float),
 *        "rgba" (4 uchar stacked in a float)
 * @return void
 *
 * WARNING: THIS FUNCTION DOES ADD ANY NECESSARY PADDING TRANSPARENTLY
 */
inline void PointCloud2Modifier::setPointCloud2FieldsByString(int n_fields, ...)
{
  cloud_msg_.fields.clear();
  cloud_msg_.fields.reserve(n_fields);
  va_list vl;
  va_start(vl, n_fields);
  int offset = 0;
  for (int i = 0; i < n_fields; ++i) {
    // Create the corresponding PointFields
    std::string
    field_name = std::string(va_arg(vl, char*));
    if (field_name == "xyz") {
      sensor_msgs::PointField point_field;
      // Do x, y and z
      offset = addPointField(cloud_msg_, "x", 1, sensor_msgs::PointField::FLOAT32, offset);
      offset = addPointField(cloud_msg_, "y", 1, sensor_msgs::PointField::FLOAT32, offset);
      offset = addPointField(cloud_msg_, "z", 1, sensor_msgs::PointField::FLOAT32, offset);
      offset += sizeOfPointField(sensor_msgs::PointField::FLOAT32);
    } else
      if ((field_name == "rgb") || (field_name == "rgba")) {
        offset = addPointField(cloud_msg_, field_name, 1, sensor_msgs::PointField::FLOAT32, offset);
        offset += 3 * sizeOfPointField(sensor_msgs::PointField::FLOAT32);
      } else
        throw std::runtime_error("Field " + field_name + " does not exist");
  }
  va_end(vl);

  // Resize the point cloud accordingly
  cloud_msg_.point_step = offset;
  cloud_msg_.row_step = cloud_msg_.width * cloud_msg_.point_step;
  cloud_msg_.data.resize(cloud_msg_.height * cloud_msg_.row_step);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace impl
{

/**
 */
template<typename T, typename TT, typename U, typename C, template <typename> class V>
PointCloud2IteratorBase<T, TT, U, C, V>::PointCloud2IteratorBase() : data_char_(0), data_(0), data_end_(0)
{
}

/**
 * @param cloud_msg_ The PointCloud2 to iterate upon
 * @param field_name The field to iterate upon
 */
template<typename T, typename TT, typename U, typename C, template <typename> class V>
PointCloud2IteratorBase<T, TT, U, C, V>::PointCloud2IteratorBase(C &cloud_msg, const std::string &field_name)
{
  int offset = set_field(cloud_msg, field_name);

  data_char_ = &(cloud_msg.data.front()) + offset;
  data_ = reinterpret_cast<TT*>(data_char_);
  data_end_ = reinterpret_cast<TT*>(&(cloud_msg.data.back()) + 1 + offset);
}

/** Assignment operator
 * @param iter the iterator to copy data from
 * @return a reference to *this
 */
template<typename T, typename TT, typename U, typename C, template <typename> class V>
V<T>& PointCloud2IteratorBase<T, TT, U, C, V>::operator =(const V<T> &iter)
{
  if (this != &iter)
  {
    point_step_ = iter.point_step_;
    data_char_ = iter.data_char_;
    data_ = iter.data_;
    data_end_ = iter.data_end_;
    is_bigendian_ = iter.is_bigendian_;
  }

  return *this;
}

/** Access the i th element starting at the current pointer (useful when a field has several elements of the same
 * type)
 * @param i
 * @return a reference to the i^th value from the current position
 */
template<typename T, typename TT, typename U, typename C, template <typename> class V>
TT& PointCloud2IteratorBase<T, TT, U, C, V>::operator [](size_t i) const
{
  return *(data_ + i);
}

/** Dereference the iterator. Equivalent to accessing it through [0]
 * @return the value to which the iterator is pointing
 */
template<typename T, typename TT, typename U, typename C, template <typename> class V>
TT& PointCloud2IteratorBase<T, TT, U, C, V>::operator *() const
{
  return *data_;
}

/** Increase the iterator to the next element
 * @return a reference to the updated iterator
 */
template<typename T, typename TT, typename U, typename C, template <typename> class V>
V<T>& PointCloud2IteratorBase<T, TT, U, C, V>::operator ++()
{
  data_char_ += point_step_;
  data_ = reinterpret_cast<TT*>(data_char_);
  return *static_cast<V<T>*>(this);
}

/** Basic pointer addition
 * @param i the amount to increase the iterator by
 * @return an iterator with an increased position
 */
template<typename T, typename TT, typename U, typename C, template <typename> class V>
V<T> PointCloud2IteratorBase<T, TT, U, C, V>::operator +(int i)
{
  V<T> res = *static_cast<V<T>*>(this);

  res.data_char_ += i*point_step_;
  res.data_ = reinterpret_cast<TT*>(res.data_char_);

  return res;
}

/** Increase the iterator by a certain amount
 * @return a reference to the updated iterator
 */
template<typename T, typename TT, typename U, typename C, template <typename> class V>
V<T>& PointCloud2IteratorBase<T, TT, U, C, V>::operator +=(int i)
{
  data_char_ += i*point_step_;
  data_ = reinterpret_cast<TT*>(data_char_);
  return *static_cast<V<T>*>(this);
}

/** Compare to another iterator
 * @return whether the current iterator points to a different address than the other one
 */
template<typename T, typename TT, typename U, typename C, template <typename> class V>
bool PointCloud2IteratorBase<T, TT, U, C, V>::operator !=(const V<T>& iter) const
{
  return iter.data_ != data_;
}

/** Return the end iterator
 * @return the end iterator (useful when performing normal iterator processing with ++)
 */
template<typename T, typename TT, typename U, typename C, template <typename> class V>
V<T> PointCloud2IteratorBase<T, TT, U, C, V>::end() const
{
  V<T> res = *static_cast<const V<T>*>(this);
  res.data_ = data_end_;
  return res;
}

/** Common code to set the field of the PointCloud2
  * @param cloud_msg the PointCloud2 to modify
  * @param field_name the name of the field to iterate upon
  * @return the offset at which the field is found
  */
template<typename T, typename TT, typename U, typename C, template <typename> class V>
int PointCloud2IteratorBase<T, TT, U, C, V>::set_field(const sensor_msgs::PointCloud2 &cloud_msg, const std::string &field_name)
{
  is_bigendian_ = cloud_msg.is_bigendian;
  point_step_ = cloud_msg.point_step;
  // make sure the channel is valid
  std::vector<sensor_msgs::PointField>::const_iterator field_iter = cloud_msg.fields.begin(), field_end =
      cloud_msg.fields.end();
  while ((field_iter != field_end) && (field_iter->name != field_name))
    ++field_iter;

  if (field_iter == field_end) {
    // Handle the special case of r,g,b,a (we assume they are understood as the channels of an rgb or rgba field)
    if ((field_name == "r") || (field_name == "g") || (field_name == "b") || (field_name == "a"))
    {
      // Check that rgb or rgba is present
      field_iter = cloud_msg.fields.begin();
      while ((field_iter != field_end) && (field_iter->name != "rgb") && (field_iter->name != "rgba"))
        ++field_iter;
      if (field_iter == field_end)
        throw std::runtime_error("Field " + field_name + " does not exist");
      if (field_name == "r")
      {
        if (is_bigendian_)
          return field_iter->offset + 1;
        else
          return field_iter->offset + 2;
      }
      if (field_name == "g")
      {
        if (is_bigendian_)
          return field_iter->offset + 2;
        else
          return field_iter->offset + 1;
      }
      if (field_name == "b")
      {
        if (is_bigendian_)
          return field_iter->offset + 3;
        else
          return field_iter->offset + 0;
      }
      if (field_name == "a")
      {
        if (is_bigendian_)
          return field_iter->offset + 0;
        else
          return field_iter->offset + 3;
      }
    } else
      throw std::runtime_error("Field " + field_name + " does not exist");
  }

  return field_iter->offset;
}

}
}

#endif// SENSOR_MSGS_IMPL_POINT_CLOUD_ITERATOR_H
