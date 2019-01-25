using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class FrameEnumerator : IEnumerator<Frame>
    {
        private readonly FrameSet fs;
        private Frame current;
        private int index;

        public FrameEnumerator(FrameSet fs)
        {
            this.fs = fs;
            index = 0;
            current = default(Frame);
        }

        public Frame Current
        {
            get
            {
                return current;
            }
        }

        object IEnumerator.Current
        {
            get
            {
                if (index == 0 || index == fs.m_count + 1)
                {
                    throw new InvalidOperationException();
                }
                return Current;
            }
        }

        public void Dispose()
        {
            // Method intentionally left empty.
        }

        public bool MoveNext()
        {
            if ((uint)index < (uint)fs.m_count)
            {
                object error;
                var ptr = NativeMethods.rs2_extract_frame(fs.m_instance.Handle, index, out error);
                current = Frame.CreateFrame(ptr);
                index++;
                return true;
            }
            index = fs.m_count + 1;
            current = null;
            return false;
        }

        public void Reset()
        {
            index = 0;
            current = null;
        }
    }
}
