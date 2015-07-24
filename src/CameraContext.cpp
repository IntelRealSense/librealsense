#include <librealsense/CameraContext.h>
#include <librealsense/R200/R200.h>
#include <librealsense/F200/F200.h>

////////////////////
// Camera Context //
////////////////////

namespace rs
{
    
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
        auto dev = list[index];
        
        uvc_ref_device(dev);

        uvc_device_descriptor_t * desc;
        if(uvc_get_device_descriptor(dev, &desc) == UVC_SUCCESS)
        {
            if (desc->idProduct == 2688)
            {
                cameras.push_back(std::make_shared<r200::R200Camera>(list[index], index));
            }
            
            else if (desc->idProduct == 2662)
            {
                cameras.push_back(std::make_shared<f200::F200Camera>(list[index], index));
            }
            
            uvc_free_device_descriptor(desc);
        }
        index++;
    }

    uvc_free_device_list(list, 1);
}

} // end namespace rs