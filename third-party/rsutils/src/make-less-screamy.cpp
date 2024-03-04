// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <string>


namespace rsutils {
namespace string {


std::string make_less_screamy( std::string res )
{
    bool first = true;
    for( char & ch : res )
    {
        if( ch != '_' )
        {
            if( ! first )
                ch = tolower( ch );
            first = false;
        }
        else
        {
            ch = ' ';
            first = true;
        }
    }
    return res;
}


}  // namespace string
}  // namespace rsutils
