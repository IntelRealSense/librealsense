// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#define CATCH_CONFIG_RUNNER
#include <unit-tests/catch/catch.hpp>
#include <string>

extern std::string context;

using namespace Catch::clara;

int main(int argc, char* argv[]) {
    Catch::Session session;
    auto cli = session.cli()
        | Opt(context, "context")
        ["--context"]
        ("Context in which to run the tests");

    session.cli(cli);

    auto ret = session.applyCommandLine(argc, argv);
    if (ret) {
        return ret;
    }

    return session.run();
}


