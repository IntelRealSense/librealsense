// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-trinsics.h>
#include <realdds/dds-utilities.h>

#include <rsutils/json.h>


namespace realdds {


nlohmann::json video_intrinsics::to_json() const
{
    return nlohmann::json::array( {
        width, height, principal_point_x, principal_point_y, focal_lenght_x, focal_lenght_y, distortion_model,
        distortion_coeffs[0], distortion_coeffs[1], distortion_coeffs[2], distortion_coeffs[3], distortion_coeffs[4]
    } );
}

/* static  */ video_intrinsics video_intrinsics::from_json( nlohmann::json const & j )
{
    video_intrinsics ret;
    int index = 0;

    ret.width = rsutils::json::get< int >( j, index++ );
    ret.height = rsutils::json::get< int >( j, index++ );
    ret.principal_point_x = rsutils::json::get< float >( j, index++ );
    ret.principal_point_y = rsutils::json::get< float >( j, index++ );
    ret.focal_lenght_x = rsutils::json::get< float >( j, index++ );
    ret.focal_lenght_y = rsutils::json::get< float >( j, index++ );
    ret.distortion_model = rsutils::json::get< int >( j, index++ );
    ret.distortion_coeffs[0] = rsutils::json::get< float >( j, index++ );
    ret.distortion_coeffs[1] = rsutils::json::get< float >( j, index++ );
    ret.distortion_coeffs[2] = rsutils::json::get< float >( j, index++ );
    ret.distortion_coeffs[3] = rsutils::json::get< float >( j, index++ );
    ret.distortion_coeffs[4] = rsutils::json::get< float >( j, index++ );

    if( index != j.size() )
        DDS_THROW( runtime_error, "expected end of json at index " + std::to_string( index ) );

    return ret;
}

nlohmann::json motion_intrinsics::to_json() const
{
    return nlohmann::json::array( {
        data[0][0], data[0][1], data[0][2], data[0][3],
        data[1][0], data[1][1], data[1][2], data[1][3],
        data[2][0], data[2][1], data[2][2], data[2][3],
        noise_variances[0], noise_variances[1], noise_variances[2],
        bias_variances[0], bias_variances[1], bias_variances[2]
    } );
}

/* static  */ motion_intrinsics motion_intrinsics::from_json( nlohmann::json const & j )
{
    motion_intrinsics ret;
    int index = 0;

    ret.data[0][0] = rsutils::json::get< float >( j, index++ );
    ret.data[0][1] = rsutils::json::get< float >( j, index++ );
    ret.data[0][2] = rsutils::json::get< float >( j, index++ );
    ret.data[0][3] = rsutils::json::get< float >( j, index++ );
    ret.data[1][0] = rsutils::json::get< float >( j, index++ );
    ret.data[1][1] = rsutils::json::get< float >( j, index++ );
    ret.data[1][2] = rsutils::json::get< float >( j, index++ );
    ret.data[1][3] = rsutils::json::get< float >( j, index++ );
    ret.data[2][0] = rsutils::json::get< float >( j, index++ );
    ret.data[2][1] = rsutils::json::get< float >( j, index++ );
    ret.data[2][2] = rsutils::json::get< float >( j, index++ );
    ret.data[2][3] = rsutils::json::get< float >( j, index++ );
    ret.noise_variances[0] = rsutils::json::get< float >( j, index++ );
    ret.noise_variances[1] = rsutils::json::get< float >( j, index++ );
    ret.noise_variances[2] = rsutils::json::get< float >( j, index++ );
    ret.bias_variances[0] = rsutils::json::get< float >( j, index++ );
    ret.bias_variances[1] = rsutils::json::get< float >( j, index++ );
    ret.bias_variances[2] = rsutils::json::get< float >( j, index++ );

    if( index != j.size() )
        DDS_THROW( runtime_error, "expected end of json at index " + std::to_string( index ) );

    return ret;
}

nlohmann::json extrinsics::to_json() const
{
    return nlohmann::json::array( {
        rotation[0], rotation[1], rotation[2],
        rotation[3], rotation[4], rotation[5],
        rotation[6], rotation[7], rotation[8],
        translation[0], translation[1], translation[2]
    } );
}

/* static  */ extrinsics extrinsics::from_json( nlohmann::json const & j )
{
    extrinsics ret;
    int index = 0;

    ret.rotation[0] = rsutils::json::get< float >( j, index++ );
    ret.rotation[1] = rsutils::json::get< float >( j, index++ );
    ret.rotation[2] = rsutils::json::get< float >( j, index++ );
    ret.rotation[3] = rsutils::json::get< float >( j, index++ );
    ret.rotation[4] = rsutils::json::get< float >( j, index++ );
    ret.rotation[5] = rsutils::json::get< float >( j, index++ );
    ret.rotation[6] = rsutils::json::get< float >( j, index++ );
    ret.rotation[7] = rsutils::json::get< float >( j, index++ );
    ret.rotation[8] = rsutils::json::get< float >( j, index++ );
    ret.translation[0] = rsutils::json::get< float >( j, index++ );
    ret.translation[1] = rsutils::json::get< float >( j, index++ );
    ret.translation[2] = rsutils::json::get< float >( j, index++ );

    if( index != j.size() )
        DDS_THROW( runtime_error, "expected end of json at index " + std::to_string( index ) );

    return ret;
}


}  // namespace realdds
