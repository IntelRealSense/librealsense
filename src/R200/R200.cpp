#include "R200.h"

#ifdef USE_UVC_DEVICES
#include "CameraHeader.h"
#include "XU.h"
#include "HardwareIO.h"
#include "../../include/librealsense/rsutil.h"

using namespace rs;

namespace r200
{
    R200Camera::R200Camera(uvc_context_t * ctx, uvc_device_t * device, int idx) : UVCCamera(ctx, device, idx)
    {

    }

    R200Camera::~R200Camera()
    {

    }

    void R200Camera::RetrieveCalibration()
    {
        auto handle = streamInterfaces[RS_STREAM_DEPTH]->uvcHandle;
        if(!handle) handle = streamInterfaces[RS_STREAM_RGB]->uvcHandle;
        if(handle)
        {
            hardware_io.reset(new DS4HardwareIO(handle));

            //uvc_print_diag(uvc_handle, stderr);

            std::cout << "Firmware Revision: " << GetFirmwareVersion(handle) << std::endl;

            if (!SetStreamIntent(handle, streamingModeBitfield))
            {
                throw std::runtime_error("Could not set stream intent. Replug camera?");
            }
        }
    }

    void R200Camera::StopStream(int streamNum)
    {
        //@tofix - uvc_stream_stop with a real stream handle -> index with map that we have
        //uvc_stop_streaming(deviceHandle);
    }

    rs_intrinsics R200Camera::GetStreamIntrinsics(int stream)
    {
        auto calib = hardware_io->GetCalibration();
        auto lr = calib.modesLR[0]; // Assumes 628x468 for now
        auto t = calib.intrinsicsThird[1]; // Assumes 640x480 for now
        switch(stream)
        {
        case RS_STREAM_DEPTH: return {{static_cast<int>(lr.rw-12),static_cast<int>(lr.rh-12)},{lr.rfx,lr.rfy},{lr.rpx-6,lr.rpy-6},{0,0,0,0,0}};
        case RS_STREAM_RGB: return {{static_cast<int>(t.w),static_cast<int>(t.h)},{t.fx,t.fy},{t.px,t.py},{t.k[0],t.k[1],t.k[2],t.k[3],t.k[4]}};
        default: throw std::runtime_error("unsupported stream");
        }
    }

    rs_extrinsics R200Camera::GetStreamExtrinsics(int from, int to)
    {
        auto calib = hardware_io->GetCalibration();
        if(from == RS_STREAM_DEPTH && to == RS_STREAM_RGB)
        {
            rs_extrinsics extrin;
            for(int i=0; i<9; ++i) extrin.rotation[i] = (float)calib.Rthird[0][i];
            extrin.translation[0] = extrin.rotation[0]*calib.T[0][0] + extrin.rotation[1]*calib.T[0][1] + extrin.rotation[2]*calib.T[0][2];
            extrin.translation[1] = extrin.rotation[3]*calib.T[0][0] + extrin.rotation[4]*calib.T[0][1] + extrin.rotation[5]*calib.T[0][2];
            extrin.translation[2] = extrin.rotation[6]*calib.T[0][0] + extrin.rotation[7]*calib.T[0][1] + extrin.rotation[8]*calib.T[0][2];
            return extrin;
        }
        else throw std::runtime_error("unsupported streams");
    }

    void R200Camera::WaitAllStreams()
    {
        if (depthFrame.updated)
        {
            std::lock_guard<std::mutex> guard(frameMutex);
            depthFrame.swap_front();

            auto depth_intrin = GetStreamIntrinsics(RS_STREAM_DEPTH);
            auto inDepth = GetDepthImage();
            auto outVert = vertices.data();
            for(int y=0; y<depth_intrin.image_size[1]; ++y)
            {
                for(int x=0; x<depth_intrin.image_size[0]; ++x)
                {
                    if(auto d = *inDepth++)
                    {
                        const float pixel[] = {x,y};
                        rs_deproject_rectified_pixel_to_point(pixel, d, depth_intrin, outVert);
                    }
                    else
                    {
                        outVert[0] = outVert[1] = outVert[2] = 0;
                    }
                    outVert += 3;
                }
            }
        }

        if (colorFrame.updated)
        {
            std::lock_guard<std::mutex> guard(frameMutex);
            colorFrame.swap_front();
        }
    }
    
} // end namespace r200
#endif
