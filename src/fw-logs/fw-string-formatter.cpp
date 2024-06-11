// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "fw-string-formatter.h"
#include "fw-logs-formatting-options.h"

#include <rsutils/string/from.h>
#include <rsutils/easylogging/easyloggingpp.h>

#include <regex>
#include <cmath>

using namespace std;

namespace librealsense
{
    namespace fw_logs
    {
        fw_string_formatter::fw_string_formatter( std::unordered_map< std::string, std::vector< kvp > > enums )
            : _enums(enums)
        {
        }

        std::string fw_string_formatter::generate_message( const string & source,
                                                           const std::vector< param_info > & params_info,
                                                           const std::vector< uint8_t > & params_blob )
        {
            map< string, string > exp_replace_map;
            map< string, int > enum_replace_map;

            if( params_info.size() > 0 && params_blob.empty() )
                return source;

            for( size_t i = 0; i < params_info.size(); i++ )
            {
                // Parsing 4 expression types - {0} / {1:x} / {2:f} / {3,Enum}
                stringstream ss_regular_exp[4];
                stringstream ss_replacement[4];
                string param_as_string = convert_param_to_string( params_info[i],
                                                                  params_blob.data() + params_info[i].offset );

                ss_regular_exp[0] << "\\{\\b(" << i << ")\\}";
                exp_replace_map[ss_regular_exp[0].str()] = param_as_string;

                if( is_integral( params_info[i] ) )
                {
                    // Print as hexadecimal number, assumes parameter was an integer number.
                    ss_regular_exp[1] << "\\{\\b(" << i << "):x\\}";
                    ss_replacement[1] << hex << setw( 2 ) << setfill( '0' ) << std::stoull( param_as_string );
                    exp_replace_map[ss_regular_exp[1].str()] = ss_replacement[1].str();

                    // Legacy format - parse parameter as 4 raw bytes of float. Parameter can be uint16_t or uint32_t.
                    if( params_info[i].size <= sizeof( uint32_t ) )
                    {
                        ss_regular_exp[2] << "\\{\\b(" << i << "):f\\}";
                        uint32_t as_int32 = 0;
                        memcpy( &as_int32, params_blob.data() + params_info[i].offset, params_info[i].size );
                        float as_float = *reinterpret_cast< const float * >( &as_int32 );
                        if( std::isfinite( as_float ) )
                            ss_replacement[2] << as_float;
                        else
                        {
                            // Values for other regular expresions can be NaN when converted to float.
                            // Prepare replacement as hex value (will probably not be used because format is not with :f).
                            ss_replacement[2] << "0x" << hex << setw( 2 ) << setfill( '0' ) << as_int32;
                        }
                        exp_replace_map[ss_regular_exp[2].str()] = ss_replacement[2].str();
                    }

                    // enum values are mapped by int (kvp) but we don't know if this parameter is related to enum, using
                    // unsigned long long because unrelated arguments can overflow int
                    ss_regular_exp[3] << "\\{\\b(" << i << "),[a-zA-Z]+\\}";
                    enum_replace_map[ss_regular_exp[3].str()] = static_cast< int >( std::stoull( param_as_string ) );
                }
            }

            return replace_params( source, exp_replace_map, enum_replace_map );
        }

        std::string fw_string_formatter::replace_params( const string & source,
                                                         const map< string, string > & exp_replace_map,
                                                         const map< string, int > & enum_replace_map )
        {
            string source_temp( source );

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
                        string enum_name = m1[exp];
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
                                LOG_WARNING( s.str() );
                            }
                            source_temp = destTemp;
                        }
                    }
                }
            }

            return source_temp;
        }

        std::string fw_string_formatter::convert_param_to_string( const param_info & info, const uint8_t * param_start ) const
        {
            switch( info.type )
            {
            case param_type::STRING:
            {
                // Using stringstream to remove the terminating null character. We use this as regex replacement string,
                // and having '\0' here will cause a terminating character in the middle of the result string.
                stringstream str;
                str << param_start;
                return str.str();
            }
            case param_type::UINT8:
                return std::to_string( *reinterpret_cast< const uint8_t * >( param_start ) );
            case param_type::SINT8:
                return std::to_string( *reinterpret_cast< const int8_t * >( param_start ) );
            case param_type::UINT16:
                return std::to_string( *reinterpret_cast< const uint16_t * >( param_start ) );
            case param_type::SINT16:
                return std::to_string( *reinterpret_cast< const int16_t * >( param_start ) );
            case param_type::SINT32:
                return std::to_string( *reinterpret_cast< const int32_t * >( param_start ) );
            case param_type::UINT32:
                return std::to_string( *reinterpret_cast< const uint32_t * >( param_start ) );
            case param_type::SINT64:
                return std::to_string( *reinterpret_cast< const int64_t * >( param_start ) );
            case param_type::UINT64:
                return std::to_string( *reinterpret_cast< const uint64_t * >( param_start ) );
            case param_type::FLOAT:
                return std::to_string( *reinterpret_cast< const float * >( param_start ) );
            case param_type::DOUBLE:
                return std::to_string( *reinterpret_cast< const double * >( param_start ) );
            default:
                throw librealsense::invalid_value_exception( rsutils::string::from()
                                                             << "Unsupported parameter type "
                                                             << static_cast< uint8_t >( info.type ) );
            }
        }

        bool fw_string_formatter::is_integral( const param_info & info ) const
        {
            if( info.type == param_type::STRING || info.type == param_type::FLOAT || info.type == param_type::DOUBLE )
                return false;

            return true;
        }

    }
}
