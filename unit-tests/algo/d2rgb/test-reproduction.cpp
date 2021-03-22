// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../src/algo/depth-to-rgb-calibration/*.cpp
//#cmake:add-file ../../../src/algo/thermal-loop/*.cpp

//#test:flag custom-args    # disable passing in of catch2 arguments

// We have our own main
#define NO_CATCH_CONFIG_MAIN
//#define CATCH_CONFIG_RUNNER

#define DISABLE_LOG_TO_STDOUT
#include "d2rgb-common.h"


template< typename T >
void read_binary_file( char const * dir, char const * bin, T * data )
{
    std::string filename = join( dir, bin );
    AC_LOG( DEBUG, "... " << filename );
    std::fstream f = std::fstream( filename, std::ios::in | std::ios::binary );
    if( ! f )
        throw std::runtime_error( "failed to read file:\n" + filename );
    f.seekg( 0, f.end );
    size_t cb = f.tellg();
    f.seekg( 0, f.beg );
    if( cb != sizeof( T ) )
        throw std::runtime_error( to_string()
            << "file size (" << cb << ") does not match data size (" << sizeof(T) << "): " << filename );
    std::vector< T > vec( cb / sizeof( T ));
    f.read( (char*) data, cb );
    f.close();
}

struct old_algo_calib
{
    algo::matrix_3x3 rot;
    algo::translation trans;
    double __fx, __fy, __ppx, __ppy;  // not in new calib!
    algo::k_matrix k_mat;
    int           width, height;
    rs2_distortion model;
    double         coeffs[5];

    operator algo::calib() const
    {
        algo::calib c;
        c.rot = rot;
        c.trans = trans;
        c.k_mat = k_mat;
        c.width = width;
        c.height = height;
        c.model = model;
        for( auto x = 0; x < 5; ++x )
            c.coeffs[x] = coeffs[x];
        return c;
    }
};


class custom_ac_logger : public ac_logger
{
    typedef ac_logger super;

    std::string _codes;

public:
    custom_ac_logger() {}

    std::string const & get_codes() const { return _codes; }
    void reset() { _codes.clear(); }

protected:
    void on_log( char severity, char const * message ) override
    {
        super::on_log( severity, message );

        // parse it for keyword
        auto cch = strlen( message );
        if( cch < 4 || message[cch - 1] != ']' )
            return;
        char const * end = message + cch - 1;
        char const * start = end - 1;
        while( *start != '[' )
        {
            if( *start == ' ' )
                return;
            if( --start == message )
                return;
        }
        if( start[-1] != ' ' )
            return;
        // parse it for parameters
        char const * param = message;
        std::string values;
        while( param < start )
        {
            if( *param == '{'  &&  ++param < start )
            {
                char const * param_end = param;
                while( *param_end != '}'  &&  param_end < start )
                    ++param_end;
                if( param_end == start || param_end - param < 1 )
                    break;
                char const * value = param_end + 1;
                char const * value_end = value;
                while( *value_end != ' ' )
                    ++value_end;
                if( value_end == value )
                    break;
                if( value_end - value > 1  &&  strchr( ".;", value_end[-1] ))
                    --value_end;
                std::string param_name( param, param_end - param );
                std::string param_value( value, value_end - value );
                if( ! values.empty() )
                    values += ' ';
                values += param_name;
                values += '=';
                values += param_value;
                param = value_end;
            }
            ++param;
        }
        std::string code( start + 1, end - start - 1 );
        if( ! values.empty() )
            code += '[' + values + ']';
        if( ! _codes.empty() )
            _codes += ' ';
        _codes += code;
    }
};


int main( int argc, char * argv[] )
{
    custom_ac_logger logger;

    bool ok = true;
    bool debug_mode = false;
    // Each of the arguments is the path to a directory to simulate
    // We skip argv[0] which is the path to the executable
    // We don't complain if no arguments -- that's how we'll run as part of unit-testing
    for( int i = 1; i < argc; ++i )
    {
        try
        {
            char const * dir = argv[i];
            if( !strcmp( dir, "--version" ) )
            {
                // The build number is only available within Jenkins and so we have to hard-
                // code it ><
                std::cout << RS2_API_VERSION_STR << ".2158" << std::endl;
                continue;
            }
            if( ! strcmp( dir, "--debug" ) || ! strcmp( dir, "-d" ) )
            {
                debug_mode = true;
                continue;
            }
            std::cout << "Processing: " << dir << " ..." << std::endl;

            algo::calib calibration;
            try
            {
                read_binary_file( dir, "rgb.calib", &calibration );
            }
            catch( std::exception const & e )
            {
                std::cout << "!! failed: " << e.what() << std::endl;
                old_algo_calib old_calibration;
                read_binary_file( dir, "rgb.calib", &old_calibration );
                calibration = old_calibration;
            }

            camera_params camera;
            camera.rgb = calibration.get_intrinsics();
            camera.extrinsics = calibration.get_extrinsics();
            algo::rs2_intrinsics_double d_intr;  // intrinsics written in double!
            read_binary_file( dir, "depth.intrinsics", &d_intr );
            camera.z = d_intr;
            read_binary_file( dir, "depth.units", &camera.z_units );
            read_binary_file( dir, "cal.info", &camera.cal_info );
            read_binary_file( dir, "cal.registers", &camera.cal_regs );
            read_binary_file( dir, "dsm.params", &camera.dsm_params );

            if( camera.cal_regs.EXTLdsmXoffset < 0. || camera.cal_regs.EXTLdsmXoffset > 100000.
                || camera.cal_regs.EXTLdsmXscale < 0. || camera.cal_regs.EXTLdsmXscale > 100000.
                || camera.cal_regs.EXTLdsmYoffset < 0 || camera.cal_regs.EXTLdsmYoffset > 100000.
                || camera.cal_regs.EXTLdsmYscale < 0 || camera.cal_regs.EXTLdsmYscale > 100000. )
            {
                throw std::invalid_argument( "cal.registers file is malformed! (hexdump -v -e '4/ \"%f  \"')" );
            }

            algo::optimizer::settings settings;
            try
            {
                read_binary_file( dir, "settings", &settings );
            }
            catch( std::exception const & e )
            {
                std::cout << "!! failed: " << e.what() << " -> assuming [MANUAL LONG 9 @40degC]" << std::endl;
                settings.digital_gain = RS2_DIGITAL_GAIN_HIGH;
                settings.hum_temp = 40;
                settings.is_manual_trigger = true;
                settings.receiver_gain = 9;
            }

            // If both the raw intrinsics (pre-thermal) and the thermal table exist, apply a thermal
            // manipulation. Otherwise just take the final intrinsics from rgb.calib.
            try
            {
                rs2_intrinsics raw_rgb_intr;
                read_binary_file( dir, "raw_rgb.intrinsics", &raw_rgb_intr );

                auto vec = read_vector_from< byte >( join( dir, "rgb_thermal_table" ) );
                thermal::l500::thermal_calibration_table thermal_table( vec );
                
                auto scale = thermal_table.get_thermal_scale( settings.hum_temp );
                AC_LOG( DEBUG, "Thermal {scale}" << scale << " [TH]" );
                raw_rgb_intr.fx = float( raw_rgb_intr.fx * scale );
                raw_rgb_intr.fy = float( raw_rgb_intr.fy * scale );
                camera.rgb = raw_rgb_intr;
            }
            catch( std::exception const & )
            {
                AC_LOG( ERROR, "Could not read raw_rgb.intrinsics or rgb_thermal_table; using rgb.calib [NO-THERMAL]" );
            }

            algo::optimizer cal( settings, debug_mode );
            std::string status;

            {
                memory_profiler profiler;
                init_algo( cal,
                           dir,
                           "rgb.raw",
                           "rgb_prev.raw",
                           "rgb_last_successful.raw",
                           "ir.raw",
                           "depth.raw",
                           camera,
                           &profiler );

                status = logger.get_codes();
                logger.reset();

                profiler.section( "is_scene_valid" );
                if( ! cal.is_scene_valid() )
                {
                    TRACE( "-E- SCENE_INVALID" );
                    if( ! status.empty() )
                        status += ' ';
                    status += "SCENE_INVALID";
                }
                profiler.section_end();

                profiler.section( "optimize" );
                size_t n_iteration = cal.optimize();
                profiler.section_end();

                std::string results;
                profiler.section( "is_valid_results" );
                if( ! cal.is_valid_results() )
                {
                    TRACE( "NOT VALID\n" );
                    results = "BAD_RESULT";
                }
                else
                {
                    results = "SUCCESSFUL";
                }
                profiler.section_end();

                if( ! logger.get_codes().empty() )
                {
                    if( ! status.empty() )
                        status += ' ';
                    status += logger.get_codes();
                    logger.reset();
                }
                if( ! status.empty() )
                    status += ' ';
                status += results;
            }

            TRACE( "\n___\nRESULTS:  (" << RS2_API_VERSION_STR << " build 2158)" );

            auto intr = cal.get_calibration().get_intrinsics();
            intr.model = RS2_DISTORTION_INVERSE_BROWN_CONRADY;  // restore LRS model
            auto extr = cal.get_calibration().get_extrinsics();
            AC_LOG( DEBUG, AC_D_PREC << "intr" << (rs2_intrinsics)intr );
            AC_LOG( DEBUG, AC_D_PREC << "extr" << (rs2_extrinsics)extr );
            AC_LOG( DEBUG, AC_D_PREC << "dsm" << cal.get_dsm_params() );

            try
            {
                algo::validate_dsm_params( cal.get_dsm_params() );
            }
            catch( librealsense::invalid_value_exception const & e )
            {
                AC_LOG( ERROR, "Exception: " << e.what() );
                if( ! status.empty() )
                    status += ' ';
                status += logger.get_codes();
                logger.reset();
            }

            TRACE( "\n___\nVS:" );
            AC_LOG( DEBUG, AC_D_PREC << "intr" << (rs2_intrinsics)calibration.get_intrinsics() );
            AC_LOG( DEBUG, AC_D_PREC << "extr" << (rs2_extrinsics)calibration.get_extrinsics() );
            AC_LOG( DEBUG, AC_D_PREC << "dsm" << camera.dsm_params );
         
            TRACE( "\n___\nSTATUS: " + status );
        }
        catch( std::exception const & e )
        {
            std::cerr << "\n___\ncaught exception: " << e.what() << std::endl;
            ok = false;
        }
        catch( ... )
        {
            std::cerr << "\n___\ncaught unknown exception!" << std::endl;
            ok = false;
        }
    }
    
    return ! ok;
}

