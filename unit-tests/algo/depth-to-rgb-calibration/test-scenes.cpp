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

#include <io.h>


class redirect_file
{
    int _no;
    int _old_no;

public:
    redirect_file( FILE * f = stdout )
        : _no( _fileno( f ))
        , _old_no( _dup( _no ))
    {
        std::freopen( std::tmpnam( nullptr ), "w", f );
    }
    ~redirect_file()
    {
        _dup2( _old_no, _no );
        _close( _old_no );
    }
};


void print_dividers()
{
    std::cout << std::right << std::setw( 7 ) << "------ ";
    std::cout << std::left << std::setw( 50 ) << "-----";
    std::cout << std::left << std::setw( 10 ) << "----------";
    std::cout << std::right << std::setw( 10 ) << "-----";
    std::cout << std::right << std::setw( 10 ) << "-------";
    std::cout << std::right << std::setw( 10 ) << "-----";
    std::cout << std::endl;
}

void print_headers()
{
    std::cout << std::right << std::setw( 7 ) << "Failed ";
    std::cout << std::left << std::setw( 50 ) << "Name";
    std::cout << std::left << std::setw( 10 ) << "Cost";
    std::cout << std::right << std::setw( 10 ) << "%diff";
    std::cout << std::right << std::setw( 10 ) << "Pixels";
    std::cout << std::right << std::setw( 10 ) << "%diff";
    std::cout << std::endl;

    print_dividers();
}

void print_scene_stats( std::string const & name, size_t n_failed, scene_stats const & scene )
{
    std::cout << std::right << std::setw( 6 ) << n_failed << ' ';

    std::cout << std::left << std::setw( 50 ) << name;

    std::cout << std::right << std::setw( 10 ) << std::fixed << std::setprecision( 2 ) << scene.cost;
    double matlab_cost = scene.cost - scene.d_cost;
    double d_cost_pct = abs( scene.d_cost ) * 100. / matlab_cost;
    if( d_cost_pct > 1. )
    {
        Catch::Colour guard( Catch::Colour::Red );
        std::cout << std::right << std::setw( 10 ) << d_cost_pct;
    }
    else
        std::cout << std::right << std::setw( 10 ) << d_cost_pct;

    std::cout << std::right << std::setw( 10 ) << scene.movement;
    double matlab_movement = scene.movement - scene.d_movement;
    double d_movement_pct = abs( scene.d_movement ) * 100. / matlab_movement;
    if( d_movement_pct > 5. )
    {
        Catch::Colour guard( Catch::Colour::Red );
        std::cout << std::right << std::setw( 10 ) << d_movement_pct;
    }
    else
        std::cout << std::right << std::setw( 10 ) << d_movement_pct;

    std::cout << std::endl;
}


int main( int argc, char * argv[] )
{
    Catch::Session session;
    LOG_TO_STDOUT.enable( false );

    Catch::ConfigData config;
    config.verbosity = Catch::Verbosity::Normal;

    bool ok = true;
    bool verbose = false;
    bool stats = false;
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
                LOG_TO_STDOUT.enable( verbose = true );
                continue;
            }
            if( !strcmp( dir, "--stats" ) )
            {
                stats = true;
                //config.outputFilename = "%debug";
                continue;
            }
            TRACE( "\n\nProcessing: " << dir << " ..." );
            Catch::CustomRunContext ctx( config );
            ctx.set_redirection( !verbose );
            size_t n_failed = 0;
            size_t n_scenes = 0;
            double total_cost = 0;
            double total_cost_diff = 0;
            double total_movement = 0;
            double total_movement_diff = 0;

            if( stats )
                print_headers();

            glob( dir, "*.rsc",
                [&]( std::string const & match_ )
                {
                    std::string match = get_parent( match_ ).empty() ? match_ : get_parent( match_ );
                    std::string scene_dir = get_parent( join( dir, match_ ) ) + native_separator;
                    scene_stats scene;
                    
                    Catch::Totals total;
                    {
                        redirect_file no( stats ? stdout : stderr );
                        total = ctx.run_test( match, [&]() {
                            REQUIRE_NOTHROW( compare_scene( scene_dir, &scene ) );
                        } );
                    }
                    
                    n_failed += total.testCases.failed;
                    ++n_scenes;
                    total_cost += scene.cost;
                    total_cost_diff += abs(scene.d_cost);
                    total_movement += scene.movement;
                    total_movement_diff += abs(scene.d_movement);

                    if( stats )
                        print_scene_stats( match, total.assertions.failed, scene );
                } );

            if( stats )
            {
                scene_stats total;
                total.cost = total_cost;
                total.d_cost = total_cost_diff;
                total.movement = total_movement;
                total.d_movement = total_movement_diff;
                print_dividers();
                print_scene_stats( "                     total:", n_scenes, total );
            }

            TRACE( "done!\n\n" );
            ok &= ! n_failed;
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
