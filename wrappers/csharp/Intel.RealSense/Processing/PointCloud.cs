using Intel.RealSense.Frames;
using Intel.RealSense.Types;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Processing
{
    public class PointCloud : ProcessingBlock
    {
        private readonly FrameQueue queue;

        public PointCloud()
        {
            queue = new FrameQueue(1);
            Instance = new HandleRef(this, NativeMethods.rs2_create_pointcloud(out var error));
            NativeMethods.rs2_start_processing_queue(Instance.Handle, queue.Instance.Handle, out error);
        }

        public Points Calculate(Frame original, FramesReleaser releaser = null)
        {
            NativeMethods.rs2_frame_add_ref(original.Instance.Handle, out var error);
            NativeMethods.rs2_process_frame(Instance.Handle, original.Instance.Handle, out error);
            return FramesReleaser.ScopedReturn(releaser, queue.WaitForFrame() as Points);
        }

        public void MapTexture(VideoFrame texture)
        {
            Options[Option.StreamFilter].Value = Convert.ToSingle(texture.Profile.Stream);
            Options[Option.StreamFormatFilter].Value = Convert.ToSingle(texture.Profile.Format);
            Options[Option.StreamIndexFilter].Value = Convert.ToSingle(texture.Profile.Index);
            NativeMethods.rs2_frame_add_ref(texture.Instance.Handle, out var error);
            NativeMethods.rs2_process_frame(Instance.Handle, texture.Instance.Handle, out error);
        }
    }
}