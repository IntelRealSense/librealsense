using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public interface IOptionsContainer : IEnumerable<IOption>
    {
        IOption this[Option option] { get; }
        string OptionValueDescription(Option option, float value);
    }
}
