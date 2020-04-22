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

template<>
bool compare_same_vectors<algo::double2, algo::double2>(std::vector< algo::double2> const & matlab, std::vector< algo::double2> const & cpp)
{
    assert(matlab.size() == cpp.size());
    size_t n_mismatches = 0;
    size_t size = matlab.size();
    auto cpp_vac = cpp;
    auto matlab_vac = matlab;
    std::sort(matlab_vac.begin(), matlab_vac.end(), [](algo::double2 d1, algo::double2 d2) {return d1.x < d2.x; });
    std::sort(cpp_vac.begin(), cpp_vac.end(), [](algo::double2 d1, algo::double2 d2) {return d1.x < d2.x; });
    for (size_t x = 0; x < size; ++x)
    {
        auto fx = matlab_vac[x];
        auto dx = cpp_vac[x];
        bool const is_comparable = std::numeric_limits<double>::is_exact || std::is_enum< double >::value;
        if (is_comparable)
        {
            if (fx != dx && ++n_mismatches <= 5)
                // bytes will be written to stdout as characters, which we never want... hence '+fx'
                AC_LOG(DEBUG, "... " << x << ": {matlab}" << fx.x << " " << fx.y << " != " << dx.x << " " << dx.y << "{c++} (exact)");
        }
        else if (fx.x != approx(dx.x) || fx.y != approx(dx.y))
        {
            if (++n_mismatches <= 5)
                AC_LOG(DEBUG, "... " << x << ": {matlab}" << std::setprecision(12) << fx.x << " " << fx.y << " != " << dx.x << " " << dx.y << "{c++}");
        }
    }
    if (n_mismatches)
        AC_LOG(DEBUG, "... " << n_mismatches << " mismatched values of " << size);
    return (n_mismatches == 0);
}

template<>
bool compare_same_vectors<algo::double3, algo::double3>(std::vector< algo::double3> const & matlab, std::vector< algo::double3> const & cpp)
{
    assert(matlab.size() == cpp.size());
    size_t n_mismatches = 0;
    size_t size = matlab.size();
    auto cpp_vac = cpp;
    auto matlab_vac = matlab;
    std::sort(matlab_vac.begin(), matlab_vac.end(), [](algo::double3 d1, algo::double3 d2) {return d1.x < d2.x; });
    std::sort(cpp_vac.begin(), cpp_vac.end(), [](algo::double3 d1, algo::double3 d2) {return d1.x < d2.x; });
    for (size_t x = 0; x < size; ++x)
    {
        auto fx = matlab_vac[x];
        auto dx = cpp_vac[x];
        bool const is_comparable = std::numeric_limits<double>::is_exact || std::is_enum< double >::value;
        if (is_comparable)
        {
            if (fx != dx && ++n_mismatches <= 5)
                // bytes will be written to stdout as characters, which we never want... hence '+fx'
                AC_LOG(DEBUG, "... " << x << ": {matlab}" << fx.x << " " << fx.y << " " << fx.z << " != " << dx.x << " " << dx.y << " " << dx.z << "{c++} (exact)");
        }
        else if (fx.x != approx(dx.x) || fx.y != approx(dx.y))
        {
            if (++n_mismatches <= 5)
                AC_LOG(DEBUG, "... " << x << ": {matlab}" << std::setprecision(12) << fx.x << " " << fx.y << " " << fx.z << " != " << dx.x << " " << dx.y << " " << dx.z << "{c++}");
        }
    }
    if (n_mismatches)
        AC_LOG(DEBUG, "... " << n_mismatches << " mismatched values of " << size);
    return (n_mismatches == 0);
}

template< typename F, typename D >
std::pair<std::vector< F >, std::vector< D > > sort_vectors(std::vector< F > const & matlab, std::vector< D > const & cpp)
{
    auto f = matlab;
    auto d = cpp;

    std::sort(f.begin(), f.end(), [](F f1, F f2) {if (isnan(f1)) return false;  return f1 < f2; });
    std::sort(d.begin(), d.end(), [](D d1, D d2) {return d1 < d2; });

    return { f,d };
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
    bool(*compare_vectors)(std::vector< F > const &, std::vector< D > const &) = nullptr,
    std::pair<std::vector< F >, std::vector< D >>(*preproccess_vectors)(std::vector< F > const &, std::vector< D > const &) = nullptr
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
        auto v = vec;
        auto b = bin;
        if (preproccess_vectors)
        {
            auto vecs = preproccess_vectors(bin, vec);
            b = vecs.first;
            v = vecs.second;
        }
        if (compare_vectors && !(*compare_vectors)(b, v))
            ok = false;
        if( !ok )
        {
            //dump_vec( vec, bin, filename, width, height );
            //AC_LOG( DEBUG, "... dump of file written to: " << filename << ".dump" );
        }
    }
    return ok;
}


bool get_calib_from_raw_data(algo::calib& calib, double& cost, char const * dir, char const * test, char const * filename)
{
    auto data_size = sizeof(algo::rotation) +
        sizeof(algo::translation) +
        sizeof(algo::k_matrix) +
        sizeof(algo::p_matrix) +
        3 * sizeof(double) + // alpha, bata, gamma
        sizeof(double); // cost

    auto bin = read_bin_file <double>(dir, test, filename);
    if (bin.size() * sizeof(double) != data_size)
    {
        TRACE(filename << ": {matlab size}" << bin.size() << " != " << data_size);
        return false;
    }

    auto data = bin.data();

    auto k = *(algo::k_matrix*)(data);
    data += sizeof(algo::k_matrix) / sizeof(double);
    auto a = *(double*)(data);
    data += 1;
    auto b = *(double*)(data);
    data += 1;
    auto g = *(double*)(data);
    data += 1;
    auto rotation = *(algo::rotation*)(data);
    data += sizeof(algo::rotation) / sizeof(double);
    auto translation = *(algo::translation*)(data);
    data += sizeof(algo::translation) / sizeof(double);
    auto p_mat = *(algo::p_matrix*)(data);
    data += sizeof(algo::p_matrix) / sizeof(double);
    cost = *(double*)(data);

    calib.k_mat = k;
    calib.rot = rotation;
    calib.trans = translation;
    calib.p_mat = p_mat;
    calib.rot_angles = { a,b,g };

    return true;
}

template< typename D>
bool compare_and_trace(D val_matlab, D val_cpp, std::string compared)
{
    if (val_matlab != approx(val_cpp))
    {
        TRACE(compared << " " << val_matlab << " -matlab != " << val_cpp << " -cpp");
        return false;
    }
    return true;
}

bool compare_calib_to_bin_file(algo::calib calib, double cost, char const * dir, char const * test, char const * filename, bool gradient = false)
{
    TRACE("Comparing " << filename << ".bin ...");
    algo::calib calib_from_file;
    double cost_matlab;
    auto res = get_calib_from_raw_data(calib_from_file, cost_matlab, dir, test, filename);

    auto intr_matlab = calib_from_file.get_intrinsics();
    auto extr_matlab = calib_from_file.get_extrinsics();
    auto pmat_matlab = calib_from_file.get_p_matrix();

    auto intr_cpp = calib.get_intrinsics();
    auto extr_cpp = calib.get_extrinsics();
    auto pmat_cpp = calib.get_p_matrix();

    bool ok = true;

    ok &= compare_and_trace(cost_matlab, cost, "cost");

    ok &= compare_and_trace(intr_matlab.fx, intr_cpp.fx, "fx");
    ok &= compare_and_trace(intr_matlab.fy, intr_cpp.fy, "fy");
    ok &= compare_and_trace(intr_matlab.ppx, intr_cpp.ppx, "ppx");
    ok &= compare_and_trace(intr_matlab.ppy, intr_cpp.ppy, "ppy");

    if (gradient)
    {
        ok &= compare_and_trace(calib.rot_angles.alpha, calib_from_file.rot_angles.alpha, "alpha");
        ok &= compare_and_trace(calib.rot_angles.beta, calib_from_file.rot_angles.beta, "beta");
        ok &= compare_and_trace(calib.rot_angles.gamma, calib_from_file.rot_angles.gamma, "gamma");
    }
    else
    {
        for (auto i = 0; i < 9; i++)
        {
            std::stringstream str;
            str << i;
            ok &= compare_and_trace(extr_matlab.rotation[i], extr_cpp.rotation[i], "rotation[" + str.str() + "]");
        }
    }

    for (auto i = 0; i < 3; i++)
    {
        std::stringstream str;
        str << i;
        ok &= compare_and_trace(extr_matlab.translation[i], extr_cpp.translation[i], "translation[" + str.str() + "]");
    }

    for (auto i = 0; i < 12; i++)
    {
        std::stringstream str;
        str << i;
        ok &= compare_and_trace(pmat_matlab.vals[i], pmat_cpp.vals[i], "pmat[" + str.str() + "]");
    }


    return ok;
}

void init_algo(algo::optimizer & cal,
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


std::string generet_file_name(std::string prefix, int num, std::string saffix)
{
    std::stringstream s;
    s << num;
    return prefix + std::string(s.str()) + saffix;
}

TEST_CASE("Weights calc", "[d2rgb]")
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
        CHECK(compare_to_bin_file< double >(yuy_data.edges, dir, "2", "YUY2_edge_1080x1920_double_00", 1080, 1920, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(yuy_data.edges_IDT, dir, "2", "YUY2_IDT_1080x1920_double_00", 1080, 1920, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(yuy_data.edges_IDTx, dir, "2", "YUY2_IDTx_1080x1920_double_00", 1080, 1920, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(yuy_data.edges_IDTy, dir, "2", "YUY2_IDTy_1080x1920_double_00", 1080, 1920, compare_same_vectors));

        //---
        CHECK(compare_to_bin_file< double >(ir_data.ir_edges, dir, "2", "I_edge_768x1024_double_00", 768, 1024, compare_same_vectors));

        //---
        CHECK(compare_to_bin_file< double >(z_data.edges, dir, "2", "Z_edge_768x1024_double_00", 768, 1024, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(z_data.supressed_edges, dir, "2", "Z_edgeSupressed_768x1024_double_00", 768, 1024, compare_same_vectors));
        CHECK(compare_to_bin_file< byte >(z_data.directions, dir, "2", "Z_dir_768x1024_uint8_00", 768, 1024, compare_same_vectors));

        CHECK(compare_to_bin_file< double >(z_data.subpixels_x, dir, "2", "Z_edgeSubPixel_768x1024_double_01", 768, 1024, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(z_data.subpixels_y, dir, "2", "Z_edgeSubPixel_768x1024_double_00", 768, 1024, compare_same_vectors));

        CHECK(compare_to_bin_file< double >(z_data.weights, dir, "2", "weightsT_5089x1_double_00", 5089, 1, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(z_data.closest, dir, "2", "Z_valuesForSubEdges_768x1024_double_00", 768, 1024, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(z_data.weights, dir, "2", "weightsT_5089x1_double_00", 5089, 1, compare_same_vectors));
        CHECK(compare_to_bin_file< algo::double3 >(z_data.vertices, dir, "2", "vertices_5089x3_double_00", 5089, 1, compare_same_vectors));

        auto cb = [&](algo::iteration_data_collect data)
        {
            auto file = generet_file_name("calib_iteration_", data.iteration + 1, "_1x32_double_00");
            CHECK(compare_calib_to_bin_file(data.params.curr_calib, data.params.cost, dir, "2", file.c_str()));

            file = generet_file_name("uvmap_iteration_", data.iteration + 1, "_5089x2_double_00");
            CHECK(compare_to_bin_file< algo::double2 >(data.uvmap, dir, "2", file.c_str(), 5089, 1, compare_same_vectors));

            file = generet_file_name("DVals_iteration_", data.iteration + 1, "_5089x1_double_00");
            CHECK(compare_to_bin_file< double >(data.d_vals, dir, "2", file.c_str(), 5089, 1, compare_same_vectors, sort_vectors));

            file = generet_file_name("DxVals_iteration_", data.iteration + 1, "_5089x1_double_00");
            CHECK(compare_to_bin_file< double >(data.d_vals_x, dir, "2", file.c_str(), 5089, 1, compare_same_vectors, sort_vectors));

            file = generet_file_name("DyVals_iteration_", data.iteration + 1, "_5089x1_double_00");
            CHECK(compare_to_bin_file< double >(data.d_vals_y, dir, "2", file.c_str(), 5089, 1, compare_same_vectors, sort_vectors));

            file = generet_file_name("grad_iteration_", data.iteration + 1, "_1x32_double_00");
            CHECK(compare_calib_to_bin_file(data.params.calib_gradients, 0, dir, "2", file.c_str(), true));
        };

        REQUIRE(cal.optimize(algo::calib(F9440687.rgb, F9440687.extrinsics), cb) == 5);  // n_iterations

        auto new_calib = cal.get_calibration();
        auto cost = cal.get_cost();

        CHECK(compare_calib_to_bin_file(new_calib, cost, dir, "2", "new_calib_1x32_double_00"));
        // ---
        CHECK( ! cal.is_scene_valid() );

        // edge distribution
        CHECK(compare_to_bin_file< double >(z_data.sum_weights_per_section, dir, "2", "depthEdgeWeightDistributionPerSectionDepth_4x1_double_00", 4, 1, compare_same_vectors));
        CHECK(compare_to_bin_file< byte >(z_data.section_map, dir, "2", "sectionMapDepth_trans_5089x1_uint8_00", 5089, 1, compare_same_vectors));
        CHECK(compare_to_bin_file< byte >(yuy_data.section_map, dir, "2", "sectionMapRgb_trans_2073600x1_uint8_00", 2073600, 1, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(yuy_data.sum_weights_per_section, dir, "2", "edgeWeightDistributionPerSectionRgb_4x1_double_00", 4, 1, compare_same_vectors));

        // gradient balanced
        CHECK(compare_to_bin_file< double >(z_data.sum_weights_per_direction, dir, "2", "edgeWeightsPerDir_4x1_double_00", 4, 1, compare_same_vectors));

        // movment check
        // 1. dilation
        CHECK(compare_to_bin_file< uint8_t >(yuy_data.prev_logic_edges, dir, "2", "logicEdges_1080x1920_uint8_00", 1080, 1920, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(yuy_data.dilated_image, dir, "2", "dilatedIm_1080x1920_double_00", 1080, 1920, compare_same_vectors));

        // 2. gausssian
        CHECK(compare_to_bin_file< double >(yuy_data.yuy_diff, dir, "2", "diffIm_01_1080x1920_double_00", 1080, 1920, compare_same_vectors));
        CHECK(compare_to_bin_file< double >(yuy_data.gaussian_filtered_image, dir, "2", "diffIm_1080x1920_double_00", 1080, 1920, compare_same_vectors));
        //--


        //--
        CHECK( ! cal.is_valid_results() );
        CHECK( cal.calc_correction_in_pixels() == approx( 2.9144 ) );
        CHECK( compare_to_bin_file< double >( z_data.cost_diff_per_section, dir, "2", "costDiffPerSection_1x4_double_00", 1, 4, compare_same_vectors ) );
    }
}
