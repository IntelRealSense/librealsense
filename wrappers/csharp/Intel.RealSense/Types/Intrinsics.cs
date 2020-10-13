// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Video stream intrinsics
    /// </summary>
    [System.Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct Intrinsics
    {
        /// <summary> Width of the image in pixels </summary>
        public int width;

        /// <summary> Height of the image in pixels </summary>
        public int height;

        /// <summary> Horizontal coordinate of the principal point of the image, as a pixel offset from the left edge </summary>
        public float ppx;

        /// <summary> Vertical coordinate of the principal point of the image, as a pixel offset from the top edge </summary>
        public float ppy;

        /// <summary> Focal length of the image plane, as a multiple of pixel width </summary>
        public float fx;

        /// <summary> Focal length of the image plane, as a multiple of pixel height </summary>
        public float fy;

        /// <summary> Distortion model of the image </summary>
        public Distortion model;

        /// <summary> Distortion coefficients, order: k1, k2, p1, p2, k3 </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 5)]
        public float[] coeffs;

        public override string ToString()
        {
            return $"(width:{width}, height:{height}, ppx:{ppx}, ppy:{ppy}, fx:{fx}, fy:{fy}, model:{model}, coeffs:[{coeffs[0]}, {coeffs[1]}, {coeffs[2]}, {coeffs[3]}, {coeffs[4]}])";
        }

        /// <summary>
        /// Gets the horizontal and vertical field of view, based on video intrinsics
        /// </summary>
        /// <value>horizontal and vertical field of view in degrees</value>
        public float[] FOV
        {
            get
            {
                return new float[]
                {
                    (float)(System.Math.Atan2(ppx + 0.5f, fx) + System.Math.Atan2(width - (ppx + 0.5f), fx)) * 57.2957795f,
                    (float)(System.Math.Atan2(ppy + 0.5f, fy) + System.Math.Atan2(height - (ppy + 0.5f), fy)) * 57.2957795f
                };
            }
        }
    }
}
