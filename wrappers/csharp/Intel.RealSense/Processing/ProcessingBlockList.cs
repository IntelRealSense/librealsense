// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    internal sealed class ProcessingBlockList : Base.Object, IEnumerable<ProcessingBlock>, ICollection
    {
        public ProcessingBlockList(IntPtr ptr)
            : base(ptr, NativeMethods.rs2_delete_recommended_processing_blocks)
        {
        }

        public IEnumerator<ProcessingBlock> GetEnumerator()
        {
            object error;

            int deviceCount = NativeMethods.rs2_get_recommended_processing_blocks_count(Handle, out error);
            for (int i = 0; i < deviceCount; i++)
            {
                var ptr = NativeMethods.rs2_get_processing_block(Handle, i, out error);
                yield return new ProcessingBlock(ptr);
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        public int Count
        {
            get
            {
                object error;
                int deviceCount = NativeMethods.rs2_get_recommended_processing_blocks_count(Handle, out error);
                return deviceCount;
            }
        }

        public object SyncRoot => this;

        public bool IsSynchronized => false;

        public T GetProcessingBlock<T>(int index)
            where T : ProcessingBlock
        {
            object error;
            return new ProcessingBlock(NativeMethods.rs2_get_processing_block(Handle, index, out error)) as T;
        }

        public void CopyTo(Array array, int index)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }

            for (int i = 0; i < Count; i++)
            {
                array.SetValue(this[i], i + index);
            }
        }

        public ProcessingBlock this[int index]
        {
            get
            {
                return GetProcessingBlock<ProcessingBlock>(index);
            }
        }
    }
}
