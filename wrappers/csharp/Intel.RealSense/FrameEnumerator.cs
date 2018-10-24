using Intel.RealSense.Frames;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace Intel.RealSense
{
    public class FrameEnumerator : IEnumerator<Frame>
    {
        public Frame Current { get; private set; }
        object IEnumerator.Current
        {
            get
            {
                if (index == 0 || index == frameSet.Count + 1)
                    throw new InvalidOperationException();

                return Current;
            }
        }        

        private readonly FrameSet frameSet;
        private int index;

        public FrameEnumerator(FrameSet frameSet)
        {
            this.frameSet = frameSet;
            index = 0;
            Current = default(Frame);
        }

        public void Dispose()
        {
            // Method intentionally left empty.
        }

        public bool MoveNext()
        {
            if ((uint)index < (uint)frameSet.Count)
            {
                var ptr = NativeMethods.rs2_extract_frame(frameSet.Instance.Handle, index, out var error);
                Current = Frame.CreateFrame(ptr);
                index++;
                return true;
            }

            index = frameSet.Count + 1;
            Current = null;
            return false;
        }

        public void Reset()
        {
            index = 0;
            Current = null;
        }
    }
}
