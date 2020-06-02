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
bool is_equal_approximetly( F fx, D dx )
{
    return dx == approx( fx );
}

template<>
bool is_equal_approximetly<algo::k_matrix, algo::k_matrix>( algo::k_matrix fx, algo::k_matrix dx )
{
    return dx.fx == approx( fx.fx ) &&
        dx.fy == approx( fx.fy ) &&
        dx.ppx == approx( fx.ppx ) &&
        dx.ppy == approx( fx.ppy );
}

template<>
bool is_equal_approximetly<algo::rotation_in_angles, algo::rotation_in_angles>( algo::rotation_in_angles fx, algo::rotation_in_angles dx )
{
    return dx.alpha == approx( fx.alpha ) &&
        dx.beta == approx( fx.beta ) &&
        dx.gamma == approx( fx.gamma );
}

template<>
bool is_equal_approximetly<algo::p_matrix, algo::p_matrix>( algo::p_matrix fx, algo::p_matrix dx )
{
    for( auto i = 0; i < 12; i++ )
    {
        if( dx.vals[i] != approx( fx.vals[i] ) )
            return false;
    }
    return true;
}

template<>
bool is_equal_approximetly<algo::double2, algo::double2>( algo::double2 f, algo::double2 d )
{
    return d.x == approx( f.x ) &&
        d.y == approx( f.y );
}

template<>
bool is_equal_approximetly<algo::double3, algo::double3>( algo::double3 f, algo::double3 d )
{
    return d.x == approx( f.x ) &&
        d.y == approx( f.y ) &&
        d.z == approx( f.z );
}

struct hexdump {

    void const * ptr;
    size_t cb;

    template< class T >
    hexdump( T const & t )
        : ptr( &t )
        , cb( sizeof( t ) ) {
    }

    hexdump( void * p, size_t cb )
        : ptr( p )
        , cb( cb ) {
    }
};


inline
std::ostream & operator<<( std::ostream & os, hexdump const & h ) {
    unsigned char const * buf = (unsigned char const *)h.ptr;
    //  os << h.cb << '@' << *(void**)&buf << ' ';
    //  os.flush();
    for( int j = int(h.cb) - 1; j >= 0; --j )
    {
        auto b = buf[j];
        auto v = (buf[j] & 0xF0) >> 4;
        os << (char)(v > 9
            ? 'a' + v - 10
            : '0' + v);
        v = buf[j] & 0x0F;
        os << (char)(v > 9
            ? 'a' + v - 10
            : '0' + v);
    }
    return os;
}

struct report_fixed
{
    int integer_width, precision;
    double n;
    report_fixed( double number, int integer_cch = 1, int fraction_cch = std::numeric_limits< double >::max_digits10 )
        : integer_width( integer_cch )
        , precision( fraction_cch )
        , n( number )
    {
    }
};


template< typename F, typename D >
void print( size_t x, F f, D d, bool is_approx = false )
{
    // bytes will be written to stdout as characters, which we never want... hence '+fx'
    AC_LOG( DEBUG,
            "... " << AC_D_PREC << x << ": {matlab} " << hexdump( f ) << ' ' << +f
                   << ( is_approx ? " !~ " : " != " ) << +d << ' ' << hexdump( d ) << " {c++}" );
}

inline
std::ostream &
operator<<( std::ostream & os,
    const report_fixed & f )
{
    // Report output
    if( !f.integer_width )
    {
        os << f.n;
    }
    else
    {
        // todo this could probably be optimized if needed
        std::ostringstream ss;
        ss << std::fixed;
        ss << std::setw( f.integer_width + 1 + f.precision );  // with leading spaces
        ss << std::setprecision( f.precision );                // will have trailing 0s
        ss << f.n;
        std::string s = ss.str();
        // Replace trailing 0s by blanks... following cast should be safe:
        char * lpsz = const_cast<char *>(s.c_str() + s.length());
        while( *--lpsz == '0' )
            *lpsz = ' ';
        os << s;
    }
    return os;
}


template<>
void print< double, double >( size_t x, double f, double d, bool is_approx )
{
    // bytes will be written to stdout as characters, which we never want... hence '+fx'
    AC_LOG( DEBUG,
        "... " << AC_D_PREC << x << ": {matlab} " << hexdump( f ) << ' ' << report_fixed( f, 5 )
        << (is_approx ? "     !~ " : "     != ") << report_fixed( d, 5 ) << "     " << hexdump( d ) << " {c++}" );
}

template<>
void print<algo::k_matrix, algo::k_matrix>( size_t x, algo::k_matrix f, algo::k_matrix d, bool is_approx )
{
    // bytes will be written to stdout as characters, which we never want... hence '+fx'
    AC_LOG( DEBUG, "... " << AC_D_PREC << std::fixed << x << ": {matlab}" << f.fx << " " << f.fy << " " << f.ppx << " " << f.ppy << (is_approx ? " !~ " : " != ")
        << d.fx << " " << d.fy << " " << d.ppx << " " << d.ppy << "{c++}" );
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

template<
    typename F, typename D,
    typename std::enable_if< !std::numeric_limits< D >::is_exact && !std::is_enum< D >::value, int >::type = 0
>
bool compare_t( F f, D d )
{
    return is_equal_approximetly( f, d );
}

template< typename F, typename D,
    typename std::enable_if< std::numeric_limits< D >::is_exact || std::is_enum< D >::value, int >::type = 0
>
bool compare_t( F f, D d )
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
        if( !compare_t( fx, dx ) )
        {
            if( ++n_mismatches <= 5 )
                print( x, fx, dx, std::is_floating_point< F >::value );
        }
    }
    if( n_mismatches )
        AC_LOG( DEBUG, "... " << n_mismatches << " mismatched values of " << size );
    return (n_mismatches == 0);
}


template< typename F, typename D >
bool compare_exact_vectors( std::vector< F > const & matlab, std::vector< D > const & cpp )
{
    assert( matlab.size() == cpp.size() );
    size_t n_mismatches = 0;
    size_t size = matlab.size();
    for( size_t x = 0; x < size; ++x )
    {
        F fx = matlab[x];
        D dx = cpp[x];
        if( !(fx == dx ) )
        {
            if( ++n_mismatches <= 5 )
                print( x, fx, dx, false );
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
    bool( *compare_vectors )(std::vector< F > const &, std::vector< D > const &) = nullptr
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
    bool( *compare_vectors )(std::vector< F > const &, std::vector< D > const &) = nullptr
   
)
{
    return compare_to_bin_file< F, D >( vec,
        scene_dir, bin_file( prefix, width, height, suffix ) + ".bin",
        height, width,
        compare_vectors);
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
        AC_LOG( DEBUG, "... " << filename << ": {matlab size}" << bin.size() * sizeof(double) << " != " << data_size );
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
    if( val_cpp != approx( val_matlab ) )
    {
        AC_LOG( DEBUG, "... " << compared << ":  {matlab} " << val_matlab << " !~ " << val_cpp << " {c++}" );
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
    auto obj_matlab = read_from< D >(bin_dir(scene_dir) + filename);

    return obj_matlab == obj_cpp;
}