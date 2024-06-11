// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <src/fw-logs/fw-logs-xml-helper.h>
#include <src/librealsense-exception.h>

#include <rsutils/string/from.h>


#include "../../third-party/rapidxml/rapidxml_utils.hpp"
using namespace rapidxml;

namespace librealsense {
namespace fw_logs {

std::vector< char > string_to_char_buffer( const std::string & xml_content )
{
    std::vector< char > buffer;

    buffer.resize( xml_content.size() + 2 );
    memcpy( buffer.data(), xml_content.data(), xml_content.size() );
    buffer[xml_content.size()] = '\0';
    buffer[xml_content.size() + 1] = '\0';

    return buffer;
}

void load_external_xml( xml_document<> * document, std::vector< char > & buffer )
{
    if( buffer.empty() )
        throw librealsense::invalid_value_exception( "Empty XML content" );

    document->parse< 0 >( buffer.data() );  // Might throw
}

xml_node<> * get_first_node( const xml_document<> * document )
{
    xml_node<> * root = document->first_node();
    if( !root )
        throw librealsense::invalid_value_exception( "Cannot find XML root" );

    std::string root_name( root->name(), root->name() + root->name_size() );
    if( root_name.compare( "Format" ) != 0 )
        throw librealsense::invalid_value_exception( "XML root should be 'Format'" );

    return root->first_node();
}

int get_id_attribute( const xml_node<> * node )
{
    for( xml_attribute<> * attribute = node->first_attribute(); attribute; attribute = attribute->next_attribute() )
    {
        std::string attr( attribute->name(), attribute->name() + attribute->name_size() );
        if( attr.compare( "id" ) == 0 )
        {
            std::string id_as_str( attribute->value(), attribute->value() + attribute->value_size() );
            return stoi( id_as_str );
        }
    }

    std::string tag( node->name(), node->name() + node->name_size() );
    throw librealsense::invalid_value_exception( rsutils::string::from()
                                                 << "Can't find attribute 'id' in node " << tag );
}

std::string get_name_attribute( const xml_node<> * node )
{
    for( xml_attribute<> * attribute = node->first_attribute(); attribute; attribute = attribute->next_attribute() )
    {
        std::string attr( attribute->name(), attribute->name() + attribute->name_size() );
        if( attr.compare( "Name" ) == 0 )
        {
            std::string attr_value( attribute->value(), attribute->value() + attribute->value_size() );
            return attr_value;
        }
    }

    std::string tag( node->name(), node->name() + node->name_size() );
    throw librealsense::invalid_value_exception( rsutils::string::from()
                                                 << "Can't find attribute 'Name' in node " << tag );
}

xml_node<> * get_source_node( int source_id, const xml_document<> * document )
{
    // Loop through all elements to find 'Source' nodes, when found look for id attribure and compare to requested id.
    for( xml_node<> * node = get_first_node( document ); node; node = node->next_sibling() )
    {
        std::string tag( node->name(), node->name() + node->name_size() );
        if( tag.compare( "Source" ) == 0 )
        {
            if( source_id == get_id_attribute( node ) )
                return node;
        }
    }

    throw librealsense::invalid_value_exception( rsutils::string::from()
                                                 << "Did not find 'Source' node with id " << source_id );
}

std::string get_file_path( const xml_node<> * source_node )
{
    // Loop through all elements to find 'File' node, when found look for 'Path' attribure.
    for( xml_node<> * node = source_node->first_node(); node; node = node->next_sibling() )
    {
        std::string tag( node->name(), node->name() + node->name_size() );
        if( tag.compare( "File" ) == 0 )
        {
            for( xml_attribute<> * attribute = node->first_attribute(); attribute; attribute = attribute->next_attribute() )
            {
                std::string attr( attribute->name(), attribute->name() + attribute->name_size() );
                if( attr.compare( "Path" ) == 0 )
                {
                    std::string path( attribute->value(), attribute->value() + attribute->value_size() );
                    return path;
                }
            }
        }
    }

    return {};
}

std::map< int, std::string > fw_logs_xml_helper::get_listed_sources( const std::string & definitions_xml )
{
    std::map< int, std::string > source_ids_to_names;

    std::vector< char > buffer = string_to_char_buffer( definitions_xml );
    xml_document<> document;
    load_external_xml( &document, buffer );

    // Loop through all elements to find 'Source' nodes, when found look for id attribure and compare to requested id.
    for( xml_node<> * node = get_first_node( &document ); node; node = node->next_sibling() )
    {
        std::string tag( node->name(), node->name() + node->name_size() );
        if( tag.compare( "Source" ) == 0 )
        {
            source_ids_to_names.insert( { get_id_attribute( node ), get_name_attribute( node ) } );
        }
    }

    return source_ids_to_names;
}


std::string fw_logs_xml_helper::get_source_parser_file_path( int source_id, const std::string & definitions_xml )
{
    std::vector< char > buffer = string_to_char_buffer( definitions_xml );
    xml_document<> document;
    load_external_xml( &document, buffer );
    xml_node<> * source_node = get_source_node( source_id, &document );

    std::string path = get_file_path( source_node );
    if( path.empty() )
        throw librealsense::invalid_value_exception( rsutils::string::from()
                                                     << "Did not find File 'Path' attribute for source " << source_id );

    return path;
}

int get_verbosity_attribute( const xml_node<> * node )
{
    for( xml_attribute<> * attribute = node->first_attribute(); attribute; attribute = attribute->next_attribute() )
    {
        std::string attr( attribute->name(), attribute->name() + attribute->name_size() );
        if( attr.compare( "verbosity" ) == 0 )
        {
            std::string as_str( attribute->value(), attribute->value() + attribute->value_size() );
            if( as_str.empty() )
                throw librealsense::invalid_value_exception( "Verbosity level cannot be empty" );

            if( std::isdigit( as_str[0] ) )
            {
                size_t converted_chars = 0;
                int as_int = stoi( as_str, &converted_chars );
                if( converted_chars == as_str.size() )  // String was a number and converted successfully
                    return as_int;
                else
                    throw librealsense::invalid_value_exception( rsutils::string::from()
                                                                 << "Bad verbosity level " << as_str );
            }

            // Convert verbosity string tokens to number
            // Verbosity level, treated as a bitmask: 0-NONE, 1-VERBOSE, 2-DEBUG, 4-INFO, 8-WARNING, 16-ERROR, 32-FATAL
            // To request INFO + WARNING + ERROR type messages the user shall request the following INFO|WARNING|ERROR
            int i = 0;
            size_t sub_start = 0;
            size_t delim_pos = 0; 
            while( delim_pos != std::string::npos )
            {
                delim_pos = as_str.find( "|", sub_start );
                std::string sub = as_str.substr( sub_start, delim_pos - sub_start );
                if( sub == "NONE" )
                    i |= 0;
                else if( sub == "VERBOSE" )
                    i |= 1;
                else if( sub == "DEBUG" )
                    i |= 2;
                else if( sub == "INFO" )
                    i |= 4;
                else if( sub == "WARNING" )
                    i |= 8;
                else if( sub == "ERROR" )
                    i |= 16;
                else if( sub == "FATAL" )
                    i |= 32;
                else 
                    throw librealsense::invalid_value_exception( rsutils::string::from() << "Illegal verbosity " << sub
                                                                 << ". Expecting NONE, VERBOSE, DEBUG, INFO, WARNING, ERROR or FATAL" );
                sub_start = delim_pos + 1;
            }

            return i;
        }
    }

    std::string tag( node->name(), node->name() + node->name_size() );
    throw librealsense::invalid_value_exception( rsutils::string::from()
                                                 << "Can't find attribute 'verbosity' in node " << tag );
}

std::map< int, int >
fw_logs_xml_helper::get_source_module_verbosity( int source_id, const std::string & definitions_xml )
{
    std::vector< char > buffer = string_to_char_buffer( definitions_xml );
    xml_document<> document;
    load_external_xml( &document, buffer );
    xml_node<> * source_node = get_source_node( source_id, &document );

    std::map< int, int > module_id_verbosity;
    for( xml_node<> * node = source_node->first_node(); node; node = node->next_sibling() )
    {
        std::string tag( node->name(), node->name() + node->name_size() );
        if( tag.compare( "Module" ) == 0 )
        {
            module_id_verbosity.insert( { get_id_attribute( node ), get_verbosity_attribute( node ) } );
        }
    }

    return module_id_verbosity;
}

std::pair< int, std::string > get_event_data( xml_node<> * node )
{
    int num_of_args = -1;
    std::string format;
    bool format_found = false;
    for( xml_attribute<> * attribute = node->first_attribute(); attribute; attribute = attribute->next_attribute() )
    {
        std::string attr( attribute->name(), attribute->name() + attribute->name_size() );
        if( attr.compare( "numberOfArguments" ) == 0 )
        {
            std::string num_of_args_as_str( attribute->value(), attribute->value() + attribute->value_size() );
            num_of_args = stoi( num_of_args_as_str );
            continue;
        }
        else if( attr.compare( "format" ) == 0 )
        {
            format_found = true; // Some users prefer an empty format, so can't check based on empty string, using flag
            format.append( attribute->value(), attribute->value() + attribute->value_size() );
            continue;
        }
    }

    if( num_of_args < 0 || !format_found )
        throw librealsense::invalid_value_exception( rsutils::string::from()
                                                     << "Can't find event 'numberOfArguments' or 'format'" );

    return { num_of_args, format };
}

std::unordered_map< int, std::pair< int, std::string > >
fw_logs_xml_helper::get_events( const std::string & parser_contents )
{
    std::vector< char > buffer = string_to_char_buffer( parser_contents );
    xml_document<> document;
    load_external_xml( &document, buffer );

    std::unordered_map< int, std::pair< int, std::string > > event_id_to_num_of_args_and_format;
    for( xml_node<> * node = get_first_node( &document ); node; node = node->next_sibling() )
    {
        std::string tag( node->name(), node->name() + node->name_size() );
        if( tag.compare( "Event" ) == 0 )
            event_id_to_num_of_args_and_format.insert( { get_id_attribute( node ), get_event_data( node ) } );
    }

    return event_id_to_num_of_args_and_format;
}

std::unordered_map< int, std::string > get_id_to_names( const std::string & parser_contents, const std::string tag )
{
    std::vector< char > buffer = string_to_char_buffer( parser_contents );
    xml_document<> document;
    load_external_xml( &document, buffer );

    std::unordered_map< int, std::string > ids_to_names;
    for( xml_node<> * node = get_first_node( &document ); node; node = node->next_sibling() )
    {
        std::string attr( node->name(), node->name() + node->name_size() );
        if( attr == tag )
        {
            ids_to_names.insert( { get_id_attribute( node ), get_name_attribute( node ) } );
        }
    }

    return ids_to_names;
}

std::unordered_map< int, std::string > fw_logs_xml_helper::get_files( const std::string & parser_contents )
{
    return get_id_to_names( parser_contents, "File" );
}

std::unordered_map< int, std::string > fw_logs_xml_helper::get_modules( const std::string & parser_contents )
{
    return get_id_to_names( parser_contents, "Module" );
}

std::unordered_map< int, std::string > fw_logs_xml_helper::get_threads( const std::string & parser_contents )
{
    return get_id_to_names( parser_contents, "Thread" );
}

std::vector< std::pair< int, std::string > > get_enum_values( xml_node<> * enum_node )
{
    std::vector< std::pair< int, std::string > > values_to_descriptions;

    for( xml_node<> * node = enum_node->first_node(); node; node = node->next_sibling() )
    {
        std::string tag( node->name(), node->name() + node->name_size() );
        if( tag.compare( "EnumValue" ) == 0 )
        {
            int key = -1;
            std::string value;
            for( xml_attribute<> * attribute = node->first_attribute(); attribute; attribute = attribute->next_attribute() )
            {
                std::string attr( attribute->name(), attribute->name() + attribute->name_size() );
                if( attr.compare( "Key" ) == 0 )
                {
                    std::string key_as_str( attribute->value(), attribute->value() + attribute->value_size() );
                    key = stoi( key_as_str );
                }
                else if( attr.compare( "Value" ) == 0 )
                {
                    value.append( attribute->value(), attribute->value() + attribute->value_size() );
                }
            }

            if( key < 0 || value.empty() )
                throw librealsense::invalid_value_exception( rsutils::string::from()
                                                             << "Can't find EnumValue 'Key' or 'Value' for enum " << tag );

            values_to_descriptions.push_back( { key, value } );
        }
    }

    return values_to_descriptions;
}

std::unordered_map< std::string, std::vector< std::pair< int, std::string > > >
fw_logs_xml_helper::get_enums( const std::string & parser_contents )
{
    std::vector< char > buffer = string_to_char_buffer( parser_contents );
    xml_document<> document;
    load_external_xml( &document, buffer );

    std::unordered_map< std::string, std::vector< std::pair< int, std::string > > > enum_names_to_literals;
    for( xml_node<> * node = get_first_node( &document ); node; node = node->next_sibling() )
    {
        std::string tag( node->name(), node->name() + node->name_size() );
        if( tag.compare( "Enums" ) == 0 )
        {
            for( xml_node<> * enum_node = node->first_node(); enum_node; enum_node = enum_node->next_sibling() )
            {
                for( xml_attribute<> * attribute = enum_node->first_attribute(); attribute; attribute = attribute->next_attribute() )
                {
                    std::string enum_name;
                    std::string attr( attribute->name(), attribute->name() + attribute->name_size() );
                    if( attr.compare( "Name" ) == 0 )
                        enum_name.append( attribute->value(), attribute->value() + attribute->value_size() );

                    std::vector< std::pair< int, std::string > > values = get_enum_values( enum_node );

                    enum_names_to_literals.insert( { enum_name, values } );
                }
            }
        }
    }

    return enum_names_to_literals;
}

std::string fw_logs_xml_helper::get_file_version( const std::string & xml_file_contents )
{
    std::vector< char > buffer = string_to_char_buffer( xml_file_contents );
    xml_document<> document;
    load_external_xml( &document, buffer );
    xml_node<> * root = document.first_node();

    for( xml_attribute<> * attribute = root->first_attribute(); attribute; attribute = attribute->next_attribute() )
    {
        std::string attr( attribute->name(), attribute->name() + attribute->name_size() );
        if( attr.compare( "version" ) == 0 )
        {
            std::string file_version( attribute->value(), attribute->value() + attribute->value_size() );
            return file_version;
        }
    }

    throw librealsense::invalid_value_exception( "Can't find file 'version' attribute" );
}

}  // namespace fw_logs
}  // namespace librealsense
