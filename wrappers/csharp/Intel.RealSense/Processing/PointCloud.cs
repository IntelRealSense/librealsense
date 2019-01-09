using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;

namespace Intel.RealSense
{
    public class PointCloud : ProcessingBlock
    {
        private readonly IOption formatFilter;
        private readonly IOption indexFilter;
        private readonly IOption streamFilter;

        public PointCloud()
        {
            object error;
            m_instance = new HandleRef(this, NativeMethods.rs2_create_pointcloud(out error));
            NativeMethods.rs2_start_processing_queue(m_instance.Handle, queue.m_instance.Handle, out error);

            streamFilter = Options[Option.StreamFilter];
            formatFilter = Options[Option.StreamFormatFilter];
            indexFilter = Options[Option.StreamIndexFilter];
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public Points Calculate(Frame original, FramesReleaser releaser = null)
        {
            return Process(original).DisposeWith(releaser) as Points;
        }

        public void MapTexture(VideoFrame texture)
        {
            using (var p = texture.Profile) {
                streamFilter.Value = (float)p.Stream;
                formatFilter.Value = (float)p.Format;
                indexFilter.Value = (float)p.Index;
            }
            using (var f = Process(texture));
        }
    }
}