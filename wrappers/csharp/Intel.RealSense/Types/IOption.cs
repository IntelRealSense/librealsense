// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;

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
