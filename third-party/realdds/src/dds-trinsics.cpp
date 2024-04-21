// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-trinsics.h>
#include <realdds/dds-utilities.h>

#include <rsutils/json.h>


namespace realdds {


rsutils::json video_intrinsics::to_json() const
{
    return rsutils::json::array( {
        width, height, principal_point_x, principal_point_y, focal_lenght_x, focal_lenght_y, distortion_model,
        distortion_coeffs[0], distortion_coeffs[1], distortion_coeffs[2], distortion_coeffs[3], distortion_coeffs[4]
    } );
}

/* static  */ video_intrinsics video_intrinsics::from_json( rsutils::json const & j )
{
    video_intrinsics ret;
    int index = 0;

    ret.width = j[index++].get< int >();
    ret.height = j[index++].get< int >();
    ret.principal_point_x = j[index++].get< float >();
    ret.principal_point_y = j[index++].get< float >();
    ret.focal_lenght_x = j[index++].get< float >();
    ret.focal_lenght_y = j[index++].get< float >();
    ret.distortion_model = j[index++].get< int >();
    ret.distortion_coeffs[0] = j[index++].get< float >();
    ret.distortion_coeffs[1] = j[index++].get< float >();
    ret.distortion_coeffs[2] = j[index++].get< float >();
    ret.distortion_coeffs[3] = j[index++].get< float >();
    ret.distortion_coeffs[4] = j[index++].get< float >();

    if( index != j.size() )
        DDS_THROW( runtime_error, "expected end of json at index " + std::to_string( index ) );

    return ret;
}

rsutils::json motion_intrinsics::to_json() const
{
    return rsutils::json::array( {
        data[0][0], data[0][1], data[0][2], data[0][3],
        data[1][0], data[1][1], data[1][2], data[1][3],
        data[2][0], data[2][1], data[2][2], data[2][3],
        noise_variances[0], noise_variances[1], noise_variances[2],
        bias_variances[0], bias_variances[1], bias_variances[2]
    } );
}

/* static  */ motion_intrinsics motion_intrinsics::from_json( rsutils::json const & j )
{
    motion_intrinsics ret;
    int index = 0;

    ret.data[0][0] = j[index++].get< float >();
    ret.data[0][1] = j[index++].get< float >();
    ret.data[0][2] = j[index++].get< float >();
    ret.data[0][3] = j[index++].get< float >();
    ret.data[1][0] = j[index++].get< float >();
    ret.data[1][1] = j[index++].get< float >();
    ret.data[1][2] = j[index++].get< float >();
    ret.data[1][3] = j[index++].get< float >();
    ret.data[2][0] = j[index++].get< float >();
    ret.data[2][1] = j[index++].get< float >();
    ret.data[2][2] = j[index++].get< float >();
    ret.data[2][3] = j[index++].get< float >();
    ret.noise_variances[0] = j[index++].get< float >();
    ret.noise_variances[1] = j[index++].get< float >();
    ret.noise_variances[2] = j[index++].get< float >();
    ret.bias_variances[0] = j[index++].get< float >();
    ret.bias_variances[1] = j[index++].get< float >();
    ret.bias_variances[2] = j[index++].get< float >();

    if( index != j.size() )
        DDS_THROW( runtime_error, "expected end of json at index " + std::to_string( index ) );

    return ret;
}

rsutils::json extrinsics::to_json() const
{
    return rsutils::json::array( {
        rotation[0], rotation[1], rotation[2],
        rotation[3], rotation[4], rotation[5],
        rotation[6], rotation[7], rotation[8],
        translation[0], translation[1], translation[2]
    } );
}

/* static  */ extrinsics extrinsics::from_json( rsutils::json const & j )
{
    extrinsics ret;
    int index = 0;

    ret.rotation[0] = j[index++].get< float >();
    ret.rotation[1] = j[index++].get< float >();
    ret.rotation[2] = j[index++].get< float >();
    ret.rotation[3] = j[index++].get< float >();
    ret.rotation[4] = j[index++].get< float >();
    ret.rotation[5] = j[index++].get< float >();
    ret.rotation[6] = j[index++].get< float >();
    ret.rotation[7] = j[index++].get< float >();
    ret.rotation[8] = j[index++].get< float >();
    ret.translation[0] = j[index++].get< float >();
    ret.translation[1] = j[index++].get< float >();
    ret.translation[2] = j[index++].get< float >();

    if( index != j.size() )
        DDS_THROW( runtime_error, "expected end of json at index " + std::to_string( index ) );

    return ret;
}


}  // namespace realdds
