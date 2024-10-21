// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsutils/json.h>
#include <rsutils/json-config.h>
#include <rsutils/os/executable-name.h>

#include <fstream>


namespace rsutils {


json const null_json = {};
json const missing_json = json::value_t::discarded;
json const empty_json_string = json::value_t::string;
json const empty_json_object = json::object();


// Recursively patches existing JSON with contents of 'overrides', which must be a JSON object.
// A 'null' value inside erases previous contents. Any other value overrides.
// See: https://json.nlohmann.me/api/basic_json/merge_patch/
// Example below, for load_app_settings.
// Use 'what' to denote what it is we're patching in, if a failure happens. The std::runtime_error will populate with
// it.
// 
// NOTE: called 'override' to agree with terminology elsewhere, and to avoid collisions with the json patch(),
// merge_patch(), existing functions.
//
void json_base::override( json_ref patches, std::string const & what )
{
    if( ! patches.is_object() )
    {
        std::string context = what.empty() ? std::string( "patch", 5 ) : what;
        throw std::runtime_error( context + ": expecting an object; got " + patches.dump() );
    }

    try
    {
        static_cast< json * >( this )->merge_patch( patches );
    }
    catch( std::exception const & e )
    {
        std::string context = what.empty() ? std::string( "patch", 5 ) : what;
        throw std::runtime_error( "failed to merge " + context + ": " + e.what() );
    }
}


// Direct copy of ::nlohmann::detail::input_stream_adapter, which is no longer available since we use JSON_NO_IO
//
class input_stream_adapter
{
    /// the associated input stream
    std::istream * is = nullptr;
    std::streambuf * sb = nullptr;

public:
    using char_type = char;


    ~input_stream_adapter()
    {
        // clear stream flags; we use underlying streambuf I/O, do not
        // maintain ifstream flags, except eof
        if( is != nullptr )
        {
            is->clear( is->rdstate() & std::ios::eofbit );
        }
    }

    explicit input_stream_adapter( std::istream & i )
        : is( &i ), sb( i.rdbuf() )
    {}

    // delete because of pointer members
    input_stream_adapter( const input_stream_adapter & ) = delete;
    input_stream_adapter & operator=( input_stream_adapter & ) = delete;
    input_stream_adapter & operator=( input_stream_adapter && ) = delete;

    input_stream_adapter( input_stream_adapter && rhs ) noexcept
        : is( rhs.is ), sb( rhs.sb )
    {
        rhs.is = nullptr;
        rhs.sb = nullptr;
    }

    // std::istream/std::streambuf use std::char_traits<char>::to_int_type, to
    // ensure that std::char_traits<char>::eof() and the character 0xFF do not
    // end up as the same value, e.g. 0xFFFFFFFF.
    std::char_traits<char>::int_type get_character()
    {
        auto res = sb->sbumpc();
        // set eof manually, as we don't use the istream interface.
        if( res == std::char_traits<char>::eof() )
        {
            is->clear( is->rdstate() | std::ios::eofbit );
        }
        return res;
    }
};


// Load the contents of a file into a JSON object.
// 
// Throws if any errors are encountered with the file or its contents.
// Returns the contents. If the file wasn't there, returns missing_json.
//
json json_config::load_from_file( std::string const & filename )
{
    std::ifstream f( filename );
    if( f.good() )
    {
        try
        {
            json result;
            ::nlohmann::detail::parser< json, input_stream_adapter >( input_stream_adapter( f ) ).parse( true, result );
            return result;
        }
        catch( std::exception const & e )
        {
            throw std::runtime_error( "failed to load configuration file (" + filename
                                      + "): " + std::string( e.what() ) );
        }
    }
    return missing_json;
}



// Loads configuration settings from 'global' content (loaded by load_from_file()?).
// E.g., a configuration file may contain:
//     {
//         "context": {
//             "dds": {
//                 "enabled": false,
//                 "domain" : 5
//             }
//         },
//         ...
//     }
// This function will load a specific key 'context' inside and return it. The result will be a disabling of dds:
// Besides this "global" key, application-specific settings can override the global settings, e.g.:
//     {
//         "context": {
//             "dds": {
//                 "enabled": false,
//                 "domain" : 5
//             }
//         },
//         "realsense-viewer": {
//             "context": {
//                 "dds": { "enabled": null }
//             }
//         },
//         ...
//     }
// If the current application is 'realsense-viewer', then the global 'context' settings will be patched with the
// application-specific 'context' and returned:
//     {
//         "dds": {
//             "domain" : 5
//         }
//     }
// See rules for patching in patch().
// The 'application' is usually any single-word executable name (without extension).
// The 'subkey' is mandatory.
// The 'error_context' is used for error reporting, to show what failed. Like application, it should be a single word
// that can be used to denote hierarchy within the global json.
//
json json_config::load_app_settings( json const & global,
                                     std::string const & application,
                                     json_key const & subkey,
                                     std::string const & error_context )
{
    // Take the global subkey settings out of the configuration
    json settings;
    if( auto global_subkey = global.nested( subkey ) )
        settings.override( global_subkey, "global " + error_context + '/' + subkey );

    // Patch any application-specific subkey settings
    if( auto application_subkey = global.nested( application, subkey ) )
        settings.override( application_subkey, error_context + '/' + application + '/' + subkey );

    return settings;
}


// Same as above, but automatically takes the application name from the executable-name
//
json json_config::load_settings( json const & global,
                                 json_key const & subkey,
                                 std::string const & error_context )
{
    return load_app_settings( global, rsutils::os::executable_name(), subkey, error_context );
}


namespace nl = ::nlohmann::detail;
#define JSON_HEDLEY_LIKELY(expr) (!!(expr))
#define JSON_HEDLEY_UNLIKELY(expr) (!!(expr))
#define JSON_ASSERT(expr) assert(expr)
#define JSON_THROW(exception) throw exception

// This is an adaptation of ::nlohmann::detail::serializer
// Main idea is to improve on its array handling to improve readability and compactness of representation.
// Instead of [
//      [
//          1,
//          2,
//          3
//      ]
//  ]
// Shows: [
//      [1,2,3]
//  ]
//
class serializer
{
    using string_t = typename json::string_t;
    using number_float_t = typename json::number_float_t;
    using number_integer_t = typename json::number_integer_t;
    using number_unsigned_t = typename json::number_unsigned_t;
    using binary_char_t = typename json::binary_t::value_type;

    static constexpr std::uint8_t UTF8_ACCEPT = 0;
    static constexpr std::uint8_t UTF8_REJECT = 1;

    /// the output of the serializer
    std::ostream & _o;

    /// a (hopefully) large enough character buffer
    std::array< char, 64 > number_buffer{ {} };

    /// the locale
    const std::lconv * loc = nullptr;
    /// the locale's thousand separator character
    const char thousands_sep;
    /// the locale's decimal point character
    const char decimal_point;

    /// string buffer
    std::array<char, 512> string_buffer{ {} };

    /// the indentation character
    const char indent_char;
    /// the indentation string
    string_t _indent_string;
    /// the number of characters in the current line
    size_t _line_width;

    /// error_handler how to react on decoding errors
    const nl::error_handler_t error_handler;

public:
    /*!
    @param[in] s  output stream to serialize to
    @param[in] ichar  indentation character to use
    @param[in] error_handler_  how to react on decoding errors
    */
    serializer( std::ostream & os, const char ichar )
        : _o( os )
        , loc( std::localeconv() )
        , thousands_sep( loc->thousands_sep == nullptr ? '\0' : std::char_traits< char >::to_char_type( *( loc->thousands_sep ) ) )
        , decimal_point( loc->decimal_point == nullptr ? '\0' : std::char_traits< char >::to_char_type( *( loc->decimal_point ) ) )
        , indent_char( ichar )
        , _indent_string( 512, indent_char )
        , error_handler( nl::error_handler_t::strict )
        , _line_width( 0 )
    {}

    // delete because of pointer members
    serializer(const serializer&) = delete;
    serializer& operator=(const serializer&) = delete;
    serializer(serializer&&) = delete;
    serializer& operator=(serializer&&) = delete;
    ~serializer() = default;

    void newline()
    {
        _o.put( '\n' );
        _line_width = 0;
    }
    void ch( char const c )
    {
        _o.put( c );
        ++_line_width;
    }
    void str( char const * lpsz, size_t cch )
    {
        _o.write( lpsz, static_cast< std::streamsize >( cch ) );
        _line_width += cch;
    }
    void str( string_t const & s )
    {
        str( s.c_str(), s.length() );
    }

    void dump(const json & val,
              const int pretty_print_width,
              const bool ensure_ascii,
              const unsigned int indent_step,
              const unsigned int current_indent = 0)
    {
        switch( val.type() )
        {
        case json::value_t::object: {

            auto const size = val.size();
            if( ! size )
            {
                str( "{}", 2 );
                return;
            }

            if( pretty_print_width )
            {
                ch( '{' );
                newline();

                // variable to hold indentation for recursive calls
                const auto new_indent = current_indent + indent_step;
                if( JSON_HEDLEY_UNLIKELY( _indent_string.size() < new_indent ) )
                {
                    _indent_string.resize( _indent_string.size() * 2, ' ' );
                }

                // first n-1 elements
                auto i = val.cbegin();
                for( std::size_t cnt = 0; cnt < size - 1; ++cnt, ++i )
                {
                    _o.write( _indent_string.c_str(), new_indent );  // not part of line width
                    ch( '\"' );
                    dump_escaped( i.key(), ensure_ascii );
                    str( "\": ", 3 );
                    dump( i.value(), pretty_print_width, ensure_ascii, indent_step, new_indent );
                    ch( ',' );
                    newline();
                }

                // last element
                _o.write( _indent_string.c_str(), new_indent );  // not part of line width
                ch( '\"' );
                dump_escaped( i.key(), ensure_ascii );
                str( "\": ", 3 );
                dump( i.value(), pretty_print_width, ensure_ascii, indent_step, new_indent );

                newline();
                _o.write( _indent_string.c_str(), current_indent );  // not part of line width
                ch( '}' );
            }
            else
            {
                ch( '{' );

                // first n-1 elements
                auto i = val.cbegin();
                for( std::size_t cnt = 0; cnt < size - 1; ++cnt, ++i )
                {
                    ch( '\"' );
                    dump_escaped( i.key(), ensure_ascii );
                    str( "\":", 2 );
                    dump( i.value(), pretty_print_width, ensure_ascii, indent_step, current_indent );
                    ch( ',' );
                }

                // last element
                ch( '\"' );
                dump_escaped( i.key(), ensure_ascii );
                str( "\":", 2 );
                dump( i.value(), pretty_print_width, ensure_ascii, indent_step, current_indent );

                ch( '}' );
            }

            return;
        }

        case json::value_t::array: {

            auto const size = val.size();
            if( ! size )
            {
                str( "[]", 2 );
                return;
            }

            const auto new_indent = current_indent + indent_step;
            if( JSON_HEDLEY_UNLIKELY( _indent_string.size() < new_indent ) )
                _indent_string.resize( _indent_string.size() * 2, ' ' );

            _o.put( '[' );

            bool need_to_indent = false;
            if( pretty_print_width )
            {
                for( auto i = val.cbegin(); i != val.cend(); ++i )
                {
                    if( i->is_structured() || i->is_binary() )
                    {
                        need_to_indent = true;
                        newline();
                        _o.write( _indent_string.c_str(), new_indent );
                        break;
                    }
                }
            }

            // first n-1 elements
            for( auto i = val.cbegin(); i != val.cend() - 1; ++i )
            {
                dump( *i, pretty_print_width, ensure_ascii, indent_step, new_indent );
                _o.put( ',' );
                if( need_to_indent || (pretty_print_width && _line_width > pretty_print_width ))
                {
                    newline();
                    _o.write( _indent_string.c_str(), new_indent );
                }
            }

            // last element
            dump( val.back(), pretty_print_width, ensure_ascii, indent_step, new_indent );
            if( need_to_indent )
            {
                newline();
                _o.write( _indent_string.c_str(), current_indent );
            }
            _o.put( ']' );
            return;
        }

        case json::value_t::string: {
            ch( '\"' );
            dump_escaped( val.string_ref(), ensure_ascii );
            ch( '\"' );
            return;
        }

        case json::value_t::binary: {
            if( pretty_print_width )
            {
                ch( '{' );
                newline();

                // variable to hold indentation for recursive calls
                const auto new_indent = current_indent + indent_step;
                if( JSON_HEDLEY_UNLIKELY( _indent_string.size() < new_indent ) )
                    _indent_string.resize( _indent_string.size() * 2, ' ' );

                _o.write( _indent_string.c_str(), new_indent );

                str( "\"bytes\": [", 10 );
                auto & binary = val.get_binary();

                if( ! binary.empty() )
                {
                    for( auto i = binary.cbegin(); i != binary.cend() - 1; ++i )
                    {
                        dump_integer( *i );
                        str( ", ", 2 );
                    }
                    dump_integer( binary.back() );
                }

                str( "],", 2 );
                newline();
                _o.write( _indent_string.c_str(), new_indent );

                str( "\"subtype\": ", 11 );
                if( binary.has_subtype() )
                    dump_integer( binary.subtype() );
                else
                    str( "null", 4 );
                newline();
                _o.write( _indent_string.c_str(), current_indent );
                ch( '}' );
            }
            else
            {
                str( "{\"bytes\":[", 10 );
                auto & binary = val.get_binary();

                if( ! binary.empty() )
                {
                    for( auto i = binary.cbegin(); i != binary.cend() - 1; ++i )
                    {
                        dump_integer( *i );
                        ch( ',' );
                    }
                    dump_integer( binary.back() );
                }

                str( "],\"subtype\":", 12 );
                if( binary.has_subtype() )
                {
                    dump_integer( binary.subtype() );
                    ch( '}' );
                }
                else
                {
                    str( "null}", 5 );
                }
            }
            return;
        }

        case json::value_t::boolean: {
            if( val.get< bool >() )
            {
                str( "true", 4 );
            }
            else
            {
                str( "false", 5 );
            }
            return;
        }

        case json::value_t::number_integer: {
            dump_integer( val.get< number_integer_t >() );
            return;
        }

        case json::value_t::number_unsigned: {
            dump_integer( val.get< number_unsigned_t >() );
            return;
        }

        case json::value_t::number_float: {
            dump_float( val.get< number_float_t >() );
            return;
        }

        case json::value_t::discarded: {
            str( "<discarded>", 11 );
            return;
        }

        case json::value_t::null: {
            str( "null", 4 );
            return;
        }

        default:
            JSON_ASSERT( false );
        }
    }

private:
    /*!
    @brief dump escaped string

    Escape a string by replacing certain special characters by a sequence of an
    escape character (backslash) and another character and other control
    characters by a sequence of "\u" followed by a four-digit hex
    representation. The escaped string is written to output stream @a o.

    @param[in] s  the string to escape
    @param[in] ensure_ascii  whether to escape non-ASCII characters with
                             \uXXXX sequences

    @complexity Linear in the length of string @a s.
    */
    void dump_escaped(const string_t& s, const bool ensure_ascii)
    {
        std::uint32_t codepoint{};
        std::uint8_t state = UTF8_ACCEPT;
        std::size_t bytes = 0;  // number of bytes written to string_buffer

        // number of bytes written at the point of the last valid byte
        std::size_t bytes_after_last_accept = 0;
        std::size_t undumped_chars = 0;

        for (std::size_t i = 0; i < s.size(); ++i)
        {
            const auto byte = static_cast<std::uint8_t>(s[i]);

            switch (decode(state, codepoint, byte))
            {
                case UTF8_ACCEPT:  // decode found a new code point
                {
                    switch (codepoint)
                    {
                        case 0x08: // backspace
                        {
                            string_buffer[bytes++] = '\\';
                            string_buffer[bytes++] = 'b';
                            break;
                        }

                        case 0x09: // horizontal tab
                        {
                            string_buffer[bytes++] = '\\';
                            string_buffer[bytes++] = 't';
                            break;
                        }

                        case 0x0A: // newline
                        {
                            string_buffer[bytes++] = '\\';
                            string_buffer[bytes++] = 'n';
                            break;
                        }

                        case 0x0C: // formfeed
                        {
                            string_buffer[bytes++] = '\\';
                            string_buffer[bytes++] = 'f';
                            break;
                        }

                        case 0x0D: // carriage return
                        {
                            string_buffer[bytes++] = '\\';
                            string_buffer[bytes++] = 'r';
                            break;
                        }

                        case 0x22: // quotation mark
                        {
                            string_buffer[bytes++] = '\\';
                            string_buffer[bytes++] = '\"';
                            break;
                        }

                        case 0x5C: // reverse solidus
                        {
                            string_buffer[bytes++] = '\\';
                            string_buffer[bytes++] = '\\';
                            break;
                        }

                        default:
                        {
                            // escape control characters (0x00..0x1F) or, if
                            // ensure_ascii parameter is used, non-ASCII characters
                            if ((codepoint <= 0x1F) || (ensure_ascii && (codepoint >= 0x7F)))
                            {
                                if (codepoint <= 0xFFFF)
                                {
                                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
                                    static_cast<void>((std::snprintf)(string_buffer.data() + bytes, 7, "\\u%04x",
                                                                      static_cast<std::uint16_t>(codepoint)));
                                    bytes += 6;
                                }
                                else
                                {
                                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
                                    static_cast<void>((std::snprintf)(string_buffer.data() + bytes, 13, "\\u%04x\\u%04x",
                                                                      static_cast<std::uint16_t>(0xD7C0u + (codepoint >> 10u)),
                                                                      static_cast<std::uint16_t>(0xDC00u + (codepoint & 0x3FFu))));
                                    bytes += 12;
                                }
                            }
                            else
                            {
                                // copy byte to buffer (all previous bytes
                                // been copied have in default case above)
                                string_buffer[bytes++] = s[i];
                            }
                            break;
                        }
                    }

                    // write buffer and reset index; there must be 13 bytes
                    // left, as this is the maximal number of bytes to be
                    // written ("\uxxxx\uxxxx\0") for one code point
                    if (string_buffer.size() - bytes < 13)
                    {
                        str( string_buffer.data(), bytes );
                        bytes = 0;
                    }

                    // remember the byte position of this accept
                    bytes_after_last_accept = bytes;
                    undumped_chars = 0;
                    break;
                }

                case UTF8_REJECT:  // decode found invalid UTF-8 byte
                {
                    switch( error_handler )
                    {
                    case nl::error_handler_t::strict: {
                        JSON_THROW( nl::type_error::create( 316,
                                                        nl::concat( "invalid UTF-8 byte at index ",
                                                                std::to_string( i ),
                                                                ": 0x",
                                                                hex_bytes( byte | 0 ) ),
                                                        nullptr ) );
                    }

                    case nl::error_handler_t::ignore:
                    case nl::error_handler_t::replace: {
                        // in case we saw this character the first time, we
                        // would like to read it again, because the byte
                        // may be OK for itself, but just not OK for the
                        // previous sequence
                        if( undumped_chars > 0 )
                        {
                            --i;
                        }

                        // reset length buffer to the last accepted index;
                        // thus removing/ignoring the invalid characters
                        bytes = bytes_after_last_accept;

                        if( error_handler == nl::error_handler_t::replace )
                        {
                            // add a replacement character
                            if( ensure_ascii )
                            {
                                string_buffer[bytes++] = '\\';
                                string_buffer[bytes++] = 'u';
                                string_buffer[bytes++] = 'f';
                                string_buffer[bytes++] = 'f';
                                string_buffer[bytes++] = 'f';
                                string_buffer[bytes++] = 'd';
                            }
                            else
                            {
                                string_buffer[bytes++]
                                    = nl::binary_writer< json, char >::to_char_type( '\xEF' );
                                string_buffer[bytes++]
                                    = nl::binary_writer< json, char >::to_char_type( '\xBF' );
                                string_buffer[bytes++]
                                    = nl::binary_writer< json, char >::to_char_type( '\xBD' );
                            }

                            // write buffer and reset index; there must be 13 bytes
                            // left, as this is the maximal number of bytes to be
                            // written ("\uxxxx\uxxxx\0") for one code point
                            if( string_buffer.size() - bytes < 13 )
                            {
                                str( string_buffer.data(), bytes );
                                bytes = 0;
                            }

                            bytes_after_last_accept = bytes;
                        }

                        undumped_chars = 0;

                        // continue processing the string
                        state = UTF8_ACCEPT;
                        break;
                    }

                    default:  // LCOV_EXCL_LINE
                        JSON_ASSERT(
                            false );  // NOLINT(cert-dcl03-c,hicpp-static-assert,misc-static-assert) LCOV_EXCL_LINE
                    }
                    break;
                }

                default:  // decode found yet incomplete multi-byte code point
                {
                    if (!ensure_ascii)
                    {
                        // code point will not be escaped - copy byte to buffer
                        string_buffer[bytes++] = s[i];
                    }
                    ++undumped_chars;
                    break;
                }
            }
        }

        // we finished processing the string
        if (JSON_HEDLEY_LIKELY(state == UTF8_ACCEPT))
        {
            // write buffer
            if (bytes > 0)
            {
                str( string_buffer.data(), bytes );
            }
        }
        else
        {
            // we finish reading, but do not accept: string was incomplete
            switch( error_handler )
            {
            case nl::error_handler_t::strict: {
                JSON_THROW(
                    nl::type_error::create( 316,
                                            nl::concat( "incomplete UTF-8 string; last byte: 0x",
                                                        hex_bytes( static_cast< std::uint8_t >( s.back() | 0 ) ) ),
                                            nullptr ) );
            }

            case nl::error_handler_t::ignore: {
                // write all accepted bytes
                str( string_buffer.data(), bytes_after_last_accept );
                break;
            }

            case nl::error_handler_t::replace: {
                // write all accepted bytes
                str( string_buffer.data(), bytes_after_last_accept );
                // add a replacement character
                if( ensure_ascii )
                {
                    str( "\\ufffd", 6 );
                }
                else
                {
                    str( "\xEF\xBF\xBD", 3 );
                }
                break;
            }

            default:
                JSON_ASSERT( false );
            }
        }
    }

  private:
    /*!
    @brief count digits

    Count the number of decimal (base 10) digits for an input unsigned integer.

    @param[in] x  unsigned integer number to count its digits
    @return    number of decimal digits
    */
    inline unsigned int count_digits(number_unsigned_t x) noexcept
    {
        unsigned int n_digits = 1;
        for (;;)
        {
            if (x < 10)
            {
                return n_digits;
            }
            if (x < 100)
            {
                return n_digits + 1;
            }
            if (x < 1000)
            {
                return n_digits + 2;
            }
            if (x < 10000)
            {
                return n_digits + 3;
            }
            x = x / 10000u;
            n_digits += 4;
        }
    }

    /*!
     * @brief convert a byte to a uppercase hex representation
     * @param[in] byte byte to represent
     * @return representation ("00".."FF")
     */
    static std::string hex_bytes(std::uint8_t byte)
    {
        std::string result = "FF";
        constexpr const char* nibble_to_hex = "0123456789ABCDEF";
        result[0] = nibble_to_hex[byte / 16];
        result[1] = nibble_to_hex[byte % 16];
        return result;
    }

    // templates to avoid warnings about useless casts
    template <typename NumberType, nl::enable_if_t<std::is_signed<NumberType>::value, int> = 0>
    bool is_negative_number(NumberType x)
    {
        return x < 0;
    }

    template < typename NumberType, nl::enable_if_t <std::is_unsigned<NumberType>::value, int > = 0 >
    bool is_negative_number(NumberType /*unused*/)
    {
        return false;
    }

    /*!
    @brief dump an integer

    Dump a given integer to output stream @a o. Works internally with
    @a number_buffer.

    @param[in] x  integer number (signed or unsigned) to dump
    @tparam NumberType either @a number_integer_t or @a number_unsigned_t
    */
    template < typename NumberType, nl::enable_if_t <
                   std::is_integral<NumberType>::value ||
                   std::is_same<NumberType, number_unsigned_t>::value ||
                   std::is_same<NumberType, number_integer_t>::value ||
                   std::is_same<NumberType, binary_char_t>::value,
                   int > = 0 >
    void dump_integer(NumberType x)
    {
        static constexpr std::array<std::array<char, 2>, 100> digits_to_99
        {
            {
                {{'0', '0'}}, {{'0', '1'}}, {{'0', '2'}}, {{'0', '3'}}, {{'0', '4'}}, {{'0', '5'}}, {{'0', '6'}}, {{'0', '7'}}, {{'0', '8'}}, {{'0', '9'}},
                {{'1', '0'}}, {{'1', '1'}}, {{'1', '2'}}, {{'1', '3'}}, {{'1', '4'}}, {{'1', '5'}}, {{'1', '6'}}, {{'1', '7'}}, {{'1', '8'}}, {{'1', '9'}},
                {{'2', '0'}}, {{'2', '1'}}, {{'2', '2'}}, {{'2', '3'}}, {{'2', '4'}}, {{'2', '5'}}, {{'2', '6'}}, {{'2', '7'}}, {{'2', '8'}}, {{'2', '9'}},
                {{'3', '0'}}, {{'3', '1'}}, {{'3', '2'}}, {{'3', '3'}}, {{'3', '4'}}, {{'3', '5'}}, {{'3', '6'}}, {{'3', '7'}}, {{'3', '8'}}, {{'3', '9'}},
                {{'4', '0'}}, {{'4', '1'}}, {{'4', '2'}}, {{'4', '3'}}, {{'4', '4'}}, {{'4', '5'}}, {{'4', '6'}}, {{'4', '7'}}, {{'4', '8'}}, {{'4', '9'}},
                {{'5', '0'}}, {{'5', '1'}}, {{'5', '2'}}, {{'5', '3'}}, {{'5', '4'}}, {{'5', '5'}}, {{'5', '6'}}, {{'5', '7'}}, {{'5', '8'}}, {{'5', '9'}},
                {{'6', '0'}}, {{'6', '1'}}, {{'6', '2'}}, {{'6', '3'}}, {{'6', '4'}}, {{'6', '5'}}, {{'6', '6'}}, {{'6', '7'}}, {{'6', '8'}}, {{'6', '9'}},
                {{'7', '0'}}, {{'7', '1'}}, {{'7', '2'}}, {{'7', '3'}}, {{'7', '4'}}, {{'7', '5'}}, {{'7', '6'}}, {{'7', '7'}}, {{'7', '8'}}, {{'7', '9'}},
                {{'8', '0'}}, {{'8', '1'}}, {{'8', '2'}}, {{'8', '3'}}, {{'8', '4'}}, {{'8', '5'}}, {{'8', '6'}}, {{'8', '7'}}, {{'8', '8'}}, {{'8', '9'}},
                {{'9', '0'}}, {{'9', '1'}}, {{'9', '2'}}, {{'9', '3'}}, {{'9', '4'}}, {{'9', '5'}}, {{'9', '6'}}, {{'9', '7'}}, {{'9', '8'}}, {{'9', '9'}},
            }
        };

        // special case for "0"
        if (x == 0)
        {
            ch('0');
            return;
        }

        // use a pointer to fill the buffer
        auto buffer_ptr = number_buffer.begin(); // NOLINT(llvm-qualified-auto,readability-qualified-auto,cppcoreguidelines-pro-type-vararg,hicpp-vararg)

        number_unsigned_t abs_value;

        unsigned int n_chars{};

        if (is_negative_number(x))
        {
            *buffer_ptr = '-';
            abs_value = remove_sign(static_cast<number_integer_t>(x));

            // account one more byte for the minus sign
            n_chars = 1 + count_digits(abs_value);
        }
        else
        {
            abs_value = static_cast<number_unsigned_t>(x);
            n_chars = count_digits(abs_value);
        }

        // spare 1 byte for '\0'
        JSON_ASSERT(n_chars < number_buffer.size() - 1);

        // jump to the end to generate the string from backward,
        // so we later avoid reversing the result
        buffer_ptr += n_chars;

        // Fast int2ascii implementation inspired by "Fastware" talk by Andrei Alexandrescu
        // See: https://www.youtube.com/watch?v=o4-CwDo2zpg
        while (abs_value >= 100)
        {
            const auto digits_index = static_cast<unsigned>((abs_value % 100));
            abs_value /= 100;
            *(--buffer_ptr) = digits_to_99[digits_index][1];
            *(--buffer_ptr) = digits_to_99[digits_index][0];
        }

        if (abs_value >= 10)
        {
            const auto digits_index = static_cast<unsigned>(abs_value);
            *(--buffer_ptr) = digits_to_99[digits_index][1];
            *(--buffer_ptr) = digits_to_99[digits_index][0];
        }
        else
        {
            *(--buffer_ptr) = static_cast<char>('0' + abs_value);
        }

        str(number_buffer.data(), n_chars);
    }

    /*!
    @brief dump a floating-point number

    Dump a given floating-point number to output stream @a o. Works internally
    with @a number_buffer.

    @param[in] x  floating-point number to dump
    */
    void dump_float(number_float_t x)
    {
        // NaN / inf
        if (!std::isfinite(x))
        {
            str("null", 4);
            return;
        }

        // If number_float_t is an IEEE-754 single or double precision number,
        // use the Grisu2 algorithm to produce short numbers which are
        // guaranteed to round-trip, using strtof and strtod, resp.
        //
        // NB: The test below works if <long double> == <double>.
        static constexpr bool is_ieee_single_or_double
            = (std::numeric_limits<number_float_t>::is_iec559 && std::numeric_limits<number_float_t>::digits == 24 && std::numeric_limits<number_float_t>::max_exponent == 128) ||
              (std::numeric_limits<number_float_t>::is_iec559 && std::numeric_limits<number_float_t>::digits == 53 && std::numeric_limits<number_float_t>::max_exponent == 1024);

        dump_float(x, std::integral_constant<bool, is_ieee_single_or_double>());
    }

    void dump_float(number_float_t x, std::true_type /*is_ieee_single_or_double*/)
    {
        auto* begin = number_buffer.data();
        auto* end = ::nlohmann::detail::to_chars(begin, begin + number_buffer.size(), x);

        str(begin, static_cast<size_t>(end - begin));
    }

    void dump_float(number_float_t x, std::false_type /*is_ieee_single_or_double*/)
    {
        // get number of digits for a float -> text -> float round-trip
        static constexpr auto d = std::numeric_limits<number_float_t>::max_digits10;

        // the actual conversion
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
        std::ptrdiff_t len = (std::snprintf)(number_buffer.data(), number_buffer.size(), "%.*g", d, x);

        // negative value indicates an error
        JSON_ASSERT(len > 0);
        // check if buffer was large enough
        JSON_ASSERT(static_cast<std::size_t>(len) < number_buffer.size());

        // erase thousands separator
        if (thousands_sep != '\0')
        {
            // NOLINTNEXTLINE(readability-qualified-auto,llvm-qualified-auto): std::remove returns an iterator, see https://github.com/nlohmann/json/issues/3081
            const auto end = std::remove(number_buffer.begin(), number_buffer.begin() + len, thousands_sep);
            std::fill(end, number_buffer.end(), '\0');
            JSON_ASSERT((end - number_buffer.begin()) <= len);
            len = (end - number_buffer.begin());
        }

        // convert decimal point to '.'
        if (decimal_point != '\0' && decimal_point != '.')
        {
            // NOLINTNEXTLINE(readability-qualified-auto,llvm-qualified-auto): std::find returns an iterator, see https://github.com/nlohmann/json/issues/3081
            const auto dec_pos = std::find(number_buffer.begin(), number_buffer.end(), decimal_point);
            if (dec_pos != number_buffer.end())
            {
                *dec_pos = '.';
            }
        }

        str(number_buffer.data(), static_cast<std::size_t>(len));

        // determine if we need to append ".0"
        const bool value_is_int_like =
            std::none_of(number_buffer.begin(), number_buffer.begin() + len + 1,
                         [](char c)
        {
            return c == '.' || c == 'e';
        });

        if (value_is_int_like)
        {
            str(".0", 2);
        }
    }

    /*!
    @brief check whether a string is UTF-8 encoded

    The function checks each byte of a string whether it is UTF-8 encoded. The
    result of the check is stored in the @a state parameter. The function must
    be called initially with state 0 (accept). State 1 means the string must
    be rejected, because the current byte is not allowed. If the string is
    completely processed, but the state is non-zero, the string ended
    prematurely; that is, the last byte indicated more bytes should have
    followed.

    @param[in,out] state  the state of the decoding
    @param[in,out] codep  codepoint (valid only if resulting state is UTF8_ACCEPT)
    @param[in] byte       next byte to decode
    @return               new state

    @note The function has been edited: a std::array is used.

    @copyright Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
    @sa http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
    */
    static std::uint8_t decode(std::uint8_t& state, std::uint32_t& codep, const std::uint8_t byte) noexcept
    {
        static const std::array<std::uint8_t, 400> utf8d =
        {
            {
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 00..1F
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 20..3F
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 40..5F
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 60..7F
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, // 80..9F
                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, // A0..BF
                8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // C0..DF
                0xA, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x3, // E0..EF
                0xB, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, // F0..FF
                0x0, 0x1, 0x2, 0x3, 0x5, 0x8, 0x7, 0x1, 0x1, 0x1, 0x4, 0x6, 0x1, 0x1, 0x1, 0x1, // s0..s0
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, // s1..s2
                1, 2, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, // s3..s4
                1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 1, // s5..s6
                1, 3, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 // s7..s8
            }
        };

        JSON_ASSERT(static_cast<size_t>(byte) < utf8d.size());
        const std::uint8_t type = utf8d[byte];

        codep = (state != UTF8_ACCEPT)
                ? (byte & 0x3fu) | (codep << 6u)
                : (0xFFu >> type) & (byte);

        const std::size_t index = 256u + static_cast<size_t>(state) * 16u + static_cast<size_t>(type);
        JSON_ASSERT(index < utf8d.size());
        state = utf8d[index];
        return state;
    }

    /*
     * Overload to make the compiler happy while it is instantiating
     * dump_integer for number_unsigned_t.
     * Must never be called.
     */
    number_unsigned_t remove_sign(number_unsigned_t x)
    {
        JSON_ASSERT(false); // NOLINT(cert-dcl03-c,hicpp-static-assert,misc-static-assert) LCOV_EXCL_LINE
        return x; // LCOV_EXCL_LINE
    }

    /*
     * Helper function for dump_integer
     *
     * This function takes a negative signed integer and returns its absolute
     * value as unsigned integer. The plus/minus shuffling is necessary as we can
     * not directly remove the sign of an arbitrary signed integer as the
     * absolute values of INT_MIN and INT_MAX are usually not the same. See
     * #1708 for details.
     */
    inline number_unsigned_t remove_sign(number_integer_t x) noexcept
    {
        JSON_ASSERT(x < 0 && x < (std::numeric_limits<number_integer_t>::max)()); // NOLINT(misc-redundant-expression)
        return static_cast<number_unsigned_t>(-(x + 1)) + 1;
    }

};

#undef JSON_THROW
#undef JSON_HEDLEY_LIKELY
#undef JSON_HEDLEY_UNLIKELY
#undef JSON_ASSERT


// Custom operator<< using our own serializer
// This collides with json::operator<<() so requires disabling json's default I/O with JSON_NO_IO
//
std::ostream & operator<<( std::ostream & o, const json & j )
{
    // read width member and use it as indentation parameter if nonzero
    const bool pretty_print = o.width() > 0;
    const auto indentation = pretty_print ? o.width() : 0;
    const int pretty_print_width = pretty_print ? 80 : 0;

    // reset width to 0 for subsequent calls to this stream
    o.width( 0 );

    // do the actual serialization
    serializer s( o, o.fill() );
    s.dump( j, pretty_print_width, false, static_cast<unsigned int>(indentation) );
    return o;
}


}  // namespace rsutils
