// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../src/algo/depth-to-rgb-calibration.h
//#cmake:add-file ../../../src/algo/depth-to-rgb-calibration.cpp
//#cmake:add-file ../algo-common.h
//#cmake:add-file ../camera-info.h
//#cmake:add-file F9440687.h

#include "../../../src/algo/depth-to-rgb-calibration.h"
#include "../algo-common.h"
#include "F9440687.h"

namespace algo = librealsense::algo::depth_to_rgb_calibration;

std::string test_dir( char const * data_dir, char const * test )
{
    std::string dir( root_data_dir );
    dir += data_dir;
    dir += '\\';
    dir += test;
    dir += '\\';
    return dir;
}

template< typename T >
std::vector< T > read_bin_file( char const * data_dir, char const * test, char const * bin )
{
    std::string filename = test_dir( data_dir, test );
    filename += "binFiles\\";
    filename += bin;
    filename += ".bin";
    std::fstream f = std::fstream( filename, std::ios::in | std::ios::binary );
    if( ! f )
        throw std::runtime_error( "failed to read file:\n" + filename );
    f.seekg( 0, f.end );
    size_t cb = f.tellg();
    f.seekg( 0, f.beg );
    if( cb % sizeof( T ) )
        throw std::runtime_error( "file size is not a multiple of data size" );
    std::vector< T > vec( cb / sizeof( T ));
    f.read( (char*) vec.data(), cb );
    f.close();
    return vec;
}

#include <type_traits>

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
                AC_LOG( DEBUG, "... " << x << ": {matlab}" << +fx << " != " << +dx << "{c++} (exact)" );
        }
        else if( fx != approx(dx) )
        {
            if( ++n_mismatches <= 5 )
                AC_LOG( DEBUG, "... " << x << ": {matlab}" << std::setprecision( 12 ) << fx << " != " << dx << "{c++}" );
        }
    }
    if( n_mismatches )
        AC_LOG( DEBUG, "... " << n_mismatches << " mismatched values of " << size );
    return (n_mismatches == 0);
}

template<class T>
std::vector< T > read_image_file( std::string const & file, size_t width, size_t height )
{
    std::ifstream f;
    f.open( file, std::ios::binary );
    if( !f.good() )
        throw std::runtime_error( "invalid file: " + file );
    std::vector< T > data( width * height );
    f.read( (char*) data.data(), width * height * sizeof( T ));
    return data;
}

template< typename T >
void dump_vec( std::vector< double > const & cpp, std::vector< T > const & matlab,
    char const * basename,
    size_t width, size_t height
)
{
    std::string filename = basename;
    filename += ".dump";
#if 0
    std::fstream f = std::fstream( filename, std::ios::out );
    if( !f )
        throw std::runtime_error( "failed to write file:\n" + filename );
    for( size_t x =  )
    std::vector< T > vec( cb / sizeof( T ) );
    f.read( (char*)vec.data(), cb );
    f.close();
    return vec;
#endif
}

template< typename F, typename D >  // F=in bin; D=in memory
bool compare_to_bin_file(
    std::vector< D > const & vec,
    char const * dir, char const * test, char const * filename,
    size_t width, size_t height,
    bool( *compare_vectors )(std::vector< F > const &, std::vector< D > const &) = nullptr
)
{
    TRACE( "Comparing " << filename << ".bin ..." );
    bool ok = true;
    auto bin = read_bin_file< F >( dir, test, filename );
    if( bin.size() != width * height )
        TRACE( filename << ": {matlab size}" << bin.size() << " != {width}" << width << "x" << height << "{height}" ), ok = false;
    if( vec.size() != bin.size() )
        TRACE( filename << ": {c++ size}" << vec.size() << " != " << bin.size() << "{matlab size}" ), ok = false;
    else
    {
        if( compare_vectors && !(*compare_vectors)(bin, vec) )
            ok = false;
        if( !ok )
        {
            //dump_vec( vec, bin, filename, width, height );
            //AC_LOG( DEBUG, "... dump of file written to: " << filename << ".dump" );
        }
    }
    return ok;
}

void init_algo( algo::optimizer & cal,
    char const * data_dir, char const * test,
    char const * yuy,
    char const * yuy_prev,
    char const * ir,
    char const * z,
    camera_info const & camera
)
{
    std::string dir = test_dir( data_dir, test );
    TRACE( "Loading " << dir << " ..." );

    cal.set_yuy_data(
        read_image_file< algo::yuy_t >( dir + yuy, camera.rgb.width, camera.rgb.height ),
        read_image_file< algo::yuy_t >( dir + yuy_prev, camera.rgb.width, camera.rgb.height ),
        camera.rgb.width, camera.rgb.height
    );

    cal.set_ir_data(
        read_image_file< algo::ir_t >( dir + ir, camera.z.width, camera.z.height ),
        camera.z.width, camera.z.height
    );

    cal.set_z_data(
        read_image_file< algo::z_t >( dir + z, camera.z.width, camera.z.height ),
        camera.z, camera.z_units
    );
}



TEST_CASE( "Weights calc", "[d2rgb]" )
{
    for (auto dir : data_dirs)
    {
        algo::optimizer cal;
        init_algo(cal, dir, "2",
            "YUY2_YUY2_1920x1080_00.00.26.6355_F9440687_0000.raw",
            "YUY2_YUY2_1920x1080_00.00.26.7683_F9440687_0001.raw",
            "I_GrayScale_1024x768_00.00.26.7119_F9440687_0000.raw",
            "Z_GrayScale_1024x768_00.00.26.7119_F9440687_0000.raw",
            F9440687);

        auto& z_data = cal.get_z_data();
        auto& ir_data = cal.get_ir_data();
        auto& yuy_data = cal.get_yuy_data();


        //---
        CHECK(compare_to_bin_file< float >(yuy_data.edges, dir, "2", "YUY2_edge_1080x1920_single_00", 1080, 1920, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(yuy_data.edges_IDT, dir, "2", "YUY2_IDT_1080x1920_double_00", 1080, 1920, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(yuy_data.edges_IDTx, dir, "2", "YUY2_IDTx_1080x1920_double_00", 1080, 1920, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(yuy_data.edges_IDTy, dir, "2", "YUY2_IDTy_1080x1920_double_00", 1080, 1920, compare_same_vectors));

        //---
        CHECK(compare_to_bin_file< double >(ir_data.ir_edges, dir, "2", "I_edge_768x1024_double_00", 768, 1024, compare_same_vectors));

        //---
        CHECK(compare_to_bin_file< float >(z_data.edges, dir, "2", "Z_edge_768x1024_single_00", 768, 1024, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(z_data.supressed_edges, dir, "2", "Z_edgeSupressed_768x1024_double_00", 768, 1024, compare_same_vectors));
        CHECK(compare_to_bin_file< byte >(z_data.directions, dir, "2", "Z_dir_768x1024_uint8_00", 768, 1024, compare_same_vectors));

        CHECK(compare_to_bin_file< double >(z_data.subpixels_x, dir, "2", "Z_edgeSubPixel_768x1024_double_01", 768, 1024, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(z_data.subpixels_y, dir, "2", "Z_edgeSubPixel_768x1024_double_00", 768, 1024, compare_same_vectors));

        CHECK(compare_to_bin_file< double >(z_data.weights, dir, "2", "weightsT_5089x1_double_00", 5089, 1, compare_same_vectors));



        cal.is_scene_valid();
        // edge distribution
        CHECK(compare_to_bin_file< double >(z_data.sum_weights_per_section, dir, "2", "depthEdgeWeightDistributionPerSectionDepth_4x1_double_00", 4, 1, compare_same_vectors));
        CHECK(compare_to_bin_file< byte >(z_data.section_map, dir, "2", "sectionMapDepth_trans_5089x1_uint8_00", 5089, 1, compare_same_vectors));
        CHECK(compare_to_bin_file< byte >(yuy_data.section_map, dir, "2", "sectionMapRgb_trans_2073600x1_uint8_00", 2073600, 1, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(yuy_data.sum_weights_per_section, dir, "2", "edgeWeightDistributionPerSectionRgb_4x1_double_00", 4, 1, compare_same_vectors));

        // gradient balanced
        CHECK(compare_to_bin_file< double >(z_data.sum_weights_per_direction, dir, "2", "edgeWeightsPerDir_4x1_double_00", 4, 1, compare_same_vectors));
    }
}
