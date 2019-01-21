using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    /// <summary>
    /// Video stream intrinsics
    /// </summary>
    [System.Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct Intrinsics
    {
        public int width;       /** Width of the image in pixels */
        public int height;      /** Height of the image in pixels */
        public float ppx;       /** Horizontal coordinate of the principal point of the image, as a pixel offset from the left edge */
        public float ppy;       /** Vertical coordinate of the principal point of the image, as a pixel offset from the top edge */
        public float fx;        /** Focal length of the image plane, as a multiple of pixel width */
        public float fy;        /** Focal length of the image plane, as a multiple of pixel height */
        public Distortion model;     /** Distortion model of the image */

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 5)]
        public float[] coeffs; /** Distortion coefficients */

        public override string ToString()
        {
            return String.Format("(width:{0}, height:{1}, ppx:{2}, ppy:{3}, fx:{4}, fy:{5}, model:{6}, coeffs:[{7}])",
                width,
                height,
                ppx,
                ppy,
                fx,
                fy,
                model,
                String.Join(", ", Array.ConvertAll(coeffs, Convert.ToString))
            );
        }
    }
}
