/*
 * Copyright (C) 2010, Willow Garage, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the names of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Some common cross platform utilities.
 */
#ifndef CPP_COMMON_PLATFORM_H_
#define CPP_COMMON_PLATFORM_H_

/******************************************************************************
* Includes
******************************************************************************/

#ifdef WIN32
  #ifdef _MSC_VER
    #define WIN32_LEAN_AND_MEAN // slimmer compile times
    #define _WINSOCKAPI_ // stops windows.h from including winsock.h (and lets us include winsock2.h)
    #define NOMINMAX // windows c++ pollutes the environment like any factory
  #endif
  #include <windows.h>
#endif



/******************************************************************************
* Cross Platform Functions
******************************************************************************/

#ifndef _MSC_VER
  #include <stdlib.h> // getenv
#endif
#include <string>

namespace ros {

/**
 * Convenient cross platform function for returning a std::string of an
 * environment variable.
 */
inline bool get_environment_variable(std::string &str, const char* environment_variable) {
	char* env_var_cstr = NULL;
	#ifdef _MSC_VER
	  _dupenv_s(&env_var_cstr, NULL,environment_variable);
	#else
	  env_var_cstr = getenv(environment_variable);
	#endif
	if ( env_var_cstr ) {
		str = std::string(env_var_cstr);
        #ifdef _MSC_VER
		  free(env_var_cstr);
        #endif
		return true;
	} else {
		str = std::string("");
		return false;
	}
}

} // namespace ros

#endif /* CPP_COMMON_PLATFORM_H_ */
