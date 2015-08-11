#include "Common.h"
#include "F200.h"

#ifndef WIN32

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
    if (streamingModeBitfield == 0)
        throw std::invalid_argument("No streams have been configured...");
    
    if (streamingModeBitfield & RS_STREAM_DEPTH)
    {
        StreamInterface * stream = new StreamInterface();
        stream->camera = this;
        
        if (!OpenStreamOnSubdevice(hardware, stream->uvcHandle, 1))
            throw std::runtime_error("Failed to open RS_STREAM_DEPTH (subdevice 1)");
        
        // Debugging
        uvc_print_stream_ctrl(&stream->ctrl, stdout);
        
        streamInterfaces.insert(std::pair<int, StreamInterface *>(RS_STREAM_DEPTH, stream));
    }
    
    if (streamingModeBitfield & RS_STREAM_RGB)
    {
        StreamInterface * stream = new StreamInterface();
        stream->camera = this;
        
        if (!OpenStreamOnSubdevice(hardware, stream->uvcHandle, 2))
            throw std::runtime_error("Failed to open RS_STREAM_RGB (subdevice 2)");
        
        // Debugging
        uvc_print_stream_ctrl(&stream->ctrl, stdout);
        
        streamInterfaces.insert(std::pair<int, StreamInterface *>(RS_STREAM_RGB, stream));
    }
    
    GetUSBInfo(hardware, usbInfo);
    std::cout << "Serial Number: " << usbInfo.serial << std::endl;
    std::cout << "USB VID: " << usbInfo.vid << std::endl;
    std::cout << "USB PID: " << usbInfo.pid << std::endl;
    
    return true;
}

void F200Camera::StartStream(int streamIdentifier, const StreamConfiguration & c)
{
    auto stream = streamInterfaces[streamIdentifier];
    
    if (stream->uvcHandle)
    {
        stream->fmt = static_cast<uvc_frame_format>(c.format);
        
        uvc_error_t status = uvc_get_stream_ctrl_format_size(stream->uvcHandle, &stream->ctrl, stream->fmt, c.width, c.height, c.fps);
        
        if (status < 0)
        {
            uvc_perror(status, "uvc_get_stream_ctrl_format_size");
            throw std::runtime_error("Open camera_handle Failed");
        }
        
        if (c.format == FrameFormat::INVR || c.format == FrameFormat::INVZ)
        {
            depthFrame.reset(new TripleBufferedFrame(c.width, c.height, 2));
        }
        
        else if (c.format == FrameFormat::YUYV)
        {
            colorFrame.reset(new TripleBufferedFrame(c.width, c.height, 3));
        }
        
        uvc_error_t startStreamResult = uvc_start_streaming(stream->uvcHandle, &stream->ctrl, &UVCCamera::cb, stream, 0);
        
        if (startStreamResult < 0)
        {
            uvc_perror(startStreamResult, "start_stream");
            throw std::runtime_error("Could not start stream");
        }
        
    }
}

void F200Camera::StopStream(int streamNum)
{
    //@tofix
}
    
} // end f200

#endif
