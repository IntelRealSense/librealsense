// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "record_device.h"

rsimpl2::record_device::record_device(std::shared_ptr<rsimpl2::device_interface> device,
                                      std::shared_ptr<rsimpl2::device_serializer> serializer):
    m_write_thread([](){return std::make_shared<dispatcher>(1);})
{

}
rsimpl2::record_device::~record_device()
{
    //m_write_thread->stop();
}
rsimpl2::sensor_interface& rsimpl2::record_device::get_sensor(unsigned int i)
{
    return *m_sensors[i];
}
unsigned int rsimpl2::record_device::get_sensors_count() const
{
    return m_sensors.size();
}

rsimpl2::record_sensor::record_sensor(std::shared_ptr<rsimpl2::sensor_interface> sensor,
                                      rsimpl2::frame_callback_ptr record_data_func)
{

}
rsimpl2::record_sensor::~record_sensor()
{

}
