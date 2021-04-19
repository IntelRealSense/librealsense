// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

// Utilities to read from/write to algo scene directories

#include <fstream>
#include <string>
#include "../../../src/algo/depth-to-rgb-calibration/k-to-dsm.h"
#include "../../filesystem.h"

inline std::string bin_dir( std::string const & scene_dir )
{
    return join( join( scene_dir, "binFiles" ), "ac2" );
}


std::string bin_file( std::string const &prefix, size_t cycle, size_t iteration, size_t w, size_t h, std::string const &suffix )
{
    return prefix + '_' + std::to_string( cycle ) + '_' + std::to_string( iteration ) + '_'
         + std::to_string( h ) + "x" + std::to_string( w ) + "_" + suffix;
}

std::string bin_file(std::string const &prefix, size_t cycle, size_t w, size_t h, std::string const &suffix)
{
    return prefix + '_' + std::to_string(cycle) + '_'
        + std::to_string(h) + "x" + std::to_string(w) + "_" + suffix;
}
std::string bin_file( std::string const &prefix, size_t w, size_t h, std::string const &suffix )
{
    return prefix + "_" + std::to_string( h ) + "x" + std::to_string( w ) + "_" + suffix;
}


template< typename T >
void read_data_from( std::string const & filename, T * data )
{
    std::fstream f = std::fstream( filename, std::ios::in | std::ios::binary );
    if( !f )
        throw std::runtime_error( "failed to read file:\n" + filename );
    f.seekg( 0, f.end );
    size_t cb = f.tellg();
    f.seekg( 0, f.beg );
    if( cb != sizeof( T ) )
        throw std::runtime_error( librealsense::to_string()
            << "file size (" << cb << ") does not match data size (" << sizeof( T ) << "): " << filename );
    std::vector< T > vec( cb / sizeof( T ) );
    f.read( (char *)data, cb );
    f.close();
}

template< typename T >
 T read_from(std::string const & filename)
{
    std::fstream f = std::fstream(filename, std::ios::in | std::ios::binary);
    if (!f)
        throw std::runtime_error("failed to read file:\n" + filename);
    f.seekg(0, f.end);
    size_t cb = f.tellg();
    f.seekg(0, f.beg);
    if (cb % sizeof(T))
        throw std::runtime_error("file size is not a multiple of data size");
    T obj;
    f.read((char *)&obj, cb);
    f.close();
    return obj;
}

template< typename T >
std::vector< T > read_vector_from( std::string const & filename, size_t size_x = 0, size_t size_y = 0)
{
    std::fstream f = std::fstream( filename, std::ios::in | std::ios::binary );
    if( !f )
        throw std::runtime_error( "failed to read file:\n" + filename );
    f.seekg( 0, f.end );
    size_t cb = f.tellg();
    f.seekg( 0, f.beg );
    if( cb % sizeof( T ) )
        throw std::runtime_error( "file size is not a multiple of data size" );
    std::vector< T > vec( cb / sizeof( T ) );
    f.read( (char *)vec.data(), cb );
    f.close();
    return vec;
}

template<>
std::vector< std::vector<double> > read_vector_from(std::string const & filename, size_t size_x, size_t size_y)
{
    std::vector< std::vector<double> > res;
    
    std::fstream f = std::fstream(filename, std::ios::in | std::ios::binary);
    if (!f)
        throw std::runtime_error("failed to read file:\n" + filename);
    f.seekg(0, f.end);
    size_t cb = f.tellg();
    f.seekg(0, f.beg);
    if (cb % sizeof(double))
        throw std::runtime_error("file size is not a multiple of data size");

    res.resize(size_x);

    for (size_t i = 0; i < size_x; i++)
    {
        res[i].resize(size_y);
        f.read((char *)res[i].data(), size_y * sizeof(double));
    }
    f.close();
    //for(auto i=0;i<)
    return res;

    /*std::fstream f = std::fstream(filename, std::ios::in | std::ios::binary);
    if (!f)
        throw std::runtime_error("failed to read file:\n" + filename);
    f.seekg(0, f.end);
    size_t cb = f.tellg();
    f.seekg(0, f.beg);
    if (cb % sizeof(T))
        throw std::runtime_error("file size is not a multiple of data size");
    std::vector< T > vec(cb / sizeof(T));
    f.read((char *)vec.data(), cb);
    f.close();
    return vec;*/
}
template < class T >
std::vector< T > read_image_file( std::string const &file, size_t width, size_t height )
{
    std::ifstream f;
    f.open( file, std::ios::in | std::ios::binary );
    if( !f.good() )
        throw std::runtime_error( "invalid file: " + file );
    f.seekg( 0, f.end );
    size_t cb = f.tellg();
    f.seekg( 0, f.beg );
    if( cb != sizeof( T ) * width * height )
        throw std::runtime_error( librealsense::to_string()
            << "file size (" << cb << ") does not match expected size (" << sizeof( T ) * width * height << "): " << file );
    std::vector< T > data( width * height );
    f.read( (char *)data.data(), width * height * sizeof( T ) );
    return data;
}

template < typename T >
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
    for( size_t x = )
        std::vector< T > vec( cb / sizeof( T ) );
    f.read( (char*)vec.data(), cb );
    f.close();
    return vec;
#endif
}

// This metadata describes what Matlab did: the files it used (sometimes there are more) and the
// iteration data/results that we need for comparison
struct scene_metadata
{
    //uint64_t n_iterations;  // how many steps through optimization, and how many iteration file sets
    uint64_t n_cycles;            // how many cycles of optimization
    double correction_in_pixels;  // XY movement
    uint64_t n_edges;             // strong edges, i.e. after suppression
    uint64_t n_valid_pixels;
    uint64_t n_relevant_pixels;
    uint64_t n_valid_ir_edges;
    bool is_scene_valid;
    bool is_output_valid;
    std::string rgb_file;
    std::string rgb_prev_file;  // TODO: looks like these need to be turned around!!!
    std::string rgb_prev_valid_file;
    std::string ir_file;
    std::string z_file;

    scene_metadata( std::string const &scene_dir )
    {
        std::ifstream( join( bin_dir( scene_dir ), "yuy_prev_z_i.files" ) ) >> rgb_file
            >> rgb_prev_file >> z_file >> ir_file >> rgb_prev_valid_file;
        if( rgb_file.empty() )
            throw std::runtime_error( "failed to read file:\n" + bin_dir( scene_dir ) + "yuy_prev_z_i.files" );
        if( ir_file.empty() )
            throw std::runtime_error( "not enough files in:\n" + bin_dir( scene_dir ) + "yuy_prev_z_i.files" );

        std::string metadata = join( bin_dir( scene_dir ), "metadata" );
        std::fstream f = std::fstream( metadata, std::ios::in | std::ios::binary );
        if( !f )
            throw std::runtime_error( "failed to read file:\n" + metadata );
        f.read( (char *)&correction_in_pixels, sizeof( correction_in_pixels ) );
        f.read( (char *)&n_edges, sizeof( n_edges ) );
        f.read( (char *)&n_valid_ir_edges, sizeof( n_valid_ir_edges ) );
        f.read( (char *)&n_valid_pixels, sizeof( n_valid_pixels ) );
        f.read((char *)&n_relevant_pixels, sizeof(n_relevant_pixels));
        f.read( (char *)&n_cycles, sizeof( n_cycles ) );
        char b;
        f.read( &b, 1 );
        is_scene_valid = b != 0;
        f.read( &b, 1 );
        is_output_valid = b != 0;
        f.close();
    }
};


// Encapsulate the calibration information for a specific camera
// All the sample images we use are usually from the same camera. I.e., their
// intrinsics & extrinsics are the same and can be reused via this structure
struct camera_params
{
    librealsense::algo::depth_to_rgb_calibration::rs2_intrinsics_double rgb;
    librealsense::algo::depth_to_rgb_calibration::rs2_intrinsics_double z;
    rs2_dsm_params dsm_params;
    librealsense::algo::depth_to_rgb_calibration::algo_calibration_info cal_info;
    librealsense::algo::depth_to_rgb_calibration::algo_calibration_registers cal_regs;
    librealsense::algo::depth_to_rgb_calibration::rs2_extrinsics_double extrinsics;
    float z_units = 0.25;
};

camera_params read_camera_params( std::string const &scene_dir, std::string const &filename )
{
    struct params_bin
    {
        // Some units are supposed to be int but we made matlab write out doubles....
        double depth_width;
        double depth_height;
        double depth_units;
        librealsense::algo::depth_to_rgb_calibration::matrix_3x3 k_depth;
        double rgb_width;
        double rgb_height;
        librealsense::algo::depth_to_rgb_calibration::matrix_3x3 k_rgb;
        double coeffs[5];
        double matrix_3x3[9];
        double translation[3];
        double p_mat[12];
    };

    params_bin param;
    read_data_from( join( bin_dir( scene_dir ), filename ), &param );

    double coeffs[5] = { 0 };
    camera_params ci;
    ci.rgb =
    {
        int( param.rgb_width ), int( param.rgb_height ),
        param.k_rgb,
        RS2_DISTORTION_BROWN_CONRADY,
        param.coeffs
    };
    ci.z =
    {
        int( param.depth_width ), int( param.depth_height ),
        param.k_depth,
        RS2_DISTORTION_NONE, coeffs
    };
    ci.extrinsics =
    {
        { param.matrix_3x3[0], param.matrix_3x3[1], param.matrix_3x3[2],
            param.matrix_3x3[3], param.matrix_3x3[4], param.matrix_3x3[5],
            param.matrix_3x3[6], param.matrix_3x3[7], param.matrix_3x3[8] },
        { param.translation[0], param.translation[1], param.translation[2] }
    };
    return ci;
}

struct dsm_params
{
    rs2_dsm_params params;
    librealsense::algo::depth_to_rgb_calibration::algo_calibration_registers algo_calibration_registers;
    librealsense::algo::depth_to_rgb_calibration::algo_calibration_info regs;
};

dsm_params read_dsm_params(std::string const &scene_dir, std::string const &filename)
{
    dsm_params res;

#pragma pack(push, 1)
    struct algo_calibration
    {
        uint8_t fovexExistenceFlag;
        float fovexNominal[4];
        float laserangleH;
        float laserangleV;
        float xfov[5];
        float yfov[5];
        float polyVars[3];
        float undistAngHorz[4];
        float pitchFixFactor;

    };
#pragma pack(pop)

    librealsense::algo::depth_to_rgb_calibration::algo_calibration_registers algo_calibration_registers;
    algo_calibration algo_calib;

    std::string dsmparams = join( bin_dir( scene_dir ), filename );
    std::fstream f = std::fstream(dsmparams, std::ios::in | std::ios::binary );
    if( !f )
        throw std::runtime_error( "failed to read file:\n" + dsmparams);
    f.read( (char *)&res.params, sizeof(rs2_dsm_params) );
    f.read( (char *)&algo_calibration_registers,
            sizeof( librealsense::algo::depth_to_rgb_calibration::algo_calibration_registers ) );
    f.read( (char *)&algo_calib, sizeof( algo_calibration ) );

    f.close();

    res.algo_calibration_registers = algo_calibration_registers;


    res.regs.FRMWfovexExistenceFlag = algo_calib.fovexExistenceFlag;

    for (auto i = 0; i < 4; i++)
    {
        res.regs.FRMWfovexNominal[i] = algo_calib.fovexNominal[i];
        res.regs.FRMWundistAngHorz[i] = algo_calib.undistAngHorz[i];
    }
    
    res.regs.FRMWlaserangleH = algo_calib.laserangleH;
    res.regs.FRMWlaserangleV = algo_calib.laserangleV;

    for (auto i = 0; i < 5; i++)
    {
        res.regs.FRMWxfov[i] = algo_calib.xfov[i];
        res.regs.FRMWyfov[i] = algo_calib.yfov[i];
    }
   
    for (auto i = 0; i < 3; i++)
    {
        res.regs.FRMWpolyVars[i] = algo_calib.polyVars[i];
    }
   
    res.regs.FRMWpitchFixFactor = algo_calib.pitchFixFactor;

    return res;
}
