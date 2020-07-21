//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once


// DEBUG HELPERS -- should be included from .cpp only!!


#define AC_F_PREC  std::setprecision( std::numeric_limits< float >::max_digits10 )
#define AC_D_PREC  std::setprecision( std::numeric_limits< double >::max_digits10 )


#define AC_LOG_PREFIX "CAH: "
#define AC_LOG_PREFIX_LEN 5

#define AC_LOG(TYPE,MSG) LOG_##TYPE( AC_LOG_PREFIX << (std::string)( librealsense::to_string() << MSG ))
//#define AC_LOG(TYPE,MSG) std::cout << (std::string)( to_string() << "-" << #TYPE [0] << "- " << MSG ) << std::endl; //LOG_INFO((std::string)( to_string() << "-" << #TYPE [0] << "- " << MSG ));
//#define AC_LOG_CONTINUE(TYPE,MSG) std::cout << (std::string)( to_string() << "-" << #TYPE [0] << "- " << MSG )

