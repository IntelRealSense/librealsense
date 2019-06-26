//
// Created by or on 6/26/19.
//

#ifndef LIBREALSENSE2_TRACKINGDEVICEHOLDER_H
#define LIBREALSENSE2_TRACKINGDEVICEHOLDER_H

namespace perc
{
    class DLL_EXPORT TrackingDeviceHolder{
    public:
        virtual ~TrackingDeviceHolder() {}

        virtual void create() = 0;
        virtual void destruct() = 0;
        virtual bool IsDeviceReady() = 0;

        virtual perc::TrackingData::DeviceInfo get_device_info() {};
        virtual perc::TrackingDevice *get_device() = 0;

    };
}

#endif //LIBREALSENSE2_TRACKINGDEVICEHOLDER_H
