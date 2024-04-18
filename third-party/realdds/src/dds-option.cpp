// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <realdds/dds-option.h>
#include <realdds/dds-exceptions.h>

#include <rsutils/json.h>
using rsutils::json;


namespace realdds {


dds_option::dds_option()
    : _flags( 0 )
    , _value( rsutils::missing_json )
    , _default_value( rsutils::missing_json )
    , _minimum_value( rsutils::null_json )
    , _maximum_value( rsutils::null_json )
    , _stepping( rsutils::null_json )
{
}


void dds_option::verify_uninitialized() const
{
    if( _value.exists() )
        DDS_THROW( runtime_error, "option is already initialized" );
}


void dds_option::init( std::string name, std::string description )
{
    verify_uninitialized();

    if( name.empty() )
        DDS_THROW( runtime_error, "empty option name" );
    if( ! _name.empty() )
        DDS_THROW( runtime_error, "already init" );

    _name = std::move( name );
    _description = std::move( description );
}


void dds_option::init_properties( option_properties && props )
{
    verify_uninitialized();

    if( props.erase( "read-only" ) )
        _flags |= flag::read_only;
    if( props.erase( "optional" ) )
        _flags |= flag::optional;

    if( ! props.empty() )
        DDS_THROW( runtime_error, "invalid option properties" );
}


void dds_option::init_default_value( json value )
{
    verify_uninitialized();

    check_value( value );
    _default_value = std::move( value );
}


void dds_option::init_range( rsutils::json minimum_value,
                             rsutils::json maximum_value,
                             rsutils::json stepping )
{
    verify_uninitialized();

    if( ! get_default_value().exists() )
        DDS_THROW( runtime_error, "cannot set a range without a default" );

    if( get_default_value() == minimum_value
        && minimum_value == maximum_value
        && stepping.get< float >() == 0. )
    {
        // This is read-only...!
        _flags |= flag::read_only;
        _default_value = rsutils::missing_json;
        return;
    }

    if( ! minimum_value.is_null() )
    {
        check_type( minimum_value );
        if( ! get_default_value().is_null() && get_default_value() < minimum_value )
            DDS_THROW( runtime_error, "default value " << get_default_value() << " < " << minimum_value << " minimum" );
        _minimum_value = std::move( minimum_value );
    }
    if( ! maximum_value.is_null() )
    {
        check_type( maximum_value );
        if( ! get_default_value().is_null() && get_default_value() > maximum_value )
            DDS_THROW( runtime_error, "default value " << get_default_value() << " > " << maximum_value << " maximum" );
        _maximum_value = std::move( maximum_value );
    }
    if( ! stepping.is_null() )
    {
        check_type( stepping );
        // TODO check default against stepping?
        _stepping = std::move( stepping );
    }
}


void dds_option::init_value( json value )
{
    verify_uninitialized();
    if( _name.empty() )
        DDS_THROW( runtime_error, "option '" << _name << "' not initialized" );

    check_value( value );

    if( ! value.is_null() )
    {
        // We only verify the range/read-only on the initial default and value
        if( ! _minimum_value.is_null() && value < _minimum_value )
            DDS_THROW( runtime_error, "value " << value << " < " << _minimum_value << " minimum" );
        if( ! _maximum_value.is_null() && value > _maximum_value )
            DDS_THROW( runtime_error, "value " << value << " > " << _maximum_value << " maximum" );
        // TODO check the stepping
    }

    // If, at this time, we haven't set a default value, then we're read-only!
    if( ! get_default_value().exists() )
        _flags |= (unsigned) flag::read_only;

    _value = std::move( value );
}


void dds_option::init_stream( std::shared_ptr< dds_stream_base > const & stream )
{
    // NOTE: this happens AFTER we're "initialized" (i.e., the value is set before we get here)
    if( _stream.lock() )
        DDS_THROW( runtime_error, "option '" << get_name() << "' already has a stream" );
    if( ! stream )
        DDS_THROW( runtime_error, "null stream" );
    _stream = stream;
}


void dds_option::set_value( rsutils::json value )
{
    check_value( value );
    _value = std::move( value );
}


void dds_option::check_value( json & value ) const
{
    if( ! value.exists() )
        DDS_THROW( runtime_error, "invalid option value" );

    if( value.is_null() )
    {
        if( ! is_optional() )
            DDS_THROW( runtime_error, "value is not optional" );
    }
    else
    {
        check_type( value );
    }
}


static dds_option::option_properties parse_option_properties( json const & j )
{
    dds_option::option_properties props;
    for( auto & p : j.back() )
    {
        if( ! p.is_string() )
            DDS_THROW( runtime_error, "non-string in properties: " << j );
        if( ! props.insert( p.string_ref() ).second )
            DDS_THROW( runtime_error, "option property '" << p.string_ref() << "' repeats: " << j );
    }
    return props;
}


template< typename... Rest >
bool type_from_value( std::string & type, json const & j )
{
    switch( j.type() )
    {
    case json::value_t::number_float:
        if( type.length() == 5 && type == "float" )
            return true;
        if( type.empty() || type == "int" )
            return type.assign( "float", 5 ), true;  // float > int
        break;

    case json::value_t::string:
        if( type.length() == 6 && type == "string" )
            return true;
        if( type.empty() )
            return type.assign( "string", 6 ), true;
        break;

    case json::value_t::number_unsigned:
    case json::value_t::number_integer:
        // Float > Integer/Unsigned
        if( type.length() == 3 && type == "int" )
            return true;
        if( type.empty() )
            return type.assign( "int", 3 ), true;
        if( type == "float" )
            return true;  // float trumps integer
        break;

    case json::value_t::boolean:
        if( type.length() == 7 && type == "boolean" )
            return true;
        if( type.empty() )
            return type.assign( "boolean", 7 ), true;
        break;

    case json::value_t::null:
        if( ! type.empty() )
            return true;
        break;
    }
    return false;
}


template< typename... Rest >
bool type_from_value( std::string & type, json const & j, json const & j2, Rest... rest )
{
    type_from_value( type, j );
    return type_from_value( type, j2, std::forward< Rest >( rest )... );
}


static std::string parse_type( json const & j, size_t size, dds_option::option_properties & props )
{
    for( auto it = props.begin(); it != props.end(); ++it )
    {
        switch( it->length() )
        {
        case 5:
            if( 0 == it->compare( "float" ) )
                break;
            continue;
        case 6:
            if( 0 == it->compare( "string" ) )
                break;
            continue;
        case 3:
            if( 0 == it->compare( "int" ) )
                break;
            continue;
        case 7:
            if( 0 == it->compare( "boolean" ) )
                break;
            continue;
        case 4:
            if( 0 == it->compare( "IPv4" ) )
                break;
            if( 5 == size && 0 == it->compare( "enum" ) )
                break;
            continue;
        default:
            continue;
        }
        std::string property = *it;
        props.erase( it );
        return property;
    }
    // If not from the properties, try from the value at index 1:
    std::string type;
    switch( size )
    {
    case 3:  // [name,value,description]  ->  read-only
        if( type_from_value( type, j[1] ) )
            return type;
        break;

    case 4:  // [name,value,default,description]
        if( type_from_value( type, j[1], j[2] ) )
            return type;
        break;

    case 7:  // [name,value,min,max,step,default,description]
        if( type_from_value( type, j[1], j[2], j[3], j[4], j[5] ) )
            return type;
        break;

    case 5:  // [name,value,[choices],default,description]
        if( type_from_value( type, j[1], j[3] ) && type != "string" )
            DDS_THROW( runtime_error, "non-string enum values" );
        return "enum";

    default:
        DDS_THROW( runtime_error, "unexected size " << size << " of option json" );
    }
    DDS_THROW( runtime_error, "cannot deduce value type: " << j );
}


/*static*/ std::shared_ptr< dds_option > create_option( std::string const & type )
{
    if( type == "float" )
        return std::make_shared< dds_float_option >();
    if( type == "string" )
        return std::make_shared< dds_string_option >();
    if( type == "int" )
        return std::make_shared< dds_integer_option >();
    if( type == "boolean" )
        return std::make_shared< dds_boolean_option >();
    if( type == "IPv4" )
        return std::make_shared< dds_ip_option >();
    if( type == "enum" )
        return std::make_shared< dds_enum_option >();
    return {};
}


/*static*/ std::shared_ptr< dds_option > dds_option::from_json( json const & j )
{
    if( ! j.is_array() )
        DDS_THROW( runtime_error, "option expected as array: " << j );
    auto size = j.size();

    if( size < 1 )
        DDS_THROW( runtime_error, "no option name: " << j );
    auto & name = j[0].string_ref_or_empty();
    if( name.empty() )
        DDS_THROW( runtime_error, "invalid option name: " << j );

    if( size < 2 )
        DDS_THROW( runtime_error, "no option value: " << j );
    auto & value = j[1];

    if( size < 3 )
        DDS_THROW( runtime_error, "no option description: " << j );

    option_properties props;
    if( j.back().is_array() )
    {
        --size;  // don't count the properties
        props = parse_option_properties( j );
    }

    auto & j_description = j[size - 1];
    if( ! j_description.is_string() )
        DDS_THROW( runtime_error, "invalid description: " << j );
    auto & description = j_description.string_ref();

    auto type = parse_type( j, size, props );
    std::shared_ptr< dds_option > option = create_option( type );
    if( ! option )
        DDS_THROW( runtime_error, "invalid " << type << " option: " << j );

    option->init( name, description );
    option->init_properties( std::move( props ) );
    switch( size )
    {
    case 3:  // [name,value,description]  ->  read-only; no default value
        break;

    case 4:  // [name,value,default,description]
        option->init_default_value( j[2] );
        break;

    case 7:  // [name,value,min,max,step,default,description]
        option->init_default_value( j[5] );
        option->init_range( j[2], j[3], j[4] );
        break;

    case 5:  // [name,value,[choices],default,description]
        std::dynamic_pointer_cast< dds_enum_option >( option )->init_choices( j[2] );
        option->init_default_value( j[3] );
        break;

    default:
        DDS_THROW( runtime_error, "unexpected option json size " << size );
    }

    // Finally, set the actual value
    option->init_value( value );
    // (and the option is now initialized)

    return option;
}


json dds_option::to_json() const
{
    json j = json::array();
    j += get_name();
    j += get_value();
    if( ! _minimum_value.is_null() || ! _maximum_value.is_null() || ! _stepping.is_null() )
    {
        j += _minimum_value.is_null() ? rsutils::null_json : _minimum_value;
        j += _maximum_value.is_null() ? rsutils::null_json : _maximum_value;
        j += _stepping.is_null() ? rsutils::null_json : _stepping;
    }
    if( get_default_value().exists() )
        j += get_default_value();
    j += get_description();

    auto props = props_to_json();
    if( ! props.empty() )
        j += props;

    return j;
}


json dds_option::props_to_json() const
{
    json props = json::array();
    if( is_optional() )
    {
        props += "optional";
        if( get_value().is_null() && ( ! get_default_value().exists() || get_default_value().is_null() ) )
            props += value_type();
    }
    if( is_read_only() && get_default_value().exists() )
        props += "read-only";
    return props;
}


/*static*/ dds_float_option::type dds_float_option::check_float( json const & value )
{
    try
    {
        // 1.1 is expressed as 1.10000002 as float but 1.1000000000000001 in double...
        // Really hard to check that we don't lose precision unless we convert back to string which will be rounded
        // anyway.
        return value.get< type >();
    }
    catch( ... )
    {
    }
    DDS_THROW( runtime_error, "not convertible to a float: " << value );
}


void dds_float_option::check_type( json & value ) const
{
    // We do not reset the value: "1.1" will become 1.100000023841858
    check_float( value );
}


/*static*/ dds_integer_option::type dds_integer_option::check_integer( json const & value )
{
    switch( value.type() )
    {
    case json::value_t::number_integer:
        return value.get< type >();

    case json::value_t::number_unsigned:
        if( value.get< json::number_unsigned_t >() > (uint64_t)std::numeric_limits< type >::max() )
            break;
        return value.get< type >();

    case json::value_t::number_float:
        if( value.get< type >() != value.get< json::number_float_t >() )
            break;
        return value.get< type >();
    }
    DDS_THROW( runtime_error, "not convertible to a signed integer: " << value );
}


void dds_integer_option::check_type( json & value ) const
{
    value = check_integer( value );
}


void dds_boolean_option::init_range( rsutils::json minimum_value,
                                     rsutils::json maximum_value,
                                     rsutils::json stepping )
{
    if( ! minimum_value.is_null() || ! maximum_value.is_null() || ! stepping.is_null() )
        DDS_THROW( runtime_error, "boolean options cannot have a range" );
    super::init_range( std::move( minimum_value ), std::move( maximum_value ), std::move( stepping ) );
}


/*static*/ dds_boolean_option::type dds_boolean_option::check_boolean( json const & value )
{
    switch( value.type() )
    {
    case json::value_t::boolean:
        return value.get< type >();

    case json::value_t::number_integer:
    case json::value_t::number_unsigned:
        switch( auto i = value.get< super::type >() )
        {
        case 0:
        case 1:
            return i != 0;
        }
        break;
    }
    DDS_THROW( runtime_error, "not convertible to a boolean: " << value );
}


void dds_boolean_option::check_type( json & value ) const
{
    value = check_boolean( value );
}


void dds_string_option::check_type( json & value ) const
{
    if( ! value.is_string() )
        DDS_THROW( runtime_error, "not a string: " << value );
}


void dds_enum_option::init_choices( rsutils::json choices )
{
    verify_uninitialized();

    if( ! choices.is_array() )
        DDS_THROW( runtime_error, "enum option requires a choices array" );
    if( ! _minimum_value.is_null() || ! _maximum_value.is_null() || ! _stepping.is_null() )
        DDS_THROW( runtime_error, "enum options cannot have a range" );

    _choices.clear();
    for( auto const & choice : choices )
    {
        if( ! choice.is_string() )
            DDS_THROW( runtime_error, "enum choices must be strings" );
        _choices.push_back( choice.string_ref() );
    }
}


int dds_enum_option::get_value_index( std::string const & value ) const
{
    for( int i = 0; i < _choices.size(); ++i )
        if( _choices[i] == value )
            return i;
    return -1;
}


void dds_enum_option::check_value( json & value ) const
{
    super::check_value( value );

    if( ! value.is_null() )
    {
        if( get_value_index( value ) < 0 )
            DDS_THROW( runtime_error, "invalid enum value: " << value );
    }
}


json dds_enum_option::to_json() const
{
    auto j = super::to_json();
    j.insert( j.begin() + 2, get_choices() );
    return j;
}


/*static*/ rsutils::string::ip_address dds_ip_option::check_ip( json const & value )
{
    ip_address ip( value.string_ref() );
    if( ip.is_valid() )
        return ip;
    DDS_THROW( runtime_error, "not an IP address: " << value );
}


void dds_ip_option::check_type( json & value ) const
{
    check_ip( value );
}


json dds_ip_option::props_to_json() const
{
    json j = super::props_to_json();
    j += "IPv4";
    return j;
}


}  // namespace realdds
