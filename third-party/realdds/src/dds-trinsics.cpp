// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <realdds/dds-trinsics.h>
#include <realdds/dds-utilities.h>
#include <realdds/topics/dds-topic-names.h>

#include <rsutils/json.h>
using rsutils::json;

#include <ostream>


namespace realdds {


std::ostream & operator<<( std::ostream & os, distortion_model model )
{
    switch( model )
    {
    case distortion_model::none:
        os << "none";
        break;

    case distortion_model::brown:
        os << "brown";
        break;

    case distortion_model::inverse_brown:
        os << "inverse-brown";
        break;

    case distortion_model::modified_brown:
        os << "modified-brown";
        break;

    default:
        DDS_THROW( runtime_error, "unknown distortion model: " << int( model ) );
    }
    return os;
}


std::ostream & operator<<( std::ostream & os, distortion_parameters const & p )
{
    os << p.model;
    switch( p.model )
    {
    case distortion_model::brown:
    case distortion_model::inverse_brown:
    case distortion_model::modified_brown:
        os << "[" << p.coeffs[0] << ',' << p.coeffs[1] << ',' << p.coeffs[2] << ',' << p.coeffs[3] << ',' << p.coeffs[4]
           << "]";
        break;
    }
    return os;
}


std::ostream & operator<<( std::ostream & os, video_intrinsics const & self )
{
    os << self.width << 'x' << self.height;
    os << " pp[" << self.principal_point.x << ',' << self.principal_point.y << "]";
    os << " fl[" << self.focal_length.x << ',' << self.focal_length.y << "]";
    os << " " << self.distortion;
    if( self.force_symmetry )
        os << " force-symmetry";
    return os;
}


json video_intrinsics::to_json() const
{
    json j = json::object( {
        { topics::notification::stream_options::intrinsics::key::width, width },
        { topics::notification::stream_options::intrinsics::key::height, height },
        { topics::notification::stream_options::intrinsics::key::principal_point, principal_point },
        { topics::notification::stream_options::intrinsics::key::focal_length, focal_length },
    } );
    switch( distortion.model )
    {
    case distortion_model::none:
        break;

    case distortion_model::brown:
        j[topics::notification::stream_options::intrinsics::key::coefficients] = distortion.coeffs;
        break;

    case distortion_model::inverse_brown:
        j[topics::notification::stream_options::intrinsics::key::model] = "inverse-brown";
        j[topics::notification::stream_options::intrinsics::key::coefficients] = distortion.coeffs;
        break;

    case distortion_model::modified_brown:
        j[topics::notification::stream_options::intrinsics::key::model] = "modified-brown";
        j[topics::notification::stream_options::intrinsics::key::coefficients] = distortion.coeffs;
        break;

    default:
        DDS_THROW( runtime_error, "unknown distortion model: " << int( distortion.model ) );
    }
    if( force_symmetry )
        j[topics::notification::stream_options::intrinsics::key::force_symmetry] = true;
    return j;
}

/* static */ video_intrinsics video_intrinsics::from_json( json const & j )
{
    video_intrinsics ret;
    if( j.is_array() )
    {
        // Legacy format; should be removed
        int index = 0;

        ret.width = j[index++].get< int >();
        ret.height = j[index++].get< int >();
        ret.principal_point.x = j[index++].get< float >();
        ret.principal_point.y = j[index++].get< float >();
        ret.focal_length.x = j[index++].get< float >();
        ret.focal_length.y = j[index++].get< float >();
        switch( j[index++].get< int >() )
        {
        case 0:  // RS2_DISTORTION_NONE
            ret.distortion.model = distortion_model::none;
            break;

        case 1:  // RS2_DISTORTION_MODIFIED_BROWN_CONRADY
            ret.distortion.model = distortion_model::modified_brown;
            break;

        case 2:  // RS2_DISTORTION_INVERSE_BROWN_CONRADY
            ret.distortion.model = distortion_model::inverse_brown;
            break;

        case 4:  // RS2_DISTORTION_BROWN_CONRADY
            ret.distortion.model = distortion_model::brown;
            break;

        default:
            DDS_THROW( runtime_error, "invalid legacy distortion model: " << j[index-1] );
        }
        ret.distortion.coeffs[0] = j[index++].get< float >();
        ret.distortion.coeffs[1] = j[index++].get< float >();
        ret.distortion.coeffs[2] = j[index++].get< float >();
        ret.distortion.coeffs[3] = j[index++].get< float >();
        ret.distortion.coeffs[4] = j[index++].get< float >();

        if( index != j.size() )
            DDS_THROW( runtime_error, "expected end of json at index " << index );
    }
    else if( j.is_object() )
    {
        using namespace topics::notification::stream_options::intrinsics;

        // The intrinsics are communicated in an object:
        //    - A `width` and `height` are for the native stream resolution, as 16 - bit integer values
        //    - A `principal-point` defined as `[<x>, <y>]` floating point values
        //    - A `focal-length`, also as `[<x>, <y>]` floats
        j.nested( key::width ).get_to( ret.width );
        j.nested( key::height ).get_to( ret.height );
        j.nested( key::principal_point ).get_to( ret.principal_point );
        j.nested( key::focal_length ).get_to( ret.focal_length );

        //    - Symmetry affects how the intrinsics are scaled to different resolutions
        ret.force_symmetry = j.nested( key::force_symmetry ).default_value( false );

        // A distortion model default to none
        ret.distortion = distortion_from_json( j );
    }
    else
        DDS_THROW( runtime_error, "unspected intrinsics json: " << j );

    return ret;
}


/*static */ distortion_parameters video_intrinsics::distortion_from_json( rsutils::json const & j )
{
    distortion_parameters result;
    using namespace topics::notification::stream_options::intrinsics;

    // A distortion model may be applied:
    //    - The `model` would specify which model is to be used, with the default of `brown`
    //    - The `coefficients` is an array of floating point values, the number and meaning which depend on the `model`
    //    - For `brown`, 5 points [k1, k2, p1, p2, k3] are needed
    auto coeffs_j = j.nested( key::coefficients );
    auto model_j = j.nested( key::model );
    switch( model_j.type() )
    {
    case json::value_t::discarded:
        if( coeffs_j.is_array() && coeffs_j.size() == 5 )
            result.model = distortion_model::brown;
        else
            result.model = distortion_model::none;
        break;

    case json::value_t::string:
        if( "brown" == model_j.string_ref() )
            result.model = distortion_model::brown;
        else if( "none" == model_j.string_ref() )
            result.model = distortion_model::none;
        else if( "inverse-brown" == model_j.string_ref() )
            result.model = distortion_model::inverse_brown;
        else if( "modified-brown" == model_j.string_ref() )
            result.model = distortion_model::modified_brown;
        else
            DDS_THROW( runtime_error, "unknown distortion model: " << model_j );
        break;

    default:
        DDS_THROW( runtime_error, "invalid distortion model: " << model_j );
    }
    switch( result.model )
    {
    case distortion_model::none:
        if( coeffs_j )
            DDS_THROW( runtime_error, "distortion coefficients without a model" );
        result.coeffs = { 0 };
        break;

    case distortion_model::brown:
    case distortion_model::inverse_brown:
    case distortion_model::modified_brown:
        if( !coeffs_j.is_array() )
            DDS_THROW( runtime_error, "invalid distortion coefficients: " << coeffs_j );
        if( coeffs_j.size() != 5 )
            DDS_THROW( runtime_error, "distortion coefficients expected [k1,k2,p1,p2,k3]: " << coeffs_j );
        coeffs_j.get_to( result.coeffs );
        break;

    default:
        DDS_THROW( runtime_error, "invalid distortion model" );
    }

    return result;
}


void video_intrinsics::override_from_json( json const & j )
{
    if( ! j.is_object() )
        DDS_THROW( runtime_error, "cannot override intrinsics if not an object: " << j );

    using namespace topics::notification::calibration_changed::intrinsics;

    int w, h;
    if( j.nested( key::width ).get_ex( w ) && w != width )
        DDS_THROW( runtime_error, "intrinsics should not change width" );
    if( j.nested( key::height ).get_ex( h ) && h != height )
        DDS_THROW( runtime_error, "intrinsics should not change height" );

    j.nested( key::principal_point ).get_ex( principal_point );
    j.nested( key::focal_length ).get_ex( focal_length );
    j.nested( key::force_symmetry ).get_ex( force_symmetry );

    if( j.nested( key::coefficients ) || j.nested( key::model ) )
        distortion = distortion_from_json( j );
}


json motion_intrinsics::to_json() const
{
    return json::array( {
        data[0][0], data[0][1], data[0][2], data[0][3],
        data[1][0], data[1][1], data[1][2], data[1][3],
        data[2][0], data[2][1], data[2][2], data[2][3],
        noise_variances[0], noise_variances[1], noise_variances[2],
        bias_variances[0], bias_variances[1], bias_variances[2]
    } );
}

/* static  */ motion_intrinsics motion_intrinsics::from_json( json const & j )
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

json extrinsics::to_json() const
{
    return json::array( {
        rotation[0], rotation[1], rotation[2],
        rotation[3], rotation[4], rotation[5],
        rotation[6], rotation[7], rotation[8],
        translation[0], translation[1], translation[2]
    } );
}

/* static  */ extrinsics extrinsics::from_json( json const & j )
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


// Scale video intrinsics at some nominal resolution to another.
// The algo is the latest as implemented for D500 devices, and adapted from there.
//
video_intrinsics video_intrinsics::scaled_to( int const new_width, int const new_height ) const
{
    if( ! width || ! height )
        DDS_THROW( runtime_error, "cannot scale with 0 width/height" );

    video_intrinsics scaled( *this );

    if( force_symmetry )
    {
        // Cropping the frame so that the principal point is at the middle
        scaled.width = 1 + 2 * static_cast< int >( std::min( principal_point.x, width - 1 - principal_point.x ) );
        scaled.height = 1 + 2 * static_cast< int >( std::min( principal_point.y, height - 1 - principal_point.y ) );
        scaled.principal_point.x = ( scaled.width - 1 ) / 2.f;
        scaled.principal_point.y = ( scaled.height - 1 ) / 2.f;
    }

    auto const scale_ratio_x = static_cast< float >( new_width ) / scaled.width;
    auto const scale_ratio_y = static_cast< float >( new_height ) / scaled.height;
    auto const scale_ratio = std::max( scale_ratio_x, scale_ratio_y );

    auto const crop_x = ( scaled.width * scale_ratio - new_width ) * 0.5f;
    auto const crop_y = ( scaled.height * scale_ratio - new_height ) * 0.5f;

    scaled.width = new_width;
    scaled.height = new_height;

    scaled.principal_point.x = ( scaled.principal_point.x + 0.5f ) * scale_ratio - crop_x - 0.5f;
    scaled.principal_point.y = ( scaled.principal_point.y + 0.5f ) * scale_ratio - crop_y - 0.5f;

    scaled.focal_length.x *= scale_ratio;
    scaled.focal_length.y *= scale_ratio;

    return scaled;
}


}  // namespace realdds
