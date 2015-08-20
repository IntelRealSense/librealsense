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

    static ResolutionMode MakeColorMode(const CameraCalibrationParameters & c, int w, int h)
    {
        rs_intrinsics intrin = {{w,h}};
        intrin.focal_length[0] = c.Kt[0][0]*0.5f;
        intrin.focal_length[1] = c.Kt[1][1]*0.5f;
        intrin.principal_point[0] = c.Kt[0][2]*0.5f + 0.5f;
        intrin.principal_point[1] = c.Kt[1][2]*0.5f + 0.5f;
        if(w*3 == h*4) // If using a 4:3 aspect ratio, adjust intrinsics (defaults to 16:9)
        {
            intrin.focal_length[0] *= 4.0f/3;
            intrin.principal_point[0] *= 4.0f/3;
            intrin.principal_point[0] -= 1.0f/6;
        }
        intrin.focal_length[0] *= w;
        intrin.focal_length[1] *= h;
        intrin.principal_point[0] *= w;
        intrin.principal_point[1] *= h;
        return {RS_COLOR, w,h,60,RS_RGB, w,h,60,UVC_FRAME_FORMAT_YUYV, intrin};
    }

    void F200Camera::RetrieveCalibration()
    {
        if(!hardware_io)
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

            modes.push_back({RS_DEPTH, 640,480,60,RS_Z16, 640,480,60,UVC_FRAME_FORMAT_INVR, unrect});
            modes.push_back(MakeColorMode(calib, 1920, 1080));
            modes.push_back(MakeColorMode(calib, 640, 480));
        }
    }

    rs_extrinsics F200Camera::GetStreamExtrinsics(int from, int to)
    {
        if(from == RS_DEPTH && to == RS_COLOR)
        {
            const CameraCalibrationParameters & calib = hardware_io->GetParameters();
            return {{calib.Rt[0][0], calib.Rt[0][1], calib.Rt[0][2],
                     calib.Rt[1][0], calib.Rt[1][1], calib.Rt[1][2],
                     calib.Rt[2][0], calib.Rt[2][1], calib.Rt[2][2]},
                    {calib.Tt[0], calib.Tt[1], calib.Tt[2]}};
        }
        else throw std::runtime_error("unsupported streams");
    }

    struct float2 { float x,y; };
    float2 distort(const float2 & coord, const float coeff[5])
    {
        float r2  = coord.x*coord.x + coord.y*coord.y;
        float r2c = 1 + coeff[0]*r2 + coeff[1]*r2*r2 + coeff[4]*r2*r2*r2;
        return {coord.x*r2c + 2*coeff[2]*coord.x*coord.y + coeff[3]*(r2 + 2*coord.x*coord.x),
                coord.y*r2c + 2*coeff[3]*coord.x*coord.y + coeff[2]*(r2 + 2*coord.y*coord.y)};
        // ??? DS4 model multiplies coord.x/coord.y by r2c BEFORE using them in final line, IVCAM model multiplies after
    }

    void F200Camera::ComputeVertexImage()
    {
        auto inst = Projection::GetInstance();
        if (!inst->m_calibration) throw std::runtime_error("MapDepthToColorCoordinates failed, m_calibration not initialized");
        const CameraCalibrationParameters & p = inst->m_calibration.params;
        const auto uint16_to_mm_ratio = p.Rmax / 65535.0f;

        auto inDepth = GetDepthImage();
        auto vert = vertices.data();
        for(int i=0; i<inst->m_currentDepthHeight; i++)
        {
            for (int j=0 ; j<inst->m_currentDepthWidth; j++)
            {
                if (uint16_t d = *inDepth++)
                {
                    // Compute depth image location in the [-1,+1] range
                    const float u = float(j) / inst->m_currentDepthWidth * 2 - 1;
                    const float v = float(i) / inst->m_currentDepthHeight * 2 - 1;

                    // Undistort camera coordinates
                    float2 dist = {(u - p.Kc[0][2]) / p.Kc[0][0], (v - p.Kc[1][2]) / p.Kc[1][1]};

                    auto undist = distort(dist, p.Invdistc);
                    // Note: unprojCoeffs[y*640+x] == Point(undist.x, undist.y)
                    // Note: buildUVCoeffs[y*640+x] == UVPreComp(p.Pt[0][0]*undist.x + p.Pt[0][1]*undist.y + p.Pt[0][2],
                    //                                           p.Pt[1][0]*undist.x + p.Pt[1][1]*undist.y + p.Pt[1][2],
                    //                                           p.Pt[2][0]*undist.x + p.Pt[2][1]*undist.y + p.Pt[2][2]);

                    // Determine location of vertex, relative to depth camera, in mm
                    const float z = d * uint16_to_mm_ratio;

                    *vert++ = z*undist.x;
                    *vert++ = z*undist.y;
                    *vert++ = z;
                }
                else
                {
                    *vert++ = 0;
                    *vert++ = 0;
                    *vert++ = 0;
                }
            }
        }
    }

} // end f200

#endif
