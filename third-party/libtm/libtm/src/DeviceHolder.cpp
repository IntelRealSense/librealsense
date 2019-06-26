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
//    m_device = nullptr;
    auto device = new perc::Device(m_libusb_device,m_dispatcher.get(),m_manager,m_manager);
    device->GetDeviceInfo(m_dev_info);
    delete device;
}

DeviceHolder::~DeviceHolder()
{
    destruct();
}

void DeviceHolder::create()
{
    m_device =  new perc::Device(m_libusb_device,m_dispatcher.get(), m_manager, m_manager);
    libusb_ref_device(m_libusb_device);
}

void DeviceHolder::destruct()
{
    libusb_unref_device(m_libusb_device);
    if(!m_device) delete m_device;
}


bool DeviceHolder::IsDeviceReady() {
    return m_device->IsDeviceReady();
}