#include "string-formatter.h"
#include <regex>
#include <sstream>
#include <iomanip>

using namespace std;

string_formatter::string_formatter(void)
{
}


string_formatter::~string_formatter(void)
{
}

bool string_formatter::genarate_message(const string& source, int num_of_params, const uint32_t* params, string* dest)
{
    std::map<string, string> exp_replace_map;

    if (params == nullptr && num_of_params > 0) return false;

    for (int i = 0; i < num_of_params; i++)
	{
        string regular_exp[2];
		string replacement[2];
        stringstream st_regular_exp[2];
        stringstream st_replacement[2];

        st_regular_exp[0] << "\\{\\b(" << i << ")\\}";
        regular_exp[0] = st_regular_exp[0].str();

        st_replacement[0] << params[i];
        replacement[0] = st_replacement[0].str();

        exp_replace_map[regular_exp[0]] = replacement[0];


        st_regular_exp[1] << "\\{\\b(" << i << "):x\\}";
        regular_exp[1] = st_regular_exp[1].str();

        st_replacement[1] << std::hex << std::setw(2) << std::setfill('0') << params[i];
        replacement[1] = st_replacement[1].str();

        exp_replace_map[regular_exp[1]] = replacement[1];
	}

    return replace_params(source, exp_replace_map, dest);
}

bool string_formatter::replace_params(const string& source, const std::map<string, string>& exp_replace_map, string* dest)
{
    string source_temp(source);

    for (auto exp_replace_it = exp_replace_map.begin(); exp_replace_it != exp_replace_map.end(); exp_replace_it++)
	{
		string destTemp;
        regex e(exp_replace_it->first);
        regex_replace(back_inserter(destTemp), source_temp.begin(), source_temp.end(), e, exp_replace_it->second);
        source_temp = destTemp;

	}
    *dest = source_temp;
	return true;
}
