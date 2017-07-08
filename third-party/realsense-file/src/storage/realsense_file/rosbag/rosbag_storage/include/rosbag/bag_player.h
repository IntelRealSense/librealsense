/*********************************************************************
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

#ifndef ROSBAG_BAG_PLAYER_H
#define ROSBAG_BAG_PLAYER_H

#include <boost/foreach.hpp>

#include "rosbag/bag.h"
#include "rosbag/view.h"

namespace rosbag
{


// A helper struct
struct BagCallback
{
    virtual ~BagCallback() {};
    virtual void call(MessageInstance m) = 0;
};

// A helper class for the callbacks
template<class T>
class BagCallbackT : public BagCallback
{
public:
    typedef boost::function<void (const boost::shared_ptr<const T>&)> Callback;

    BagCallbackT(Callback cb) :
        cb_(cb)
    {}

    void call(MessageInstance m) {
        cb_(m.instantiate<T>());
    }

private:
    Callback cb_;
};


/* A class for playing back bag files at an API level. It supports
   relatime, as well as accelerated and slowed playback. */
class BagPlayer
{
public:
  /* Constructor expecting the filename of a bag */
  BagPlayer(const std::string &filename) throw(BagException);

  /* Register a callback for a specific topic and type */
  template<class T>
  void register_callback(const std::string &topic,
                         typename BagCallbackT<T>::Callback f);

  /* Unregister a callback for a topic already registered */
  void unregister_callback(const std::string &topic);

  /* Set the time in the bag to start.  
   * Default is the first message */
  void set_start(const ros::Time &start);

  /* Set the time in the bag to stop. 
   * Default is the last message */
  void set_end(const ros::Time &end);

  /* Set the speed to playback.  1.0 is the default. 
   * 2.0 would be twice as fast, 0.5 is half realtime.  */
  void set_playback_speed(double scale);

  /* Start playback of the bag file using the parameters previously
     set */
  void start_play();
  
  /* Get the current time of the playback */
  ros::Time get_time();

  // Destructor
  virtual ~BagPlayer();
  

  // The bag file interface loaded in the constructor. 
  Bag bag;
  
private:
    ros::Time real_time(const ros::Time &msg_time);

    std::map<std::string, BagCallback *> cbs_;
    ros::Time bag_start_;
    ros::Time bag_end_;
    ros::Time last_message_time_;
    double playback_speed_;
    ros::Time play_start_;
};

template<class T>
void BagPlayer::register_callback(const std::string &topic,
        typename BagCallbackT<T>::Callback cb) {
    cbs_[topic] = new BagCallbackT<T>(cb);
}

}

#endif
