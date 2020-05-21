//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "optimizer.h"
#include "cost.h"
#include "debug.h"

using namespace librealsense::algo::depth_to_rgb_calibration;


// Return the average pixel movement from the calibration
double optimizer::calc_correction_in_pixels( calib const & from_calibration ) const
{
    //%    [uvMap,~,~] = OnlineCalibration.aux.projectVToRGB(frame.vertices,params.rgbPmat,params.Krgb,params.rgbDistort);
    //% [uvMapNew,~,~] = OnlineCalibration.aux.projectVToRGB(frame.vertices,newParams.rgbPmat,newParams.Krgb,newParams.rgbDistort);
    auto old_uvmap = get_texture_map( _z.vertices, from_calibration, from_calibration.calc_p_mat());
    auto new_uvmap = get_texture_map( _z.vertices, _final_calibration, _final_calibration.calc_p_mat());
    if( old_uvmap.size() != new_uvmap.size() )
        throw std::runtime_error( to_string() << "did not expect different uvmap sizes (" << old_uvmap.size() << " vs " << new_uvmap.size() << ")" );
    // uvmap is Nx[x,y]

    //% xyMovement = mean(sqrt(sum((uvMap-uvMapNew).^2,2)));
    // note: "the mean of a vector containing NaN values is also NaN"
    // note: .^ = element-wise power
    // note: "if A is a matrix, then sum(A,2) is a column vector containing the sum of each row"
    // -> so average of sqrt( dx^2 + dy^2 )

    size_t const n_pixels = old_uvmap.size();
    if( !n_pixels )
        throw std::runtime_error( "no pixels found in uvmap" );
    double sum = 0;
    for( auto i = 0; i < n_pixels; i++ )
    {
        double2 const & o = old_uvmap[i];
        double2 const & n = new_uvmap[i];
        double dx = o.x - n.x, dy = o.y - n.y;
        double movement = sqrt( dx * dx + dy * dy );
        sum += movement;
    }
    return sum / n_pixels;
}

// Movement of pixels should clipped by a certain number of pixels
// This function actually changes the calibration if it exceeds this number of pixels!
void optimizer::clip_pixel_movement( size_t iteration_number )
{
    double xy_movement = calc_correction_in_pixels();

    // Clip any (average) movement of pixels if it's too big
    AC_LOG( DEBUG, "... average pixel movement= " << xy_movement );

    size_t n_max_movements = sizeof( _params.max_xy_movement_per_calibration ) / sizeof( _params.max_xy_movement_per_calibration[0] );
    double const max_movement = _params.max_xy_movement_per_calibration[std::min( n_max_movements - 1, iteration_number )];

    if( xy_movement > max_movement )
    {
        AC_LOG( WARNING, "Pixel movement too big: clipping at limit for iteration (" << iteration_number << ")= " << max_movement );

        //% mulFactor = maxMovementInThisIteration/xyMovement;
        double const mul_factor = max_movement / xy_movement;

        //% if ~strcmp( params.derivVar, 'P' )
        // -> assuming params.derivVar == 'KrgbRT'!!
        //%     optParams = { 'xAlpha'; 'yBeta'; 'zGamma'; 'Trgb'; 'Kdepth'; 'Krgb' };
        // -> note we don't do Kdepth at this time!
        //%     for fn = 1:numel( optParams )
        //%         diff = newParams.(optParams{ fn }) - params.(optParams{ fn });
        //%         newParams.(optParams{ fn }) = params.(optParams{ fn }) + diff * mulFactor;
        //%     end
        calib const & old_calib = _original_calibration;
        calib & new_calib = _final_calibration;
        new_calib = old_calib + (new_calib - old_calib) * mul_factor;

    }
}

std::vector< double > optimizer::cost_per_section_diff(calib const & old_calib, calib const & new_calib)
{
    // We require here that the section_map is initialized and ready
    if (_z.section_map.size() != _z.weights.size())
        throw std::runtime_error("section_map has not been initialized");

    auto uvmap_old = get_texture_map(_z.vertices, old_calib, old_calib.calc_p_mat());
    auto uvmap_new = get_texture_map(_z.vertices, new_calib, new_calib.calc_p_mat());

    size_t const n_sections_x = _params.num_of_sections_for_edge_distribution_x;
    size_t const n_sections_y = _params.num_of_sections_for_edge_distribution_y;
    size_t const n_sections = n_sections_x * n_sections_y;

    std::vector< double > cost_per_section_diff(n_sections, 0.);
    std::vector< size_t > N_per_section(n_sections, 0);

    //old
    auto d_vals_old = biliniar_interp(_yuy.edges_IDT, _yuy.width, _yuy.height, uvmap_old);

    auto cost_per_vertex_old = calc_cost_per_vertex(d_vals_old, _z, _yuy,
        [&](size_t i, double d_val, double weight, double vertex_cost) {});

    //new
    auto d_vals_new = biliniar_interp(_yuy.edges_IDT, _yuy.width, _yuy.height, uvmap_new);

    auto cost_per_vertex_new = calc_cost_per_vertex(d_vals_new, _z, _yuy,
        [&](size_t i, double d_val, double weight, double vertex_cost) {});


    for (auto i = 0; i < cost_per_vertex_new.size(); i++)
    {
        if (d_vals_old[i] != std::numeric_limits<double>::max() && d_vals_new[i] != std::numeric_limits<double>::max())
        {
            byte section = _z.section_map[i];
            cost_per_section_diff[section] += (cost_per_vertex_new[i] - cost_per_vertex_old[i]);
            ++N_per_section[section];
        }
    }

    for (size_t x = 0; x < n_sections; ++x)
    {

        double & cost = cost_per_section_diff[x];
        size_t N = N_per_section[x];
        if (N)
            cost /= N;
    }

    return cost_per_section_diff;
}

bool optimizer::is_valid_results()
{
    // Clip any (average) movement of pixels if it's too big
    clip_pixel_movement();

    // Based on (possibly new, clipped) calibration values, see if we've strayed too
    // far away from the camera's original factory calibration -- which we may not have
    if( _factory_calibration.width  &&  _factory_calibration.height )
    {
        double xy_movement = calc_correction_in_pixels();
        AC_LOG( DEBUG, "... average pixel movement from factory calibration= " << xy_movement );
        if( xy_movement > _params.max_xy_movement_from_origin )
        {
            AC_LOG( ERROR, "Calibration has moved too far from the original factory calibration (" << xy_movement << " pixels)" );
            return false;
        }
    }
    else
    {
        AC_LOG( DEBUG, "... no factory calibration available; skipping distance check" );
    }

    //%% Check and see that the score didn't increased by a lot in one image section and decreased in the others
    //% [c1, costVecOld] = OnlineCalibration.aux.calculateCost( frame.vertices, frame.weights, frame.rgbIDT, params );
    //% [c2, costVecNew] = OnlineCalibration.aux.calculateCost( frame.vertices, frame.weights, frame.rgbIDT, newParams );
    //% scoreDiffPerVertex = costVecNew - costVecOld;
    //% for i = 0:(params.numSectionsH*params.numSectionsV) - 1
    //%     scoreDiffPersection( i + 1 ) = nanmean( scoreDiffPerVertex( frame.sectionMapDepth == i ) );
    //% end

    _z.cost_diff_per_section = cost_per_section_diff(_original_calibration, _final_calibration);
    //% validOutputStruct.minImprovementPerSection = min( scoreDiffPersection );
    //% validOutputStruct.maxImprovementPerSection = max( scoreDiffPersection );
    double min_cost_diff = *std::min_element( _z.cost_diff_per_section.begin(), _z.cost_diff_per_section.end() );
    double max_cost_diff = *std::max_element( _z.cost_diff_per_section.begin(), _z.cost_diff_per_section.end() );
    AC_LOG( DEBUG, "... min cost diff= " << min_cost_diff << "  max= " << max_cost_diff );
    if( min_cost_diff < 0. )
    {
        AC_LOG( ERROR, "Some image sections were hurt by the optimization; invalidating calibration!" );
        for( size_t x = 0; x < _z.cost_diff_per_section.size(); ++x )
            AC_LOG( DEBUG, "... cost diff in section " << x << "= " << _z.cost_diff_per_section[x] );
        return false;
    }

    return true;
}

