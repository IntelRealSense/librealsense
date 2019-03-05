using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class SoftwareSensor : Sensor
    {
        internal SoftwareSensor(IntPtr ptr) : base(ptr)
        {
        }

        public void AddVideoFrame(SoftwareVideoFrame f)
        {
            object error;
            NativeMethods.rs2_software_sensor_on_video_frame(m_instance, f, out error);
        }


        public void AddVideoFrame<T>(T[] pixels, int stride, int bpp, double timestamp, TimestampDomain domain, int frameNumber, VideoStreamProfile profile)
        {
            //TODO: avoid copy by adding void* user_data to native methods, so we can pass GCHandle.ToIntPtr() and free in deleter
            IntPtr hglobal = Marshal.AllocHGlobal(profile.Height * stride);

            var handle = GCHandle.Alloc(pixels, GCHandleType.Pinned);
            
            try
            {
                NativeMethods.memcpy(hglobal, handle.AddrOfPinnedObject(), profile.Height * stride);
            }
            finally
            {
                handle.Free();
            }

            AddVideoFrame(new SoftwareVideoFrame
            {
                pixels = hglobal,
                deleter = (p) => { Marshal.FreeHGlobal(p); },
                stride = stride,
                bpp = bpp,
                timestamp = timestamp,
                domain = domain,
                frame_number = frameNumber,
                profile = profile.m_instance.Handle
            });
        }

        public void AddMotionFrame(SoftwareMotionFrame f)
        {
            object error;
            NativeMethods.rs2_software_sensor_on_motion_frame(m_instance, f, out error);
        }

        public void AddPoseFrame(SoftwarePoseFrame f)
        {
            object error;
            NativeMethods.rs2_software_sensor_on_pose_frame(m_instance, f, out error);
        }

        public VideoStreamProfile AddVideoStream(SoftwareVideoStream profile)
        {
            object error;
            var ptr = NativeMethods.rs2_software_sensor_add_video_stream(m_instance, profile, out error);
            return StreamProfile.Create<VideoStreamProfile>(ptr);
        }

        public MotionStreamProfile AddMotionStream(SoftwareMotionStream profile)
        {
            object error;
            var ptr = NativeMethods.rs2_software_sensor_add_motion_stream(m_instance, profile, out error);
            return StreamProfile.Create<MotionStreamProfile>(ptr);
        }

        public PoseStreamProfile AddMotionStream(SoftwarePoseStream profile)
        {
            object error;
            var ptr = NativeMethods.rs2_software_sensor_add_pose_stream(m_instance, profile, out error);
            return StreamProfile.Create<PoseStreamProfile>(ptr);
        }

        public void SetMetadata(FrameMetadataValue type, long value)
        {
            object error;
            NativeMethods.rs2_software_sensor_set_metadata(m_instance, value, type, out error);
        }

        public void AddReadOnlyOption(Option opt, float value)
        {
            object error;
            NativeMethods.rs2_software_sensor_add_read_only_option(m_instance, opt, value, out error);
        }

        public void UpdateReadOnlyOption(Option opt, float value)
        {
            object error;
            NativeMethods.rs2_software_sensor_update_read_only_option(m_instance, opt, value, out error);
        }
    }
}
