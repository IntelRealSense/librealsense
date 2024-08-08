// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Video stream profile instance which contains additional video attributes
    /// </summary>
    public class VideoStreamProfile : StreamProfile
    {
        internal override void Initialize()
        {
            base.Initialize();
            object error;
            NativeMethods.rs2_get_video_stream_resolution(Handle, out width, out height, out error);
        }

        internal VideoStreamProfile(IntPtr ptr)
            : base(ptr)
        {
            this.Initialize();
        }

        /// <summary>
        /// Returns this profile's <see cref="Intrinsics"/>
        /// </summary>
        /// <returns>resulting intrinsics for the video profile</returns>
        public Intrinsics GetIntrinsics()
        {
            object error;
            Intrinsics intrinsics;
            NativeMethods.rs2_get_video_stream_intrinsics(Handle, out intrinsics, out error);
            return intrinsics;
        }


        
        
        /// <summary>
        /// Clone current profile and change the type, index and format to input parameters
        /// </summary>
        /// <param name="type">will change the stream type from the cloned profile.</param>
        /// <param name="index">will change the stream index from the cloned profile.</param>
        /// <param name="format">will change the stream format from the cloned profile.</param>
        /// <param name="width">will change the width of the profile.</param>
        /// <param name="height">will change the height of the profile.</param>
        /// <param name="intr">will change the intrinsics of the profile.</param>
        /// <returns>the cloned stream profile.</returns>
        public StreamProfile Clone(Stream type, int index, Format format, int width, int height, Intrinsics intr)
        {
            object error;
            var ptr = NativeMethods.rs2_clone_video_stream_profile(Handle, type, index, format, width, height, intr, out error);
            var p = StreamProfile.Create<VideoStreamProfile>(ptr);
            p.clone = new Base.DeleterHandle(ptr, StreamProfileReleaser);
            return p;
        }

        /// <summary>
        /// Gets the width in pixels of the video stream
        /// </summary>
        public int Width
        {
            get { return width; }
        }

        /// <summary>
        /// Gets the height in pixels of the video stream
        /// </summary>
        public int Height
        {
            get { return height; }
        }

        private int width;
        private int height;
    }
}
