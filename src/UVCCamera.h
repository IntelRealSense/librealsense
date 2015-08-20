#pragma once
#ifndef LIBREALSENSE_UVC_CAMERA_H
#define LIBREALSENSE_UVC_CAMERA_H

#include "rs-internal.h"

#ifdef USE_UVC_DEVICES
#include <libuvc/libuvc.h>

namespace rs
{
    inline void CheckUVC(const char * call, uvc_error_t status)
    {
        if (status < 0)
        {
            throw std::runtime_error(ToString() << call << "(...) returned " << uvc_strerror(status));
        }
    }

    inline void GetUSBInfo(uvc_device_t * dev, rs::USBDeviceInfo & info)
    {
        uvc_device_descriptor_t * desc;
        if (uvc_get_device_descriptor(dev, &desc) == UVC_SUCCESS)
        {
            if (desc->serialNumber) info.serial = desc->serialNumber;
            if (desc->idVendor) info.vid = desc->idVendor;
            if (desc->idProduct) info.pid = desc->idProduct;
            uvc_free_device_descriptor(desc);
        }
        else throw std::runtime_error("call to uvc_get_device_descriptor() failed");
    }

    struct ResolutionMode
    {
        int stream;                 // RS_DEPTH, RS_COLOR, etc.

        int width, height;          // Resolution visible to the library client
        int fps;                    // Framerate visible to the library client
        rs::FrameFormat format;     // Format visible to the library client

        int uvcWidth, uvcHeight;    // Resolution advertised over UVC
        int uvcFps;                 // Framerate advertised over UVC
        uvc_frame_format uvcFormat; // Format advertised over UVC

        rs_intrinsics intrinsics;   // Image intrinsics
    };

    class UVCCamera : public rs_camera
    {
        NO_MOVE(UVCCamera);

    protected:
        
        struct StreamInterface
        {
            uvc_device_handle_t * uvcHandle = nullptr;
            uvc_stream_ctrl_t ctrl = uvc_stream_ctrl_t{}; // {0};
            TripleBuffer buffer;
            ResolutionMode mode;
        };

        uvc_context_t * internalContext;
        uvc_device_t * hardware = nullptr;
        std::unique_ptr<StreamInterface> streams[2];
        std::vector<float> vertices;

        std::vector<ResolutionMode> modes;

    public:

        UVCCamera(uvc_context_t * ctx, uvc_device_t * device, int num);
        ~UVCCamera();

        void EnableStream(int stream, int width, int height, int fps, FrameFormat format) override final;
        void StartStreaming() override final;
        void StopStreaming() override final;
        void WaitAllStreams() override final;

        const uint8_t * GetColorImage() const override final { return streams[RS_COLOR] ? streams[RS_COLOR]->buffer.front_data() : nullptr; }
        const uint16_t * GetDepthImage() const override final { return streams[RS_DEPTH] ? reinterpret_cast<const uint16_t *>(streams[RS_DEPTH]->buffer.front_data()) : nullptr; }
        const float * GetVertexImage() const override final { return vertices.data(); }

        rs_intrinsics GetStreamIntrinsics(int stream) const override final;

        virtual int GetDepthCameraNumber() const = 0;
        virtual int GetColorCameraNumber() const = 0;
        virtual void RetrieveCalibration() = 0;
        virtual void SetStreamIntent(bool depth, bool color) = 0;
        virtual void ComputeVertexImage() = 0;
    };
    
} // end namespace rs
#endif

#endif
