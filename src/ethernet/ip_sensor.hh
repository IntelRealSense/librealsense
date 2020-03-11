/*
this class hold ip sensor data.
mainly: 
1. sw sensor
2. relative streams
3. state -> on/off
*/

#include <librealsense2/rs.hpp>
#include "software-device.h"
#include <list>

class ip_sensor
{
private:
    
public:

    //todo: remove smart ptr here
    std::shared_ptr<rs2::software_sensor> sw_sensor;

    std::list<long long int> active_streams_keys;

    std::map<rs2_option,float> sensors_option;

    bool is_enabled;

    //TODO: get smart ptr from rtsp client creator
    IRsRtsp* rtsp_client;

    ip_sensor(/* args */);
    ~ip_sensor();
};

ip_sensor::ip_sensor(/* args */)
{
    is_enabled=false;
}

ip_sensor::~ip_sensor()
{
}
