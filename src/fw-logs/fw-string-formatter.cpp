// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "fw-string-formatter.h"
#include "fw-logs-formating-options.h"

#include <rsutils/string/from.h>
#include <rsutils/easylogging/easyloggingpp.h>

#include <regex>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cmath>

using namespace std;

namespace librealsense
{
    namespace fw_logs
    {
        fw_string_formatter::fw_string_formatter( std::unordered_map< std::string, std::vector< kvp > > enums )
            :_enums(enums)
        {
        }

        bool fw_string_formatter::generate_message( const string & source,
                                                    const std::vector< param_info > & params_info,
                                                    const std::vector< uint8_t > & params_blob,
                                                    string * dest )
        {
            map<string, string> exp_replace_map;
            map<string, int> enum_replace_map;

            if( params_info.size() > 0 && params_blob.empty() )
                return false;

            for( size_t i = 0; i < params_info.size(); i++ )
            {
                string regular_exp[4];
                string replacement[4];
                stringstream st_regular_exp[4];
                stringstream st_replacement[4];
                string param_as_string = convert_param_to_string( params_info[i],
                                                                  params_blob.data() + params_info[i].offset );

                st_regular_exp[0] << "\\{\\b(" << i << ")\\}";
                regular_exp[0] = st_regular_exp[0].str();

                st_replacement[0] << param_as_string;
                replacement[0] = st_replacement[0].str();

                exp_replace_map[regular_exp[0]] = replacement[0];


                st_regular_exp[1] << "\\{\\b(" << i << "):x\\}";
                regular_exp[1] = st_regular_exp[1].str();

                st_replacement[1] << hex << setw( 2 ) << setfill( '0' ) << param_as_string;
                replacement[1] = st_replacement[1].str();

                exp_replace_map[regular_exp[1]] = replacement[1];

                st_regular_exp[2] << "\\{\\b(" << i << "):f\\}";
                regular_exp[2] = st_regular_exp[2].str();
                // Parse int32_t as 4 raw bytes of float
                float tmp = *reinterpret_cast< const float * >( &params[i] );
                if( std::isfinite( tmp ) )
                    st_replacement[2] << tmp;
                else
                {
                    LOG_ERROR( "Expecting a number, received infinite or NaN" );
                    st_replacement[2] << "0x" << hex << setw( 2 ) << setfill( '0' ) << params[i];
                }
                replacement[2] = st_replacement[2].str();
                exp_replace_map[regular_exp[2]] = replacement[2];


                st_regular_exp[3] << "\\{\\b(" << i << "),[a-zA-Z]+\\}";
                regular_exp[3] = st_regular_exp[3].str();

                enum_replace_map[regular_exp[3]] = std::stoi( param_as_string ); // enum values are maped by int (kvp)
            }

            return replace_params(source, exp_replace_map, enum_replace_map, dest);
        }

        bool fw_string_formatter::replace_params( const string & source,
                                                  const map< string, string > & exp_replace_map,
                                                  const map< string, int > & enum_replace_map,
                                                  string * dest )
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

                for (size_t exp = 0; exp < m.size(); exp++)
                {
                    string str = m[exp];

                    std::smatch m1;

                    regex e2 = e1;
                    std::regex_search(str, m1, std::regex(e2));

                    for (size_t exp = 0; exp < m1.size(); exp++)
                    {
                        enum_name = m1[exp];
                        if (_enums.size() > 0 && _enums.find(enum_name) != _enums.end())
                        {
                            auto vec = _enums[enum_name];
                            regex e3 = e;
                            // Verify user's input is within the enumerated range
                            int val = exp_replace_it->second;
                            auto it = std::find_if(vec.begin(), vec.end(), [val](kvp& entry) { return entry.first == val; });
                            if (it != vec.end())
                            {
                                regex_replace(back_inserter(destTemp), source_temp.begin(), source_temp.end(), e3, it->second);
                            }
                            else
                            {
                                stringstream s;
                                s << "Protocol Error recognized!\nImproper log message received: " << source_temp
                                    << ", invalid parameter: " << exp_replace_it->second << ".\n The range of supported values is \n";
                                for_each(vec.begin(), vec.end(), [&s](kvp& entry) { s << entry.first << ":" << entry.second << " ,"; });
                                std::cout << s.str().c_str() << std::endl;;
                            }
                            source_temp = destTemp;
                        }
                    }
                }
            }

            *dest = source_temp;
            return true;
        }

        std::string fw_string_formatter::convert_param_to_string( const param_info & info, const uint8_t * param_start ) const
        {
            switch( info.type )
            {
            case param_type::STRING:
                return std::string( reinterpret_cast< const char * >( param_start ), info.size );
            case param_type::UINT8:
                return rsutils::string::from() << *reinterpret_cast< const uint8_t * >( param_start );
            case param_type::SINT8:
                return rsutils::string::from() << *reinterpret_cast< const int8_t * >( param_start );
            case param_type::UINT16:
                return rsutils::string::from() << *reinterpret_cast< const uint16_t * >( param_start );
            case param_type::SINT16:
                return rsutils::string::from() << *reinterpret_cast< const int16_t * >( param_start );
            case param_type::SINT32:
                return rsutils::string::from() << *reinterpret_cast< const int32_t * >( param_start );
            case param_type::UINT32:
                return rsutils::string::from() << *reinterpret_cast< const uint32_t * >( param_start );
            case param_type::SINT64:
                return rsutils::string::from() << *reinterpret_cast< const int64_t * >( param_start );
            case param_type::UINT64:
                return rsutils::string::from() << *reinterpret_cast< const uint64_t * >( param_start );
            case param_type::FLOAT:
                return rsutils::string::from() << *reinterpret_cast< const float * >( param_start );
            case param_type::DOUBLE:
                return rsutils::string::from() << *reinterpret_cast< const double * >( param_start );
            default:
                throw librealsense::invalid_value_exception( rsutils::string::from()
                                                             << "Unsupported parameter type "
                                                             << static_cast< uint8_t >( info.type ) );
            }
        }
    }
}
