// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

//#cmake:dependencies realsense2 rsutils

#include <unit-tests/catch.h>

#include <src/ds/advanced_mode/json_loader.hpp>

#include <clocale>
#include <locale>
#include <string>


// Regression test for issue #15746: on systems whose OS locale uses a comma
// as the decimal separator (e.g. German, Danish), loading an advanced-mode
// JSON truncated every fractional parameter ("0.482143" -> 0, "1.8125" -> 1),
// because the conversion used ::atof(), which follows the process C locale.
//
// The comma-decimal environment is simulated with a custom numpunct facet, so
// the test does not depend on a de_DE / da_DK locale being installed. Where
// such a C locale does exist, it is switched in as well, since ::atof() (the
// old code) follows the C locale rather than the C++ one.

namespace {

class comma_decimal_numpunct : public std::numpunct< char >
{
protected:
    char do_decimal_point() const override { return ','; }
    char do_thousands_sep() const override { return '.'; }
    std::string do_grouping() const override { return "\3"; }
};


// Installs a comma-decimal locale for the duration of a test; restores the
// previous locales on destruction.
struct comma_decimal_locale_guard
{
    std::locale previous_cpp_locale;
    std::string previous_c_locale;
    bool c_locale_changed = false;

    comma_decimal_locale_guard()
        : previous_cpp_locale( std::locale::global(
              std::locale( std::locale::classic(), new comma_decimal_numpunct ) ) )
    {
        if( auto current = std::setlocale( LC_NUMERIC, nullptr ) )
            previous_c_locale = current;
        const char * candidates[] = { "de_DE.UTF-8", "de_DE", "da_DK.UTF-8", "da_DK",
                                      "fr_FR.UTF-8", "German_Germany", "de-DE" };
        for( auto name : candidates )
        {
            if( std::setlocale( LC_NUMERIC, name ) )
            {
                c_locale_changed = true;
                break;
            }
        }
    }

    ~comma_decimal_locale_guard()
    {
        std::locale::global( previous_cpp_locale );
        if( c_locale_changed && ! previous_c_locale.empty() )
            std::setlocale( LC_NUMERIC, previous_c_locale.c_str() );
    }
};

}  // namespace


TEST_CASE( "advanced-mode JSON float values survive a comma-decimal locale",
           "[ds][advanced-mode]" )
{
    comma_decimal_locale_guard guard;

    // The exact values from issue #15746
    librealsense::preset pr{};
    librealsense::preset_param_group p( pr );
    auto fields = librealsense::initialize_field_parsers( p );

    auto remove = fields["param-rsmremovethreshold"];
    auto diff = fields["param-rsmdiffthreshold"];
    remove->load( "0.482143" );
    diff->load( "1.8125" );

    // removeThresh is an integer field stored scaled by 168: 0.482143 * 168 -> 81
    CHECK( p.rsm.vals[0].removeThresh == 81 );
    CHECK( p.rsm.vals[0].diffThresh == Catch::Approx( 1.8125f ) );

    // A load -> save round-trip must reproduce the input byte-for-byte
    CHECK( remove->save() == "0.482143" );
    CHECK( diff->save() == "1.8125" );
}
