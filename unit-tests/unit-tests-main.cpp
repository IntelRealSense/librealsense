#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"
#include "unit-tests-common.h"

int main(int argc, char* const argv[])
{
    command_line_params::instance(argc, argv);

    std::vector<char*> new_argvs;
    for (auto i = 0; i < argc; i++)
    {
        std::string param(argv[i]);
        if (param != "into" && param != "from")
        {
            new_argvs.push_back(argv[i]);
        }
        else
        {
            i++;
        }
    }

    auto result = Catch::Session().run(static_cast<int>(new_argvs.size()), new_argvs.data());

    if(!command_line_params::instance()._found_any_section)
    {
        std::cout<<"didn't run any test\n";
        return -1;
    }
    return result;
}
