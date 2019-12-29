#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

#include <iostream>
#include <string>

#include "unit-tests-common.h"

void add_product_line(std::vector<char*>* new_argvs, char* arg);

int main(int argc, char* const argv[])
{
    command_line_params::instance(argc, argv);

    std::vector<char*> new_argvs;
    std::string arg_s;

    for (auto i = 0; i < argc; i++)
    {
        std::string param(argv[i]);
        if (param != "into" && param != "from")
        {
            if (i != 0 && std::string(argv[i - 1]) == "-i")
            {
                rs2::context ctx;
                auto devices = ctx.query_devices();
                for (auto&& device : devices)
                {
                    auto pl = get_device_product_line(device);
                    pl.insert(pl.begin(), '[');
                    pl.insert(pl.end(), ']');
                    arg_s = std::string(argv[i]);
                    arg_s.append(pl);
                    new_argvs.push_back(const_cast<char*>(arg_s.c_str()));
                }
            }
            else
            {
                new_argvs.push_back(argv[i]);
            }
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

    for (auto&& arg : new_argvs)
    {
        std::cout << std::string(arg) << " ";
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

void add_product_line(std::vector<char*>* new_argvs, char* arg)
{
    rs2::context ctx;
    auto devices = ctx.query_devices();
    for (auto&& device : devices)
    {
        auto pl = get_device_product_line(device);
        pl.insert(pl.begin(), '[');
        pl.insert(pl.end(), ']');
        std::string arg_s(arg);
        arg_s.append(pl);
        new_argvs->push_back(const_cast<char*>(arg_s.c_str()));
    }
}