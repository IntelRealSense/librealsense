# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

set(IMAGE_TOPIC_HEADER_FILES
   "${CMAKE_CURRENT_LIST_DIR}/imagePubSubTypes.h"
   "${CMAKE_CURRENT_LIST_DIR}/image.h"
   "${CMAKE_CURRENT_LIST_DIR}/image-msg.h"
)
set(IMAGE_TOPIC_SOURCE_FILES
   "${CMAKE_CURRENT_LIST_DIR}/imagePubSubTypes.cpp"
   "${CMAKE_CURRENT_LIST_DIR}/image.cpp"
)
set( IMAGE_TOPIC_FILES ${IMAGE_TOPIC_HEADER_FILES} )
