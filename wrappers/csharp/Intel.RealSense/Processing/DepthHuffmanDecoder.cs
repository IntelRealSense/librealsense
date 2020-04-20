// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;

    public class DepthHuffmanDecoder : ProcessingBlock
    {
        private static IntPtr Create()
        {
            object error;
            return NativeMethods.rs2_create_huffman_depth_decompress_block(out error);
        }

        internal DepthHuffmanDecoder(IntPtr ptr)
            : base(ptr)
        {
        }

        public DepthHuffmanDecoder()
            : base(Create())
        {
        }

        [Obsolete("This method is obsolete. Use Process method instead")]
        public VideoFrame ApplyFilter(Frame original, FramesReleaser releaser)
        {
            return Process(original).DisposeWith(releaser) as VideoFrame;
        }
    }
}