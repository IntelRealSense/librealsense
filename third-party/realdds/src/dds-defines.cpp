// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-defines.h>
#include <realdds/dds-utilities.h>


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

    ret.width = utilities::json::get< int >( j, index++ );
    ret.height = utilities::json::get< int >( j, index++ );
    ret.principal_point_x = utilities::json::get< float >( j, index++ );
    ret.principal_point_y = utilities::json::get< float >( j, index++ );
    ret.focal_lenght_x = utilities::json::get< float >( j, index++ );
    ret.focal_lenght_y = utilities::json::get< float >( j, index++ );
    ret.distortion_model = utilities::json::get< int >( j, index++ );
    ret.distortion_coeffs[0] = utilities::json::get< float >( j, index++ );
    ret.distortion_coeffs[1] = utilities::json::get< float >( j, index++ );
    ret.distortion_coeffs[2] = utilities::json::get< float >( j, index++ );
    ret.distortion_coeffs[3] = utilities::json::get< float >( j, index++ );
    ret.distortion_coeffs[4] = utilities::json::get< float >( j, index++ );

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

    ret.data[0][0] = utilities::json::get< float >( j, index++ );
    ret.data[0][1] = utilities::json::get< float >( j, index++ );
    ret.data[0][2] = utilities::json::get< float >( j, index++ );
    ret.data[0][3] = utilities::json::get< float >( j, index++ );
    ret.data[1][0] = utilities::json::get< float >( j, index++ );
    ret.data[1][1] = utilities::json::get< float >( j, index++ );
    ret.data[1][2] = utilities::json::get< float >( j, index++ );
    ret.data[1][3] = utilities::json::get< float >( j, index++ );
    ret.data[2][0] = utilities::json::get< float >( j, index++ );
    ret.data[2][1] = utilities::json::get< float >( j, index++ );
    ret.data[2][2] = utilities::json::get< float >( j, index++ );
    ret.data[2][3] = utilities::json::get< float >( j, index++ );
    ret.noise_variances[0] = utilities::json::get< float >( j, index++ );
    ret.noise_variances[1] = utilities::json::get< float >( j, index++ );
    ret.noise_variances[2] = utilities::json::get< float >( j, index++ );
    ret.bias_variances[0] = utilities::json::get< float >( j, index++ );
    ret.bias_variances[1] = utilities::json::get< float >( j, index++ );
    ret.bias_variances[2] = utilities::json::get< float >( j, index++ );

    if( index != j.size() )
        DDS_THROW( runtime_error, "expected end of json at index " + std::to_string( index ) );

    return ret;
}


}  // namespace realdds
