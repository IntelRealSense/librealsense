#include "../Common.h"
#include "F200.h"
#include "Projection.h"

#ifndef WIN32

using namespace rs;

namespace f200
{
    
    F200Camera::F200Camera(uvc_context_t * ctx, uvc_device_t * device, int idx) : UVCCamera(ctx, device, idx)
    {
        
    }

    F200Camera::~F200Camera()
    {
        
    }

    void F200Camera::RetrieveCalibration()
    {
        hardware_io.reset(new IVCAMHardwareIO(internalContext));
        
        // Set up calibration parameters for internal undistortion
        
        const CameraCalibrationParameters & calib = hardware_io->GetParameters();
        const OpticalData & od = hardware_io->GetOpticalData();
        
        rs_intrinsics rect = {}, unrect = {};
        rs_extrinsics rotation = {{1.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f,1.f},{0.f,0.f,0.f}};
        
        const int ivWidth = 640;
        const int ivHeight = 480;
        
        rect.image_size[0] = ivWidth;
        rect.image_size[1] = ivHeight;
        rect.focal_length[0] = od.IRUndistortedFocalLengthPxl.x;
        rect.focal_length[1] = od.IRUndistortedFocalLengthPxl.y;
        rect.principal_point[0] = (ivWidth / 2); // Leo hack not to use real pp
        rect.principal_point[1] = (ivHeight / 2); // Ditto
        
        unrect.image_size[0] = ivWidth;
        unrect.image_size[1] = ivHeight;
        unrect.focal_length[0] = od.IRDistortedFocalLengthPxl.x;
        unrect.focal_length[1] =  od.IRDistortedFocalLengthPxl.x; // Leo hack for "Squareness"
        unrect.principal_point[0] = od.IRPrincipalPoint.x + (ivWidth / 2);
        unrect.principal_point[1] = od.IRPrincipalPoint.y + (ivHeight / 2);
        unrect.distortion_coeff[0] = calib.Distc[0];
        unrect.distortion_coeff[1] = calib.Distc[1];
        unrect.distortion_coeff[2] = calib.Distc[2];
        unrect.distortion_coeff[3] = calib.Distc[3];
        unrect.distortion_coeff[4] = calib.Distc[4];

        rectifier.reset(new IVRectifier(ivWidth, ivHeight, rect, unrect, rotation));

        return true;
    }

    void F200Camera::StopStream(int streamNum)
    {
        //@tofix
    }
        
    rs_intrinsics F200Camera::GetStreamIntrinsics(int stream)
    {
        const CameraCalibrationParameters & calib = hardware_io->GetParameters();
        const OpticalData & od = hardware_io->GetOpticalData();
        switch(stream)
        {
        case RS_STREAM_DEPTH: return {{640,480},{od.IRUndistortedFocalLengthPxl.x,od.IRUndistortedFocalLengthPxl.y},{320,240},{1,0,0,0,0}};
        case RS_STREAM_RGB: return {{640,480},{od.RGBUndistortedFocalLengthPxl.x,od.RGBUndistortedFocalLengthPxl.y},{320,240},{calib.Distt[0],calib.Distt[1],calib.Distt[2],calib.Distt[3],calib.Distt[4]}};
        default: throw std::runtime_error("unsupported stream");
        }
    }

    rs_extrinsics F200Camera::GetStreamExtrinsics(int from, int to)
    {
        const CameraCalibrationParameters & calib = hardware_io->GetParameters();
        float scale = 0.001f / GetDepthScale();
        return {{calib.Rt[0][0], calib.Rt[0][1], calib.Rt[0][2],
                 calib.Rt[1][0], calib.Rt[1][1], calib.Rt[1][2],
                 calib.Rt[2][0], calib.Rt[2][1], calib.Rt[2][2]},
                {calib.Tt[0] * scale, calib.Tt[1] * scale, calib.Tt[2] * scale}};
    }
        
    float F200Camera::GetDepthScale()
    {
        IVCAMCalibrator<float> * calibration = Projection::GetInstance()->GetCalibrationObject();
        return calibration->ivcamToMM(1) * 0.001f;
    }

    const uint16_t * F200Camera::GetDepthImage()
    {
        if (depthFrame.updated)
        {
            std::lock_guard<std::mutex> guard(frameMutex);
            depthFrame.swap_front();

            float uvs[640 * 480 * 2];

            auto inst = Projection::GetInstance();
            if (!inst->m_calibration) throw std::runtime_error("MapDepthToColorCoordinates failed, m_calibration not initialized");
            const int xShift = (inst->m_currentDepthWidth == 320) ? 1 : 0;
            const int yShift = (inst->m_currentDepthHeight == 240) ? 1 : 0;
            const bool aspectRatio43 = (inst->m_currentColorWidth * 3 == inst->m_currentColorHeight * 4);
            const CameraCalibrationParameters & p = inst->m_calibration.params;

            auto inDepth = reinterpret_cast<const uint16_t *>(depthFrame.front.data());
            auto outUV = uvs;
            for(int i=0; i<inst->m_currentDepthHeight; i++)
            {
                for (int j=0 ; j<inst->m_currentDepthWidth; j++)
                {
                    if (uint16_t d = *inDepth++)
                    {
                        int pixelX = j << xShift, pixelY = i << yShift;
                        const Point point = inst->m_calibration.computeUnprojectCoeffs(pixelX, pixelY); // unprojCoeffs[y*640+x]
                        const UVPreComp puvc = inst->m_calibration.computeBuildUVCoeffs(point); // buildUVCoeffs[y*640+x]

                        const float z = inst->m_calibration.ivcamToMM(d);
                        const float D = 0.5f / (puvc.d*z + p.Pt[2][3]);
                        float u = (puvc.u*z + p.Pt[0][3]) * D + 0.5f;
                        float v = (puvc.v*z + p.Pt[1][3]) * D + 0.5f;
                        if (aspectRatio43) u = u * (4.0f/3) - (1.0f/6);

                        *outUV++ = u;
                        *outUV++ = v;
                    }
                    else
                    {
                        *outUV++ = 0;
                        *outUV++ = 0;
                    }
                }
            }

            rectifier->rectify(reinterpret_cast<const uint16_t *>(depthFrame.front.data()), uvs);
        }
        return rectifier->getDepth();
    }
        
    const uint8_t * F200Camera::GetColorImage()
    {
        if (colorFrame.updated)
        {
            std::lock_guard<std::mutex> guard(frameMutex);
            colorFrame.swap_front();
        }
        return reinterpret_cast<const uint8_t *>(colorFrame.front.data());
    }

    const float * F200Camera::GetUVMap() { return rectifier->getUVs(); }

} // end f200

#endif
