#pragma once
#include <string>
#include <map>
#include <stdint.h>

namespace fw_logger
{
    class string_formatter
    {
    public:
        string_formatter(void);
        ~string_formatter(void);

        bool generate_message(const std::string& source, int num_of_params, const uint32_t* params, std::string* dest);

    private:
        bool replace_params(const std::string& source, const std::map<std::string, std::string>& exp_replace_map, std::string* dest) const;
    };
}
