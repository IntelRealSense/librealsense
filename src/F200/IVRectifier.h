#pragma once

#ifndef LIBREALSENSE_F200_IVRECTIFIER_H
#define LIBREALSENSE_F200_IVRECTIFIER_H

#include "../rs-internal.h"

#ifdef USE_UVC_DEVICES
#include "../UVCCamera.h"
#include "F200Types.h"

namespace f200
{
    class IVRectifier
    {
        std::vector<uint32_t> uvTable;
        std::vector<uint16_t> undistortedDepth;
        uint32_t width;
        uint32_t height;
        
        void TransformFromOtherCameraToNonRectOtherImage(const rs_intrinsics & OtherIntrinsics, const float OtherCamera[3], float OtherImage[2])
        {
            float t[2];
            t[0] = OtherCamera[0] / OtherCamera[2];
            t[1] = OtherCamera[1] / OtherCamera[2];
            
            const float * k = OtherIntrinsics.distortion_coeff;
            float r2 = t[0] * t[0] + t[1] * t[1];
            float r = static_cast<float>(1 + r2 * (k[0] + r2 * (k[1] + r2 * k[4])));
            t[0] *= r;
            t[1] *= r;
            
            OtherImage[0] = static_cast<float>(t[0] + 2 * k[2] * t[0] * t[1] + k[3] * (r2 + 2 * t[0] * t[0]));
            OtherImage[1] = static_cast<float>(t[1] + 2 * k[3] * t[0] * t[1] + k[2] * (r2 + 2 * t[1] * t[1]));
            
            OtherImage[0] = OtherIntrinsics.focal_length[0] * OtherImage[0] + OtherIntrinsics.principal_point[0];
            OtherImage[1] = OtherIntrinsics.focal_length[1] * OtherImage[1] + OtherIntrinsics.principal_point[1];
        }
        
        void TransformFromRectOtherImageToNonRectOtherImage(const rs_intrinsics & rect, const float rotation[9], const rs_intrinsics & nonRect, const float rectImage[2], float nonRectImage[2])
        {
            float rectCamera[3];
            rectCamera[0] = (rectImage[0] - rect.principal_point[0]) / rect.focal_length[0];
            rectCamera[1] = (rectImage[1] - rect.principal_point[1]) / rect.focal_length[1];
            rectCamera[2] = 1;
            
            float nonRectCamera[3];
            nonRectCamera[0] = static_cast<float>(rotation[0] * rectCamera[0] + rotation[1] * rectCamera[1] + rotation[2] * rectCamera[2]);
            nonRectCamera[1] = static_cast<float>(rotation[3] * rectCamera[0] + rotation[4] * rectCamera[1] + rotation[5] * rectCamera[2]);
            nonRectCamera[2] = static_cast<float>(rotation[6] * rectCamera[0] + rotation[7] * rectCamera[1] + rotation[8] * rectCamera[2]);
            TransformFromOtherCameraToNonRectOtherImage(nonRect, nonRectCamera, nonRectImage);
        }
        
        void BuildRectificationTable(const rs_intrinsics & sourceIntrinsics, const float rotation[9], const rs_intrinsics & destIntrinsics)
        {
            auto table = uvTable.data();
            
            for (unsigned int i = 0; i < destIntrinsics.image_size[1]; i++)
            {
                for (unsigned int j = 0; j < destIntrinsics.image_size[0]; j++)
                {
                    const float ji[2] = {(float)j, (float)i};
                    float uv[2];
                    TransformFromRectOtherImageToNonRectOtherImage(destIntrinsics, rotation, sourceIntrinsics, ji, uv);
                    /*
                        if (uv[0] < 0) uv[0] = 0;
                        if (uv[1] < 0) uv[1] = 0;
                        if (uv[0] >= sourceIntrinsics.image_size[0]) uv[0] = static_cast<float>(sourceIntrinsics.image_size[0] - 1);
                        if (uv[1] >= sourceIntrinsics.image_size[1]) uv[1] = static_cast<float>(sourceIntrinsics.image_size[1] - 1);
                    */
                    uint16_t uf = (uint16_t)(uv[0] * 32) >> 5; // 11.5 fixed point representation  11  bits integer and 5 bits fractional
                    uint16_t vf = (uint16_t)(uv[1] * 32) >> 5;
                    *table++ = uf | ((int)vf) << 16;
                }
            }
        }
        
    public:
        
        IVRectifier();

        IVRectifier(uint32_t w, uint32_t h, rs_intrinsics rect, rs_intrinsics unrect, rs_extrinsics r) : width(w), height(h)
        {
            uvTable.resize(width * height);
            undistortedDepth.resize(width * height);
            BuildRectificationTable(unrect, r.rotation, rect);
        }
        
        uint16_t * rectify(uint16_t * srcImg)
        {
            /*for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    uint32_t coeff = uvTable[y * width + x];
                    uint16_t sampleX =  coeff & 0x0000FFFF;
                    uint16_t sampleY = (coeff & 0xFFFF0000) >> 16;
                    bool invalidLocation = sampleX < 0 || sampleX >= width || sampleY < 0 || sampleY >= height;
                    undistortedDepth[y * width + x] = invalidLocation ? 0 : srcImg[sampleY * width + sampleX];
                }
            }*/
            return srcImg; //undistortedDepth.data();
        }
        
    };
    
} // end namespace f200

#endif

#endif
