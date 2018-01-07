#pragma once
#include <unordered_map>
#include <string>
#include <stdint.h>

#ifdef ANDROID
#include "../../common/android_helpers.h"
#endif

namespace fw_logger
{
    struct fw_log_event
    {
        int num_of_params;
        std::string line;

        fw_log_event();
        fw_log_event(int input_num_of_params, const std::string& input_line);
    };

    class fw_logs_xml_helper;

    class fw_logs_formating_options
    {
    public:
        fw_logs_formating_options(const std::string& xml_full_file_path);
        ~fw_logs_formating_options(void);


        bool get_event_data(int id, fw_log_event* log_event_data) const;
        bool get_file_name(int id, std::string* file_name) const;
        bool get_thread_name(uint32_t thread_id, std::string* thread_name) const;
        bool initialize_from_xml();

    private:
        friend fw_logs_xml_helper;
        std::unordered_map<int, fw_log_event> _fw_logs_event_list;
        std::unordered_map<int, std::string> _fw_logs_file_names_list;
        std::unordered_map<int, std::string> _fw_logs_thread_names_list;
        std::string _xml_full_file_path;
    };
}
