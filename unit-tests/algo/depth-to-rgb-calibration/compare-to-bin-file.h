// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <type_traits>


template< typename F, typename D >
bool is_equal_approximetly( F fx, D dx )
{
    return fx == approx( dx );
}

template<>
bool is_equal_approximetly<algo::k_matrix, algo::k_matrix>( algo::k_matrix fx, algo::k_matrix dx )
{
    return fx.fx == approx( dx.fx ) &&
        fx.fy == approx( dx.fy ) &&
        fx.ppx == approx( dx.ppx ) &&
        fx.ppy == approx( dx.ppy );
}

template<>
bool is_equal_approximetly<algo::rotation_in_angles, algo::rotation_in_angles>( algo::rotation_in_angles fx, algo::rotation_in_angles dx )
{
    return fx.alpha == approx( dx.alpha ) &&
        fx.beta == approx( dx.beta ) &&
        fx.gamma == approx( dx.gamma );
}

template<>
bool is_equal_approximetly<algo::p_matrix, algo::p_matrix>( algo::p_matrix fx, algo::p_matrix dx )
{
    for( auto i = 0; i < 12; i++ )
    {
        if( fx.vals[i] != approx( dx.vals[i] ) )
            return false;
    }
    return true;
}

template<>
bool is_equal_approximetly<algo::double2, algo::double2>( algo::double2 f, algo::double2 d )
{
    return f.x == approx( d.x ) &&
        f.y == approx( d.y );
}

template<>
bool is_equal_approximetly<algo::double3, algo::double3>( algo::double3 f, algo::double3 d )
{
    return f.x == approx( d.x ) &&
        f.y == approx( d.y ) &&
        f.z == approx( d.z );
}

template< typename F, typename D >
void print( size_t x, F f, D d, bool is_approx = false )
{
    // bytes will be written to stdout as characters, which we never want... hence '+fx'
    AC_LOG( DEBUG, "... " << std::setprecision( 15 ) << std::fixed << x << ": {matlab}" << +f << (is_approx ? " !~ " : " != ") << +d << "{c++}" );
}

template<>
void print<algo::k_matrix, algo::k_matrix>( size_t x, algo::k_matrix f, algo::k_matrix d, bool is_approx )
{
    // bytes will be written to stdout as characters, which we never want... hence '+fx'
    AC_LOG( DEBUG, "... " << std::setprecision( 15 ) << std::fixed << x << ": {matlab}" << f.fx << " " << f.fy << " " << f.ppx << " " << f.ppy << (is_approx ? " !~ " : " != ")
        << d.fx << " " << d.fy << " " << d.ppx << " " << d.ppy << "{c++}" );
}

template<>
void print<algo::double2, algo::double2>( size_t x, algo::double2 f, algo::double2 d, bool is_approx )
{
    // bytes will be written to stdout as characters, which we never want... hence '+fx'
    AC_LOG( DEBUG, "... " << std::setprecision( 15 ) << std::fixed << x << ": {matlab}" << f.x << " " << f.y << (is_approx ? " !~ " : " != ")
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
        bool const is_comparable = std::numeric_limits< D >::is_exact || std::is_enum< D >::value;
        if( is_comparable )
        {
            if( fx != dx && ++n_mismatches <= 5 )
                // bytes will be written to stdout as characters, which we never want... hence '+fx'
                print( x, fx, dx );
        }
        else if( !is_equal_approximetly( fx, dx ) )
        {
            if( ++n_mismatches <= 5 )
                print( x, fx, dx, true );
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
    bool( *compare_vectors )(std::vector< F > const &, std::vector< D > const &) = nullptr,
    std::pair<std::vector< F >, std::vector< D >>( *preproccess_vectors )(std::vector< F > const &, std::vector< D > const &) = nullptr
)
{
    TRACE( "Comparing " << filename << " ..." );
    bool ok = true;
    auto bin = read_vector_from< F >( bin_dir( scene_dir ) + filename );
    if( bin.size() != width * height )
        TRACE( filename << ": {matlab size}" << bin.size() << " != {width}" << width << "x" << height << "{height}" ), ok = false;
    if( vec.size() != bin.size() )
        TRACE( filename << ": {c++ size}" << vec.size() << " != " << bin.size() << "{matlab size}" ), ok = false;
    else
    {
        auto v = vec;
        auto b = bin;
        if( preproccess_vectors )
        {
            auto vecs = preproccess_vectors( bin, vec );
            b = vecs.first;
            v = vecs.second;
        }
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
    const char * prefix,
    size_t width, size_t height,
    const char * suffix,
    bool( *compare_vectors )(std::vector< F > const &, std::vector< D > const &) = nullptr,
    std::pair<std::vector< F >, std::vector< D >>( *preproccess_vectors )(std::vector< F > const &, std::vector< D > const &) = nullptr
)
{
    return compare_to_bin_file< F, D >( vec,
        scene_dir, bin_file( prefix, width, height, suffix ) + ".bin",
        height, width,
        compare_vectors, preproccess_vectors );
}

bool get_calib_from_raw_data(
    algo::calib& calib,
    double& cost,
    std::string const & scene_dir,
    std::string const & filename
)
{
    auto data_size = sizeof( algo::rotation ) +
        sizeof( algo::translation ) +
        sizeof( algo::k_matrix ) +
        sizeof( double ); // cost

    auto bin = read_vector_from< double >( bin_dir( scene_dir ) + filename );
    if( bin.size() * sizeof( double ) != data_size )
    {
        TRACE( filename << ": {matlab size}" << bin.size() * sizeof(double) << " != " << data_size );
        return false;
    }

    auto data = bin.data();

    auto k = *(algo::k_matrix*)(data);
    data += sizeof( algo::k_matrix ) / sizeof( double );
    auto rotation = *(algo::rotation*)(data);
    data += sizeof( algo::rotation ) / sizeof( double );
    auto translation = *(algo::translation*)(data);
    data += sizeof( algo::translation ) / sizeof( double );
    cost = *(double*)(data);

    calib.k_mat = k;
    calib.rot = rotation;
    calib.trans = translation;
    
    return true;
}

template< typename D>
bool compare_and_trace( D val_matlab, D val_cpp, std::string const & compared )
{
    if( val_matlab != approx( val_cpp ) )
    {
        TRACE( compared << " " << val_matlab << " -matlab != " << val_cpp << " -cpp" );
        return false;
    }
    return true;
}

bool compare_calib_to_bin_file(
    algo::calib const & calib,
    double cost,
    std::string const & scene_dir,
    std::string const & filename,
    bool gradient = false
)
{
    TRACE( "Comparing " << filename << " ..." );
    algo::calib calib_from_file;
    double cost_matlab;
    auto res = get_calib_from_raw_data( calib_from_file, cost_matlab, scene_dir, filename );

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


bool compare_calib_to_bin_file(
    algo::calib const & calib,
    double cost,
    std::string const & scene_dir,
    char const * prefix,
    size_t w, size_t h,
    char const * suffix,
    bool gradient = false
)
{
    auto filename = bin_file( prefix, w, h, suffix ) + ".bin";
    return compare_calib_to_bin_file( calib, cost, scene_dir, filename, gradient );
}
