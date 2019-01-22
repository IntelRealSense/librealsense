using System;
#if NET35
/*
 * ---------------------------------------------------------------------
 * This code block is only for legacy Unity (version < 2018.1) support.
 * ---------------------------------------------------------------------
 */
#else
using System.Collections.Concurrent;
#endif
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Intel.RealSense.Frames
{
    public class FrameSetPool
    {
#if NET35
        /*
         * ---------------------------------------------------------------------
         * This code block is only for legacy Unity (version < 2018.1) support.
         * ---------------------------------------------------------------------
         */
        private readonly Stack<FrameSet> stack;
        private readonly object stackLock;
#else
        private readonly ConcurrentStack<FrameSet> stack;
#endif

        public FrameSetPool()
        {
#if NET35
            stack = new Stack<FrameSet>();
            stackLock = new object();
#else
            stack = new ConcurrentStack<FrameSet>();            
#endif
        }
#if NET35
        /*
         * ---------------------------------------------------------------------
         * This code block is only for legacy Unity (version < 2018.1) support.
         * ---------------------------------------------------------------------
         */

        public FrameSet Get(IntPtr ptr)
        {
            int count = 0;

            lock (stackLock)
            {
                count = stack.Count;
            }

            if (count > 0)
            {
                FrameSet frameSet;

                lock (stackLock)
                {
                    frameSet = stack.Pop();
                }

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
            lock (stackLock)
            {
                stack.Push(t);
            }
        }
#else
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
#endif
    }
}
