using System;
using Intel.RealSense;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class PoseFrame : Frame
    {
        public PoseFrame(IntPtr ptr) : base(ptr)
        {
        }

        public Pose PoseData {
            get
            {
                object error;
                Pose pose = new Pose();
                NativeMethods.rs2_pose_frame_get_pose_data(m_instance.Handle, pose, out error);
                return pose;
            }
        }

        public void CopyTo<T>(out T pose) where T : struct
        {
            pose = (T)Marshal.PtrToStructure(Data, typeof(T));
        }

        public void CopyTo<T>(T pose) where T : class
        {
            if (pose == null)
                throw new ArgumentNullException(nameof(pose));
            Marshal.PtrToStructure(Data, pose);
        }
    }
}
