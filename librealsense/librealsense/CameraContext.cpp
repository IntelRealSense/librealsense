#include "CameraContext.h"

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <thread>
#include <chrono>

CameraContext::CameraContext()
{
    uvc_error_t initStatus;

    initStatus = uvc_init(&privateContext, NULL);
    
    if (initStatus < 0)
    {
        uvc_perror(initStatus, "uvc_init");
        throw std::runtime_error("Could not initialize a UVC context");
    }
    
    QueryDeviceList();
}

CameraContext::~CameraContext()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    if (privateContext)
    {
        uvc_exit(privateContext);
    }
}

void CameraContext::QueryDeviceList()
{
    uvc_device_t **list;
    uvc_error_t status;
    
    status = uvc_get_device_list(privateContext, &list);
    if (status != UVC_SUCCESS)
    {
        uvc_perror(status, "uvc_get_device_list");
        return;
    }
    
    size_t index = 0;
    while (list[index] != nullptr)
    {
        uvc_ref_device(list[index]);
        cameras.push_back(std::make_shared<UVCCamera>(list[index], index));
        index++;
    }
    
    uvc_free_device_list(list, 1);
}