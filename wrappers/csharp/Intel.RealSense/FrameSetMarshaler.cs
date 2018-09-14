using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    class FrameSetMarshaler : ICustomMarshaler
    {
        private static FrameSetMarshaler instance;

        public static ICustomMarshaler GetInstance(string s)
        {
            if (instance == null)
            {
                instance = new FrameSetMarshaler();
            }
            return instance;
        }

        public void CleanUpManagedData(object ManagedObj)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
        }

        public int GetNativeDataSize()
        {
            return -1;
        }

        public IntPtr MarshalManagedToNative(object ManagedObj)
        {
            throw new NotImplementedException();
        }

        public object MarshalNativeToManaged(IntPtr pNativeData)
        {
            return new FrameSet(pNativeData);
        }
    }
}
