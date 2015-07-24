#include <librealsense/Common.h>
#include <librealsense/F200/F200.h>

using namespace rs;

namespace f200
{
    F200Camera::F200Camera(uvc_device_t * device, int idx) : UVCCamera(device, idx)
    {
        
    }
    
    F200Camera::~F200Camera()
    {
        
    }
    
    bool F200Camera::ConfigureStreams()
    {
        return false;
    }
    
    void F200Camera::StartStream(int streamIdentifier, const StreamConfiguration & config)
    {
        
    }
    
    void F200Camera::StopStream(int streamNum)
    {
        
    }
}
