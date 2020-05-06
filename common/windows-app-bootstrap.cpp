// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

// This file converts the call to WinMain to a call to cross-platform main
// We need WinMain on Windows to offer proper Windows application and not console application
// Should not be used in CMake on Linux

#include <Windows.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <iterator>

#include "metadata-helper.h"

int main(int argv, const char** argc);

int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    int argCount;

    std::shared_ptr<LPWSTR> szArgList(CommandLineToArgvW(GetCommandLine(), &argCount), LocalFree);
    if (szArgList == NULL) return main(0, nullptr);

    std::vector<std::string> args;
    for (int i = 0; i < argCount; i++)
    {
        std::wstring ws = szArgList.get()[i];
        std::string s(ws.begin(), ws.end());

        if (s == rs2::metadata_helper::get_command_line_param())
        {
            try
            {
                rs2::metadata_helper::instance().enable_metadata();
                exit(0);
            }
            catch (...) { exit(-1); }
        }

        args.push_back(s);
    }

    std::vector<const char*> argc;
    std::transform(args.begin(), args.end(), std::back_inserter(argc), [](const std::string& s) { return s.c_str(); });

    return main(static_cast<int>(argc.size()), argc.data());
}