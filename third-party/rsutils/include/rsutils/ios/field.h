// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <string>
#include <iosfwd>


namespace rsutils {
namespace ios {


// Many times when we write the contents of some structure to a stream, multiple "fields" are involved. We want to allow
// a standard way of providing:
//     - field separation
//     - value separation
//     - both same-line and indented (multiline) options for big dumps
//     - flexibility: not requiring a field name, value, nor any formatting or either
// 
// Indented output puts each field on its own line. To enable multiline output, simply set the indent:
//      os << rsutils::ios::indent();
//      os << my_structure;
//      os << rsutils::ios::unindent();
// See group() for another way of doing this.
//
struct field
{
    // Separate one field from the previous:
    //     os << field::separator << "whatever";
    // If any indent is set, this will be on a separate line!
    static std::ostream & separator( std::ostream & );

    // Separate one field from the previous, but force inline:
    //     os << field::sameline << "another one";
    static std::ostream & sameline( std::ostream & );

    // Inserts a separator between the field name and value
    //     os << field::separator << "key" << field::value << value;
    static std::ostream & value( std::ostream & );

    // Start a "group" of fields that are indented, e.g.:
    //      os << "quality-of-service" << field::group() << _qos;
    // Which will output:
    //      quality-of-service
    //          field1 value1
    //          field2 value2
    //          ...
    // When inline:
    //      quality-of-service[ field1 value1 field2 value2 ... ]
    // Only valid to use while the ostream is valid!
    //
    struct group
    {
        mutable std::ostream * pos = nullptr;
        ~group();  // needed to unindent/close the group
    };
};


std::ostream & operator<<( std::ostream &, field::group const & );


}  // namespace ios
}  // namespace rsutils
