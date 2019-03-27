// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Runtime.InteropServices;

    /// <summary>
    /// The pipeline profile includes a device and a selection of active streams, with specific profile.
    /// <para>
    /// The profile is a selection of the above under filters and conditions defined by the pipeline.
    /// Streams may belong to more than one sensor of the device.
    /// </para>
    /// </summary>
    public class PipelineProfile : Base.Object
    {
        public PipelineProfile(IntPtr ptr)
            : base(ptr, NativeMethods.rs2_delete_pipeline_profile)
        {
        }

        /// <summary>
        /// Gets the device used by the pipeline.
        /// </summary>
        public Device Device
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_pipeline_profile_get_device(Handle, out error);
                return Device.Create<Device>(ptr, NativeMethods.rs2_delete_device);
            }
        }

        /// <summary>
        /// Gets the selected streams profiles, which are enabled in this profile.
        /// </summary>
        public ReadOnlyCollection<StreamProfile> Streams
        {
            get
            {
                object error;
                using (var pl = new StreamProfileList(NativeMethods.rs2_pipeline_profile_get_streams(Handle, out error)))
                {
                    var profiles = new StreamProfile[pl.Count];
                    pl.CopyTo(profiles, 0);
                    return Array.AsReadOnly(profiles);
                }
            }
        }

        public StreamProfile GetStream(Stream s, int index = -1)
        {
            return GetStream<StreamProfile>(s, index);
        }

        /// <summary>
        /// Return the selected stream profile, which are enabled in this profile.
        /// </summary>
        /// <typeparam name="T"><see cref="StreamProfile"/> type or subclass</typeparam>
        /// <param name="s">Stream type of the desired profile</param>
        /// <param name="index">Stream index of the desired profile. -1 for any matching.</param>
        /// <returns>The first matching stream profile</returns>
        /// <exception cref="ArgumentException">Thrown when the <see cref="PipelineProfile"/> does not contain the request stream</exception>
        public T GetStream<T>(Stream s, int index = -1)
            where T : StreamProfile
        {
            object error;
            using (var streams = new StreamProfileList(NativeMethods.rs2_pipeline_profile_get_streams(Handle, out error)))
            {
                int count = streams.Count;
                for (int i = 0; i < count; i++)
                {
                    var ptr = NativeMethods.rs2_get_stream_profile(streams.Handle, i, out error);
                    var t = StreamProfile.Create<T>(ptr);
                    if (t.Stream == s && (index == -1 || t.Index == index))
                    {
                        return t;
                    }

                    t.Dispose();
                }

                throw new ArgumentException("Profile does not contain the requested stream", nameof(s));
            }
        }
    }
}
