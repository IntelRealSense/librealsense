// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;
    using Intel.RealSense;

    public class PoseFrame : Frame
    {
        public PoseFrame(IntPtr ptr)
            : base(ptr)
        {
        }

        /// <summary>
        /// Gets the transformation represented by the pose data
        /// </summary>
        public Pose PoseData
        {
            get
            {
                object error;
                Pose pose = new Pose();
                NativeMethods.rs2_pose_frame_get_pose_data(Handle, pose, out error);
                return pose;
            }
        }

        /// <summary>
        /// Copy pose data to managed object
        /// </summary>
        /// <typeparam name="T">struct type with layout matching <see cref="Pose"/></typeparam>
        /// <param name="pose">object to copy data to</param>
        public void CopyTo<T>(out T pose)
            where T : struct
        {
            pose = (T)Marshal.PtrToStructure(Data, typeof(T));
        }

        /// <summary>
        /// Copy pose data to managed object
        /// </summary>
        /// <typeparam name="T">class type with layout matching <see cref="Pose"/></typeparam>
        /// <param name="pose">object to copy data to</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="pose"/> is null</exception>
        public void CopyTo<T>(T pose)
            where T : class
        {
            if (pose == null)
            {
                throw new ArgumentNullException(nameof(pose));
            }

            Marshal.PtrToStructure(Data, pose);
        }
    }
}
