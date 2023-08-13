// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-guid.h>
#include <realdds/dds-participant.h>
#include <sstream>


namespace realdds {


std::string print( dds_guid const & guid,
                   dds_guid_prefix const & base_prefix,
                   bool readable_name )
{
    std::ostringstream output;
    if( guid != eprosima::fastrtps::rtps::c_Guid_Unknown )
    {
        if( guid.guidPrefix != base_prefix )
        {
            std::string participant;
            if( readable_name )
                participant = dds_participant::name_from_guid( guid );
            if( ! participant.empty() )
            {
                output << participant;
            }
            else
            {
                output << std::hex;
                char old_fill = output.fill( '0' );

                size_t i = 0;
                if( base_prefix != eprosima::fastrtps::rtps::c_Guid_Unknown.guidPrefix )
                {
                    while( i < base_prefix.size && guid.guidPrefix.value[i] == base_prefix.value[i] )
                        ++i;
                }
                for( ; i < guid.guidPrefix.size; ++i )
                    output << std::setw( 2 ) << +guid.guidPrefix.value[i];  // << ".";

                output.fill( old_fill );
                output << std::dec;
            }
            output << '.';  // between prefix and entity, DDS uses '|'
        }
        output << std::hex << guid.entityId.to_uint32() << std::dec;
    }
    else
    {
        output << "|GUID UNKNOWN|";  // same as in FastDDS
    }
    return output.str();
}


}  // namespace realdds
