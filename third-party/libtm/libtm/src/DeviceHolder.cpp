// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
//
// Created by or on 6/25/19.
//

#include <iostream>
#include "DeviceHolder.h"
#include "Manager.h"



DeviceHolder::DeviceHolder(libusb_device *_device, std::shared_ptr<perc::Dispatcher> _dispatcher, perc::Manager *_manager):
m_dispatcher(_dispatcher),
m_libusb_device(_device),
m_manager(_manager)
{
    auto device = new perc::Device(m_libusb_device,m_dispatcher.get(),m_manager,m_manager);
    device->GetDeviceInfo(m_dev_info);
    delete device;
}

DeviceHolder::~DeviceHolder()
{
    libusb_unref_device(m_libusb_device);
}


std::shared_ptr<perc::TrackingDevice> DeviceHolder::get_device() {
    auto device = std::make_shared<perc::Device>(m_libusb_device,m_dispatcher.get(), m_manager, m_manager);
    libusb_ref_device(m_libusb_device);
    return device;
}
