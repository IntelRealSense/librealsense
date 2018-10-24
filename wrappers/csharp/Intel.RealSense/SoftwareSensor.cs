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

        public void AddVideoFrame(SoftwareVideoFrame f, IntPtr user_data) 
            => NativeMethods.rs2_software_sensor_on_video_frame(Instance, f, user_data, out var error);
        public void AddVideoFrame<T>(T[] pixels, int stride, int bpp, double timestamp, TimestampDomain domain, int frameNumber, VideoStreamProfile profile)
        {
            var h = GCHandle.Alloc(pixels, GCHandleType.Pinned);

            AddVideoFrame(new SoftwareVideoFrame
            {
                pixels = h.AddrOfPinnedObject(),
                deleter = (f, p) => { GCHandle.FromIntPtr(p).Free(); },
                stride = stride,
                bpp = bpp,
                timestamp = timestamp,
                domain = domain,
                frame_number = frameNumber,
                profile = profile.Instance.Handle
            }, GCHandle.ToIntPtr(h));
        }
        public void AddVideoFrame(IntPtr pixels, int stride, int bpp, double timestamp, TimestampDomain domain, int frameNumber, VideoStreamProfile profile)
        {
            AddVideoFrame(new SoftwareVideoFrame
            {
                pixels = pixels,
                deleter = delegate { },
                stride = stride,
                bpp = bpp,
                timestamp = timestamp,
                domain = domain,
                frame_number = frameNumber,
                profile = profile.Instance.Handle
            }, IntPtr.Zero);
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
