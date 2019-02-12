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

        public void AddVideoFrame(byte[] pixels, int stride, int bpp, double timestamp, TimestampDomain domain, int frameNumber, VideoStreamProfile profile)
        {
            IntPtr hglobal = Marshal.AllocHGlobal(profile.Height * stride);
            Marshal.Copy(pixels, 0, hglobal, profile.Height * stride);

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

        public VideoStreamProfile AddVideoStream(VideoStream profile)
        {
            object error;
            return new VideoStreamProfile(NativeMethods.rs2_software_sensor_add_video_stream(m_instance, profile, out error));
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
