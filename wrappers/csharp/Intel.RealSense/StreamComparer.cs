using Intel.RealSense.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace Intel.RealSense
{
    public struct StreamComparer : IEqualityComparer<Stream>
    {
        public static readonly StreamComparer Default;

        static StreamComparer()
        {
            Default = new StreamComparer();
        }


        public bool Equals(Stream x, Stream y)
            => x == y;

        public int GetHashCode(Stream obj) =>
            // you need to do some thinking here,
            (int)obj;
    }
}
