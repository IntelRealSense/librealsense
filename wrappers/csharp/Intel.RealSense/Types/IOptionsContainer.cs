// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    public interface IOptionsContainer : IEnumerable<IOption>
    {
        IOption this[Option option] { get; }

        string OptionValueDescription(Option option, float value);
    }
}
