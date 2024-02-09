// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/json.h>
#include <rsutils/string/ip-address.h>

#include <string>
#include <vector>
#include <memory>
#include <set>


namespace realdds {


class dds_stream_base;


// Abstract base class for all option types
// 
// Options values can be:
//      - Read-only: No default value or range
//      - Optional: A 'null' value is accepted, meaning no value is set
// 
// An option is meant to be expressed in JSON, and can be created from it:
//      - R/O options    [name, value,                    description, [properties]]
//      - Simple options [name, value,           default, description, [properties]]
//      - Ranged options [name, value, range..., default, description, [properties]]
// Where range... is one or more parameters that describe the range.
// The value (and default) must adhere to the range (if not null).
//
class dds_option
{
protected:
    enum flag : unsigned
    {
        read_only = 0x01,  // only changed by server
        optional = 0x02    // can be 'null'
    };

    std::string _name;
    std::string _description;
    unsigned _flags;  // from the properties

    rsutils::json _j;
    rsutils::json _default_j;

    rsutils::json _minimum_value;
    rsutils::json _maximum_value;
    rsutils::json _stepping;

private:
    friend class dds_stream_base;
    std::weak_ptr< dds_stream_base > _stream;
    void init_stream( std::shared_ptr< dds_stream_base > const & );

public:
    using option_properties = std::set< std::string >;

    dds_option();

    // These init functions must happen before the first set_value()
    void init( std::string name, std::string description );
    void init_default_value( rsutils::json );  
    void init_range( rsutils::json const & min, rsutils::json const & max, rsutils::json const & step );
    void init_properties( option_properties && );
    void init_value( rsutils::json );

    bool is_read_only() const { return _flags & flag::read_only; }
    bool is_optional() const { return _flags & flag::optional; }

    virtual char const * value_type() const = 0;  // just for introspection; affects no logic

    const std::string & get_name() const { return _name; }
    const std::string & get_description() const { return _description; }

    std::shared_ptr< dds_stream_base > stream() const { return _stream.lock(); }

    rsutils::json const & get_value() const { return _j; }
    bool is_valid() const { return ! get_value().is_null(); }

    rsutils::json const & get_minimum_value() const { return _minimum_value; }
    rsutils::json const & get_maximum_value() const { return _maximum_value; }
    rsutils::json const & get_stepping() const { return _stepping; }

    void set_value( rsutils::json );
    virtual void check_value( rsutils::json & ) const;
    virtual void check_type( rsutils::json const & ) const = 0;

    rsutils::json const & get_default_value() const { return _default_j; }
    bool is_default_valid() const { return ! get_default_value().is_null(); }

    virtual rsutils::json to_json() const;
    static std::shared_ptr< dds_option > from_json( rsutils::json const & j );

protected:
    void verify_uninitialized() const;  // throws if already has a value (use by init_ functions)
    virtual rsutils::json props_to_json() const;
};

typedef std::vector< std::shared_ptr< dds_option > > dds_options;


class dds_float_option : public dds_option
{
    using super = dds_option;

public:
    using type = float; // rsutils::json::number_float_t;

public:
    type get_float() const { return get_value().get< type >(); }
    type get_default_float() const { return super::get_default_value().get< type >(); }

    char const * value_type() const override { return "float"; }

    void check_type( rsutils::json const & value ) const override;
    static type check_float( rsutils::json const & );
};


class dds_integer_option : public dds_option
{
    using super = dds_option;

public:
    using type = rsutils::json::number_integer_t;

    type get_integer() const { return get_value().get< type >(); }
    type get_default_integer() const { return super::get_default_value().get< type >(); }

    char const * value_type() const override { return "int"; }

    void check_type( rsutils::json const & ) const override;
    static type check_integer( rsutils::json const & );
};


class dds_string_option : public dds_option
{
    using super = dds_option;

public:
    char const * value_type() const override { return "string"; }

    void check_type( rsutils::json const & ) const override;

    std::string get_string() const { return get_value().get< std::string >(); }
};


class dds_ip_option : public dds_string_option
{
    using super = dds_string_option;

public:
    using ip_address = rsutils::string::ip_address;

    char const * value_type() const override { return "ip-address"; }

    void check_type( rsutils::json const & ) const override;
    static ip_address check_ip( rsutils::json const & );

    ip_address get_ip() const { return ip_address( get_value().string_ref() ); }

protected:
    rsutils::json props_to_json() const override;
};


}  // namespace realdds
