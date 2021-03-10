/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2010, Willow Garage, Inc.
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
 *
 * $Id$
 *
 */

#ifndef SENSOR_MSGS_POINT_CLOUD_CONVERSION_H
#define SENSOR_MSGS_POINT_CLOUD_CONVERSION_H

#include <sensor_msgs/PointCloud.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/point_field_conversion.h>

/** 
  * \brief Convert between the old (sensor_msgs::PointCloud) and the new (sensor_msgs::PointCloud2) format.
  * \author Radu Bogdan Rusu
  */
namespace sensor_msgs
{
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Get the index of a specified field (i.e., dimension/channel)
    * \param points the the point cloud message
    * \param field_name the string defining the field name
    */
static inline int getPointCloud2FieldIndex (const sensor_msgs::PointCloud2 &cloud, const std::string &field_name)
{
  // Get the index we need
  for (size_t d = 0; d < cloud.fields.size (); ++d)
    if (cloud.fields[d].name == field_name)
      return (d);
  return (-1);
}

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Convert a sensor_msgs::PointCloud message to a sensor_msgs::PointCloud2 message.
    * \param input the message in the sensor_msgs::PointCloud format
    * \param output the resultant message in the sensor_msgs::PointCloud2 format
    */ 
static inline bool convertPointCloudToPointCloud2 (const sensor_msgs::PointCloud &input, sensor_msgs::PointCloud2 &output)
{
  output.header = input.header;
  output.width  = input.points.size ();
  output.height = 1;
  output.fields.resize (3 + input.channels.size ());
  // Convert x/y/z to fields
  output.fields[0].name = "x"; output.fields[1].name = "y"; output.fields[2].name = "z";
  int offset = 0;
  // All offsets are *4, as all field data types are float32
  for (size_t d = 0; d < output.fields.size (); ++d, offset += 4)
  {
    output.fields[d].offset = offset;
    output.fields[d].datatype = sensor_msgs::PointField::FLOAT32;
    output.fields[d].count  = 1;
  }
  output.point_step = offset;
  output.row_step   = output.point_step * output.width;
  // Convert the remaining of the channels to fields
  for (size_t d = 0; d < input.channels.size (); ++d)
    output.fields[3 + d].name = input.channels[d].name;
  output.data.resize (input.points.size () * output.point_step);
  output.is_bigendian = false;  // @todo ?
  output.is_dense     = false;

  // Copy the data points
  for (size_t cp = 0; cp < input.points.size (); ++cp)
  {
    memcpy (&output.data[cp * output.point_step + output.fields[0].offset], &input.points[cp].x, sizeof (float));
    memcpy (&output.data[cp * output.point_step + output.fields[1].offset], &input.points[cp].y, sizeof (float));
    memcpy (&output.data[cp * output.point_step + output.fields[2].offset], &input.points[cp].z, sizeof (float));
    for (size_t d = 0; d < input.channels.size (); ++d)
    {
      if (input.channels[d].values.size() == input.points.size())
      {
        memcpy (&output.data[cp * output.point_step + output.fields[3 + d].offset], &input.channels[d].values[cp], sizeof (float));
      }
    }
  }
  return (true);
}

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Convert a sensor_msgs::PointCloud2 message to a sensor_msgs::PointCloud message.
    * \param input the message in the sensor_msgs::PointCloud2 format
    * \param output the resultant message in the sensor_msgs::PointCloud format
    */ 
static inline bool convertPointCloud2ToPointCloud (const sensor_msgs::PointCloud2 &input, sensor_msgs::PointCloud &output)
{

  output.header = input.header;
  output.points.resize (input.width * input.height);
  output.channels.resize (input.fields.size () - 3);
  // Get the x/y/z field offsets
  int x_idx = getPointCloud2FieldIndex (input, "x");
  int y_idx = getPointCloud2FieldIndex (input, "y");
  int z_idx = getPointCloud2FieldIndex (input, "z");
  if (x_idx == -1 || y_idx == -1 || z_idx == -1)
  {
    std::cerr << "x/y/z coordinates not found! Cannot convert to sensor_msgs::PointCloud!" << std::endl;
    return (false);
  }
  int x_offset = input.fields[x_idx].offset;
  int y_offset = input.fields[y_idx].offset;
  int z_offset = input.fields[z_idx].offset;
  uint8_t x_datatype = input.fields[x_idx].datatype;
  uint8_t y_datatype = input.fields[y_idx].datatype;
  uint8_t z_datatype = input.fields[z_idx].datatype;
   
  // Convert the fields to channels
  int cur_c = 0;
  for (size_t d = 0; d < input.fields.size (); ++d)
  {
    if ((int)input.fields[d].offset == x_offset || (int)input.fields[d].offset == y_offset || (int)input.fields[d].offset == z_offset)
      continue;
    output.channels[cur_c].name = input.fields[d].name;
    output.channels[cur_c].values.resize (output.points.size ());
    cur_c++;
  }

  // Copy the data points
  for (size_t cp = 0; cp < output.points.size (); ++cp)
  {
    // Copy x/y/z
    output.points[cp].x = sensor_msgs::readPointCloud2BufferValue<float>(&input.data[cp * input.point_step + x_offset], x_datatype);
    output.points[cp].y = sensor_msgs::readPointCloud2BufferValue<float>(&input.data[cp * input.point_step + y_offset], y_datatype);
    output.points[cp].z = sensor_msgs::readPointCloud2BufferValue<float>(&input.data[cp * input.point_step + z_offset], z_datatype);
    // Copy the rest of the data
    int cur_c = 0;
    for (size_t d = 0; d < input.fields.size (); ++d)
    {
      if ((int)input.fields[d].offset == x_offset || (int)input.fields[d].offset == y_offset || (int)input.fields[d].offset == z_offset)
        continue;
      output.channels[cur_c++].values[cp] = sensor_msgs::readPointCloud2BufferValue<float>(&input.data[cp * input.point_step + input.fields[d].offset], input.fields[d].datatype);
    }
  }
  return (true);
}
}
#endif// SENSOR_MSGS_POINT_CLOUD_CONVERSION_H
