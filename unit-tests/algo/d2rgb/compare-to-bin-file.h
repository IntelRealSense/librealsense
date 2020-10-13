// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <type_traits>


//
// In all the following:
//     F = the type in the bin files, i.e. the "golden" value that we compare to
//     D = the type in memory, i.e. the result of our own computations
// Since approx compares approximately to a "golden", the golden needs to go to
// approx!
//


template< typename F, typename D >
bool is_equal_approximetly( F fx, D dx, bool print = true)
{
    return dx == approx( fx );
}

template< typename D>
bool compare_and_trace(D val_matlab, D val_cpp, std::string const & compared)
{
    if (val_cpp != approx(val_matlab))
    {
        AC_LOG(DEBUG, "... " << std::setprecision(16) << compared << ":  {matlab} " << val_matlab << " !~ " << val_cpp << " {cpp} Diff = " << val_matlab - val_cpp);
        return false;
    }
    return true;
}

template<>
bool is_equal_approximetly<algo::k_matrix, algo::k_matrix>( algo::k_matrix fx, algo::k_matrix dx, bool print)
{
    bool ok = true;

    ok = compare_and_trace(dx.get_fx(), fx.get_fx(), "fx") && ok;
    ok = compare_and_trace(dx.get_fy(), fx.get_fy(), "fy") && ok;
    ok = compare_and_trace(dx.get_ppx(), fx.get_ppx(), "ppx") && ok;
    ok = compare_and_trace(dx.get_ppy(), fx.get_ppy(), "ppy") && ok;

    return ok;
}

template<>
bool is_equal_approximetly<algo::rotation_in_angles, algo::rotation_in_angles>( algo::rotation_in_angles fx, algo::rotation_in_angles dx, bool print)
{
    return dx.alpha == approx( fx.alpha ) &&
        dx.beta == approx( fx.beta ) &&
        dx.gamma == approx( fx.gamma );
}

template<>
bool is_equal_approximetly<algo::p_matrix, algo::p_matrix>( algo::p_matrix fx, algo::p_matrix dx, bool print)
{
    bool ok = true;
    for( auto i = 0; i < 12; i++ )
    {
        if (print)
            ok = compare_and_trace(dx.vals[i], fx.vals[i], "p_matrix") && ok;
        else
            ok = is_equal_approximetly(dx.vals[i], fx.vals[i], false) && ok;

    }
    return ok;
}

template<>
bool is_equal_approximetly<std::vector<double>, std::vector<double>>(std::vector<double> fx, std::vector<double> dx, bool print)
{
    if(fx.size() != dx.size())
        return false;

    for (auto i = 0; i < fx.size(); i++)
    {
        if (dx[i] != approx(fx[i]))
            return false;
    }
    return true;
}

template<>
bool is_equal_approximetly<algo::double2, algo::double2>(algo::double2 f, algo::double2 d, bool print)
{
    if (print)
    {
        bool ok = true;
        ok &= compare_and_trace(d.x, f.x, "x");
        ok &= compare_and_trace(d.y, f.y, "y");

        return ok;
    }

    return d.x == approx(f.x) && d.y == approx(f.y);
}

template<>
bool is_equal_approximetly<algo::double3, algo::double3>(algo::double3 f, algo::double3 d, bool print)
{
    if (print)
    {
        bool ok = true;
        ok &= compare_and_trace(d.x, f.x, "x");
        ok &= compare_and_trace(d.y, f.y, "y");
        ok &= compare_and_trace(d.z, f.z, "z");

        return ok;
    }

    return d.x == approx(f.x) && d.y == approx(f.y) && d.z == approx(f.z);
}

template<>
bool is_equal_approximetly<algo::algo_calibration_registers, algo::algo_calibration_registers>(algo::algo_calibration_registers f, algo::algo_calibration_registers d, bool print)
{
    bool ok = true;

    ok &= compare_and_trace(d.EXTLdsmXoffset, f.EXTLdsmXoffset, "Xoffset");
    ok &= compare_and_trace(d.EXTLdsmXscale, f.EXTLdsmXscale, "Xscale");
    ok &= compare_and_trace(d.EXTLdsmYoffset, f.EXTLdsmYoffset, "Yoffset");
    ok &= compare_and_trace(d.EXTLdsmYscale, f.EXTLdsmYscale, "Yscale");

    return ok;
}

template<>
bool is_equal_approximetly<algo::los_shift_scaling, algo::los_shift_scaling>(algo::los_shift_scaling f, algo::los_shift_scaling d, bool print)
{
    bool ok = true;

    ok &= compare_and_trace(d.los_scaling_x, f.los_scaling_x, "los_scaling_x");
    ok &= compare_and_trace(d.los_scaling_y, f.los_scaling_y, "los_scaling_y");
    ok &= compare_and_trace(d.los_shift_x, f.los_shift_x, "los_shift_x");
    ok &= compare_and_trace(d.los_shift_y, f.los_shift_y, "los_shift_y");

    return ok;
}
template< typename F, typename D >
void print( size_t x, F f, D d, bool is_approx = false )
{
    // bytes will be written to stdout as characters, which we never want... hence '+fx'
    AC_LOG( DEBUG, "... " << AC_D_PREC << x << ": {matlab}" << +f << (is_approx ? " !~ " : " != ") << +d << "{c++}"  );
}

template<>
void print<algo::k_matrix, algo::k_matrix>( size_t x, algo::k_matrix f, algo::k_matrix d, bool is_approx )
{
    // bytes will be written to stdout as characters, which we never want... hence '+fx'
    AC_LOG( DEBUG, "... " << AC_D_PREC << std::fixed << x << ": {matlab}" << f.get_fx() << " " << f.get_fy() << " " << f.get_ppx() << " " << f.get_ppy() << (is_approx ? " !~ " : " != ")
        << d.get_fx() << " " << d.get_fy() << " " << d.get_ppx() << " " << d.get_ppy() << "{c++}" );
}

template<>
void print<algo::double2, algo::double2>( size_t x, algo::double2 f, algo::double2 d, bool is_approx )
{
    // bytes will be written to stdout as characters, which we never want... hence '+fx'
    AC_LOG( DEBUG, "... " << AC_D_PREC << std::fixed << x << ": {matlab}" << f.x << " " << f.y << (is_approx ? " !~ " : " != ")
        << d.x << " " << d.y << "{c++}" );
}

template<>
void print<algo::double3, algo::double3>( size_t x, algo::double3 f, algo::double3 d, bool is_approx )
{
    // bytes will be written to stdout as characters, which we never want... hence '+fx'
    AC_LOG( DEBUG, "... " << std::setprecision( 15 ) << std::fixed << x << ": {matlab}" << f.x << " " << f.y << " " << f.z << (is_approx ? " !~ " : " != ")
        << d.x << " " << d.y << " " << d.z << "{c++}" );
}

template<>
void print<algo::rotation_in_angles, algo::rotation_in_angles>( size_t x, algo::rotation_in_angles f, algo::rotation_in_angles d, bool is_approx )
{
    // bytes will be written to stdout as characters, which we never want... hence '+fx'
    AC_LOG( DEBUG, "... " << std::setprecision( 15 ) << x << ": {matlab}" << f.alpha << " " << f.beta << " " << f.gamma << (is_approx ? " !~ " : " != ")
        << d.alpha << " " << d.beta << " " << d.gamma << "{c++}" );
}

template<>
void print<algo::p_matrix, algo::p_matrix>( size_t x, algo::p_matrix f, algo::p_matrix d, bool is_approx )
{
    std::ostringstream s;

    for( auto i = 0; i < 12; i++ )
    {
        if( !is_equal_approximetly( f.vals[i], d.vals[i] ) )
        {
            s << i << ": " << std::setprecision( 15 ) << std::fixed << ": {matlab}" << f.vals[i] << (is_approx ? " !~ " : " != ");
            s << std::setprecision( 15 ) << "{c++}" << d.vals[i] << "\n";
        }

    }

    AC_LOG( DEBUG, "... " << std::setprecision( 15 ) << std::fixed << x << " " << s.str() );
}

template<>
void print<std::vector<double>, std::vector<double>>(size_t x, std::vector<double> f, std::vector<double> d, bool is_approx)
{
    std::ostringstream s;

    for (auto i = 0; i < f.size(); i++)
    {
        if (!is_equal_approximetly(f[i], d[i]))
        {
            s << i << ": " << std::setprecision(15) << std::fixed << ": {matlab}" << f[i] << (is_approx ? " !~ " : " != ");
            s << std::setprecision(15) << "{c++}" << d[i] << "\n";
        }

    }

    AC_LOG(DEBUG, "... " << std::setprecision(15) << std::fixed << x << " " << s.str());
}

template<
    typename F, typename D,
    typename std::enable_if< !std::numeric_limits< D >::is_exact && !std::is_enum< D >::value, int >::type = 0
>
bool compare_t( F f, D d , bool print = true)
{
    return is_equal_approximetly( f, d, print);
}

template< typename F, typename D,
    typename std::enable_if< std::numeric_limits< D >::is_exact || std::is_enum< D >::value, int >::type = 0
>
bool compare_t( F f, D d, bool print = false)
{
    return f == d;
}


template< typename F, typename D >
bool compare_same_vectors( std::vector< F > const & matlab, std::vector< D > const & cpp )
{
    assert( matlab.size() == cpp.size() );
    size_t n_mismatches = 0;
    size_t size = matlab.size();
    for( size_t x = 0; x < size; ++x )
    {
        F fx = matlab[x];
        D dx = cpp[x];
        if( !compare_t( fx, dx, false ) )
        {
            if( ++n_mismatches <= 5 )
                print( x, fx, dx, std::is_floating_point< F >::value );
        }
    }
    if( n_mismatches )
        AC_LOG( DEBUG, "... " << n_mismatches << " mismatched values of " << size );
    return (n_mismatches == 0);
}

template< typename F, typename D >  // F=in bin; D=in memory
bool compare_to_bin_file(
    std::vector< D > const & vec,
    std::string const & scene_dir,
    std::string const & filename,
    size_t width, size_t height,
    size_t size,
    bool( *compare_vectors )(std::vector< F > const &, std::vector< D > const &) = nullptr
)
{
    TRACE( "Comparing " << filename << " ..." );
    bool ok = true;
    auto bin = read_vector_from< F >( join( bin_dir( scene_dir ), filename ), width, height );
    if( bin.size() != size)
        TRACE( filename << ": {matlab size}" << bin.size() << " != {width}" << width << "x" << height << "{height}" ), ok = false;
    if( vec.size() != bin.size() )
        TRACE( filename << ": {c++ size}" << vec.size() << " != " << bin.size() << "{matlab size}" ), ok = false;
    else
    {
        auto v = vec;
        auto b = bin;

        if( compare_vectors && !(*compare_vectors)(b, v) )
            ok = false;
        if( !ok )
        {
            //dump_vec( vec, bin, filename, width, height );
            //AC_LOG( DEBUG, "... dump of file written to: " << filename << ".dump" );
        }
    }
    return ok;
}

template< typename F, typename D >  // F=in bin; D=in memory
bool compare_to_bin_file(
    std::vector< D > const & vec,
    std::string const & scene_dir,
    std::string const & filename,
    size_t width, size_t height,
    bool(*compare_vectors)(std::vector< F > const &, std::vector< D > const &) = nullptr
)
{
    return compare_to_bin_file(vec, scene_dir, filename, width, height, width*height, compare_vectors);
}

template< typename F, typename D >  // F=in bin; D=in memory
bool compare_to_bin_file(
    std::vector< D > const & vec,
    std::string const & scene_dir,
    const char * prefix,
    size_t width, size_t height,
    const char * suffix,
    bool( *compare_vectors )(std::vector< F > const &, std::vector< D > const &) = nullptr
   
)
{
    return compare_to_bin_file< F, D >( vec,
        scene_dir, bin_file( prefix, width, height, suffix ) + ".bin",
        height, width,
        compare_vectors);
}


bool get_calib_and_cost_from_raw_data(
    algo::calib& calib,
    double& cost,
    std::string const & scene_dir,
    std::string const & filename
)
{
    auto data_size = sizeof( algo::matrix_3x3 ) +
        sizeof( algo::translation ) +
        sizeof( algo::matrix_3x3 ) +
        sizeof( double ); // cost

    auto bin = read_vector_from< double >( join( bin_dir( scene_dir ), filename ) );
    if( bin.size() * sizeof( double ) != data_size )
    {
        AC_LOG( DEBUG, "... " << filename << ": {matlab size}" << bin.size() * sizeof(double) << " != " << data_size );
        return false;
    }

    auto data = bin.data();

    algo::matrix_3x3 k = *(algo::matrix_3x3*)(data);
    data += sizeof( algo::matrix_3x3) / sizeof( double );
    auto r = *(algo::matrix_3x3*)(data);
    data += sizeof( algo::matrix_3x3 ) / sizeof( double );
    auto t = *(algo::translation*)(data);
    data += sizeof( algo::translation ) / sizeof( double );
    cost = *(double*)(data);

    calib.k_mat = algo::k_matrix( k );
    calib.rot = r;
    calib.trans = t;
    
    return true;
}


bool compare_calib( algo::calib const & calib,
                    double cost,
                    algo::calib calib_from_file,
                    double cost_matlab )
{
    auto intr_matlab = calib_from_file.get_intrinsics();
    auto extr_matlab = calib_from_file.get_extrinsics();
    
    auto intr_cpp = calib.get_intrinsics();
    auto extr_cpp = calib.get_extrinsics();
    
    bool ok = true;

    ok &= compare_and_trace( cost_matlab, cost, "cost" );

    ok &= compare_and_trace( intr_matlab.fx, intr_cpp.fx, "fx" );
    ok &= compare_and_trace( intr_matlab.fy, intr_cpp.fy, "fy" );
    ok &= compare_and_trace( intr_matlab.ppx, intr_cpp.ppx, "ppx" );
    ok &= compare_and_trace( intr_matlab.ppy, intr_cpp.ppy, "ppy" );

    for( auto i = 0; i < 9; i++ )
        ok &= compare_and_trace( extr_matlab.rotation[i], extr_cpp.rotation[i], "rotation[" + std::to_string( i ) + "]" );

    for( auto i = 0; i < 3; i++ )
        ok &= compare_and_trace( extr_matlab.translation[i], extr_cpp.translation[i], "translation[" + std::to_string( i ) + "]" );

    return ok;
}


bool operator==(const algo::algo_calibration_registers& first, const algo::algo_calibration_registers& second)
{
    bool ok = true;

    ok &= compare_and_trace(first.EXTLdsmXoffset, second.EXTLdsmXoffset, "dsm_x_offset");
    ok &= compare_and_trace(first.EXTLdsmYoffset, second.EXTLdsmYoffset, "dsm_y_offset");
    ok &= compare_and_trace(first.EXTLdsmXscale, second.EXTLdsmXscale, "dsm_x_scale");
    ok &= compare_and_trace(first.EXTLdsmYscale, second.EXTLdsmYscale, "dsm_y_scale");

    return ok;
}

bool operator==(const algo::los_shift_scaling& first, const algo::los_shift_scaling& second)
{
    bool ok = true;

    ok &= compare_and_trace(first.los_scaling_x, second.los_scaling_x, "los_scaling_x");
    ok &= compare_and_trace(first.los_scaling_y, second.los_scaling_y, "los_scaling_y");
    ok &= compare_and_trace(first.los_shift_x, second.los_shift_x, "los_shift_x");
    ok &= compare_and_trace(first.los_shift_y, second.los_shift_y, "los_shift_x");

    return ok;
}
template< typename D>  // F=in bin; D=in memory
bool compare_to_bin_file(
    D const & obj_cpp,
    std::string const & scene_dir,
    std::string const & filename
)
{
    TRACE("Comparing " << filename << " ...");
    bool ok = true;
    auto obj_matlab = read_from< D >( join( bin_dir( scene_dir ), filename ) );

    return compare_t(obj_matlab, obj_cpp);
}

bool read_thermal_data( std::string dir,
                              double hum_temp,
                              double & scale )
{
    try
    {
        auto vec = read_vector_from< byte >(
            join( bin_dir( dir ), "rgb_thermal_table" ) );

        std::vector<byte> thermal_vec( 16, 0 );  // table header
        thermal_vec.insert( thermal_vec.end(), vec.begin(), vec.end() );
        thermal::l500::thermal_calibration_table thermal_table( thermal_vec );
        scale = thermal_table.get_thermal_scale( hum_temp );
        return true;
    }
    catch( std::exception const & )
    {
        return false;
    }
}