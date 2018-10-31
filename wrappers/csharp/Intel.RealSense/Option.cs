using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Intel.RealSense
{
    public interface IOption
    {
        Option Key { get; }
        bool Supported { get; }
        float Value { get; set; }
        float Min { get; }
        float Max { get; }
        float Step { get; }
        float Default { get; }
        bool ReadOnly { get; }
        string Description { get; }
        string ValueDescription { get; }
    }
}
