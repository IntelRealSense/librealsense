//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "optimizer.h"
#include <librealsense2/rsutil.h>
#include <algorithm>
#include <array>
#include "coeffs.h"
#include "debug.h"

using namespace librealsense::algo::depth_to_rgb_calibration;


namespace
{
    std::vector<double> calc_intensity( std::vector<double> const & image1, std::vector<double> const & image2 )
    {
        std::vector<double> res( image1.size(), 0 );

        for( auto i = 0; i < image1.size(); i++ )
        {
            res[i] = sqrt( pow( image1[i], 2 ) + pow( image2[i], 2 ) );
        }
        return res;
    }

    template<class T>
    double dot_product( std::vector<T> const & sub_image, std::vector<double> const & mask )
    {
        double res = 0;

        for( auto i = 0; i < sub_image.size(); i++ )
        {
            res += sub_image[i] * mask[i];
        }

        return res;
    }

    template<class T>
    std::vector<double> convolution(std::vector<T> const& image,
        size_t image_width, size_t image_height,
        size_t mask_width, size_t mask_height,
        std::function< double(std::vector<T> const& sub_image) > convolution_operation
        )
    {
        std::vector<double> res(image.size(), 0);

        for (auto i = 0; i < image_height - mask_height + 1; i++)
        {
            for (size_t j = 0; j < image_width - mask_width + 1; j++)
            {
                std::vector<T> sub_image(mask_width * mask_height, 0);
                auto ind = 0;
                for (size_t l = 0; l < mask_height; l++)
                {
                    for (size_t k = 0; k < mask_width; k++)
                    {
                        size_t p = (i + l) * image_width + j + k;
                        sub_image[ind++] = (image[p]);
                    }

                }
                auto mid = (i + mask_height / 2) * image_width + j + mask_width / 2;

                res[mid] = convolution_operation(sub_image);

            }
        }
        return res;
    }
   
    template<class T>
    std::vector<double> calc_horizontal_gradient( std::vector<T> const & image, size_t image_width, size_t image_height )
    {
        std::vector<double> horizontal_gradients = { -1, -2, -1,
                                                      0,  0,  0,
                                                      1,  2,  1 };

        return convolution<T>( image, image_width, image_height, 3, 3, [&]( std::vector<T> const & sub_image )
            {return dot_product( sub_image, horizontal_gradients ) / (double)8; } );
    }

    template<class T>
    std::vector<double> calc_vertical_gradient( std::vector<T> const & image, size_t image_width, size_t image_height )
    {
        std::vector<double> vertical_gradients = { -1, 0, 1,
                                                   -2, 0, 2,
                                                   -1, 0, 1 };

        return convolution<T>( image, image_width, image_height, 3, 3, [&]( std::vector<T> const & sub_image )
            {return dot_product( sub_image, vertical_gradients ) / (double)8; } );;
    }

    template<class T>
    std::vector<double> calc_edges( std::vector<T> const & image, size_t image_width, size_t image_height )
    {
        auto vertical_edge = calc_vertical_gradient( image, image_width, image_height );

        auto horizontal_edge = calc_horizontal_gradient( image, image_width, image_height );

        auto edges = calc_intensity( vertical_edge, horizontal_edge );

        return edges;
    }

    std::pair<int, int> const dir_map[direction::deg_none] =
    {
        { 1, 0},  // dir_0
        { 1, 1},  // dir_45
        { 0, 1},  // deg_90
        {-1, 1}   // deg_135
    };

}


optimizer::optimizer()
{
}

static std::vector< double > get_direction_deg(
    std::vector<double> const & gradient_x,
    std::vector<double> const & gradient_y
)
{
#define PI 3.14159265
    std::vector<double> res( gradient_x.size(), deg_none );

    for( auto i = 0; i < gradient_x.size(); i++ )
    {
        int closest = -1;
        auto angle = atan2( gradient_y[i], gradient_x[i] )* 180.f / PI;
        angle = angle < 0 ? 180 + angle : angle;
        auto dir = fmod( angle, 180 );


        res[i] = dir;
    }
    return res;
}

static
std::pair< int, int > get_prev_index(
    direction dir,
    int i, int j,
    size_t width, size_t height )
{
    int edge_minus_idx = 0;
    int edge_minus_idy = 0;

    auto & d = dir_map[dir];
    if( j < d.first )
        edge_minus_idx = int(width) - 1 - j;
    else if( j - d.first >= width )
        edge_minus_idx = 0;
    else
        edge_minus_idx = j - d.first;

    if( i - d.second < 0 )
        edge_minus_idy = int(height) - 1 - i;
    else if( i - d.second >= height )
        edge_minus_idy = 0;
    else
        edge_minus_idy = i - d.second;

    return { edge_minus_idx, edge_minus_idy };
}

static
std::pair< int, int > get_next_index(
    direction dir,
    int i, int j,
    size_t width, size_t height
)
{
    auto & d = dir_map[dir];

    int edge_plus_idx = 0;
    int edge_plus_idy = 0;

    if( j + d.first < 0 )
        edge_plus_idx = int(width) - 1 - j;
    else if( j + d.first >= width )
        edge_plus_idx = 0;
    else
        edge_plus_idx = j + d.first;

    if( i + d.second < 0 )
        edge_plus_idy = int(height) - 1 - i;
    else if( i + d.second >= height )
        edge_plus_idy = 0;
    else
        edge_plus_idy = i + d.second;

    return { edge_plus_idx, edge_plus_idy };
}

// Return Z edges with weak edges zeroed out
static void suppress_weak_edges(
    z_frame_data & z_data,
    ir_frame_data const & ir_data,
    params const & params )
{
    std::vector< double > & res = z_data.supressed_edges;
    res = z_data.edges;
    double const grad_ir_threshold = params.grad_ir_threshold;
    double const grad_z_threshold = params.grad_z_threshold;
    z_data.n_strong_edges = 0;
    for( auto i = 0; i < z_data.height; i++ )
    {
        for( auto j = 0; j < z_data.width; j++ )
        {
            auto idx = i * z_data.width + j;

            auto edge = z_data.edges[idx];

            //if (edge == 0)  continue;

            auto edge_prev_idx = get_prev_index( z_data.directions[idx], i, j, z_data.width, z_data.height );

            auto edge_next_idx = get_next_index( z_data.directions[idx], i, j, z_data.width, z_data.height );

            auto edge_minus_idx = edge_prev_idx.second * z_data.width + edge_prev_idx.first;

            auto edge_plus_idx = edge_next_idx.second * z_data.width + edge_next_idx.first;

            auto z_edge_plus = z_data.edges[edge_plus_idx];
            auto z_edge = z_data.edges[idx];
            auto z_edge_minus = z_data.edges[edge_minus_idx];

            if( z_edge_minus > z_edge || z_edge_plus > z_edge || ir_data.ir_edges[idx] <= grad_ir_threshold || z_data.edges[idx] <= grad_z_threshold )
            {
                res[idx] = 0;
            }
            else
            {
                ++z_data.n_strong_edges;
            }
        }
    }
}

static
std::pair<
    std::vector< double >,
    std::vector< double >
>
calc_subpixels(
    z_frame_data const & z_data,
    ir_frame_data const & ir_data,
    double const grad_ir_threshold,
    double const grad_z_threshold,
    size_t const width, size_t const height
)
{
    std::vector< double > subpixels_x;
    std::vector< double > subpixels_y;

    subpixels_x.reserve( z_data.edges.size() );
    subpixels_y.reserve( z_data.edges.size() );

    for( auto i = 0; i < height; i++ )
    {
        for( auto j = 0; j < width; j++ )
        {
            auto idx = i * width + j;

            auto edge = z_data.edges[idx];

            //if( edge == 0 )  continue;   // TODO commented out elsewhere...

            // Note:
            // The original matlab code shifts in the opposite direction:
            //     Z_plus  = circshift( frame.z, -currDir );
            //     Z_minus = circshift( frame.z, +currDir );
            // But here we're looking at a specific index and what its value would be
            // AFTER the shift. E.g. (with 1 dimension for simplicity, with dir=[1]):
            //     original: [ 1 2 3 4 ]
            //     Z_plus  : [ 2 3 4 1 ]   (shift left [-1])
            //     Z_minus : [ 4 1 2 3 ]   (shift right [1])
            // At index [2] (0-based) there was a 3 but we now need Z_plus-Z_minus (see below)
            // or (4-2). Note that, for Z_plus, the shift was left but the new value was from
            // the right! In other words, we do not need to negate currDir!

            auto edge_prev_idx = get_prev_index( z_data.directions[idx], i, j, width, height );
            auto edge_next_idx = get_next_index( z_data.directions[idx], i, j, width, height );

            auto edge_minus_idx = edge_prev_idx.second * width + edge_prev_idx.first;
            auto edge_plus_idx = edge_next_idx.second * width + edge_next_idx.first;

            auto z_plus = z_data.edges[edge_plus_idx];
            auto z_edge = z_data.edges[idx];
            auto z_minus = z_data.edges[edge_minus_idx];

            auto dir = z_data.directions[idx];
            double x = 0, y = 0;
          
            if(z_data.supressed_edges[idx])
            {
                //% fraqStep = (-0.5*(zEdge_plus-zEdge_minus)./(zEdge_plus+zEdge_minus-2*zEdge)); % The step we need to move to reach the subpixel gradient i nthe gradient direction
                //% subGrad_d = fraqStep.*reshape(currDir,1,1,[]);
                //% subGrad_d = subGrad_d + cat(3,gridY,gridX);% the location of the subpixel gradient
                //% ...
                //% zEdgeSubPixel(cat(3,supressedEdges_d,supressedEdges_d)) = subGrad_d(cat(3,supressedEdges_d,supressedEdges_d));

                double fraq_step = 0;
                if( double( z_plus + z_minus - (double)2 * z_edge ) == 0 )
                    fraq_step = 0;
                else
                    fraq_step = double( (-0.5f*double( z_plus - z_minus )) / double( z_plus + z_minus - 2 * z_edge ) );

                // NOTE:
                // We adjust by +1 to fit the X/Y to matlab's 1-based index convention
                // TODO make sure this fits in with our own USAGE of these coordinates (where we would
                // likely have to do -1)
                y = i + 1 + fraq_step * (double)dir_map[dir].second;
                x = j + 1 + fraq_step * (double)dir_map[dir].first;
            }

            subpixels_y.push_back( y );
            subpixels_x.push_back( x );
        }
    }
    assert( subpixels_x.size() == z_data.edges.size() );
    return { subpixels_x, subpixels_y };
}

void optimizer::set_z_data(
    std::vector< z_t > && z_data,
    rs2_intrinsics_double const & depth_intrinsics,
    float depth_units )
{
    _params.set_depth_resolution( depth_intrinsics.width, depth_intrinsics.height );
    _z.width = depth_intrinsics.width;
    _z.height = depth_intrinsics.height;
    _z.intrinsics = depth_intrinsics;
    _z.depth_units = depth_units;

    _z.frame = std::move( z_data );

    _z.gradient_x = calc_vertical_gradient( _z.frame, depth_intrinsics.width, depth_intrinsics.height );
    _z.gradient_y = calc_horizontal_gradient( _z.frame, depth_intrinsics.width, depth_intrinsics.height );
    _z.edges = calc_intensity( _z.gradient_x, _z.gradient_y );
    _z.directions = get_direction( _z.gradient_x, _z.gradient_y );
    _z.direction_deg = get_direction_deg( _z.gradient_x, _z.gradient_y );
    suppress_weak_edges( _z, _ir, _params );

    auto subpixels = calc_subpixels( _z, _ir,
        _params.grad_ir_threshold, _params.grad_z_threshold,
        depth_intrinsics.width, depth_intrinsics.height );
    _z.subpixels_x = subpixels.first;
    _z.subpixels_y = subpixels.second;

    _z.closest = get_closest_edges( _z, _ir, depth_intrinsics.width, depth_intrinsics.height );

    calculate_weights( _z );

    auto vertices = subedges2vertices( _z, depth_intrinsics, depth_units );
}
 

void optimizer::set_yuy_data(
    std::vector< yuy_t > && yuy_data,
    std::vector< yuy_t > && prev_yuy_data,
    calib const & calibration
)
{
    _original_calibration = calibration;
    _original_calibration.calc_p_mat();
    _yuy.width = calibration.width;
    _yuy.height = calibration.height;
    _params.set_rgb_resolution( _yuy.width, _yuy.height );

    _yuy.orig_frame = std::move( yuy_data );
    _yuy.prev_frame = std::move( prev_yuy_data );

    _yuy.lum_frame = get_luminance_from_yuy2( _yuy.orig_frame );
    _yuy.prev_lum_frame = get_luminance_from_yuy2( _yuy.prev_frame );

    _yuy.edges = calc_edges( _yuy.lum_frame, _yuy.width, _yuy.height );
    _yuy.prev_edges = calc_edges(_yuy.prev_lum_frame, _yuy.width, _yuy.height);

    _yuy.edges_IDT = blur_edges( _yuy.edges, _yuy.width, _yuy.height );

    _yuy.edges_IDTx = calc_vertical_gradient( _yuy.edges_IDT, _yuy.width, _yuy.height );

    _yuy.edges_IDTy = calc_horizontal_gradient( _yuy.edges_IDT, _yuy.width, _yuy.height );
}

void optimizer::set_ir_data(
    std::vector< ir_t > && ir_data,
    size_t width,
    size_t height
)
{
    _ir.width = width;
    _ir.height = height;
    
    _ir.ir_frame = std::move( ir_data );
    _ir.ir_edges = calc_edges( _ir.ir_frame, width, height );
}


void optimizer::zero_invalid_edges( z_frame_data & z_data, ir_frame_data const & ir_data )
{
    for( auto i = 0; i < ir_data.ir_edges.size(); i++ )
    {
        if( ir_data.ir_edges[i] <= _params.grad_ir_threshold || z_data.edges[i] <= _params.grad_z_threshold )
        {
            z_data.supressed_edges[i] = 0;
            z_data.subpixels_x[i] = 0;
            z_data.subpixels_y[i] = 0;
            z_data.closest[i] = 0;
        }
    }
}

std::vector< direction > optimizer::get_direction( std::vector<double> gradient_x, std::vector<double> gradient_y )
{
#define PI 3.14159265
    std::vector<direction> res( gradient_x.size(), deg_none );

    std::map<int, direction> angle_dir_map = { {0, deg_0}, {45,deg_45} , {90,deg_90}, {135,deg_135} };

    for( auto i = 0; i < gradient_x.size(); i++ )
    {
        int closest = -1;
        auto angle = atan2( gradient_y[i], gradient_x[i] )* 180.f / PI;
        angle = angle < 0 ? 180 + angle : angle;
        auto dir = fmod( angle, 180 );

        for( auto d : angle_dir_map )
        {
            closest = closest == -1 || abs( dir - d.first ) < abs( dir - closest ) ? d.first : closest;
        }
        res[i] = angle_dir_map[closest];
    }
    return res;
}

std::vector< uint16_t > optimizer::get_closest_edges(
    z_frame_data const & z_data,
    ir_frame_data const & ir_data,
    size_t width, size_t height )
{
    std::vector< uint16_t > z_closest;
    z_closest.reserve( z_data.edges.size() );

    for( auto i = 0; i < int(height); i++ )
    {
        for( auto j = 0; j < int(width); j++ )
        {
            auto idx = i * width + j;

            auto edge = z_data.edges[idx];

            //if (edge == 0)  continue;

            auto edge_prev_idx = get_prev_index( z_data.directions[idx], i, j, width, height );

            auto edge_next_idx = get_next_index( z_data.directions[idx], i, j, width, height );

            auto edge_minus_idx = edge_prev_idx.second * width + edge_prev_idx.first;

            auto edge_plus_idx = edge_next_idx.second * width + edge_next_idx.first;

            auto z_edge_plus = z_data.edges[edge_plus_idx];
            auto z_edge = z_data.edges[idx];
            auto z_edge_minus = z_data.edges[edge_minus_idx];

           
            if (z_data.supressed_edges[idx])
            {
                z_closest.push_back(std::min(z_data.frame[edge_minus_idx], z_data.frame[edge_plus_idx]));
            }
            else
            {
                z_closest.push_back(0);
            }
        }
    }
    return z_closest;
}

/* Given pixel coordinates and depth in an image with no distortion or inverse distortion coefficients, compute the corresponding point in 3D space relative to the same camera */
static void deproject_pixel_to_point(double point[3], const struct rs2_intrinsics_double * intrin, const double pixel[2], double depth)
{
    double x = (double)(pixel[0] - intrin->ppx) / intrin->fx;
    double y = (double)(pixel[1] - intrin->ppy) / intrin->fy;

    point[0] = depth * x;
    point[1] = depth * y;
    point[2] = depth;
}
/* Transform 3D coordinates relative to one sensor to 3D coordinates relative to another viewpoint */
static void transform_point_to_point(double to_point[3], const rs2_extrinsics_double & extr, const double from_point[3])
{
    to_point[0] = (double)extr.rotation[0] * from_point[0] + (double)extr.rotation[3] * from_point[1] + (double)extr.rotation[6] * from_point[2] + (double)extr.translation[0];
    to_point[1] = (double)extr.rotation[1] * from_point[0] + (double)extr.rotation[4] * from_point[1] + (double)extr.rotation[7] * from_point[2] + (double)extr.translation[1];
    to_point[2] = (double)extr.rotation[2] * from_point[0] + (double)extr.rotation[5] * from_point[1] + (double)extr.rotation[8] * from_point[2] + (double)extr.translation[2];
}

static void transform_point_to_uv(double pixel[2], const struct p_matrix& pmat, const double point[3])
{
    auto p = pmat.vals;
    double to_point[3];
    to_point[0] = p[0] * point[0] + p[1] * point[1] + p[2] * point[2] + p[3];
    to_point[1] = p[4] * point[0] + p[5] * point[1] + p[6] * point[2] + p[7];
    to_point[2] = p[8] * point[0] + p[9] * point[1] + p[10] * point[2] + p[11];

    pixel[0] = to_point[0] / to_point[2];
    pixel[1] = to_point[1] / to_point[2];
}

/* Given a point in 3D space, compute the corresponding pixel coordinates in an image with no distortion or forward distortion coefficients produced by the same camera */
static void project_point_to_pixel(double pixel[2], const struct rs2_intrinsics_double * intrin, const double point[3])
{
    double x = point[0] / point[2], y = point[1] / point[2];

    if( intrin->model == RS2_DISTORTION_BROWN_CONRADY )
    {
        double r2 = x * x + y * y;
        double f = 1 + intrin->coeffs[0] * r2 + intrin->coeffs[1] * r2*r2 + intrin->coeffs[4] * r2*r2*r2;

        double xcd = x * f;
        double ycd = y * f;

        double dx = xcd + 2 * intrin->coeffs[2] * x*y + intrin->coeffs[3] * (r2 + 2 * x*x);
        double dy = ycd + 2 * intrin->coeffs[3] * x*y + intrin->coeffs[2] * (r2 + 2 * y*y);

        x = dx;
        y = dy;
    }

    pixel[0] = x * (double)intrin->fx + (double)intrin->ppx;
    pixel[1] = y * (double)intrin->fy + (double)intrin->ppy;
}

/* Given a point in 3D space, compute the corresponding pixel coordinates in an image with no distortion or forward distortion coefficients produced by the same camera */
static void distort_pixel(double pixel[2], const struct rs2_intrinsics_double * intrin, const double point[2])
{
    double x = (point[0] - intrin->ppx) / intrin->fx;
    double y = (point[1] - intrin->ppy) / intrin->fy;

    if (intrin->model == RS2_DISTORTION_BROWN_CONRADY)
    {
        double r2 = x * x + y * y;
        double r2c = 1 + intrin->coeffs[0] * r2 + intrin->coeffs[1] * r2*r2 + intrin->coeffs[4] * r2*r2*r2;

        double xcd = x * r2c;
        double ycd = y * r2c;

        double dx = xcd + 2 * intrin->coeffs[2] * x*y + intrin->coeffs[3] * (r2 + 2 * x*x);
        double dy = ycd + 2 * intrin->coeffs[3] * x*y + intrin->coeffs[2] * (r2 + 2 * y*y);

        x = dx;
        y = dy;
    }

    pixel[0] = x * (double)intrin->fx + (double)intrin->ppx;
    pixel[1] = y * (double)intrin->fy + (double)intrin->ppy;
}

std::vector<double> optimizer::blur_edges(std::vector<double> const & edges, size_t image_width, size_t image_height)
{
    std::vector<double> res = edges;

    for( auto i = 0; i < image_height; i++ )
        for( auto j = 0; j < image_width; j++ )
        {
            if( i == 0 && j == 0 )
                continue;
            else if( i == 0 )
                res[j] = std::max( res[j], res[j - 1] * _params.gamma );
            else if( j == 0 )
                res[i*image_width + j] = std::max(
                    res[i*image_width + j],
                    res[(i - 1)*image_width + j] * _params.gamma );
            else
                res[i*image_width + j] = std::max(
                    res[i*image_width + j],
                    std::max(
                        res[ i     *image_width + j - 1] * _params.gamma,
                        res[(i - 1)*image_width + j    ] * _params.gamma ) );
        }


    for( int i = int(image_height) - 1; i >= 0; i-- )  // note: must be signed because will go under 0!
        for( int j = int(image_width) - 1; j >= 0; j-- )
        {
            if( i == image_height - 1 && j == image_width - 1 )
                continue;
            else if( i == image_height - 1 )
                res[i*image_width + j] = std::max( res[i*image_width + j], res[i*image_width + j + 1] * _params.gamma );
            else if( j == image_width - 1 )
                res[i*image_width + j] = std::max( res[i*image_width + j], res[(i + 1)*image_width + j] * _params.gamma );
            else
                res[i*image_width + j] = std::max( res[i*image_width + j], (std::max( res[i*image_width + j + 1] * _params.gamma, res[(i + 1)*image_width + j] * _params.gamma )) );
        }

    for( auto i = 0; i < image_height; i++ )
        for( auto j = 0; j < image_width; j++ )
            res[i*image_width + j] = _params.alpha * edges[i*image_width + j] + (1 - _params.alpha) * res[i*image_width + j];
    return res;
}


std::vector<uint8_t> optimizer::get_luminance_from_yuy2( std::vector<uint16_t> yuy2_imagh )
{
    std::vector<uint8_t> res( yuy2_imagh.size(), 0 );
    auto yuy2 = (uint8_t*)yuy2_imagh.data();
    for( auto i = 0; i < res.size(); i++ )
        res[i] = yuy2[i * 2];

    return res;
}

std::vector<uint8_t> optimizer::get_logic_edges( std::vector<double> edges )
{
    std::vector<uint8_t> logic_edges( edges.size(), 0 );
    auto max = std::max_element( edges.begin(), edges.end() );
    auto thresh = *max*_params.edge_thresh4_logic_lum;

    for( auto i = 0; i < edges.size(); i++ )
    {
        logic_edges[i] = abs( edges[i] ) > thresh ? 1 : 0;
    }
    return logic_edges;
}



void optimizer::sum_per_section(
    std::vector< double > & sum_weights_per_section,
    std::vector< byte > const & section_map,
    std::vector< double > const & weights,
    size_t num_of_sections
)
{/*sumWeightsPerSection = zeros(params.numSectionsV*params.numSectionsH,1);
for ix = 1:params.numSectionsV*params.numSectionsH
    sumWeightsPerSection(ix) = sum(weights(sectionMap == ix-1));
end*/
    sum_weights_per_section.resize( num_of_sections );
    auto p_sum = sum_weights_per_section.data();
    for( byte i = 0; i < num_of_sections; ++i, ++p_sum )
    {
        *p_sum = 0;

        auto p_section = section_map.data();
        auto p_weight = weights.data();
        for( size_t ii = 0; ii < section_map.size(); ++ii, ++p_section, ++p_weight )
        {
            if( *p_section == i )
                *p_sum += *p_weight;
        }
    }
}


double get_max(double x, double y)
{
    return x > y ? x : y;
}
double get_min(double x, double y)
{
    return x < y ? x : y;
}

std::vector<double> optimizer::calculate_weights(z_frame_data& z_data)
{
    std::vector<double> res;

    for (auto i = 0; i < z_data.supressed_edges.size(); i++)
    {
        if (z_data.supressed_edges[i])
            z_data.weights.push_back(
                get_min(get_max(z_data.supressed_edges[i] - _params.grad_z_min, (double)0),
                    _params.grad_z_max - _params.grad_z_min));
    }

    return res;
}

void deproject_sub_pixel(
    std::vector<double3>& points,
    const rs2_intrinsics_double& intrin,
    std::vector< double > const & edges,
    const double* x,
    const double* y,
    const uint16_t* depth, double depth_units
)
{
    auto ptr = (double*)points.data();
    double const * edge = edges.data();
    for (size_t i = 0; i < edges.size(); ++i)
    {
        if (!edge[i])
            continue;
        const double pixel[] = { x[i] - 1, y[i] - 1 };
        deproject_pixel_to_point(ptr, &intrin, pixel, depth[i] * depth_units);

        ptr += 3;
    }
}

std::vector<double3> optimizer::subedges2vertices(z_frame_data& z_data, const rs2_intrinsics_double& intrin, double depth_units)
{
    std::vector<double3> res(z_data.n_strong_edges);
    deproject_sub_pixel(res, intrin, z_data.supressed_edges, z_data.subpixels_x.data(), z_data.subpixels_y.data(), z_data.closest.data(), depth_units);
    z_data.vertices = res;
    return res;
}

std::vector<double> biliniar_interp(std::vector<double> const & vals, size_t width, size_t height, std::vector<double2> const & uv)
{
    std::vector<double> res(uv.size());

    for (auto i = 0; i < uv.size(); i++)
    {
        auto x = uv[i].x;
        auto x1 = floor(x);
        auto x2 = ceil(x);
        auto y = uv[i].y;
        auto y1 = floor(y);
        auto y2 = ceil(y);

        if (x1 < 0 || x1 >= width || x2 < 0 || x2 >= width ||
            y1 < 0 || y1 >= height || y2 < 0 || y2 >= height)
        {
            res[i] = std::numeric_limits<double>::max();
            continue;
        }

        // x1 y1    x2 y1
        // x1 y2    x2 y2

        // top_l    top_r
        // bot_l    bot_r

        auto top_l = vals[int(y1*width + x1)];
        auto top_r = vals[int(y1*width + x2)];
        auto bot_l = vals[int(y2*width + x1)];
        auto bot_r = vals[int(y2*width + x2)];

        double interp_x_top, interp_x_bot;
        if (x1 == x2)
        {
            interp_x_top = top_l;
            interp_x_bot = bot_l;
        }
        else
        {
            interp_x_top = ((double)(x2 - x) / (double)(x2 - x1))*(double)top_l + ((double)(x - x1) / (double)(x2 - x1))*(double)top_r;
            interp_x_bot = ((double)(x2 - x) / (double)(x2 - x1))*(double)bot_l + ((double)(x - x1) / (double)(x2 - x1))*(double)bot_r;
        }

        if (y1 == y2)
        {
            res[i] = interp_x_bot;
            continue;
        }

        auto interp_y_x = ((double)(y2 - y) / (double)(y2 - y1))*(double)interp_x_top + ((double)(y - y1) / (double)(y2 - y1))*(double)interp_x_bot;
        res[i] = interp_y_x;
    }

#if 0
    std::ofstream f;
    f.open("interp_y_x");
    f.precision(16);
    for (auto i = 0; i < res.size(); i++)
    {
        f << res[i] << std::endl;
    }
    f.close();
#endif
    return res;
}

static
std::vector< double2 > get_texture_map(
    std::vector<double3> const & points,
    calib const & curr_calib
)
{
    auto intrinsics = curr_calib.get_intrinsics();
    auto extr = curr_calib.get_extrinsics();
    auto p = curr_calib.get_p_matrix();

    std::vector<double2> uv_map( points.size() );

    for( auto i = 0; i < points.size(); ++i )
    {
        double2 uv = {};
        transform_point_to_uv( &uv.x, p, &points[i].x );

        double2 uvmap = {};
        distort_pixel( &uvmap.x, &intrinsics, &uv.x );
        uv_map[i] = uvmap;
        /* double3 p = {};
         transform_point_to_point(&p.x, extr, &points[i].x);

         double2 pixel = {};
         project_point_to_pixel(&pixel.x, &intrinsics, &p.x);
         uv[i] = pixel;*/
    }

    return uv_map;
}


template< typename T >
std::vector< double > calc_cost_per_vertex_fn(
    z_frame_data const & z_data,
    yuy2_frame_data const & yuy_data,
    std::vector< double2 > const & uv,
    T fn
)
{
    auto d_vals = biliniar_interp( yuy_data.edges_IDT, yuy_data.width, yuy_data.height, uv );

    double cost = 0;
    std::vector< double > cost_per_vertex;
    cost_per_vertex.reserve( uv.size() );

    for( size_t i = 0; i < z_data.weights.size(); i++ )
    {
        double d_val = d_vals[i];  // may be std::numeric_limits<double>::max()?
        double weight = z_data.weights[i];
        double cost = d_val * weight;
        fn( i, d_val, weight, cost );
    }

    return d_vals;
}


static double calc_cost(
    const z_frame_data & z_data,
    const yuy2_frame_data & yuy_data,
    const std::vector< double2 > & uv,
    iteration_data_collect * data = nullptr
)
{
    double cost = 0;
    size_t N = 0;
    auto d_vals = calc_cost_per_vertex_fn(z_data, yuy_data, uv,
        [&](size_t i, double d_val, double weight, double vertex_cost)
        {
            if( d_val != std::numeric_limits<double>::max() )
            {
                cost += vertex_cost;
                ++N;
            }
        } );
    if( data )
        data->d_vals = d_vals;
    return N ? cost / N : 0.;
}

static translation calc_translation_gradients( const z_frame_data & z_data, const yuy2_frame_data & yuy_data, std::vector<double> interp_IDT_x, std::vector<double> interp_IDT_y, const calib& yuy_intrin_extrin, const std::vector<double>& rc, const std::vector<double2>& xy )
{
    auto coefs = calc_translation_coefs( z_data, yuy_data, yuy_intrin_extrin, rc, xy );
    auto w = z_data.weights;

    translation sums = { 0 };
    auto sum_of_valids = 0;

    for( auto i = 0; i < coefs.x_coeffs.size(); i++ )
    {
        if( interp_IDT_x[i] == std::numeric_limits<double>::max() || interp_IDT_y[i] == std::numeric_limits<double>::max() )
            continue;

        sum_of_valids++;

        sums.t1 += w[i] * (interp_IDT_x[i] * coefs.x_coeffs[i].t1 + interp_IDT_y[i] * coefs.y_coeffs[i].t1);
        sums.t2 += w[i] * (interp_IDT_x[i] * coefs.x_coeffs[i].t2 + interp_IDT_y[i] * coefs.y_coeffs[i].t2);
        sums.t3 += w[i] * (interp_IDT_x[i] * coefs.x_coeffs[i].t3 + interp_IDT_y[i] * coefs.y_coeffs[i].t3);
    }

    translation averages{ (double)sums.t1 / (double)sum_of_valids,
        (double)sums.t2 / (double)sum_of_valids,
        (double)sums.t3 / (double)sum_of_valids };


    return averages;
}

static rotation_in_angles calc_rotation_gradients(
    const z_frame_data & z_data,
    const yuy2_frame_data & yuy_data,
    std::vector<double> interp_IDT_x,
    std::vector<double> interp_IDT_y,
    const calib & yuy_intrin_extrin,
    const std::vector<double>& rc,
    const std::vector<double2>& xy
)
{
    auto coefs = calc_rotation_coefs( z_data, yuy_data, yuy_intrin_extrin, rc, xy );
    auto w = z_data.weights;

    rotation_in_angles sums = { 0 };
    double sum_alpha = 0;
    double sum_beta = 0;
    double sum_gamma = 0;

    auto sum_of_valids = 0;

    for( auto i = 0; i < coefs.x_coeffs.size(); i++ )
    {

        if( interp_IDT_x[i] == std::numeric_limits<double>::max() || interp_IDT_y[i] == std::numeric_limits<double>::max() )
            continue;

        sum_of_valids++;

        sum_alpha += (double)w[i] * (double)((double)interp_IDT_x[i] * (double)coefs.x_coeffs[i].alpha + (double)interp_IDT_y[i] * (double)coefs.y_coeffs[i].alpha);
        sum_beta += (double)w[i] * (double)((double)interp_IDT_x[i] * (double)coefs.x_coeffs[i].beta + (double)interp_IDT_y[i] * (double)coefs.y_coeffs[i].beta);
        sum_gamma += (double)w[i] * (double)((double)interp_IDT_x[i] * (double)coefs.x_coeffs[i].gamma + (double)interp_IDT_y[i] * (double)coefs.y_coeffs[i].gamma);
    }

    rotation_in_angles averages{ sum_alpha / (double)sum_of_valids,
        sum_beta / (double)sum_of_valids,
        sum_gamma / (double)sum_of_valids };

    return averages;
}

static k_matrix calc_k_gradients( const z_frame_data & z_data, const yuy2_frame_data & yuy_data, std::vector<double> interp_IDT_x, std::vector<double> interp_IDT_y, const calib & yuy_intrin_extrin, const std::vector<double>& rc, const std::vector<double2>& xy )
{
    auto coefs = calc_k_gradients_coefs( z_data, yuy_data, yuy_intrin_extrin, rc, xy );

    auto w = z_data.weights;

    rotation_in_angles sums = { 0 };
    double sum_fx = 0;
    double sum_fy = 0;
    double sum_ppx = 0;
    double sum_ppy = 0;

    auto sum_of_valids = 0;

    for( auto i = 0; i < coefs.x_coeffs.size(); i++ )
    {

        if( interp_IDT_x[i] == std::numeric_limits<double>::max() || interp_IDT_y[i] == std::numeric_limits<double>::max() )
            continue;

        sum_of_valids++;

        sum_fx += w[i] * (interp_IDT_x[i] * coefs.x_coeffs[i].fx + interp_IDT_y[i] * coefs.y_coeffs[i].fx);
        sum_fy += w[i] * (interp_IDT_x[i] * coefs.x_coeffs[i].fy + interp_IDT_y[i] * coefs.y_coeffs[i].fy);
        sum_ppx += w[i] * (interp_IDT_x[i] * coefs.x_coeffs[i].ppx + interp_IDT_y[i] * coefs.y_coeffs[i].ppx);
        sum_ppy += w[i] * (interp_IDT_x[i] * coefs.x_coeffs[i].ppy + interp_IDT_y[i] * coefs.y_coeffs[i].ppy);

    }
    k_matrix averages{ sum_fx / (double)sum_of_valids,
        sum_fy / (double)sum_of_valids,
        sum_ppx / (double)sum_of_valids,
        sum_ppy / (double)sum_of_valids };

    return averages;
}


static
std::pair< std::vector<double2>, std::vector<double>> calc_rc(
    const z_frame_data & z_data,
    const yuy2_frame_data & yuy_data,
    const calib& curr_calib
)
{
    auto v = z_data.vertices;

    std::vector<double2> f1( z_data.vertices.size() );
    std::vector<double> r2( z_data.vertices.size() );
    std::vector<double> rc( z_data.vertices.size() );

    auto yuy_intrin = curr_calib.get_intrinsics();
    auto yuy_extrin = curr_calib.get_extrinsics();

    auto fx = (double)yuy_intrin.fx;
    auto fy = (double)yuy_intrin.fy;
    auto ppx = (double)yuy_intrin.ppx;
    auto ppy = (double)yuy_intrin.ppy;

    auto & r = yuy_extrin.rotation;
    auto & t = yuy_extrin.translation;

    double mat[3][4] = {
        fx*(double)r[0] + ppx * (double)r[2], fx*(double)r[3] + ppx * (double)r[5], fx*(double)r[6] + ppx * (double)r[8], fx*(double)t[0] + ppx * (double)t[2],
        fy*(double)r[1] + ppy * (double)r[2], fy*(double)r[4] + ppy * (double)r[5], fy*(double)r[7] + ppy * (double)r[8], fy*(double)t[1] + ppy * (double)t[2],
        r[2], r[5], r[8], t[2] };

    for( auto i = 0; i < z_data.vertices.size(); ++i )
    {
        //rs2_vertex p = {};
        //rs2_transform_point_to_point(&p.xyz[0], &yuy_extrin, &v[i].xyz[0]);
        double x = v[i].x;
        double y = v[i].y;
        double z = v[i].z;

        double x1 = (double)mat[0][0] * (double)x + (double)mat[0][1] * (double)y + (double)mat[0][2] * (double)z + (double)mat[0][3];
        double y1 = (double)mat[1][0] * (double)x + (double)mat[1][1] * (double)y + (double)mat[1][2] * (double)z + (double)mat[1][3];
        double z1 = (double)mat[2][0] * (double)x + (double)mat[2][1] * (double)y + (double)mat[2][2] * (double)z + (double)mat[2][3];

        auto x_in = x1 / z1;
        auto y_in = y1 / z1;

        auto x2 = ((x_in - ppx) / fx);
        auto y2 = ((y_in - ppy) / fy);

        f1[i].x = x2;
        f1[i].y = y2;

        auto r2 = (x2 * x2 + y2 * y2);

        rc[i] = 1 + (double)yuy_intrin.coeffs[0] * r2 + (double)yuy_intrin.coeffs[1] * r2 * r2 + (double)yuy_intrin.coeffs[4] * r2 * r2 * r2;
    }

    return { f1,rc };
}

static calib calc_gradients(
    const z_frame_data& z_data,
    const yuy2_frame_data& yuy_data,
    const std::vector<double2>& uv,
    const calib& curr_calib,
    iteration_data_collect * data = nullptr
)
{
    calib res;
    auto interp_IDT_x = biliniar_interp( yuy_data.edges_IDTx, yuy_data.width, yuy_data.height, uv );
    if( data )
        data->d_vals_x = interp_IDT_x;
#if 0
    std::ofstream f;
    f.open( "res" );
    f.precision( 16 );
    for( auto i = 0; i < interp_IDT_x.size(); i++ )
    {
        f << uv[i].x << " " << uv[i].y << std::endl;
    }
    f.close();
#endif

    auto interp_IDT_y = biliniar_interp( yuy_data.edges_IDTy, yuy_data.width, yuy_data.height, uv );
    if( data )
        data->d_vals_y = interp_IDT_y;

    auto rc = calc_rc( z_data, yuy_data, curr_calib );

    res.rot_angles = calc_rotation_gradients( z_data, yuy_data, interp_IDT_x, interp_IDT_y, curr_calib, rc.second, rc.first );
    res.trans = calc_translation_gradients( z_data, yuy_data, interp_IDT_x, interp_IDT_y, curr_calib, rc.second, rc.first );
    res.k_mat = calc_k_gradients( z_data, yuy_data, interp_IDT_x, interp_IDT_y, curr_calib, rc.second, rc.first );
    res.rot = extract_rotation_from_angles( res.rot_angles );
    return res;
}

std::pair<double, calib> calc_cost_and_grad(
    const z_frame_data & z_data,
    const yuy2_frame_data & yuy_data,
    const calib& curr_calib,
    iteration_data_collect * data = nullptr
)
{
    auto uvmap = get_texture_map(z_data.vertices, curr_calib);
    if( data )
        data->uvmap = uvmap;

    auto cost = calc_cost(z_data, yuy_data, uvmap, data);
    auto grad = calc_gradients(z_data, yuy_data, uvmap, curr_calib, data);
    return { cost, grad };
}

params::params()
{
    normelize_mat.k_mat = { 0.354172020000000, 0.265703050000000,1.001765500000000, 1.006649100000000 };
    normelize_mat.rot_angles = { 1508.94780000000, 1604.94300000000,649.384340000000 };
    normelize_mat.trans = { 0.913008390000000, 0.916982890000000, 0.433054570000000 };

    // NOTE: until we know the resolution, the current state is just the default!
    // We need to get the depth and rgb resolutions to make final decisions!
}

void params::set_depth_resolution( size_t width, size_t height )
{
    AC_LOG( DEBUG, "... depth resolution= " << width << "x" << height );
    // Some parameters are resolution-dependent
    bool const XGA = (width == 1024 && height == 768);
    if( XGA )
    {
        AC_LOG( DEBUG, "... changing IR threshold: " << grad_ir_threshold << " -> " << 1.5 << "  (because of resolution)" );
        grad_ir_threshold = 1.5;
    }
}

void params::set_rgb_resolution( size_t width, size_t height )
{
    AC_LOG( DEBUG, "... RGB resolution= " << width << "x" << height );
}

// Return the average pixel movement from the calibration
double optimizer::calc_correction_in_pixels( calib const & from_calibration ) const
{
    //%    [uvMap,~,~] = OnlineCalibration.aux.projectVToRGB(frame.vertices,params.rgbPmat,params.Krgb,params.rgbDistort);
    //% [uvMapNew,~,~] = OnlineCalibration.aux.projectVToRGB(frame.vertices,newParams.rgbPmat,newParams.Krgb,newParams.rgbDistort);
    auto old_uvmap = get_texture_map( _z.vertices, from_calibration );
    auto new_uvmap = get_texture_map( _z.vertices, _params_curr.curr_calib );
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
        calib & new_calib = _params_curr.curr_calib;
        new_calib = old_calib + (new_calib - old_calib) * mul_factor;

        //%     newParams.Rrgb = OnlineCalibration.aux.calcRmatRromAngs( newParams.xAlpha, newParams.yBeta, newParams.zGamma );
        new_calib.rot = extract_rotation_from_angles( new_calib.rot_angles );
        new_calib.calc_p_mat();
        //%     newParams.rgbPmat = newParams.Krgb*[newParams.Rrgb, newParams.Trgb];
        // -> we don't use rgbPmat
    }
}

std::vector< double > optimizer::cost_per_section( calib const & calibration )
{
    // We require here that the section_map is initialized and ready
    if( _z.section_map.size() != _z.weights.size() )
        throw std::runtime_error( "section_map has not been initialized" );

    auto uvmap = get_texture_map( _z.vertices, calibration );

    size_t const n_sections_x = _params.num_of_sections_for_edge_distribution_x;
    size_t const n_sections_y = _params.num_of_sections_for_edge_distribution_y;
    size_t const n_sections = n_sections_x * n_sections_y;

    std::vector< double > cost_per_section(n_sections, 0.);
    std::vector< size_t > N_per_section(n_sections, 0);
    calc_cost_per_vertex_fn(_z, _yuy, uvmap,
        [&](size_t i, double d_val, double weight, double vertex_cost)
    {
        if (d_val != std::numeric_limits<double>::max())
        {
            byte section = _z.section_map[i];
            cost_per_section[section] += vertex_cost;
            ++N_per_section[section];
        }
    }
    );
    for( size_t x = 0; x < n_sections; ++x )
    {
        double & cost = cost_per_section[x];
        size_t N = N_per_section[x];
        if( N )
            cost /= N;
    }
    return cost_per_section;
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
    auto cost_per_section_old = cost_per_section( _original_calibration );
    auto cost_per_section_new = cost_per_section( _params_curr.curr_calib );
    _z.cost_diff_per_section = cost_per_section_new;
    for( size_t x = 0; x < cost_per_section_old.size(); ++x )
        _z.cost_diff_per_section[x] -= cost_per_section_old[x];
    //% validOutputStruct.minImprovementPerSection = min( scoreDiffPersection );
    //% validOutputStruct.maxImprovementPerSection = max( scoreDiffPersection );
    double min_cost_diff = *std::min_element( _z.cost_diff_per_section.begin(), _z.cost_diff_per_section.end() );
    double max_cost_diff = *std::max_element( _z.cost_diff_per_section.begin(), _z.cost_diff_per_section.end() );
    AC_LOG( DEBUG, "... min cost diff= " << min_cost_diff << "  max= " << max_cost_diff );
    if( min_cost_diff < 0. )
    {
        AC_LOG( ERROR, "Some image sections were hurt by the optimization; invalidating calibration!" );
        for( size_t x = 0; x < cost_per_section_old.size(); ++x )
            AC_LOG( DEBUG, "... cost diff in section " << x << "= " << _z.cost_diff_per_section[x] );
        return false;
    }

    return true;
}

calib const & optimizer::get_calibration() const
{
    return _params_curr.curr_calib;
}

double optimizer::get_cost() const
{
    return _params_curr.cost;
}

static
void write_to_file( void const * data, size_t cb,
    std::string const & dir,
    char const * filename
)
{
    std::string path = dir + '\\' + filename;
    std::fstream f = std::fstream( path, std::ios::out | std::ios::binary );
    if( !f )
        throw std::runtime_error( "failed to open file:\n" + path );
    f.write( (char const *) data, cb );
    f.close();
}

template< typename T >
void write_obj( std::fstream & f, T const & o )
{
    f.write( (char const *)&o, sizeof( o ) );
}

template< typename T >
void write_vector_to_file( std::vector< T > const & v,
    std::string const & dir,
    char const * filename
)
{
    write_to_file( v.data(), v.size() * sizeof( T ), dir, filename );
}

void write_matlab_camera_params_file(
    rs2_intrinsics const & _intr_depth,
    calib const & rgb_calibration,
    float _depth_units,
    std::string const & dir,
    char const * filename
)
{
    std::string path = dir + '\\' + filename;
    std::fstream f = std::fstream( path, std::ios::out | std::ios::binary );
    if( !f )
        throw std::runtime_error( "failed to open file:\n" + path );


    //depth intrinsics
    write_obj( f, (double)_intr_depth.width );
    write_obj( f, (double)_intr_depth.height );
    write_obj( f, (double)_depth_units );

    double k_depth[9] = { _intr_depth.fx, 0, _intr_depth.ppx,
                        0, _intr_depth.fy, _intr_depth.ppy,
                        0, 0, 1 };
    for( auto i = 0; i < 9; i++ )
    {
        write_obj( f, k_depth[i] );
    }

    //color intrinsics
    rs2_intrinsics _intr_rgb = rgb_calibration.get_intrinsics();
    
    write_obj( f, (double)_intr_rgb.width );
    write_obj( f, (double)_intr_rgb.height );

    double k_rgb[9] = { _intr_rgb.fx, 0, _intr_rgb.ppx,
                        0, _intr_rgb.fy, _intr_rgb.ppy,
                        0, 0, 1 };


    for( auto i = 0; i < 9; i++ )
    {
        write_obj( f, k_rgb[i] );
    }

    for( auto i = 0; i < 5; i++ )
    {
        write_obj( f, (double)_intr_rgb.coeffs[i] );
    }

    //extrinsics
    rs2_extrinsics _extr = rgb_calibration.get_extrinsics();
    for( auto i = 0; i < 9; i++ )
    {
        write_obj( f, (double)_extr.rotation[i] );
    }
    //extrinsics
    for( auto i = 0; i < 3; i++ )
    {
        write_obj( f, (double)_extr.translation[i] );
    }

    auto p_mat = rgb_calibration.get_p_matrix();

    for( auto i = 0; i < 12; i++ )
    {
        write_obj( f, p_mat.vals[i] );
    }

    f.close();
}

void optimizer::write_data_to( std::string const & dir )
{
    AC_LOG( DEBUG, "... writing data to: " << dir );
    
    try
    {
        write_vector_to_file( _yuy.orig_frame, dir, "rgb.raw" );
        write_vector_to_file( _yuy.prev_frame, dir, "rgb_prev.raw" );
        write_vector_to_file( _ir.ir_frame, dir, "ir.raw" );
        write_vector_to_file( _z.frame, dir, "depth.raw" );

        write_to_file( &_original_calibration, sizeof( _original_calibration ), dir, "rgb.calib" );
        write_to_file( &_z.intrinsics, sizeof( _z.intrinsics ), dir, "depth.intrinsics" );
        write_to_file( &_z.depth_units, sizeof( _z.depth_units ), dir, "depth.units" );

        // This file is meant for matlab -- it packages all the information needed
        write_matlab_camera_params_file(
            _z.intrinsics,
            _original_calibration,
            _z.depth_units,
            dir, "camera_params.matlab"
        );
    }
    catch( std::exception const & err )
    {
        AC_LOG( ERROR, "Failed to write data: " << err.what() );
    }
    catch( ... )
    {
        AC_LOG( ERROR, "Failed to write data (unknown error)" );
    }
}

optimaization_params optimizer::back_tracking_line_search( const z_frame_data & z_data, const yuy2_frame_data& yuy_data, optimaization_params curr_params )
{
    optimaization_params new_params;

    auto orig = curr_params;
    auto grads_norm = curr_params.calib_gradients.normalize();
    auto normalized_grads = grads_norm / _params.normelize_mat;
    auto normalized_grads_norm = normalized_grads.get_norma();
    auto unit_grad = normalized_grads.normalize();

    auto t = calc_t( curr_params );
    auto step_size = calc_step_size( curr_params );

    curr_params.curr_calib.rot_angles = extract_angles_from_rotation( curr_params.curr_calib.rot.rot );
    auto movement = unit_grad * step_size;
    new_params.curr_calib = curr_params.curr_calib + movement;
    new_params.curr_calib.rot = extract_rotation_from_angles( new_params.curr_calib.rot_angles );
    new_params.curr_calib.calc_p_mat();

    auto uvmap = get_texture_map( z_data.vertices, curr_params.curr_calib );
    curr_params.cost = calc_cost( z_data, yuy_data, uvmap );

    uvmap = get_texture_map( z_data.vertices, new_params.curr_calib );
    new_params.cost = calc_cost( z_data, yuy_data, uvmap );

    auto iter_count = 0;
    while( (curr_params.cost - new_params.cost) >= step_size * t && abs( step_size ) > _params.min_step_size && iter_count++ < _params.max_back_track_iters )
    {
        AC_LOG( DEBUG, "    back tracking line search cost= " << std::fixed << std::setprecision( 15 ) << new_params.cost );
        step_size = _params.tau*step_size;

        new_params.curr_calib = curr_params.curr_calib + unit_grad * step_size;
        new_params.curr_calib.rot = extract_rotation_from_angles( new_params.curr_calib.rot_angles );
        new_params.curr_calib.calc_p_mat();

        uvmap = get_texture_map( z_data.vertices, new_params.curr_calib );
        new_params.cost = calc_cost( z_data, yuy_data, uvmap );
    }

    if( curr_params.cost - new_params.cost >= step_size * t )
    {
        new_params = orig;
    }

    new_params.curr_calib.calc_p_mat();
    new_params.curr_calib.rot = extract_rotation_from_angles( new_params.curr_calib.rot_angles );
    return new_params;
}

double optimizer::calc_step_size( optimaization_params opt_params )
{
    auto grads_norm = opt_params.calib_gradients.normalize();
    auto normalized_grads = grads_norm / _params.normelize_mat;
    auto normalized_grads_norm = normalized_grads.get_norma();
    auto unit_grad = normalized_grads.normalize();
    auto unit_grad_norm = unit_grad.get_norma();

    return _params.max_step_size*normalized_grads_norm / unit_grad_norm;
}

double optimizer::calc_t( optimaization_params opt_params )
{
    auto grads_norm = opt_params.calib_gradients.normalize();
    auto normalized_grads = grads_norm / _params.normelize_mat;
    auto normalized_grads_norm = normalized_grads.get_norma();
    auto unit_grad = normalized_grads.normalize();

    auto t_vals = normalized_grads * -_params.control_param / unit_grad;
    return t_vals.sum();
}

size_t optimizer::optimize( std::function< void( iteration_data_collect const & data ) > cb )
{
    optimaization_params params_orig;
    params_orig.curr_calib = _original_calibration;
    params_orig.curr_calib.rot_angles = extract_angles_from_rotation( params_orig.curr_calib.rot.rot );
    params_orig.curr_calib.calc_p_mat();
    auto res = calc_cost_and_grad( _z, _yuy, params_orig.curr_calib );
    params_orig.cost = res.first;
    params_orig.calib_gradients = res.second;

    _params_curr = params_orig;

    size_t n_iterations = 0;
    while( 1 )
    {
        iteration_data_collect data;
        data.iteration = n_iterations;

        auto res = calc_cost_and_grad( _z, _yuy, _params_curr.curr_calib, &data );
        _params_curr.cost = res.first;
        _params_curr.calib_gradients = res.second;
        AC_LOG( DEBUG, n_iterations << ": Cost = " << std::fixed << std::setprecision( 15 ) << _params_curr.cost );

        data.params = _params_curr;

        if( cb )
            cb( data );

        auto prev_params = _params_curr;
        _params_curr = back_tracking_line_search( _z, _yuy, _params_curr );
        auto norm = (_params_curr.curr_calib - prev_params.curr_calib).get_norma();
        if( norm < _params.min_rgb_mat_delta )
        {
            AC_LOG( DEBUG, "... {normal(new-curr)} " << norm << " < " << _params.min_rgb_mat_delta << " {min_rgb_mat_delta}  -->  stopping" );
            break;
        }

        auto delta = _params_curr.cost - prev_params.cost;
        AC_LOG( DEBUG, "    delta= " << delta );
        delta = abs( delta );
        if( delta < _params.min_cost_delta )
        {
            AC_LOG( DEBUG, "... delta < " << _params.min_cost_delta << "  -->  stopping" );
            break;
        }

        if( ++n_iterations >= _params.max_optimization_iters )
        {
            AC_LOG( DEBUG, "... exceeding max iterations  -->  stopping" );
            break;
        }
    }
    if( !n_iterations )
    {
        AC_LOG( INFO, "Calibration not necessary; nothing done" );
    }
    else
    {
        AC_LOG( INFO, "Calibration finished after " << n_iterations << " iterations; original cost= " << params_orig.cost << "  optimized cost= " << _params_curr.cost );
    }

    return n_iterations;
}
