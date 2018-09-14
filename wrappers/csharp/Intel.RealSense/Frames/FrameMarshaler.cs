using Intel.RealSense.Frames;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Devices
{
    internal class FrameMarshaler : ICustomMarshaler
    {
        private static FrameMarshaler Instance;

        public static ICustomMarshaler GetInstance(string s) 
            => Instance ?? new FrameMarshaler();

        public void CleanUpManagedData(object ManagedObj)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
        }

        public int GetNativeDataSize()
        {
            //return IntPtr.Size;
            return -1;
        }

        public IntPtr MarshalManagedToNative(object ManagedObj)
        {
            throw new NotImplementedException();
        }

        public object MarshalNativeToManaged(IntPtr pNativeData) 
            => new Frame(pNativeData);
    }
}
