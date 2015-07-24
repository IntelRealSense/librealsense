#include <librealsense/Common.h>
#include <librealsense/R200/R200.h>

using namespace rs;

namespace r200
{
    
R200Camera::R200Camera(uvc_device_t * device, int idx) : UVCCamera(device, idx)
{
    
}

R200Camera::~R200Camera()
{
    
}

//@tofix protect against calling this multiple times
bool R200Camera::ConfigureStreams()
{
    if (streamingModeBitfield == 0)
        throw std::invalid_argument("No streams have been configured...");
    
    uvc_ref_device(hardware);
    
    //@tofix: Test for successful open
    if (streamingModeBitfield & STREAM_DEPTH)
    {
        StreamInterface * stream = new StreamInterface();
        stream->camera = this;
        
        OpenStreamOnSubdevice(hardware, stream->uvcHandle, 1);
        streamInterfaces.insert(std::pair<int, StreamInterface *>(STREAM_DEPTH, stream));
        //DumpInfo();
    }
    
    if (streamingModeBitfield & STREAM_RGB)
    {
        StreamInterface * stream = new StreamInterface();
        stream->camera = this;
        
        OpenStreamOnSubdevice(hardware, stream->uvcHandle, 2);
        streamInterfaces.insert(std::pair<int, StreamInterface *>(STREAM_RGB, stream));
        //DumpInfo();
    }
    
    //DumpInfo();
    
    /*
     if (streamingModeBitfield & STREAM_LR)
     {
     openDevice(0);
     }
     */
    
    uvc_device_descriptor_t * desc;
    if(uvc_get_device_descriptor(hardware, &desc) == UVC_SUCCESS)
    {
        if (desc->serialNumber) usbInfo.serial = desc->serialNumber;
        if (desc->idVendor) usbInfo.vid = desc->idVendor;
        if (desc->idProduct) usbInfo.pid = desc->idProduct;
        uvc_free_device_descriptor(desc);
    }
    
    std::cout << "Serial Number: " << usbInfo.serial << std::endl;
    std::cout << "USB VID: " << usbInfo.vid << std::endl;
    std::cout << "USB PID: " << usbInfo.pid << std::endl;
    
    auto oneTimeInitialize = [&](uvc_device_handle_t * uvc_handle)
    {
        //@tofix - if first time:
        spiInterface.reset(new SPI_Interface(uvc_handle));
        spiInterface->Initialize();
        auto calibParams = spiInterface->GetRectifiedParameters();
        
        std::cout << "Firmware Revision: " << GetFirmwareVersion(uvc_handle) << std::endl;
        
        // Debugging Camera Firmware:
        std::cout << "Calib Version Number: " << calibParams.versionNumber << std::endl;
        
        spiInterface->PrintHeaderInfo();
        
        auto streamIntent = SetStreamIntent(uvc_handle, streamingModeBitfield); //@tofix - proper streaming mode, assume color and depth
        if (!streamIntent)
        {
            throw std::runtime_error("Could not set stream intent");
        }
    };
    
    // We only need to do this once, so check if any stream has been configured
    if (streamInterfaces[STREAM_DEPTH]->uvcHandle)
    {
        oneTimeInitialize(streamInterfaces[STREAM_DEPTH]->uvcHandle);
    }
    
    else if (streamInterfaces[STREAM_RGB]->uvcHandle)
    {
        oneTimeInitialize(streamInterfaces[STREAM_RGB]->uvcHandle);
    }
    
    return true;
}

void R200Camera::StartStream(int streamIdentifier, const StreamConfiguration & c)
{
    //  if (isStreaming) throw std::runtime_error("Camera is already streaming");
    
    auto stream = streamInterfaces[streamIdentifier];
    
    if (stream->uvcHandle)
    {
        stream->fmt = c.format;
        
        uvc_error_t status = uvc_get_stream_ctrl_format_size(stream->uvcHandle, &stream->ctrl, c.format, c.width, c.height, c.fps);
        
        if (status < 0)
        {
            uvc_perror(status, "uvc_get_stream_ctrl_format_size");
            throw std::runtime_error("Open camera_handle Failed");
        }
        
        //@tofix - check streaming mode as well
        if (c.format == UVC_FRAME_FORMAT_Z16)
            depthFrame.reset(new TripleBufferedFrame(c.width, c.height, 2)); // uint8_t * 2 (uint16_t)

        else if (c.format == UVC_FRAME_FORMAT_YUYV)
            colorFrame.reset(new TripleBufferedFrame(c.width, c.height, 3)); // uint8_t * 3
        
        uvc_error_t startStreamResult = uvc_start_streaming(stream->uvcHandle, &stream->ctrl, &UVCCamera::cb, stream, 0);
        
        if (startStreamResult < 0)
        {
            uvc_perror(startStreamResult, "start_stream");
            throw std::runtime_error("Could not start stream");
        }
        
    }
    
    //@tofix - else what?
    
}

void R200Camera::StopStream(int streamNum)
{
    //@tofix - uvc_stream_stop with a real stream handle -> index with map that we have
    //uvc_stop_streaming(deviceHandle);
}

} // end namespace r200