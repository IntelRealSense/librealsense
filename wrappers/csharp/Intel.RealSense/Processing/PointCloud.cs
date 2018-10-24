using Intel.RealSense.Frames;
using Intel.RealSense.Types;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Processing
{
    public class PointCloud : ProcessingBlock
    {
        private readonly IOption formatFilter;
        private readonly IOption indexFilter;
        private readonly IOption streamFilter;

        public PointCloud()
        {
            Instance = new HandleRef(this, NativeMethods.rs2_create_pointcloud(out var error));
            NativeMethods.rs2_start_processing_queue(Instance.Handle, queue.Instance.Handle, out error);

            streamFilter = Options[Option.StreamFilter];
            formatFilter = Options[Option.StreamFormatFilter];
            indexFilter = Options[Option.StreamIndexFilter];
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public Points Calculate(Frame original, FramesReleaser releaser = null)
            => Process(original).DisposeWith(releaser) as Points;

        public void MapTexture(VideoFrame texture)
        {
            using (var p = texture.Profile)
            {
                streamFilter.Value = (float)p.Stream;
                formatFilter.Value = (float)p.Format;
                indexFilter.Value = p.Index;
            }

#pragma warning disable CS0642 // empty statement
            using (var f = Process(texture)) ;
#pragma warning restore CS0642 // empty statement
        }
    }
}