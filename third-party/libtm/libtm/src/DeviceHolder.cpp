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
    auto status = device->GetDeviceInfo(m_dev_info);
    std::cout << std::hex << (m_dev_info.serialNumber >> 16) << std::endl;
    delete device;
}

DeviceHolder::~DeviceHolder()
{
    if(m_device) delete m_device;
    if(m_libusb_device) delete m_libusb_device;
    libusb_unref_device(m_libusb_device);
}

void DeviceHolder::create()
{
    m_device =  new perc::Device(m_libusb_device,m_dispatcher.get(), m_manager, m_manager);
    libusb_ref_device(m_libusb_device);
}

//void DeviceHolder::addTask(std::shared_ptr<CompleteTask>& newTask)
//{
//    m_manager->addTask(newTask);
//}
//
//void DeviceHolder::removeTasks(void * owner, bool completeTasks)
//{
//    m_manager->removeTasks(owner,completeTasks);
//}

bool DeviceHolder::IsDeviceReady() {
    return m_device->IsDeviceReady();
}