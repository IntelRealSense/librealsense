// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../src/algo/depth-to-rgb-calibration/*.cpp
//#cmake:add-file ../../../src/algo/thermal-loop/*.cpp

//#test:flag custom-args    # disable passing in of catch2 arguments

// We have our own main
#define NO_CATCH_CONFIG_MAIN
#define CATCH_CONFIG_RUNNER

#define DISABLE_LOG_TO_STDOUT
#include "d2rgb-common.h"
#include "compare-to-bin-file.h"

#include "compare-scene.h"
#include "../../filesystem.h"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#define _log log
#define _close close
#define _fileno fileno
#define _dup dup
#define _dup2 dup2
#endif


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
    std::cout << std::left << std::setw( 70 ) << "-----";
    std::cout << std::left << std::setw( 10 ) << "----------";
    std::cout << std::right << std::setw( 10 ) << "-----";
    std::cout << std::right << std::setw( 10 ) << "-------";
    std::cout << std::right << std::setw( 6 ) << "---";
    std::cout << std::right << std::setw( 2 ) << " ";
    std::cout << std::right << std::setw( 4 ) << "---";
    std::cout << std::right << std::setw( 2 ) << " ";
    std::cout << std::right << std::setw( 4 ) << "---";
    std::cout << std::right << std::setw( 2 ) << " ";
    std::cout << std::right << std::setw( 7 ) << "-----";
    std::cout << std::right << std::setw(2) << " ";
    std::cout << std::right << std::setw(4) << "---";
    std::cout << std::right << std::setw( 2 ) << " ";
    std::cout << std::right << std::setw( 8 ) << "---";
    std::cout << std::endl;
}

void print_headers()
{
    std::cout << std::right << std::setw( 7 ) << "Failed ";
    std::cout << std::left << std::setw( 70 ) << "Name";
    std::cout << std::left << std::setw( 10 ) << "Cost";
    std::cout << std::right << std::setw( 10 ) << "%diff";
    std::cout << std::right << std::setw( 10 ) << "Pixels";
    std::cout << std::right << std::setw( 6 ) << "SV";
    std::cout << std::right << std::setw( 2 ) << " ";
    std::cout << std::right << std::setw( 4 ) << "OV";
    std::cout << std::right << std::setw( 2 ) << " ";
    std::cout << std::right << std::setw( 4 ) << "Con";
    std::cout << std::right << std::setw( 2 ) << " ";
    std::cout << std::right << std::setw( 7 ) << "dPix";
    std::cout << std::right << std::setw(2) << " ";
    std::cout << std::right << std::setw(4) << "nC";
    std::cout << std::right << std::setw( 2 ) << " ";
    std::cout << std::right << std::setw( 8 ) << "MEM";
    std::cout << std::endl;

    print_dividers();
}

void print_scene_stats( std::string const & name, size_t n_failed, scene_stats const & scene )
{
    std::cout << std::right << std::setw( 6 ) << n_failed << ' ';

    std::cout << std::left << std::setw( 70 ) << name;

    std::cout << std::right << std::setw( 10 ) << std::fixed << std::setprecision( 2 ) << scene.cost;
    double matlab_cost = scene.cost - scene.d_cost;
    double d_cost_pct = abs( scene.d_cost ) * 100. / matlab_cost;
    std::cout << std::right << std::setw( 10 ) << d_cost_pct;

    std::cout << std::right << std::setw( 10 ) << scene.movement;

    std::cout << std::right << std::setw( 6 ) << scene.n_valid_scene;
    std::cout << std::left << std::setw( 2 ) << ( scene.n_valid_scene_diff ? "!" : "" );
    std::cout << std::right << std::setw( 4 ) << scene.n_valid_result;
    std::cout << std::left << std::setw( 2 ) << (scene.n_valid_result_diff ? "!" : "");
    std::cout << std::right << std::setw( 4 ) << scene.n_converged;
    std::cout << std::left << std::setw( 2 ) << (scene.n_converged_diff ? "!" : "");

    std::cout << std::right << std::setw( 7 ) << scene.d_movement;
    std::cout << std::right << std::setw(6) << scene.n_cycles;
    std::cout << std::right << std::setw( 10 ) << scene.memory_consumption_peak;
    std::cout << std::endl;
}


int main( int argc, char * argv[] )
{
    Catch::Session session;
    ac_logger LOG_TO_STDOUT( false );

    Catch::ConfigData config;
    config.verbosity = Catch::Verbosity::Normal;

    bool ok = true;
    bool verbose = false;
    bool stats = false;
    bool debug_mode = false;
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
            if( ! strcmp( dir, "-d" ) || ! strcmp( dir, "--debug" ) )
            {
                debug_mode = true;
                continue;
            }
            if( ! strcmp( dir, "--stats" ) )
            {
                stats = true;
                //config.outputFilename = "%debug";
                continue;
            }
            TRACE( "\n\nProcessing: " << dir << " ..." );
            Catch::CustomRunContext ctx( config );
            ctx.set_redirection( !verbose );
            scene_stats total = { 0 };
            size_t n_failed = 0;
            size_t n_scenes = 0;

            if( stats )
                print_headers();

            glob( dir, "yuy_prev_z_i.files",
                [&]( std::string const & match )
                {
                    // <scene_dir>/binFiles/ac2/<match>
                    std::string scene_dir = get_parent( join( dir, match ) );  // .../ac2
                    std::string ac2;
                    scene_dir = get_parent( scene_dir, &ac2 );  // .../binFiles
                    if( ac2 != "ac2" )
                        return;
                    std::string binFiles;
                    scene_dir = get_parent( scene_dir, &binFiles );
                    if( binFiles != "binFiles" )
                        return;
                    auto x = strlen( dir ) + 1;
                    std::string test_name = x > scene_dir.length() ? "." : scene_dir.substr( x );
                    scene_dir += native_separator;

                    scene_stats scene;
                    
                    Catch::Totals catch_total;
                    {
                        redirect_file no( stats ? stdout : stderr );
                        catch_total = ctx.run_test( test_name, [&]() {
                            REQUIRE_NOTHROW( compare_scene( scene_dir, debug_mode, &scene ) );
                        } );
                    }
                    
                    n_failed += catch_total.testCases.failed;
                    ++n_scenes;
                    total.cost += scene.cost;
                    total.d_cost += abs(scene.d_cost);
                    total.movement += scene.movement;
                    total.d_movement += abs(scene.d_movement);
                    total.n_valid_scene += scene.n_valid_scene;
                    total.n_valid_scene_diff += scene.n_valid_scene_diff;
                    total.n_valid_result += scene.n_valid_result;
                    total.n_valid_result_diff += scene.n_valid_result_diff;
                    total.n_converged += scene.n_converged;
                    total.n_converged_diff += scene.n_converged_diff;
                    total.memory_consumption_peak += scene.memory_consumption_peak;

                    if( stats )
                        print_scene_stats( test_name, catch_total.assertions.failed, scene );
                } );

            if( stats )
            {
                print_dividers();
                print_scene_stats( to_string()
                                       << "                                             " << n_scenes << " scene totals:",
                                   n_failed,
                                   total );
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
