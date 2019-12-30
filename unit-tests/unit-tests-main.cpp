#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

#include <iostream>
#include <string>

#include "unit-tests-common.h"

int main(int argc, char* const argv[])
{
    command_line_params::instance(argc, argv);

    std::vector<std::string> new_args;
    std::string param;

    for (auto i = 0; i < argc; i++)
    {
        param = argv[i];
        if (param != "into" && param != "from")
        {
            if (i != 0 && std::string(argv[i - 1]) == "-i")
            {
                param = generate_product_line_param(param);
            }
            new_args.push_back(param);
        }
        else
        {
            i++;
            if (i < argc && param == "from")
            {
                auto filename = argv[i];
                std::ifstream f(filename);
                if (!f.good())
                {
                    std::cout << "Could not load " << filename << "!"  << std::endl;
                    return EXIT_FAILURE;
                }
            }
        }
    }

    std::vector<char*> new_argvs;
    for (auto&& arg : new_args)
    {
        std::cout << arg << " ";
        new_argvs.push_back(const_cast<char*>(arg.c_str()));
    }
    std::cout << std::endl;

    auto result = Catch::Session().run(static_cast<int>(new_argvs.size()), new_argvs.data());

    if(!command_line_params::instance()._found_any_section)
    {
        std::cout << "Didn't run any tests!\n";
        return EXIT_FAILURE;
    }
    return result;
}