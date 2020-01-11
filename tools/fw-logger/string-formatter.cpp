// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "string-formatter.h"
#include <regex>
#include <sstream>
#include <iomanip>

using namespace std;

namespace fw_logger
{
    string_formatter::string_formatter(std::unordered_map<std::string, std::vector<std::string>> enums)
        :_enums(enums)
    {
    }


    string_formatter::~string_formatter(void)
    {
    }

    bool string_formatter::generate_message(const string& source, int num_of_params, const uint32_t* params, string* dest)
    {
        map<string, string> exp_replace_map;
        map<string, int> enum_replace_map;

        if (params == nullptr && num_of_params > 0) return false;

        for (int i = 0; i < num_of_params; i++)
        {
            string regular_exp[3];
            string replacement[3];
            stringstream st_regular_exp[3];
            stringstream st_replacement[3];

            st_regular_exp[0] << "\\{\\b(" << i << ")\\}";
            regular_exp[0] = st_regular_exp[0].str();

            st_replacement[0] << params[i];
            replacement[0] = st_replacement[0].str();

            exp_replace_map[regular_exp[0]] = replacement[0];


            st_regular_exp[1] << "\\{\\b(" << i << "):x\\}";
            regular_exp[1] = st_regular_exp[1].str();

            st_replacement[1] << hex << setw(2) << setfill('0') << params[i];
            replacement[1] = st_replacement[1].str();

            exp_replace_map[regular_exp[1]] = replacement[1];

            st_regular_exp[2] << "\\{\\b(" << i << "),[a-zA-Z]+\\}";
            regular_exp[2] = st_regular_exp[2].str();

            enum_replace_map[regular_exp[2]] = params[i];
        }

        return replace_params(source, exp_replace_map, enum_replace_map, dest);
    }

    bool string_formatter::replace_params(const string& source, const map<string, string>& exp_replace_map, const map<string, int>& enum_replace_map, string* dest)
    {
        string source_temp(source);

        for (auto exp_replace_it = exp_replace_map.begin(); exp_replace_it != exp_replace_map.end(); exp_replace_it++)
        {
            string destTemp;
            regex e(exp_replace_it->first);
            auto res = regex_replace(back_inserter(destTemp), source_temp.begin(), source_temp.end(), e, exp_replace_it->second);
            source_temp = destTemp;
        }

        for (auto exp_replace_it = enum_replace_map.begin(); exp_replace_it != enum_replace_map.end(); exp_replace_it++)
        {
            string destTemp;
            regex e(exp_replace_it->first);
            std::smatch m;
            std::regex_search(source_temp, m, std::regex(e));

            string enum_name;

            string st_regular_exp = "[a-zA-Z]+";
            regex e1(st_regular_exp);

            for(auto exp = 0; exp<m.size(); exp++)
            {
                string str = m[exp];

                std::smatch m1;

                regex e2 = e1;
                std::regex_search(str, m1, std::regex(e2));

                for (auto exp = 0; exp < m1.size(); exp++)
                {
                    enum_name = m1[exp];
                    if (_enums.size()>0 && _enums.find(enum_name) != _enums.end())
                    {
                        auto vec = _enums[enum_name];
                        regex e3 = e;
                        auto res1 = regex_replace(back_inserter(destTemp), source_temp.begin(), source_temp.end(), e3, vec[exp_replace_it->second]);
                        source_temp = destTemp;
                    }
                }
            }
        }

        *dest = source_temp;
        return true;
    }
}
