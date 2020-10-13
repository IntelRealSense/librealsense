// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "calibration-types.h"
#include <string>

namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {

    void ndgrid_my(const double vec1[5], const double vec2[5], double yScalingGrid[25], double xScalingGrid[25]); //auto generated code
    void inv(const double x[9], double y[9]); //auto generated code
    void transpose(const double x[9], double y[9]);
    void rotate_180(const uint8_t* A, uint8_t* B, uint32_t w, uint32_t h); //auto generated code
    std::vector< double > interp1( const std::vector< double > & ind,
                                   const std::vector< double > & vals,
                                   const std::vector< double > & intrp );
    double get_norma(const std::vector<double3>& vec);
    double rad_to_deg(double rad); 
    double deg_to_rad(double deg);
    std::vector<double> direct_inv(std::vector<double> A, uint32_t s);
    void direct_inv_2x2(const double A[4], const double B[2], double C[2]);
    void direct_inv_6x6(const double A[36], const double B[6], double C[6]);
    void pinv_3x3( const double in[9], double out[9] );  // in pinv_3x3.cpp
    void svd_3x4( const double in[12], double out[3] );  // in svd_3x4.cpp

    double3x3 cholesky3x3( double3x3 const & mat );

    // Check that the DSM parameters given do not exceed certain boundaries, and
    // throw invalid_value_exception if they do.
    void validate_dsm_params( struct rs2_dsm_params const & dsm_params );

    void
    write_to_file( void const * data, size_t cb, std::string const & dir, char const * filename );

    template < typename T >
    void write_vector_to_file( std::vector< T > const & v,
                               std::string const & dir,
                               char const * filename )
    {
        write_to_file( v.data(), v.size() * sizeof( T ), dir, filename );
    }

}
}
}

