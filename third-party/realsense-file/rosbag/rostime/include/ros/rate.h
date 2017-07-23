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
#ifndef ROSLIB_RATE_H
#define ROSLIB_RATE_H

#include "ros/time.h"
#include "rostime_decl.h"

namespace ros
{
class Duration;

/**
 * @class Rate
 * @brief Class to help run loops at a desired frequency
 */
class ROSTIME_DECL Rate
{
public:
  /**
   * @brief  Constructor, creates a Rate
   * @param  frequency The desired rate to run at in Hz
   */
  Rate(double frequency);
  explicit Rate(const Duration&);

  /**
   * @brief  Sleeps for any leftover time in a cycle. Calculated from the last time sleep, reset, or the constructor was called.
   * @return True if the desired rate was met for the cycle, false otherwise.
   */
  bool sleep();

  /**
   * @brief  Sets the start time for the rate to now
   */
  void reset();

  /**
   * @brief  Get the actual run time of a cycle from start to sleep
   * @return The runtime of the cycle
   */
  Duration cycleTime() const;

  /**
   * @brief Get the expected cycle time -- one over the frequency passed in to the constructor
   */
  Duration expectedCycleTime() const { return expected_cycle_time_; }

private:
  Time start_;
  Duration expected_cycle_time_, actual_cycle_time_;
};

/**
 * @class WallRate
 * @brief Class to help run loops at a desired frequency.  This version always uses wall-clock time.
 */
class ROSTIME_DECL WallRate
{
public:
  /**
   * @brief  Constructor, creates a Rate
   * @param  frequency The desired rate to run at in Hz
   */
  WallRate(double frequency);
  explicit WallRate(const Duration&);

  /**
   * @brief  Sleeps for any leftover time in a cycle. Calculated from the last time sleep, reset, or the constructor was called.
   * @return Passes through the return value from WallDuration::sleep() if it slept, false otherwise.
   */
  bool sleep();

  /**
   * @brief  Sets the start time for the rate to now
   */
  void reset();

  /**
   * @brief  Get the actual run time of a cycle from start to sleep
   * @return The runtime of the cycle
   */
  WallDuration cycleTime() const;

  /**
   * @brief Get the expected cycle time -- one over the frequency passed in to the constructor
   */
  WallDuration expectedCycleTime() const { return expected_cycle_time_; }

private:
  WallTime start_;
  WallDuration expected_cycle_time_, actual_cycle_time_;
};

}

#endif
