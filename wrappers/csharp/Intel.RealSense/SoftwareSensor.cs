using Intel.RealSense.Profiles;
using Intel.RealSense.Types;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class SoftwareSensor : Sensor
    {
        internal SoftwareSensor(IntPtr ptr) : base(ptr)
        {
        }

        public void AddVideoFrame(byte[] pixels, int stride, int bpp, double timestamp, TimestampDomain domain, int frameNumber, VideoStreamProfile profile)
        {
            IntPtr hglobal = Marshal.AllocHGlobal(profile.Height * stride);
            var del = new frame_deleter(p => { Marshal.FreeHGlobal(p); });
            Marshal.Copy(pixels, 0, hglobal, profile.Height * stride);

            var s = new NativeMethods.SoftwareVideoFrame
            {
                pixels = hglobal,
                deleter = del,
                stride = stride,
                bpp = bpp,
                timestamp = timestamp,
                domain = domain,
                frame_number = frameNumber,
                profile = profile.Instance.Handle
            };
            NativeMethods.rs2_software_sensor_on_video_frame(instance, s, out var error);
        }

        public VideoStreamProfile AddVideoStream(VideoStream profile)
            => new VideoStreamProfile(NativeMethods.rs2_software_sensor_add_video_stream(instance, profile, out var error));

        public void SetMetadata(FrameMetadataValue type, long value)
            => NativeMethods.rs2_software_sensor_set_metadata(instance, value, type, out var error);

        public void AddReadOnlyOption(Option opt, float value)
            => NativeMethods.rs2_software_sensor_add_read_only_option(instance, opt, value, out var error);

        public void UpdateReadOnlyOption(Option opt, float value)
            => NativeMethods.rs2_software_sensor_update_read_only_option(instance, opt, value, out var error);
    }
}
