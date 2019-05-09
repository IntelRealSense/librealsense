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
