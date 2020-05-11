// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "calibration-types.h"
#include <types.h>  // librealsense types (intr/extr)


namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {


    /** \brief Video stream intrinsics. */
    struct rs2_intrinsics_double
    {
        rs2_intrinsics_double() = default;

        rs2_intrinsics_double( const int width, const int height,
            const k_matrix& k_mat, const rs2_distortion model, const double coeffs[5] )
            :width( width ), height( height ),
            ppx( k_mat.ppx ), ppy( k_mat.ppy ),
            fx( k_mat.fx ), fy( k_mat.fy ),
            model( model ),
            coeffs{ coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4] }
        {}

        rs2_intrinsics_double( const rs2_intrinsics& obj )
            :width( obj.width ), height( obj.height ),
            ppx( obj.ppx ), ppy( obj.ppy ),
            fx( obj.fx ), fy( obj.fy ),
            model( obj.model ),
            coeffs{ obj.coeffs[0], obj.coeffs[1], obj.coeffs[2], obj.coeffs[3], obj.coeffs[4] }
        {}

        operator rs2_intrinsics()
        {
            return
            { width, height,
                float( ppx ), float( ppy ),
                float( fx ), float( fy ),
                model,
            {float( coeffs[0] ), float( coeffs[1] ), float( coeffs[2] ), float( coeffs[3] ), float( coeffs[4] )} };
        }

        int           width;     /**< Width of the image in pixels */
        int           height;    /**< Height of the image in pixels */
        double         ppx;       /**< Horizontal coordinate of the principal point of the image, as a pixel offset from the left edge */
        double         ppy;       /**< Vertical coordinate of the principal point of the image, as a pixel offset from the top edge */
        double         fx;        /**< Focal length of the image plane, as a multiple of pixel width */
        double         fy;        /**< Focal length of the image plane, as a multiple of pixel height */
        rs2_distortion model;    /**< Distortion model of the image */
        double         coeffs[5]; /**< Distortion coefficients */
    };

    /** \brief Cross-stream extrinsics: encodes the topology describing how the different devices are oriented. */
    struct rs2_extrinsics_double
    {
        rs2_extrinsics_double() {}
        rs2_extrinsics_double( const rotation& rot, const translation& trans )
            :rotation{ rot.rot[0], rot.rot[1],rot.rot[2],
                  rot.rot[3], rot.rot[4], rot.rot[5],
                  rot.rot[6], rot.rot[7], rot.rot[8] },
            translation{ trans.t1, trans.t2 , trans.t3 }
        {}

        rs2_extrinsics_double( const rs2_extrinsics& other )
            :rotation{ other.rotation[0], other.rotation[1], other.rotation[2],
                  other.rotation[3], other.rotation[4], other.rotation[5],
                  other.rotation[6], other.rotation[7], other.rotation[8] },
            translation{ other.translation[0], other.translation[1] , other.translation[2] }
        {}

        operator rs2_extrinsics()
        {
            return { {float( rotation[0] ), float( rotation[1] ), float( rotation[2] ),
            float( rotation[3] ), float( rotation[4] ), float( rotation[5] ),
            float( rotation[6] ), float( rotation[7] ), float( rotation[8] )} ,
            {float( translation[0] ), float( translation[1] ), float( translation[2] )} };
        }

        double rotation[9];    /**< Column-major 3x3 rotation matrix */
        double translation[3]; /**< Three-element translation vector, in meters */
    };

    struct calib
    {
        rotation_in_angles rot_angles = { 0 };
        rotation rot = { { 0 } };
        translation trans = { 0 };
        k_matrix k_mat = { 0 };
        p_matrix p_mat = { { 0 } };
        int           width = 0;
        int           height = 0;
        rs2_distortion model;
        double         coeffs[5];

        calib() = default;
        calib( calib const & ) = default;
        explicit calib( rs2_intrinsics_double const & rgb_intrinsics, rs2_extrinsics_double const & depth_to_rgb_extrinsics );
        explicit calib( rs2_intrinsics const & rgb_intrinsics, rs2_extrinsics const & depth_to_rgb_extrinsics );

        rs2_intrinsics_double get_intrinsics() const;
        rs2_extrinsics_double get_extrinsics() const;
        p_matrix const & get_p_matrix() const;

        p_matrix const & calc_p_mat();

        void copy_coefs( calib & obj ) const;
        calib operator*( double step_size ) const;
        calib operator/( double factor ) const;
        calib operator+( const calib& c ) const;
        calib operator-( const calib& c ) const;
        calib operator/( const calib& c ) const;
        double get_norma();
        double sum();
        calib normalize();
    };


    // Map 3D points to a UV-map given a calibration
    std::vector< double2 > get_texture_map(
        std::vector< double3 > const & points,
        calib const & curr_calib
    );


}
}
}

