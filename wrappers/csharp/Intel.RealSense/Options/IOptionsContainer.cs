// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System.Collections.Generic;

    public interface IOptionsContainer : IEnumerable<IOption>
    {
        IOption this[Option option] { get; }

        bool Supports(Option option);

        string OptionValueDescription(Option option, float value);
    }
}
