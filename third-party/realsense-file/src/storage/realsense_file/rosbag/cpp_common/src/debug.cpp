/*
 * Copyright (C) 2009, Willow Garage, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the names of Stanford University or Willow Garage, Inc. nor the names of its
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

#include "ros/debug.h"

#if defined(HAVE_EXECINFO_H)
#include <execinfo.h>
#endif

#if defined(HAVE_CXXABI_H)
#include <cxxabi.h>
#endif

#include <cstdlib>
#include <cstdio>
#include <sstream>

namespace ros
{
namespace debug
{

void getBacktrace(V_void& addresses)
{
#if HAVE_GLIBC_BACKTRACE
  void *array[64];

  size_t size = backtrace(array, 64);
  for (size_t i = 1; i < size; i++)
  {
    addresses.push_back(array[i]);
  }
#endif
}

void translateAddresses(const V_void& addresses, V_string& lines)
{
#if HAVE_GLIBC_BACKTRACE
  if (addresses.empty())
  {
    return;
  }

  size_t size = addresses.size();
  char **strings = backtrace_symbols(&addresses.front(), size);

  for (size_t i = 0; i < size; ++i)
  {
    lines.push_back(strings[i]);
  }

  free(strings);
#endif
}

std::string demangleName(const std::string& name)
{
#if HAVE_CXXABI_H
  int status;
  char* demangled = abi::__cxa_demangle(name, 0, 0, &status);
  std::string out;
  if (demangled)
  {
    out = demangled;
    free(demangled);
  }
  else
  {
    out = name;
  }

  return out;
#else
  return name;
#endif
}

std::string demangleBacktraceLine(const std::string& line)
{
  // backtrace_symbols outputs in the form:
  // executable(function+offset) [address]
  // We want everything between ( and + to send to demangleName()
  size_t paren_pos = line.find('(');
  size_t plus_pos = line.find('+');
  if (paren_pos == std::string::npos || plus_pos == std::string::npos)
  {
    return line;
  }

  std::string name(line, paren_pos + 1, plus_pos - paren_pos - 1);
  return line.substr(0, paren_pos + 1) + demangleName(name) + line.substr(plus_pos);
}

void demangleBacktrace(const V_string& lines, V_string& demangled)
{
  V_string::const_iterator it = lines.begin();
  V_string::const_iterator end = lines.end();
  for (; it != end; ++it)
  {
    demangled.push_back(demangleBacktraceLine(*it));
  }
}

std::string backtraceToString(const V_void& addresses)
{
  V_string lines, demangled;
  translateAddresses(addresses, lines);
  demangleBacktrace(lines, demangled);

  std::stringstream ss;
  V_string::const_iterator it = demangled.begin();
  V_string::const_iterator end = demangled.end();
  for (; it != end; ++it)
  {
    ss << *it << std::endl;
  }

  return ss.str();
}

std::string getBacktrace()
{
  V_void addresses;
  getBacktrace(addresses);
  return backtraceToString(addresses);
}

}

}
