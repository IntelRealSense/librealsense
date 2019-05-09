// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;

    public class PointCloud : ProcessingBlock
    {
        private readonly IOption formatFilter;
        private readonly IOption indexFilter;
        private readonly IOption streamFilter;

        private static IntPtr Create()
        {
            object error;
            return NativeMethods.rs2_create_pointcloud(out error);
        }

        internal PointCloud(IntPtr ptr)
            : base(ptr)
        {
        }

        public PointCloud()
            : base(Create())
        {
            streamFilter = Options[Option.StreamFilter];
            formatFilter = Options[Option.StreamFormatFilter];
            indexFilter = Options[Option.StreamIndexFilter];
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public Points Calculate(Frame original, FramesReleaser releaser = null)
        {
            return Process(original).DisposeWith(releaser).As<Points>();
        }

        public void MapTexture(VideoFrame texture)
        {
            using (var p = texture.Profile)
            {
                streamFilter.Value = (float)p.Stream;
                formatFilter.Value = (float)p.Format;
                indexFilter.Value = (float)p.Index;
            }

            using (var f = Process(texture))
            {
                // Intentionally empty
            }
        }
    }
}