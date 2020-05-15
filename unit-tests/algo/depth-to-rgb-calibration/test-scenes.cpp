// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../src/algo/depth-to-rgb-calibration/*.cpp

// We have our own main
#define NO_CATCH_CONFIG_MAIN
#define CATCH_CONFIG_RUNNER

#include "d2rgb-common.h"
#include "compare-to-bin-file.h"

#include "compare-scene.h"
#include "../../filesystem.h"


int main( int argc, char * argv[] )
{
    Catch::Session session;
    LOG_TO_STDOUT.enable( false );

    Catch::ConfigData config;
    config.verbosity = Catch::Verbosity::NoOutput;

    bool ok = true;
    // Each of the arguments is the path to a directory to simulate
    // We skip argv[0] which is the path to the executable
    // We don't complain if no arguments -- that's how we'll run as part of unit-testing
    for( int i = 1; i < argc; ++i )
    {
        try
        {

            char const * dir = argv[i];
            if( !strcmp( dir, "-v" ) )
            {
                LOG_TO_STDOUT.enable();
                continue;
            }
            TRACE( "\n\nProcessing: " << dir << " ..." );
            Catch::CustomRunContext ctx( new Catch::Config( config ));

            glob( dir, "*.rsc",
                [&]( std::string const & match )
                {
                    TRACE( match );
                    ctx.test_case( match,
                        [&]()
                        {
                            REQUIRE_NOTHROW( compare_scene( get_parent( join( dir, match ) ) + native_separator ) );
                        } );
                } );

            TRACE( "done!\n\n" );
        }
        catch( std::exception const & e )
        {
            std::cerr << "caught exception: " << e.what() << std::endl;
            ok = false;
        }
        catch( ... )
        {
            std::cerr << "caught unknown exception!" << std::endl;
            ok = false;
        }
    }

    return !ok;
}
