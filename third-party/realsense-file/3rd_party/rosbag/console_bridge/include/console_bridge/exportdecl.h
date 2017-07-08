/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Author: Thomas Moulard */

#ifndef CONSOLE_BRIDGE_EXPORTDECL_H
# define CONSOLE_BRIDGE_EXPORTDECL_H

// Handle portable symbol export.
// Defining manually which symbol should be exported is required
// under Windows whether MinGW or MSVC is used.
//
// The headers then have to be able to work in two different modes:
// - dllexport when one is building the library,
// - dllimport for clients using the library.
//
// On Linux, set the visibility accordingly. If C++ symbol visibility
// is handled by the compiler, see: http://gcc.gnu.org/wiki/Visibility
# if defined _WIN32 || defined __CYGWIN__
// On Microsoft Windows, use dllimport and dllexport to tag symbols.
//#  define CONSOLE_BRIDGE_DLLIMPORT __declspec(dllimport)
//#  define CONSOLE_BRIDGE_DLLEXPORT __declspec(dllexport)
//#  define CONSOLE_BRIDGE_DLLLOCAL
# else
// On Linux, for GCC >= 4, tag symbols using GCC extension.
#  if __GNUC__ >= 4
#   define CONSOLE_BRIDGE_DLLIMPORT __attribute__ ((visibility("default")))
#   define CONSOLE_BRIDGE_DLLEXPORT __attribute__ ((visibility("default")))
#   define CONSOLE_BRIDGE_DLLLOCAL  __attribute__ ((visibility("hidden")))
#  else
// Otherwise (GCC < 4 or another compiler is used), export everything.
#   define CONSOLE_BRIDGE_DLLIMPORT
#   define CONSOLE_BRIDGE_DLLEXPORT
#   define CONSOLE_BRIDGE_DLLLOCAL
#  endif // __GNUC__ >= 4
# endif // defined _WIN32 || defined __CYGWIN__

# ifdef CONSOLE_BRIDGE_STATIC
// If one is using the library statically, get rid of
// extra information.
#  define CONSOLE_BRIDGE_DLLAPI
#  define CONSOLE_BRIDGE_LOCAL
# else
// Depending on whether one is building or using the
// library define DLLAPI to import or export.
#  ifdef console_bridge_EXPORTS
#   define CONSOLE_BRIDGE_DLLAPI CONSOLE_BRIDGE_DLLEXPORT
#  else
#   define CONSOLE_BRIDGE_DLLAPI CONSOLE_BRIDGE_DLLIMPORT
#  endif // CONSOLE_BRIDGE_EXPORTS
#  define CONSOLE_BRIDGE_LOCAL CONSOLE_BRIDGE_DLLLOCAL
# endif // CONSOLE_BRIDGE_STATIC
#endif //! CONSOLE_BRIDGE_EXPORTDECL_H
