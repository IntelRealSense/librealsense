/*********************************************************************
 *
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
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
 *   * Neither the name of the Willow Garage nor the names of its
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
 * Author: Eitan Marder-Eppstein
 *********************************************************************/
#include <ros/rate.h>

namespace rs2rosinternal
{

Rate::Rate(double frequency)
: start_(Time::now())
, expected_cycle_time_(1.0 / frequency)
, actual_cycle_time_(0.0)
{ }

Rate::Rate(const Duration& d)
  : start_(Time::now())
  , expected_cycle_time_(d.sec, d.nsec)
  , actual_cycle_time_(0.0)
{ }



bool Rate::sleep()
{
  Time expected_end = start_ + expected_cycle_time_;

  Time actual_end = Time::now();

  // detect backward jumps in time
  if (actual_end < start_)
  {
    expected_end = actual_end + expected_cycle_time_;
  }

  //calculate the time we'll sleep for
  Duration sleep_time = expected_end - actual_end;

  //set the actual amount of time the loop took in case the user wants to know
  actual_cycle_time_ = actual_end - start_;

  //make sure to reset our start time
  start_ = expected_end;

  //if we've taken too much time we won't sleep
  if(sleep_time <= Duration(0.0))
  {
    // if we've jumped forward in time, or the loop has taken more than a full extra
    // cycle, reset our cycle
    if (actual_end > expected_end + expected_cycle_time_)
    {
      start_ = actual_end;
    }
    // return false to show that the desired rate was not met
    return false;
  }

  return sleep_time.sleep();
}

void Rate::reset()
{
  start_ = Time::now();
}

Duration Rate::cycleTime() const
{
  return actual_cycle_time_;
}

WallRate::WallRate(double frequency)
: start_(WallTime::now())
, expected_cycle_time_(1.0 / frequency)
, actual_cycle_time_(0.0)
{}

WallRate::WallRate(const Duration& d)
: start_(WallTime::now())
, expected_cycle_time_(d.sec, d.nsec)
, actual_cycle_time_(0.0)
{}

bool WallRate::sleep()
{
  WallTime expected_end = start_ + expected_cycle_time_;

  WallTime actual_end = WallTime::now();

  // detect backward jumps in time
  if (actual_end < start_)
  {
    expected_end = actual_end + expected_cycle_time_;
  }

  //calculate the time we'll sleep for
  WallDuration sleep_time = expected_end - actual_end;

  //set the actual amount of time the loop took in case the user wants to know
  actual_cycle_time_ = actual_end - start_;

  //make sure to reset our start time
  start_ = expected_end;

  //if we've taken too much time we won't sleep
  if(sleep_time <= WallDuration(0.0))
  {
    // if we've jumped forward in time, or the loop has taken more than a full extra
    // cycle, reset our cycle
    if (actual_end > expected_end + expected_cycle_time_)
    {
      start_ = actual_end;
    }
    return false;
  }

  return sleep_time.sleep();
}

void WallRate::reset()
{
  start_ = WallTime::now();
}

WallDuration WallRate::cycleTime() const
{
  return actual_cycle_time_;
}


}
