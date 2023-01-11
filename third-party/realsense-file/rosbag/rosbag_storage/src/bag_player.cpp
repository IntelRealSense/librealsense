#include "rosbag/bag_player.h"

namespace rosbag
{

BagPlayer::BagPlayer(const std::string &fname) {
    bag.open(fname, rosbag::bagmode::Read);
    rs2rosinternal::Time::init();
    View v(bag);
    bag_start_ = v.getBeginTime();
    bag_end_ = v.getEndTime();
    last_message_time_ = rs2rosinternal::Time(0);
    playback_speed_ = 1.0;
}

BagPlayer::~BagPlayer() {
    bag.close();
}

rs2rosinternal::Time BagPlayer::get_time() {
    return last_message_time_;
}

void BagPlayer::set_start(const rs2rosinternal::Time &start) {
    bag_start_ = start;
}

void BagPlayer::set_end(const rs2rosinternal::Time &end) {
    bag_end_ = end;
}

void BagPlayer::set_playback_speed(double scale) {
  if (scale > 0.0)
    playback_speed_ = scale;
}

rs2rosinternal::Time BagPlayer::real_time(const rs2rosinternal::Time &msg_time) {
  return play_start_ + (msg_time - bag_start_) * (1 / playback_speed_);
}

void BagPlayer::start_play() {

    std::vector<std::string> topics;
    for( auto const & cb : cbs_ )
        topics.push_back(cb.first);

    View view(bag, TopicQuery(topics), bag_start_, bag_end_);
    play_start_ = rs2rosinternal::Time::now();

    for( auto const & m : view )
    {
        if (cbs_.find(m.getTopic()) == cbs_.end())
            continue;

        rs2rosinternal::Time::sleepUntil(real_time(m.getTime()));

        last_message_time_ = m.getTime(); /* this is the recorded time */
        cbs_[m.getTopic()]->call(m);
    }
}

void BagPlayer::unregister_callback(const std::string &topic) {
    delete cbs_[topic];
    cbs_.erase(topic);
}

}

