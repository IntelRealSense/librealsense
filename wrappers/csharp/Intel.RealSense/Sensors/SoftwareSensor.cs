// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;

    public class SoftwareSensor : Sensor
    {
        internal SoftwareSensor(IntPtr ptr)
            : base(ptr)
        {
        }

        public void AddVideoFrame(SoftwareVideoFrame f)
        {
            object error;
            NativeMethods.rs2_software_sensor_on_video_frame(Handle, f, out error);
        }

        public void AddVideoFrame<T>(T[] pixels, int stride, int bpp, double timestamp, TimestampDomain domain, int frameNumber, VideoStreamProfile profile)
        {
            // TODO: avoid copy by adding void* user_data to native methods, so we can pass GCHandle.ToIntPtr() and free in deleter
            IntPtr hglobal = Marshal.AllocHGlobal(profile.Height * stride);

            var handle = GCHandle.Alloc(pixels, GCHandleType.Pinned);

            try
            {
                NativeMethods.Memcpy(hglobal, handle.AddrOfPinnedObject(), profile.Height * stride);
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
                profile = profile.Handle
            });
        }

        public void AddMotionFrame(SoftwareMotionFrame f)
        {
            object error;
            NativeMethods.rs2_software_sensor_on_motion_frame(Handle, f, out error);
        }

        public void AddPoseFrame(SoftwarePoseFrame f)
        {
            object error;
            NativeMethods.rs2_software_sensor_on_pose_frame(Handle, f, out error);
        }

        public VideoStreamProfile AddVideoStream(SoftwareVideoStream profile)
        {
            object error;
            var ptr = NativeMethods.rs2_software_sensor_add_video_stream(Handle, profile, out error);
            return StreamProfile.Create<VideoStreamProfile>(ptr);
        }

        public MotionStreamProfile AddMotionStream(SoftwareMotionStream profile)
        {
            object error;
            var ptr = NativeMethods.rs2_software_sensor_add_motion_stream(Handle, profile, out error);
            return StreamProfile.Create<MotionStreamProfile>(ptr);
        }

        public PoseStreamProfile AddPoseStream(SoftwarePoseStream profile)
        {
            object error;
            var ptr = NativeMethods.rs2_software_sensor_add_pose_stream(Handle, profile, out error);
            return StreamProfile.Create<PoseStreamProfile>(ptr);
        }

        /// <summary>
        /// Set frame metadata for the upcoming frames
        /// </summary>
        /// <param name="type">metadata key to set</param>
        /// <param name="value">metadata value</param>
        public void SetMetadata(FrameMetadataValue type, long value)
        {
            object error;
            NativeMethods.rs2_software_sensor_set_metadata(Handle, value, type, out error);
        }

        /// <summary>
        /// Register option that will be supported by the sensor
        /// </summary>
        /// <param name="opt">the option</param>
        /// <param name="value">the initial value</param>
        public void AddReadOnlyOption(Option opt, float value)
        {
            object error;
            NativeMethods.rs2_software_sensor_add_read_only_option(Handle, opt, value, out error);
        }

        /// <summary>
        /// Update value of registered option
        /// </summary>
        /// <param name="opt">the option</param>
        /// <param name="value">updated value</param>
        public void UpdateReadOnlyOption(Option opt, float value)
        {
            object error;
            NativeMethods.rs2_software_sensor_update_read_only_option(Handle, opt, value, out error);
        }
    }
}
