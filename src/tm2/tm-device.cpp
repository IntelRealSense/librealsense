#include "tm-device.h"

namespace librealsense
{
	tm2_device::tm2_device(
		std::shared_ptr<perc::TrackingManager> manager,
		perc::TrackingDevice* dev,
		std::shared_ptr<context> ctx,
		const platform::backend_device_group& group)
		: device(ctx, group),
		  _dev(dev), _manager(manager)
	{

	}
}