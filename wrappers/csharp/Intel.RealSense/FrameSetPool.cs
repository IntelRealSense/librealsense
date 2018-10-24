using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Intel.RealSense
{
    public class FrameSetPool
    {
        private readonly ConcurrentStack<FrameSet> stack;

        public FrameSetPool()
        {
            stack = new ConcurrentStack<FrameSet>();
        }

        public FrameSet Get(IntPtr ptr)
        {
            if (stack.TryPop(out FrameSet frameSet))
            {
                frameSet.Instance = new HandleRef(frameSet, ptr);
                frameSet.disposedValue = false;
                frameSet.Count = NativeMethods.rs2_embedded_frames_count(frameSet.Instance.Handle, out var error);
                frameSet.enumerator.Reset();
                frameSet.disposables.Clear();
                return frameSet;
            }
            else
            {
                return new FrameSet(ptr);
            }
        }

        public void Release(FrameSet t)
        {
            stack.Push(t);
        }
    }
}
