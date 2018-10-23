// License: Apache 2.0. See LICENSE file in root directory.
//

#pragma once

namespace irsa {

enum irsa_event_type {
    IRSA_ERROR             = 100,
    IRSA_INFO              = 200,
};

enum irsa_info_type {
    IRSA_INFO_PREVIEW_START,
    IRSA_INFO_PREVIEW_STOP,
    IRSA_INFO_SETPREVIEWDISPLAY,
    IRSA_INFO_SETRENDERID,
    IRSA_INFO_OPEN,
    IRSA_INFO_CLOSE,
    IRSA_INFO_FA,
    IRSA_INFO_PROBE_RS,
    IRSA_INFO_PREVIEW
};

enum irsa_error_type {
    IRSA_ERROR_PREVIEW_START,
    IRSA_ERROR_PREVIEW_STOP,
    IRSA_ERROR_SETPREVIEWDISPLAY,
    IRSA_ERROR_SETRENDERID,
    IRSA_ERROR_OPEN,
    IRSA_ERROR_CLOSE,
    IRSA_ERROR_FA,
    IRSA_ERROR_PROBE_RS,
    IRSA_ERROR_PREVIEW
};

} 
