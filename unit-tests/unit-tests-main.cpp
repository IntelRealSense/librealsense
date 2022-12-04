#define CATCH_CONFIG_RUNNER
#include "unit-tests-common.h"
#include <iostream>

int main( int argc, char const * const * argv )
{
    command_line_params::init( argc, argv );

    std::vector< char const * > new_argvs;

    std::cout << "Running tests with the following parameters: ";
    for (auto i = 0; i < argc; i++)
    {
        std::string param(argv[i]);
        std::cout << param << " ";
    }
    std::cout << std::endl;

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

    auto result = Catch::Session().run(static_cast<int>(new_argvs.size()), new_argvs.data());

    if( ! found_any_section() )
    {
        std::cout << "Didn't run any tests!\n";
        return EXIT_FAILURE;
    }
    return result;
}
