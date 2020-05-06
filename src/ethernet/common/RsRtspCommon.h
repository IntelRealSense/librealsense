// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

enum RsRtspReturnCode
{
    OK,
    ERROR_GENERAL,
    ERROR_NETWROK,
    ERROR_TIME_OUT,
    ERROR_WRONG_FLOW
};

struct RsRtspReturnValue
{
    RsRtspReturnCode exit_code;
    std::string msg;
};
