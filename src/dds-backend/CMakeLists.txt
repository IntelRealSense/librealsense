# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

target_link_libraries(${LRS_TARGET} PRIVATE dds)
add_dependencies(${LRS_TARGET} fastdds)

