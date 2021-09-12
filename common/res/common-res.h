// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

// This function calculates and returns the zero padded amount of bytes added to the last index of the uint32_t vector (32 bit alignment)
// Note: If the value itself is zero it will threat it as padding (use with cautions)
inline int calc_alignment_padding(uint32_t last_value)
{
    int cnt = 0;
    for (int i = 3; i > 0; --i)
    {
        if ((last_value & (0xFF << (i * 8))) == 0) cnt++;
        else break;
    }
    return cnt;
}
