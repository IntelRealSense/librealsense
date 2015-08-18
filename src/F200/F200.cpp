#include "../Common.h"
#include "F200.h"
#include "Projection.h"
#include "../../include/librealsense/rsutil.h"

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

    static std::array<float,3> DeprojectPixelToPoint(const CameraCalibrationParameters & p, int pixelX, int pixelY, uint16_t d)
    {
        // Compute depth "UVs" in the [-1,+1] range
        const float u = float(pixelX) / 640 * 2 - 1;
        const float v = float(pixelY) / 480 * 2 - 1;

        // Distort camera coordinates
        float xc  = (u - p.Kc[0][2]) / p.Kc[0][0];
        float yc  = (v - p.Kc[1][2]) / p.Kc[1][1];
        float r2  = xc*xc + yc*yc;
        float r2c = 1 + p.Invdistc[0]*r2 + p.Invdistc[1]*r2*r2 + p.Invdistc[4]*r2*r2*r2;
        float xcd = xc*r2c + 2*p.Invdistc[2]*xc*yc + p.Invdistc[3]*(r2 + 2*xc*xc);
        float ycd = yc*r2c + 2*p.Invdistc[3]*xc*yc + p.Invdistc[2]*(r2 + 2*yc*yc);
        // Note: unprojCoeffs[y*640+x] == Point(xcd, ycd)
        // Note: buildUVCoeffs[y*640+x] == UVPreComp(p.Pt[0][0]*xcd + p.Pt[0][1]*ycd + p.Pt[0][2],
        //                                           p.Pt[1][0]*xcd + p.Pt[1][1]*ycd + p.Pt[1][2],
        //                                           p.Pt[2][0]*xcd + p.Pt[2][1]*ycd + p.Pt[2][2]);

        // Determine location of vertex, relative to depth camera, in mm
        const auto uint16_to_mm_ratio = p.Rmax / 65535.0f;
        const float z = d * uint16_to_mm_ratio, x = xcd * z, y = ycd * z;
        return {x, y, z};
    }

    const uint16_t * F200Camera::GetDepthImage()
    {
        if (depthFrame.updated)
        {
            std::lock_guard<std::mutex> guard(frameMutex);
            depthFrame.swap_front();

            auto inst = Projection::GetInstance();
            if (!inst->m_calibration) throw std::runtime_error("MapDepthToColorCoordinates failed, m_calibration not initialized");
            const int xShift = (inst->m_currentDepthWidth == 320) ? 1 : 0;
            const int yShift = (inst->m_currentDepthHeight == 240) ? 1 : 0;
            const CameraCalibrationParameters & p = inst->m_calibration.params;          

            // Produce intrinsics for color camera
            rs_intrinsics colorIntrin;
            colorIntrin.focal_length[0] = p.Kt[0][0]*0.5f;
            colorIntrin.focal_length[1] = p.Kt[1][1]*0.5f;
            colorIntrin.principal_point[0] = p.Kt[0][2]*0.5f + 0.5f;
            colorIntrin.principal_point[1] = p.Kt[1][2]*0.5f + 0.5f;
            if(inst->m_currentColorWidth * 3 == inst->m_currentColorHeight * 4) // If using a 4:3 aspect ratio, adjust intrinsics (defaults to 16:9)
            {
                colorIntrin.focal_length[0] *= 4.0f/3;
                colorIntrin.principal_point[0] *= 4.0f/3;
                colorIntrin.principal_point[0] -= 1.0f/6;
            }
            colorIntrin.image_size[0] = 640;
            colorIntrin.image_size[1] = 480;
            colorIntrin.focal_length[0] *= 640;
            colorIntrin.focal_length[1] *= 480;
            colorIntrin.principal_point[0] *= 640;
            colorIntrin.principal_point[1] *= 480;

            // Produce extrinsics between depth and color camera
            rs_extrinsics extrin;
            for(int i=0; i<3; ++i) for(int j=0; j<3; ++j) extrin.rotation[i*3+j] = p.Rt[i][j];
            for(int i=0; i<3; ++i) extrin.translation[i] = p.Tt[i];

            uvs.resize(640*480*2);
            vertices.resize(640*480*3);
            auto inDepth = reinterpret_cast<const uint16_t *>(depthFrame.front.data());
            auto vert = vertices.data();
            auto outUV = uvs.data();
            for(int i=0; i<inst->m_currentDepthHeight; i++)
            {
                for (int j=0 ; j<inst->m_currentDepthWidth; j++)
                {
                    if (uint16_t d = *inDepth++)
                    {
                        // Produce vertex location relative to depth camera
                        int pixelX = j << xShift, pixelY = i << yShift;
                        const auto point = DeprojectPixelToPoint(p, pixelX, pixelY, d);
                        *vert++ = point[0];
                        *vert++ = point[1];
                        *vert++ = point[2];

                        // Also produce the point and pixel relative to color camera
                        float colorPoint[3], colorPixel[2];
                        rs_transform_point_to_point(point.data(), extrin, colorPoint);
                        rs_project_point_to_rectified_pixel(colorPoint, colorIntrin, colorPixel);
                        *outUV++ = colorPixel[0] / 640;
                        *outUV++ = colorPixel[1] / 480;
                    }
                    else
                    {
                        *vert++ = 0;
                        *vert++ = 0;
                        *vert++ = 0;
                        *outUV++ = 0;
                        *outUV++ = 0;
                    }
                }
            }

            rectifier->rectify(reinterpret_cast<const uint16_t *>(depthFrame.front.data()), uvs.data());
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

} // end f200

#endif
