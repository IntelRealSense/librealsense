/*
 * Software License Agreement (BSD License)
 *
 * Robot Operating System code by the University of Osnabrück
 * Copyright (c) 2015, University of Osnabrück
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above 
 *      copyright notice, this list of conditions and the following
 *      disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above 
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *   3. Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *
 * point_field_conversion.h
 *
 * Created on: 16.07.2015
 *  Authors: Sebastian Pütz <spuetz@uni-osnabrueck.de>
 */

#ifndef SENSOR_MSGS_POINT_FIELD_CONVERSION_H
#define SENSOR_MSGS_POINT_FIELD_CONVERSION_H

/** 
  * \brief  This file provides a type to enum mapping for the different
  *         PointField types and methods to read and write in 
  *         a PointCloud2 buffer for the different PointField types.
  * \author Sebastian Pütz
  */
namespace sensor_msgs{
  /*!
   * \Enum to type mapping.
   */
  template<int> struct pointFieldTypeAsType {};
  template<> struct pointFieldTypeAsType<sensor_msgs::PointField::INT8>    { typedef int8_t   type; };
  template<> struct pointFieldTypeAsType<sensor_msgs::PointField::UINT8>   { typedef uint8_t  type; };
  template<> struct pointFieldTypeAsType<sensor_msgs::PointField::INT16>   { typedef int16_t  type; };
  template<> struct pointFieldTypeAsType<sensor_msgs::PointField::UINT16>  { typedef uint16_t type; };
  template<> struct pointFieldTypeAsType<sensor_msgs::PointField::INT32>   { typedef int32_t  type; };
  template<> struct pointFieldTypeAsType<sensor_msgs::PointField::UINT32>  { typedef uint32_t type; };
  template<> struct pointFieldTypeAsType<sensor_msgs::PointField::FLOAT32> { typedef float    type; };
  template<> struct pointFieldTypeAsType<sensor_msgs::PointField::FLOAT64> { typedef double   type; };
  
  /*!
   * \Type to enum mapping.
   */
  template<typename T> struct typeAsPointFieldType {};
  template<> struct typeAsPointFieldType<int8_t>   { static const uint8_t value = sensor_msgs::PointField::INT8;    };
  template<> struct typeAsPointFieldType<uint8_t>  { static const uint8_t value = sensor_msgs::PointField::UINT8;   };
  template<> struct typeAsPointFieldType<int16_t>  { static const uint8_t value = sensor_msgs::PointField::INT16;   };
  template<> struct typeAsPointFieldType<uint16_t> { static const uint8_t value = sensor_msgs::PointField::UINT16;  };
  template<> struct typeAsPointFieldType<int32_t>  { static const uint8_t value = sensor_msgs::PointField::INT32;   };
  template<> struct typeAsPointFieldType<uint32_t> { static const uint8_t value = sensor_msgs::PointField::UINT32;  };
  template<> struct typeAsPointFieldType<float>    { static const uint8_t value = sensor_msgs::PointField::FLOAT32; };
  template<> struct typeAsPointFieldType<double>   { static const uint8_t value = sensor_msgs::PointField::FLOAT64; };
 
  /*!
   * \Converts a value at the given pointer position, interpreted as the datatype
   *  specified by the given template argument point_field_type, to the given
   *  template type T and returns it.
   * \param data_ptr            pointer into the point cloud 2 buffer
   * \tparam point_field_type   sensor_msgs::PointField datatype value
   * \tparam T                  return type
   */
  template<int point_field_type, typename T>
    inline T readPointCloud2BufferValue(const unsigned char* data_ptr){
      typedef typename pointFieldTypeAsType<point_field_type>::type type;
      return static_cast<T>(*(reinterpret_cast<type const *>(data_ptr)));
    }
  
  /*!
   * \Converts a value at the given pointer position interpreted as the datatype
   *  specified by the given datatype parameter to the given template type and returns it.
   * \param data_ptr    pointer into the point cloud 2 buffer
   * \param datatype    sensor_msgs::PointField datatype value
   * \tparam T          return type
   */
  template<typename T>
    inline T readPointCloud2BufferValue(const unsigned char* data_ptr, const unsigned char datatype){
      switch(datatype){
        case sensor_msgs::PointField::INT8:
          return readPointCloud2BufferValue<sensor_msgs::PointField::INT8, T>(data_ptr);
        case sensor_msgs::PointField::UINT8:
          return readPointCloud2BufferValue<sensor_msgs::PointField::UINT8, T>(data_ptr);
        case sensor_msgs::PointField::INT16:
          return readPointCloud2BufferValue<sensor_msgs::PointField::INT16, T>(data_ptr);
        case sensor_msgs::PointField::UINT16:
          return readPointCloud2BufferValue<sensor_msgs::PointField::UINT16, T>(data_ptr);
        case sensor_msgs::PointField::INT32:
          return readPointCloud2BufferValue<sensor_msgs::PointField::INT32, T>(data_ptr);
        case sensor_msgs::PointField::UINT32:
          return readPointCloud2BufferValue<sensor_msgs::PointField::UINT32, T>(data_ptr);
        case sensor_msgs::PointField::FLOAT32:
          return readPointCloud2BufferValue<sensor_msgs::PointField::FLOAT32, T>(data_ptr);
        case sensor_msgs::PointField::FLOAT64:
          return readPointCloud2BufferValue<sensor_msgs::PointField::FLOAT64, T>(data_ptr);
      }
    }

  /*!
   * \Inserts a given value at the given point position interpreted as the datatype
   *  specified by the template argument point_field_type.
   * \param data_ptr            pointer into the point cloud 2 buffer
   * \param value               the value to insert
   * \tparam point_field_type   sensor_msgs::PointField datatype value
   * \tparam T                  type of the value to insert
   */
  template<int point_field_type, typename T>
    inline void writePointCloud2BufferValue(unsigned char* data_ptr, T value){
      typedef typename pointFieldTypeAsType<point_field_type>::type type;
      *(reinterpret_cast<type*>(data_ptr)) = static_cast<type>(value);
    }

  /*!
   * \Inserts a given value at the given point position interpreted as the datatype
   *  specified by the given datatype parameter.
   * \param data_ptr    pointer into the point cloud 2 buffer
   * \param datatype    sensor_msgs::PointField datatype value
   * \param value       the value to insert
   * \tparam T          type of the value to insert
   */
  template<typename T>
    inline void writePointCloud2BufferValue(unsigned char* data_ptr, const unsigned char datatype, T value){
      switch(datatype){
        case sensor_msgs::PointField::INT8:
          writePointCloud2BufferValue<sensor_msgs::PointField::INT8, T>(data_ptr, value);
          break;
        case sensor_msgs::PointField::UINT8:
          writePointCloud2BufferValue<sensor_msgs::PointField::UINT8, T>(data_ptr, value);
          break;
        case sensor_msgs::PointField::INT16:
          writePointCloud2BufferValue<sensor_msgs::PointField::INT16, T>(data_ptr, value);
          break;
        case sensor_msgs::PointField::UINT16:
          writePointCloud2BufferValue<sensor_msgs::PointField::UINT16, T>(data_ptr, value);
          break;
        case sensor_msgs::PointField::INT32:
          writePointCloud2BufferValue<sensor_msgs::PointField::INT32, T>(data_ptr, value);
          break;
        case sensor_msgs::PointField::UINT32:
          writePointCloud2BufferValue<sensor_msgs::PointField::UINT32, T>(data_ptr, value);
          break;
        case sensor_msgs::PointField::FLOAT32:
          writePointCloud2BufferValue<sensor_msgs::PointField::FLOAT32, T>(data_ptr, value);
          break;
        case sensor_msgs::PointField::FLOAT64:
          writePointCloud2BufferValue<sensor_msgs::PointField::FLOAT64, T>(data_ptr, value);
          break;
      }
    }
}

#endif /* point_field_conversion.h */
