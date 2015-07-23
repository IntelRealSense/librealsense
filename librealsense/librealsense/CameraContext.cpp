#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <thread>
#include <chrono>

#include "CameraContext.h"
#include "UVCCamera.h"

////////////////////
// Camera Context //
////////////////////

CameraContext::CameraContext()
{
    uvc_error_t initStatus = uvc_init(&privateContext, NULL);
    
    if (initStatus < 0)
    {
        uvc_perror(initStatus, "uvc_init");
        throw std::runtime_error("Could not initialize UVC context");
    }
    
    QueryDeviceList();
}

CameraContext::~CameraContext()
{
    cameras.clear(); // tear down cameras before context
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    if (privateContext)
    {
        uvc_exit(privateContext);
    }
}

void CameraContext::QueryDeviceList()
{
    uvc_device_t **list;
    
    uvc_error_t status = uvc_get_device_list(privateContext, &list);
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