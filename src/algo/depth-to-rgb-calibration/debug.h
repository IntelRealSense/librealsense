//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once


// DEBUG HELPERS -- should be included from .cpp only!!


#define AC_LOG_PREFIX "AC1: "
#define AC_LOG(TYPE,MSG) LOG_##TYPE( AC_LOG_PREFIX << (std::string)( to_string() << MSG ))
//#define AC_LOG(TYPE,MSG) LOG_ERROR( AC_LOG_PREFIX << MSG )
//#define AC_LOG(TYPE,MSG) std::cout << (std::string)( to_string() << "-" << #TYPE [0] << "- " << MSG ) << std::endl; //LOG_INFO((std::string)( to_string() << "-" << #TYPE [0] << "- " << MSG ));
//#define AC_LOG_CONTINUE(TYPE,MSG) std::cout << (std::string)( to_string() << "-" << #TYPE [0] << "- " << MSG )

