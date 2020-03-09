#ifndef _RS_DEVICE_HH
#define _RS_DEVICE_HH

#include <librealsense2/rs.hpp>
#include "RsSensor.hh"

class RsDevice
{
public:
	RsDevice();
	~RsDevice();
	std::vector<RsSensor> &getSensors() { return m_sensors; }
private:
	rs2::device m_device;
	std::vector<RsSensor> m_sensors;
};

#endif
