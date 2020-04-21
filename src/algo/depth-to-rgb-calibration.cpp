//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "algo/depth-to-rgb-calibration.h"
#include "../include/librealsense2/rsutil.h"
#include <algorithm>
#include <array>

#define AC_LOG_PREFIX "AC1: "
//#define AC_LOG(TYPE,MSG) LOG_##TYPE( AC_LOG_PREFIX << MSG )
//#define AC_LOG(TYPE,MSG) LOG_ERROR( AC_LOG_PREFIX << MSG )
#define AC_LOG(TYPE,MSG) std::cout << (std::string)( to_string() << "-" << #TYPE [0] << "- " << MSG ) << std::endl
#define AC_LOG_CONTINUE(TYPE,MSG) std::cout << (std::string)( to_string() << "-" << #TYPE [0] << "- " << MSG )
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
            for (auto j = 0; j < image_width - mask_width + 1; j++)
            {
                std::vector<T> sub_image(mask_width * mask_height, 0);
                auto ind = 0;
                for (auto l = 0; l < mask_height; l++)
                {
                    for (auto k = 0; k < mask_width; k++)
                    {
                        auto p = (i + l) * image_width + j + k;
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
    std::vector<double> gauss_convolution(std::vector<T> const& image,
        size_t image_width, size_t image_height,
        size_t mask_width, size_t mask_height,
        std::function< double(std::vector<T> const& sub_image) > convolution_operation
        )
    {
        std::vector<double> res(image.size(), 0);
        std::vector<T> sub_image(mask_width * mask_height, 0);
        auto ind = 0;
        int arr[3] = { 0, 1, image_height - 1 };
        int lines[3] = { 2, 1, 2 };
        //std::allocator<double> alloc;
        //double* debug = alloc.allocate(image.size());

        // poundaries handling 
        // Extend - The nearest border pixels are conceptually extended as far as necessary to provide values for the convolution.
        // Corner pixels are extended in 90° wedges.Other edge pixels are extended in lines.
        // first 2 lines
        for (auto arr_i = 0; arr_i < 3; arr_i++) {
            for (auto jj = 0; jj < image_width - mask_width + 1; jj++)
            {
                if ((arr_i == 0) || (arr_i == 1)) {
                    ind = mask_width * lines[arr_i]; // skip first 2 lines for padding - start from 3rd line
                    for (auto l = lines[arr_i]; l < mask_height; l++)
                    {
                        for (auto k = 0; k < mask_width; k++)
                        {
                            auto p = (l - lines[arr_i] + arr[arr_i]) * image_width + jj + k;
                            sub_image[ind++] = (image[p]);
                        }
                    }
                    // fill first 2 lines to same values as 3rd line
                    ind = mask_width * lines[arr_i];
                    for (auto k = 0; k < mask_width * lines[arr_i]; k++)
                    {
                        sub_image[k] = sub_image[k % mask_width + ind];
                    }
                }
                if (arr_i == 2) { // last line
                    // mask will start from 2 previous lines and last 2 lines are padded
                    ind = 0;
                    for (auto l = 0; l < mask_height-2; l++) // only for first 3 rows
                    {
                        for (auto k = 0; k < mask_width; k++)
                        {
                            auto p = (l - lines[arr_i] + arr[arr_i]) * image_width + jj + k;
                            sub_image[ind++] = (image[p]);
                        }
                    }
                    // last 2 mask rows are the same as 3rd row
                    ind = mask_width * (lines[arr_i]+1); // 4th,5th rows padding
                    auto ind_pad = mask_width * lines[arr_i];
                    for (auto k = 0; k < mask_width * lines[arr_i]; k++)
                    {
                        sub_image[ind+k] = sub_image[k % mask_width + ind_pad];
                    }
                }
                auto mid = jj + mask_width / 2 + arr[arr_i] * image_width;
                res[mid] = convolution_operation(sub_image);
                //*(debug+mid)=res[mid];
            }
        }
        // first 2 columns
        arr[0] = 0;
        arr[1] = 1;
        arr[2] = image_height - 1 ;
        int columns[3] = { 2, 1, 0 };
        for (auto arr_i = 0; arr_i < 2; arr_i++) {
            for (auto ii = 0; ii < image_height - mask_height + 1; ii++)
            {
                ind = 0; // skip first 2 columns for padding - start from 3rd column
                for (auto l = 0; l < mask_height; l++)
                {
                    ind += columns[arr_i];
                    for (auto k = columns[arr_i]; k < mask_width; k++)
                    {
                        auto p = (ii + l) * image_width + k + arr[arr_i] - columns[arr_i];
                        sub_image[ind++] = (image[p]);
                    }
                }
                // fill first 2 columns to same values as 3rd column
                ind = columns[arr_i];
                for (auto l = 0; l < mask_height; l++)
                {
                    ind = columns[arr_i] + l * mask_width;
                    for (auto k = 1; k <= columns[arr_i]; k++)
                    {
                        sub_image[ind - k] = sub_image[ind];
                    }
                }
                auto mid = (ii + mask_height / 2) * image_width + arr[arr_i];
                res[mid] = convolution_operation(sub_image);
            }
        }
        for (auto i = 0; i < image_height - mask_height + 1; i++)
        {
            for (auto j = 0; j < image_width - mask_width + 1; j++)
            {
                ind = 0;
                for (auto l = 0; l < mask_height; l++)
                {
                    for (auto k = 0; k < mask_width; k++)
                    {
                        auto p = (i + l) * image_width + j + k;
                        sub_image[ind++] = (image[p]);
                    }

                }
                auto mid = (i + mask_height / 2) * image_width + j + mask_width / 2;
                res[mid] = convolution_operation(sub_image);
                //*(debug + mid) = res[mid];
            }
        }
        return res;
    }
    template<class T>
    std::vector<uint8_t> dilation_convolution(std::vector<T> const& image,
        size_t image_width, size_t image_height,
        size_t mask_width, size_t mask_height,
        std::function< double(std::vector<T> const& sub_image) > convolution_operation
        )
    {
        std::vector<uint8_t> res(image.size(), 0);
        std::vector<T> sub_image(mask_width * mask_height, 0);
        auto ind = 0;
        int arr[2] = { 0, image_height - 1 };

        for (auto arr_i = 0; arr_i < 2; arr_i++) {
            for (auto jj = 0; jj < image_width - mask_width + 1; jj++)
            {
                ind = 0;
                for (auto l = 0; l < mask_height; l++)
                {
                    for (auto k = 0; k < mask_width; k++)
                    {
                        auto p = (l + arr[arr_i]) * image_width + jj + k;
                        if (arr_i != 0)
                        {
                            p = p - 2*image_width;
                        }
                        sub_image[ind] = (image[p]);
                        bool cond1 = (l == 2) && (arr_i == 0); // first row
                        bool cond2 = (l == 0) && (arr_i == 1);
                        if (cond1 || cond2) {
                            sub_image[ind] = 0;
                        }
                        ind++;
                    }
                }
                auto mid = jj + mask_width / 2 + arr[arr_i] * image_width;
                res[mid] = convolution_operation(sub_image);
            }
        }

    arr[0] = 0;
    arr[1] = image_width - 1;
    for (auto arr_i = 0; arr_i < 2; arr_i++) {
        for (auto ii = 0; ii < image_height - mask_height + 1; ii++)
        {
            ind = 0;
            for (auto l = 0; l < mask_height; l++)
            {
                for (auto k = 0; k < mask_width; k++)
                {
                    auto p = (ii + l) * image_width + k+ arr[arr_i];
                    if (arr_i !=0)
                    {
                        p = p - 2;
                    }
                    sub_image[ind] = (image[p]);
                    bool cond1 = (k == 2) && (arr_i ==0);
                    bool cond2 = (k == 0) && (arr_i == 1);
                    if (cond1 || cond2) {
                        sub_image[ind] = 0;
                    }
                    ind++;
                }
                auto mid = (ii + mask_height / 2) * image_width+ arr[arr_i];
                res[mid] = convolution_operation(sub_image);
            }
        }
    }
    for (auto i = 0; i < image_height - mask_height + 1; i++)
    {
        for (auto j = 0; j < image_width - mask_width + 1; j++)
        {
            ind = 0;
            for (auto l = 0; l < mask_height; l++)
            {
                for (auto k = 0; k < mask_width; k++)
                {
                    auto p = (i + l) * image_width + j + k;
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

rotation_in_angles extract_angles_from_rotation(const double rotation[9])
{
    rotation_in_angles res;
    auto epsilon = 0.00001;
    res.alpha = atan2(-rotation[7], rotation[8]);
    res.beta = asin(rotation[6]);
    res.gamma = atan2(-rotation[3], rotation[0]);

    double rot[9];

    rot[0] = cos(res.beta)*cos(res.gamma);
    rot[1] = -cos(res.beta)*sin(res.gamma);
    rot[2] = sin(res.beta);
    rot[3] = cos(res.alpha)*sin(res.gamma) + cos(res.gamma)*sin(res.alpha)*sin(res.beta);
    rot[4] = cos(res.alpha)*cos(res.gamma) - sin(res.alpha)*sin(res.beta)*sin(res.gamma);
    rot[5] = -cos(res.beta)*sin(res.alpha);
    rot[6] = sin(res.alpha)*sin(res.gamma) - cos(res.alpha)*cos(res.gamma)*sin(res.beta);
    rot[7] = cos(res.gamma)*sin(res.alpha) + cos(res.alpha)*sin(res.beta)*sin(res.gamma);
    rot[8] = cos(res.alpha)*cos(res.beta);

    double sum = 0;
    for (auto i = 0; i < 9; i++)
    {
        sum += rot[i] - rotation[i];
    }

    if ((abs(sum)) > epsilon)
        throw "No fit";
    return res;
}

size_t optimizer::optimize( calib const & original_calibration )
{
    optimaization_params params_orig;
    params_orig.curr_calib = original_calibration;

    auto const original_cost = calc_cost_and_grad( _z, _yuy, original_calibration ).second;
    AC_LOG( DEBUG, "Original cost = " << original_cost );

    _params_curr = params_orig;
    _params_curr.curr_calib.rot_angles = extract_angles_from_rotation(_params_curr.curr_calib.rot.rot);

    size_t n_iterations = 0;
    for( auto i = 0; i < _params.max_optimization_iters; i++ )
    {
        auto res = calc_cost_and_grad( _z, _yuy, _params_curr.curr_calib );

        _params_curr.cost = res.second;
        _params_curr.calib_gradients = res.first;

        auto new_params = back_tracking_line_search( _z, _yuy, _params_curr );
        auto norm = (new_params.curr_calib - _params_curr.curr_calib).get_norma();
        if( norm < _params.min_rgb_mat_delta )
        {
            AC_LOG( DEBUG, "{normal(new-curr)} " << norm << " < " << _params.min_rgb_mat_delta << " {min_rgb_mat_delta}  -->  stopping" );
            break;
        }

        auto delta = abs( new_params.cost - _params_curr.cost );
        if( delta < _params.min_cost_delta )
        {
            AC_LOG( DEBUG, "Current cost = " << new_params.cost << "; delta= " << delta << " < " << _params.min_cost_delta << "  -->  stopping" );
            break;
        }

        _params_curr = new_params;
        ++n_iterations;
        AC_LOG( DEBUG, "Cost = " << _params_curr.cost << "; delta= " << delta );
    }
    if( !n_iterations )
    {
        AC_LOG( INFO, "Calibration not necessary; nothing done" );
    }
    else
    {
        AC_LOG( INFO, "Calibration finished; original cost= " << original_cost << "  optimized cost= " << _params_curr.cost );
    }

    return n_iterations;
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
                    fraq_step = std::numeric_limits<double>::max();

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

    _z.frame = std::move( z_data );

    _z.gradient_x = calc_vertical_gradient( _z.frame, depth_intrinsics.width, depth_intrinsics.height );
    _z.gradient_y = calc_horizontal_gradient( _z.frame, depth_intrinsics.width, depth_intrinsics.height );
    _z.edges = calc_intensity( _z.gradient_x, _z.gradient_y );
    _z.directions = get_direction( _z.gradient_x, _z.gradient_y );
    _z.direction_deg = get_direction_deg( _z.gradient_x, _z.gradient_y );
    AC_LOG( DEBUG, "... suppressing edges, IR threshold= " << _params.grad_ir_threshold << "  Z threshold= " << _params.grad_z_threshold );
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
    size_t width,
    size_t height
)
{
    _params.set_rgb_resolution( width, height );

    _yuy.yuy2_frame = get_luminance_from_yuy2( yuy_data );
    _yuy.yuy2_prev_frame = get_luminance_from_yuy2( prev_yuy_data );

    AC_LOG( DEBUG, "... calc_edges" );
    _yuy.edges = calc_edges( _yuy.yuy2_frame, width, height );
    _yuy.prev_edges = calc_edges(_yuy.yuy2_prev_frame, width, height);

    AC_LOG( DEBUG, "... blur_edges" );
    _yuy.edges_IDT = blur_edges( _yuy.edges, width, height );

    AC_LOG( DEBUG, "... calc_vertical_gradient" );
    _yuy.edges_IDTx = calc_vertical_gradient( _yuy.edges_IDT, width, height );

    AC_LOG( DEBUG, "... calc_horizontal_gradient" );
    _yuy.edges_IDTy = calc_horizontal_gradient( _yuy.edges_IDT, width, height );

    _yuy.width = width;
    _yuy.height = height;
}

void optimizer::set_ir_data(
    std::vector< ir_t > && ir_data,
    size_t width,
    size_t height
)
{
    _ir.width = width;
    _ir.height = height;

    _ir.ir_edges = calc_edges( ir_data, width, height );
}

static rotation extract_rotation_from_angles( const rotation_in_angles & rot_angles )
{
    rotation res;

    auto sin_a = sin( rot_angles.alpha );
    auto sin_b = sin( rot_angles.beta );
    auto sin_g = sin( rot_angles.gamma );

    auto cos_a = cos( rot_angles.alpha );
    auto cos_b = cos( rot_angles.beta );
    auto cos_g = cos( rot_angles.gamma );

    res.rot[0] = cos_b * cos_g;
    res.rot[3] = -cos_b * sin_g;
    res.rot[6] = sin_b;
    res.rot[1] = cos_a * sin_g + cos_g * sin_a*sin_b;
    res.rot[4] = cos_a * cos_g - sin_a * sin_b*sin_g;
    res.rot[7] = -cos_b * sin_a;
    res.rot[2] = sin_a * sin_g - cos_a * cos_g*sin_b;
    res.rot[5] = cos_g * sin_a + cos_a * sin_b*sin_g;
    res.rot[8] = cos_a * cos_b;

    return res;
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

std::vector<double> optimizer::blur_edges( std::vector<double> const & edges, size_t image_width, size_t image_height )
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

void optimizer::check_edge_distribution(
    std::vector<double>& sum_weights_per_section,
    double& min_max_ratio,
    bool& is_edge_distributed,
    double distribution_min_max_ratio,
    double min_weighted_edge_per_section_depth
)
{
    /*minMaxRatio = min(sumWeightsPerSection)/max(sumWeightsPerSection);
if minMaxRatio < params.edgeDistributMinMaxRatio
    isDistributed = false;
    fprintf('isEdgeDistributed: Ratio between min and max is too small: %0.5f, threshold is %0.5f\n',minMaxRatio, params.edgeDistributMinMaxRatio);
    return;
end*/
    double z_max = *std::max_element( sum_weights_per_section.begin(), sum_weights_per_section.end() );
    double z_min = *std::min_element( sum_weights_per_section.begin(), sum_weights_per_section.end() );
    min_max_ratio = z_min / z_max;
    if( min_max_ratio < distribution_min_max_ratio )
    {
        is_edge_distributed = false;
        AC_LOG_CONTINUE(DEBUG, "check_edge_distribution: Ratio between min and max is too small:  " << min_max_ratio);
        AC_LOG(DEBUG, "check_edge_distribution: threshold is:  " << distribution_min_max_ratio);
        return;
    }
    /*if any(sumWeightsPerSection< params.minWeightedEdgePerSection)
    isDistributed = false;
    printVals = num2str(sumWeightsPerSection(1));
    for k = 2:numel(sumWeightsPerSection)
        printVals = [printVals,',',num2str(sumWeightsPerSection(k))];
    end
    disp(['isEdgeDistributed: weighted edge per section is too low: ' printVals ', threshold is ' num2str(params.minWeightedEdgePerSection)]);
    return;
end*/
    for( auto it = sum_weights_per_section.begin(); it != sum_weights_per_section.end(); ++it )
    {
        if( *it < min_weighted_edge_per_section_depth )
        {
            is_edge_distributed = false;
            break;
        }
    }
    if (!is_edge_distributed) {
        AC_LOG_CONTINUE(DEBUG, "check_edge_distribution: weighted edge per section is too low:  ");
        for (auto it = sum_weights_per_section.begin(); it != sum_weights_per_section.end(); ++it)
        {
            AC_LOG_CONTINUE(DEBUG, " " << *it);
        }
        AC_LOG(DEBUG, "threshold is: " << min_weighted_edge_per_section_depth);
        return;
    }
    is_edge_distributed = true;
}

bool optimizer::is_edge_distributed( z_frame_data & z, yuy2_frame_data & yuy )
{
    size_t num_of_sections = _params.num_of_sections_for_edge_distribution_x * _params.num_of_sections_for_edge_distribution_y;

    // depth frame
    AC_LOG( DEBUG, "... sum_per_section(z), weights.size()= " << z.weights.size() );
    sum_per_section( z.sum_weights_per_section, z.section_map, z.weights, num_of_sections );
    AC_LOG( DEBUG, "... check_edge_distribution" );
    check_edge_distribution( z.sum_weights_per_section, z.min_max_ratio, z.is_edge_distributed, _params.edge_distribution_min_max_ratio, _params.min_weighted_edge_per_section_depth );
    // yuy frame
    AC_LOG( DEBUG, "... sum_per_section(yuy)" );
    sum_per_section( yuy.sum_weights_per_section, yuy.section_map, yuy.edges_IDT, num_of_sections );
    AC_LOG( DEBUG, "... check_edge_distribution" );
    check_edge_distribution( yuy.sum_weights_per_section, yuy.min_max_ratio, yuy.is_edge_distributed, _params.edge_distribution_min_max_ratio, _params.min_weighted_edge_per_section_depth );

    return (z.is_edge_distributed && yuy.is_edge_distributed);
}

bool optimizer::is_grad_dir_balanced(
    z_frame_data& z_data
    )
{
    /*function [isBalanced,dirRatio1,perpRatio,dirRatio2,weightsPerDir] = isGradDirBalanced(frame,params)
isBalanced = false;
dirRatio2 = nan;
perpRatio = nan;
iWeights = frame.zEdgeSupressed>0;
weightIm = frame.zEdgeSupressed;
weightIm(iWeights) = frame.weights;
weightsPerDir = [sum(weightIm(frame.dirI == 1));sum(weightIm(frame.dirI == 2));sum(weightIm(frame.dirI == 3));sum(weightIm(frame.dirI == 4))];
*/
//bool is_balanced = false;
    auto i_weights = z_data.supressed_edges; // vector of 0,1 - put 1 when supressed edges is > 0
    auto weights_im = z_data.supressed_edges;

    auto i_weights_iter = i_weights.begin();
    auto weights_im_iter = weights_im.begin();
    auto supressed_edges_iter = z_data.supressed_edges.begin();
    auto weights_iter = z_data.weights.begin();
    auto j = 0;
    for (auto i = 0; i < z_data.supressed_edges.size(); ++i)
    {
        if (*(supressed_edges_iter + i) > 0)
        {
            *(i_weights_iter + i) = 1; // is it needed ??
            *(weights_im_iter + i) = *(weights_iter + j);
            j++;
        }
    }
    std::vector<double> weights_per_dir(deg_none); // deg_non is number of directions
    auto directions_iter = z_data.directions.begin();
    auto weights_per_dir_iter = weights_per_dir.begin();
    for (auto i = 0; i < deg_none; ++i)
    {
        *(weights_per_dir_iter + i) = 0; // init sum per direction
        for (auto ii = 0; ii < z_data.directions.size(); ++ii) // directions size = z_data size = weights_im size
        {
            if (*(directions_iter + ii) == i)
            {
                *(weights_per_dir_iter + i) += *(weights_im_iter + ii);
            }
        }
    }
    z_data.sum_weights_per_direction = weights_per_dir;
    /*
    [maxVal,maxIx] = max(weightsPerDir);
    ixMatch = mod(maxIx+2,4);
    if ixMatch == 0
        ixMatch = 4;
    end
    if weightsPerDir(ixMatch) < 1e-3 %Don't devide by zero...
        dirRatio1 = 1e6;
    else
        dirRatio1 = maxVal/weightsPerDir(ixMatch);
    end
    */
    auto max_val = max_element(weights_per_dir.begin(), weights_per_dir.end());
    auto max_ix = distance(weights_per_dir.begin(), max_val);
    auto ix_match = (max_ix + 2) % 4; // NOHA :: TODO :: check value
    double dir_ratio1;
    double dir_ratio2;
    if (weights_per_dir.at(ix_match) < 1e-3) //%Don't devide by zero...
    {
        dir_ratio1 = 1e6;
    }
    else
    {
        dir_ratio1 = *max_val / weights_per_dir.at(ix_match);
    }
    /*
if dirRatio1 > params.gradDirRatio
    ixCheck = true(size(weightsPerDir));
    ixCheck([maxIx,ixMatch]) = false;
    [maxValPerp,~] = max(weightsPerDir(ixCheck));
    perpRatio = maxVal/maxValPerp;
    if perpRatio > params.gradDirRatioPerp
        fprintf('isGradDirBalanced: gradient direction is not balanced: %0.5f, threshold is %0.5f\n',dirRatio1, params.gradDirRatio );
        return;
    end
    if min(weightsPerDir(ixCheck)) < 1e-3 %Don't devide by zero...
        fprintf('isGradDirBalanced: gradient direction is not balanced: %0.5f, threshold is %0.5f\n',dirRatio1, params.gradDirRatio );
        dirRatio2 = nan;
        return;
    end
       dirRatio2 = maxValPerp/min(weightsPerDir(ixCheck));
    if dirRatio2 > params.gradDirRatio
        fprintf('isGradDirBalanced: gradient direction is not balanced: %0.5f, threshold is %0.5f\n',dirRatio1, params.gradDirRatio );
        return;
    end
end
isBalanced = true;
    */

    if (dir_ratio1 > _params.grad_dir_ratio)
    {
        std::vector<double> ix_check(deg_none); // deg_non is number of directions
        auto ix_check_iter = ix_check.begin();
        double max_val_perp = *weights_per_dir.begin();
        double min_val_perp = *weights_per_dir.begin();
        for (auto i = 0; i < deg_none; ++i)
        {
            *(ix_check_iter + i) = true;
            if ((i != max_ix) && (i != ix_match))
            {
                if (max_val_perp < *(weights_per_dir_iter + i))
                {
                    max_val_perp = *(weights_per_dir_iter + i);
                }
                if (min_val_perp > * (weights_per_dir_iter + i))
                {
                    min_val_perp = *(weights_per_dir_iter + i);
                }
            }
        }
        ix_check.at(max_ix) = false;
        ix_check.at(ix_match) = false;

        auto perp_ratio = *max_val / max_val_perp;
        if (perp_ratio > _params.grad_dir_ratio_prep)
        {
            AC_LOG_CONTINUE(DEBUG, "is_grad_dir_balanced: gradient direction is not balanced : " << dir_ratio1);
            AC_LOG(DEBUG, "threshold is: " << _params.grad_dir_ratio);
            return false;
        }
        if (min_val_perp < 1e-3)// % Don't devide by zero...
        {
            AC_LOG_CONTINUE(DEBUG, "is_grad_dir_balanced: gradient direction is not balanced : " << dir_ratio1);
            AC_LOG(DEBUG, "threshold is: " << _params.grad_dir_ratio);
            dir_ratio2 = DBL_MAX; // = nan 
            return false;
        }
        dir_ratio2 = max_val_perp / min_val_perp;
        if (dir_ratio2 > _params.grad_dir_ratio)
        {
            AC_LOG_CONTINUE(DEBUG, "is_grad_dir_balanced: gradient direction is not balanced : " << dir_ratio1);
            AC_LOG(DEBUG, "threshold is: " << _params.grad_dir_ratio);
            return false;
        }
    }
    return true;
}
void optimizer::section_per_pixel(
    frame_data const & f,
    size_t const section_w,
    size_t const section_h,
    byte * const section_map
)
{
    //% [gridX,gridY] = meshgrid(0:res(2)-1,0:res(1)-1);
    //% gridX = floor(gridX/res(2)*params.numSectionsH);
    //% gridY = floor(gridY/res(1)*params.numSectionsV);

    // res(2) is width; res(1) is height
    //    -->  section_x = x * section_w / width
    //    -->  section_y = y * section_h / height

    // We need to align the pixel-map orientation the same as the image data
    // In Matlab, it's always height-oriented (data + x*h + y) whereas our frame
    // data is width-oriented (data + y*w + x)
    //    -->  we iterate over cols within rows

    assert( section_w * section_h <= 256 );

    byte * section = section_map;
    for( size_t row = 0; row < f.height; row++ )
    {
        size_t const section_y = row * section_h / f.height;  // note not a floating point division!
        for( size_t col = 0; col < f.width; col++ )
        {
            size_t const section_x = col * section_w / f.width;  // note not a floating point division!
            //% sectionMap = gridY + gridX*params.numSectionsH;   TODO BUGBUGBUG!!!
            *section++ = byte( section_y + section_x * section_h );
        }
    }
}

//void optimizer::edge_sobel_XY(yuy2_frame_data& yuy, BYTE* pImgE)
//{
//    /*function [E,Ix,Iy] = edgeSobelXY(I)
//[Ix,Iy] = imgradientxy(I);% Sobel image gradients [-1,0,1;-2,0,2;-1,0,1]
//Ix = Ix/8;
//Iy = Iy/8;
//mask = zeros(size(Ix));
//mask(2:end-1,2:end-1) = 1;
//Ix(~mask) = 0;
//Iy(~mask) = 0;
//E = single(sqrt(Ix.^2+Iy.^2));
//end*/
//
////CKingimageDoc* pDoc = GetDocument(); // get picture
////int iBitPerPixel = pDoc->_bmp->bitsperpixel; // used to see if grayscale(8b) or RGB(24b)
////int iWidth = pDoc->_bmp->width;
////int iHeight = pDoc->_bmp->height;
////BYTE* pImg = //pDoc->_bmp->point; // pointer used to point at pixels in the image
////const int area = iWidth * iHeight;
////BYTE* pImg2 = new BYTE[area];
//
//    auto pImg = yuy.yuy2_prev_frame;
//    int iWidth = yuy.width;
//    int iHeight = yuy.height;
//    //const int area = iWidth * iHeight;
//    //BYTE* pImg2 = new BYTE[area];
//    //yuy.edge_sobel_XY.resize(area);
//    int pixel_x;
//    int pixel_y;
//
//
//    float sobel_x[3][3] =
//    { { -1, 0, 1 },
//      { -2, 0, 2 },
//      { -1, 0, 1 } };
//
//    float sobel_y[3][3] =
//    { { -1, -2, -1 },
//      { 0,  0,  0 },
//      { 1,  2,  1 } };
//
//    for (int x = 1; x < iWidth - 1; x++)
//    {
//        for (int y = 1; y < iHeight - 1; y++)
//        {
//
//            pixel_x = (sobel_x[0][0] * pImg[iWidth * (y - 1) + (x - 1)])
//                + (sobel_x[0][1] * pImg[iWidth * (y - 1) + x])
//                + (sobel_x[0][2] * pImg[iWidth * (y - 1) + (x + 1)])
//                + (sobel_x[1][0] * pImg[iWidth * y + (x - 1)])
//                + (sobel_x[1][1] * pImg[iWidth * y + x])
//                + (sobel_x[1][2] * pImg[iWidth * y + (x + 1)])
//                + (sobel_x[2][0] * pImg[iWidth * (y + 1) + (x - 1)])
//                + (sobel_x[2][1] * pImg[iWidth * (y + 1) + x])
//                + (sobel_x[2][2] * pImg[iWidth * (y + 1) + (x + 1)]);
//
//            pixel_y = (sobel_y[0][0] * pImg[iWidth * (y - 1) + (x - 1)])
//                + (sobel_y[0][1] * pImg[iWidth * (y - 1) + x])
//                + (sobel_y[0][2] * pImg[iWidth * (y - 1) + (x + 1)])
//                + (sobel_y[1][0] * pImg[iWidth * y + (x - 1)])
//                + (sobel_y[1][1] * pImg[iWidth * y + x])
//                + (sobel_y[1][2] * pImg[iWidth * y + (x + 1)])
//                + (sobel_y[2][0] * pImg[iWidth * (y + 1) + (x - 1)])
//                + (sobel_y[2][1] * pImg[iWidth * (y + 1) + x])
//                + (sobel_y[2][2] * pImg[iWidth * (y + 1) + (x + 1)]);
//
//            int val = (int)sqrt((pixel_x * pixel_x) + (pixel_y * pixel_y));
//
//            if (val < 0) val = 0;
//            if (val > 255) val = 255;
//
//            pImgE[iHeight * y + x] = val;
//            //yuy.edge_sobel_XY.push_back(val);
//          
//        }
//    }
//    auto sobel_iter = yuy.edge_sobel_XY.begin();
//    for (int x = 1; x < iWidth - 1; x++)
//    {
//        for (int y = 1; y < iHeight - 1; y++)
//        {
//            *(sobel_iter + iWidth * y + x);
//        }
//    }
//    return;
//}

template<class T>
uint8_t dilation_calc(std::vector<T> const& sub_image, std::vector<uint8_t> const& mask)
{
    uint8_t res = 0;

    for (auto i = 0; i < sub_image.size(); i++)
    {
        res = res || (uint8_t)(sub_image[i] * mask[i]);
    }

    return res;
}


void optimizer::images_dilation(yuy2_frame_data& yuy)
{
    int area = yuy.height * yuy.width;
    std::vector<uint8_t> dilation_mask = { 1, 1, 1,
                                              1,  1,  1,
                                              1,  1,  1 };

    yuy.dilated_image = dilation_convolution<uint8_t>(yuy.prev_logic_edges, yuy.width, yuy.height, _params.dilation_size, _params.dilation_size, [&](std::vector<uint8_t> const& sub_image)
        {return dilation_calc(sub_image, dilation_mask); });

}
template<class T>
double gaussian_calc(std::vector<T> const& sub_image, std::vector<double> const& mask)
{
    double res = 0;

    for (auto i = 0; i < sub_image.size(); i++)
    {
        res = res + (double)(sub_image[i] * mask[i]);
    }

    return res;
}
void calc_gaussian_kernel(std::vector<double>& gaussian_kernel, double gause_kernel_size, double sigma)
{
    /*%Design the Gaussian Kernel
      Exp_comp = -(x.^2+y.^2)/(2*sigma*sigma);
      Kernel= exp(Exp_comp)/(2*pi*sigma*sigma);
    */
    /*double sigma = 1;
int W = 5;
double kernel[W][W];
double mean = W/2;
double sum = 0.0; // For accumulating the kernel values
for (int x = 0; x < W; ++x) 
    for (int y = 0; y < W; ++y) {
        kernel[x][y] = exp( -0.5 * (pow((x-mean)/sigma, 2.0) + pow((y-mean)/sigma,2.0)) )
                         / (2 * M_PI * sigma * sigma);

        // Accumulate the kernel values
        sum += kernel[x][y];
    }

// Normalize the kernel
for (int x = 0; x < W; ++x) 
    for (int y = 0; y < W; ++y)
        kernel[x][y] /= sum;*/
    double area = gause_kernel_size * gause_kernel_size;

    std::vector<double> x = { -1, 0, 1,
                              -1, 0, 1,
                              -1, 0, 1 };
    std::vector<double> y = { -1,-1,-1,
                               0, 0, 0,
                               1, 1, 1 };
    double exp_comp;
    std::vector<double>::iterator x_iter = x.begin();
    std::vector<double>::iterator y_iter = y.begin();
    for (auto i = 0; i < area; i++, x_iter++, y_iter++)
    {
        exp_comp = (-(*x_iter) * (*y_iter) / (2 * sigma * sigma));
        gaussian_kernel.push_back(exp(exp_comp) / (2 * M_PI * sigma * sigma));
    }

    //return gauss_kernel;
}
void optimizer::gaussian_filter(yuy2_frame_data& yuy)
{
    int area = yuy.height * yuy.width;

    std::vector<double>  gaussian_kernel = { 0.002969,  0.013306,  0.021938,  0.013306,  0.002969,
       0.013306,  0.059634,  0.09832 ,  0.059634,  0.013306,
       0.021938,  0.09832 ,  0.162103,  0.09832 ,  0.021938,
       0.013306,  0.059634,  0.09832 ,  0.059634,  0.013306,
       0.002969,  0.013306,  0.021938,  0.013306,  0.002969 };
    //calc_gaussian_kernel(gaussian_kernel, _params.gause_kernel_size, _params.gauss_sigma);

    std::vector<uint8_t>::iterator yuy_iter = yuy.yuy2_frame.begin();
    std::vector<uint8_t>::iterator yuy_prev_iter = yuy.yuy2_prev_frame.begin();
    for (auto i = 0; i < area; i++, yuy_iter++, yuy_prev_iter++)
    {
        yuy.yuy_diff.push_back((double)(*yuy_prev_iter) - (double)(*yuy_iter)); // used for testing only
    }
    yuy.gaussian_filtered_image = gauss_convolution<double>(yuy.yuy_diff, yuy.width, yuy.height, _params.gause_kernel_size, _params.gause_kernel_size, [&](std::vector<double> const& sub_image)
        {return gaussian_calc(sub_image, gaussian_kernel); });
    return;
}
bool optimizer::is_movement_in_images(yuy2_frame_data& yuy)
{
    /*function [isMovement,movingPixels] = isMovementInImages(im1,im2, params)
isMovement = false;

[edgeIm1,~,~] = OnlineCalibration.aux.edgeSobelXY(uint8(im1));
logicEdges = abs(edgeIm1) > params.edgeThresh4logicIm*max(edgeIm1(:));
SE = strel('square', params.seSize);
dilatedIm = imdilate(logicEdges,SE);
*/
    yuy.prev_logic_edges = get_logic_edges(yuy.prev_edges);
    images_dilation(yuy);
    gaussian_filter(yuy);
    /*


% diffIm = abs(im1-im2);
diffIm = imgaussfilt(im1-im2,params.moveGaussSigma);
IDiffMasked = abs(diffIm);
IDiffMasked(dilatedIm) = 0;
% figure; imagesc(IDiffMasked); title('IDiffMasked');impixelinfo; colorbar;
ixMoveSuspect = IDiffMasked > params.moveThreshPixVal;
if sum(ixMoveSuspect(:)) > params.moveThreshPixNum
    isMovement = true;
end
movingPixels = sum(ixMoveSuspect(:));
disp(['isMovementInImages: # of pixels above threshold ' num2str(sum(ixMoveSuspect(:))) ', allowed #: ' num2str(params.moveThreshPixNum)]);
end*/
    
    return false;
}

bool optimizer::is_scene_valid()
{
    //return true;


    std::vector< byte > section_map_depth( _z.width * _z.height );
    std::vector< byte > section_map_rgb( _yuy.width * _yuy.height );

    size_t const section_w = _params.num_of_sections_for_edge_distribution_x;  //% params.numSectionsH
    size_t const section_h = _params.num_of_sections_for_edge_distribution_y;  //% params.numSectionsH

    // Get a map for each pixel to its corresponding section
    AC_LOG( DEBUG, "... " );
    section_per_pixel( _z, section_w, section_h, section_map_depth.data() );
    AC_LOG( DEBUG, "... " );
    section_per_pixel( _yuy, section_w, section_h, section_map_rgb.data() );

    // remove pixels in section map that were removed in weights
    AC_LOG( DEBUG, "... " << _z.supressed_edges.size() << " total edges" );
    for( auto i = 0; i < _z.supressed_edges.size(); i++ )
    {
        if( _z.supressed_edges[i] )
        {
            _z.section_map.push_back( section_map_depth[i] );
        }
        // NOHA :: TODO :: 
        // 1. throw exception when section map depth is wrong
        // 2. allocate section vector using reserve() - size = z_data.weights (keep push_back)
    }
    AC_LOG( DEBUG, "... " << _z.section_map.size() << " not suppressed" );

    // remove pixels in section map where edges_IDT > 0
    int i = 0;
    AC_LOG( DEBUG, "... " << _z.supressed_edges.size() << " total edges IDT" );
    for( auto it = _yuy.edges_IDT.begin(); it != _yuy.edges_IDT.end(); ++it, ++i )
    {
        if( *it > 0 )
        {
            _yuy.section_map.push_back( section_map_rgb[i] );
        }
    }
    AC_LOG( DEBUG, "... " << _yuy.section_map.size() << " not suppressed" );

    bool res_movement = is_movement_in_images(_yuy);
    bool res_edges = is_edge_distributed( _z, _yuy );
    bool res_gradient = is_grad_dir_balanced(_z);

    return ((!res_movement) && res_edges && res_gradient);
}

double get_max( double x, double y )
{
    return x > y ? x : y;
}
double get_min( double x, double y )
{
    return x < y ? x : y;
}

std::vector<double> optimizer::calculate_weights( z_frame_data& z_data )
{
    std::vector<double> res;

    for( auto i = 0; i < z_data.supressed_edges.size(); i++ )
    {
        if( z_data.supressed_edges[i] )
            z_data.weights.push_back(
                get_min( get_max( z_data.supressed_edges[i] - _params.grad_z_min, (double)0 ),
                    _params.grad_z_max - _params.grad_z_min ) );
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
    for( size_t i = 0; i < edges.size(); ++i )
    {
        if( !edge[i] )
            continue;
        const double pixel[] = { x[i] - 1, y[i] - 1 };
        deproject_pixel_to_point( ptr, &intrin, pixel, depth[i] * depth_units );
        
        ptr += 3;
    }
}

std::vector<double3> optimizer::subedges2vertices(z_frame_data& z_data, const rs2_intrinsics_double& intrin, double depth_units)
{
    std::vector<double3> res( z_data.n_strong_edges );
    deproject_sub_pixel( res, intrin, z_data.supressed_edges, z_data.subpixels_x.data(), z_data.subpixels_y.data(), z_data.closest.data(), depth_units );
    z_data.vertices = res;
    return res;
}

rs2_intrinsics_double calib::get_intrinsics() const
{
    return {
        width, height,
        k_mat,
        model, coeffs};
}

rs2_extrinsics_double calib::get_extrinsics() const
{
    auto & r = rot.rot;
    auto & t = trans;
    return {
        { float( r[0] ), float( r[1] ), float( r[2] ), float( r[3] ), float( r[4] ), float( r[5] ), float( r[6] ), float( r[7] ), float( r[8] ) },
        { float( t.t1 ), float( t.t2 ), float( t.t3 ) }
    };
}

static
std::vector< double2 > get_texture_map(
    /*const*/ std::vector< double3 > const & points,
    calib const & curr_calib )
{
    auto intrinsics = curr_calib.get_intrinsics();
    auto extr = curr_calib.get_extrinsics();

    std::vector<double2> uv( points.size() );

    for( auto i = 0; i < points.size(); ++i )
    {
        double3 p = {};
        transform_point_to_point( &p.x, extr, &points[i].x );

        double2 pixel = {};
        project_point_to_pixel( &pixel.x, &intrinsics, &p.x );
        uv[i] = pixel;
    }

    return uv;
}

std::vector<double> biliniar_interp( std::vector<double> const & vals, size_t width, size_t height, std::vector<double2> const & uv )
{
    std::vector<double> res( uv.size() );

    for( auto i = 0; i < uv.size(); i++ )
    {
        auto x = uv[i].x;
        auto x1 = floor( x );
        auto x2 = ceil( x );
        auto y = uv[i].y;
        auto y1 = floor( y );
        auto y2 = ceil( y );

        if( x1 < 0 || x1 >= width  || x2 < 0 || x2 >= width ||
            y1 < 0 || y1 >= height || y2 < 0 || y2 >= height )
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
        if( x1 == x2 )
        {
            interp_x_top = top_l;
            interp_x_bot = bot_l;
        }
        else
        {
            interp_x_top = ((double)(x2 - x) / (double)(x2 - x1))*(double)top_l + ((double)(x - x1) / (double)(x2 - x1))*(double)top_r;
            interp_x_bot = ((double)(x2 - x) / (double)(x2 - x1))*(double)bot_l + ((double)(x - x1) / (double)(x2 - x1))*(double)bot_r;
        }

        if( y1 == y2 )
        {
            res[i] = interp_x_bot;
            continue;
        }

        auto interp_y_x = ((double)(y2 - y) / (double)(y2 - y1))*(double)interp_x_top + ((double)(y - y1) / (double)(y2 - y1))*(double)interp_x_bot;
        res[i] = interp_y_x;
    }
    return res;
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

    auto uvmap = get_texture_map( z_data.vertices, curr_params.curr_calib );
    curr_params.cost = calc_cost( z_data, yuy_data, uvmap );

    uvmap = get_texture_map( z_data.vertices, new_params.curr_calib );
    new_params.cost = calc_cost( z_data, yuy_data, uvmap );

    auto iter_count = 0;

    while( (curr_params.cost - new_params.cost) >= step_size * t && abs( step_size ) > _params.min_step_size && iter_count++ < _params.max_back_track_iters )
    {
        step_size = _params.tau*step_size;

        new_params.curr_calib = curr_params.curr_calib + unit_grad * step_size;
        new_params.curr_calib.rot = extract_rotation_from_angles( new_params.curr_calib.rot_angles );

        uvmap = get_texture_map( z_data.vertices, new_params.curr_calib );
        new_params.cost = calc_cost( z_data, yuy_data, uvmap );

        AC_LOG( DEBUG, "Current back tracking line search cost = " << std::fixed << std::setprecision( 15 ) << new_params.cost );
    }

    if( curr_params.cost - new_params.cost >= step_size * t )
    {
        new_params = orig;
    }

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


double optimizer::calc_cost( const z_frame_data & z_data, const yuy2_frame_data& yuy_data, const std::vector<double2>& uv )
{
    auto d_vals = biliniar_interp( yuy_data.edges_IDT, yuy_data.width, yuy_data.height, uv );
    double cost = 0;

    auto sum_of_elements = 0;
    for( auto i = 0; i < z_data.weights.size(); i++ )
    {
        if( d_vals[i] != std::numeric_limits<double>::max() )
        {
            sum_of_elements++;
            cost += d_vals[i] * z_data.weights[i];
        }
    }
    if( sum_of_elements == 0 )
        return 0;
    //throw "didnt convertege";
    cost /= (double)sum_of_elements;
    return cost;
}

calib optimizer::calc_gradients( const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const std::vector<double2>& uv,
    const calib& curr_calib )
{
    calib res;
    auto interp_IDT_x = biliniar_interp( yuy_data.edges_IDTx, yuy_data.width, yuy_data.height, uv );
#if 0
    std::ofstream f;
    f.open( "res" );
    f.precision(16);
    for( auto i = 0; i < interp_IDT_x.size(); i++ )
    {
        f << uv[i].x << " " << uv[i].y << std::endl;
    }
    f.close();
#endif

    auto interp_IDT_y = biliniar_interp( yuy_data.edges_IDTy, yuy_data.width, yuy_data.height, uv );

    auto rc = calc_rc( z_data, yuy_data, curr_calib );

    res.rot_angles = calc_rotation_gradients( z_data, yuy_data, interp_IDT_x, interp_IDT_y, curr_calib, rc.second, rc.first );
    res.trans = calc_translation_gradients( z_data, yuy_data, interp_IDT_x, interp_IDT_y, curr_calib, rc.second, rc.first );
    res.k_mat = calc_k_gradients( z_data, yuy_data, interp_IDT_x, interp_IDT_y, curr_calib, rc.second, rc.first );
    res.rot = extract_rotation_from_angles( res.rot_angles );
    return res;
}

translation optimizer::calc_translation_gradients( const z_frame_data & z_data, const yuy2_frame_data & yuy_data, std::vector<double> interp_IDT_x, std::vector<double> interp_IDT_y, const calib& yuy_intrin_extrin, const std::vector<double>& rc, const std::vector<double2>& xy )
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

rotation_in_angles optimizer::calc_rotation_gradients( const z_frame_data & z_data, const yuy2_frame_data & yuy_data, std::vector<double> interp_IDT_x, std::vector<double> interp_IDT_y, const calib & yuy_intrin_extrin, const std::vector<double>& rc, const std::vector<double2>& xy )
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

k_matrix optimizer::calc_k_gradients( const z_frame_data & z_data, const yuy2_frame_data & yuy_data, std::vector<double> interp_IDT_x, std::vector<double> interp_IDT_y, const calib & yuy_intrin_extrin, const std::vector<double>& rc, const std::vector<double2>& xy )
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


std::pair< std::vector<double2>, std::vector<double>> optimizer::calc_rc( const z_frame_data & z_data, const yuy2_frame_data & yuy_data, const calib& curr_calib )
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

translation optimizer::calculate_translation_y_coeff( double3 v, double rc, double2 xy, const calib& yuy_intrin_extrin )
{
    translation res;

    auto x1 = (double)xy.x;
    auto y1 = (double)xy.y;

    auto x2 = x1 * x1;
    auto y2 = y1 * y1;
    auto xy2 = x2 + y2;
    auto x2_y2 = xy2 * xy2;

    auto r = yuy_intrin_extrin.rot.rot;
    double t[3] = { yuy_intrin_extrin.trans.t1, yuy_intrin_extrin.trans.t2, yuy_intrin_extrin.trans.t3 };
    auto d = yuy_intrin_extrin.coeffs;
    auto ppx = (double)yuy_intrin_extrin.k_mat.ppx;
    auto ppy = (double)yuy_intrin_extrin.k_mat.ppy;
    auto fx = (double)yuy_intrin_extrin.k_mat.fx;
    auto fy = (double)yuy_intrin_extrin.k_mat.fy;

    auto x = (double)v.x;
    auto y = (double)v.y;
    auto z = (double)v.z;

    auto exp1 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];
    auto exp2 = fx * (double)r[2] * x + fx * (double)r[5] * y + fx * (double)r[8] * z + fx * (double)t[2];
    auto exp3 = fx * exp1 * exp1;
    auto exp4 = fy * (2 * (double)d[2] * x1 + 2 * (double)d[3] * y1 + y1 *
        (2 * (double)d[0] * x1 + 4 * (double)d[1] * x1*xy2 + 6 * (double)d[4] * x1*x2_y2))*exp2;

    res.t1 = exp4 / exp3;

    exp1 = rc + 2 * (double)d[3] * x1 + 6 * (double)d[2] * y1 + y1 *
        (2 * (double)d[0] * y1 + 4 * (double)d[1] * y1*xy2 + 6 * (double)d[4] * y1*x2_y2);
    exp2 = -fy * (double)r[2] * x - fy * (double)r[5] * y - fy * (double)r[8] * z - fy * (double)t[2];
    exp3 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];

    res.t2 = -(exp1*exp2) / (exp3* exp3);

    exp1 = rc + 2 * (double)d[3] * x1 + 6 * (double)d[2] * y1 + y1 * (2 * (double)d[0] * y1 + 4 * (double)d[1] * y1*xy2 + 6 * (double)d[4] * y1*x2_y2);
    exp2 = fy * (double)r[1] * x + fy * (double)r[4] * y + fy * (double)r[7] * z + fy * (double)t[1];
    exp3 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];
    exp4 = 2 * (double)d[2] * x1 + 2 * (double)d[3] * y1 + y1 *
        (2 * (double)d[0] * x1 + 4 * (double)d[1] * x1*xy2 + 6 * (double)d[4] * x1*x2_y2);

    auto exp5 = fx * (double)r[0] * x + fx * (double)r[3] * y + fx * (double)r[6] * z + fx * (double)t[0];
    auto exp6 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];
    auto exp7 = fx * exp6 * exp6;

    res.t3 = -(exp1 * exp2) / (exp3 * exp3) - (fy*(exp4)*(exp5)) / exp7;

    return res;
}


translation optimizer::calculate_translation_x_coeff( double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin )
{
    translation res;

    auto x1 = (double)xy.x;
    auto y1 = (double)xy.y;

    auto x2 = x1 * x1;
    auto y2 = y1 * y1;
    auto xy2 = x2 + y2;
    auto x2_y2 = xy2 * xy2;

    auto r = yuy_intrin_extrin.rot.rot;
    double t[3] = { yuy_intrin_extrin.trans.t1, yuy_intrin_extrin.trans.t2, yuy_intrin_extrin.trans.t3 };
    auto d = yuy_intrin_extrin.coeffs;
    auto ppx = (double)yuy_intrin_extrin.k_mat.ppx;
    auto ppy = (double)yuy_intrin_extrin.k_mat.ppy;
    auto fx = (double)yuy_intrin_extrin.k_mat.fx;
    auto fy = (double)yuy_intrin_extrin.k_mat.fy;

    auto x = (double)v.x;
    auto y = (double)v.y;
    auto z = (double)v.z;

    auto exp1 = rc + 6 * (double)d[3] * x1 + 2 * (double)d[2] * y1 + x1 *
        (2 * (double)d[0] * x1 + 4 * (double)d[1] * x1*(xy2)+6 * (double)d[4] * x1*(x2_y2));
    auto exp2 = fx * (double)r[2] * x + fx * (double)r[5] * y + fx * (double)r[8] * z + fx * (double)t[2];
    auto exp3 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];

    res.t1 = (exp1 * exp2) / (exp3 * exp3);

    auto exp4 = 2 * (double)d[2] * x1 + 2 * (double)d[3] * y1 + x1 *
        (2 * (double)d[0] * y1 + 4 * (double)d[1] * y1*xy2 + 6 * (double)d[4] * y1*x2_y2);
    auto exp5 = -fy * (double)r[2] * x - fy * (double)r[5] * y - fy * (double)r[8] * z - fy * (double)t[2];
    auto exp6 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];

    res.t2 = -(fx*exp4 * exp5) / (fy*exp6 * exp6);

    exp1 = rc + 6 * (double)d[3] * x1 + 2 * (double)d[2] * y1 + x1
        * (2 * (double)d[0] * x1 + 4 * (double)d[1] * x1*(xy2)+6 * (double)d[4] * x1*x2_y2);
    exp2 = fx * (double)r[0] * x + fx * (double)r[3] * y + fx * (double)r[6] * z + fx * (double)t[0];
    exp3 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];
    exp4 = fx * (2 * (double)d[2] * x1 + 2 * (double)d[3] * y1 +
        x1 * (2 * (double)d[0] * y1 + 4 * (double)d[1] * y1*(xy2)+6 * (double)d[4] * y1*x2_y2));
    exp5 = +fy * (double)r[1] * x + fy * (double)r[4] * y + fy * (double)r[7] * z + fy * (double)t[1];
    exp6 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];

    res.t3 = -(exp1 * exp2) / (exp3 * exp3) - (exp4 * exp5) / (fy*exp6 * exp6);

    return res;

}

coeffs<rotation_in_angles> optimizer::calc_rotation_coefs( const z_frame_data & z_data, const yuy2_frame_data & yuy_data, const calib & yuy_intrin_extrin, const std::vector<double>& rc, const std::vector<double2>& xy )
{
    coeffs<rotation_in_angles> res;
    auto engles = extract_angles_from_rotation( yuy_intrin_extrin.rot.rot );
    auto v = z_data.vertices;
    res.x_coeffs.resize( v.size() );
    res.y_coeffs.resize( v.size() );

    for( auto i = 0; i < v.size(); i++ )
    {
        res.x_coeffs[i].alpha = calculate_rotation_x_alpha_coeff( engles, v[i], rc[i], xy[i], yuy_intrin_extrin );
        res.x_coeffs[i].beta = calculate_rotation_x_beta_coeff( engles, v[i], rc[i], xy[i], yuy_intrin_extrin );
        res.x_coeffs[i].gamma = calculate_rotation_x_gamma_coeff( engles, v[i], rc[i], xy[i], yuy_intrin_extrin );

        res.y_coeffs[i].alpha = calculate_rotation_y_alpha_coeff( engles, v[i], rc[i], xy[i], yuy_intrin_extrin );
        res.y_coeffs[i].beta = calculate_rotation_y_beta_coeff( engles, v[i], rc[i], xy[i], yuy_intrin_extrin );
        res.y_coeffs[i].gamma = calculate_rotation_y_gamma_coeff( engles, v[i], rc[i], xy[i], yuy_intrin_extrin );
    }

    return res;
}

coeffs<k_matrix> optimizer::calc_k_gradients_coefs( const z_frame_data & z_data, const yuy2_frame_data & yuy_data, const calib & yuy_intrin_extrin, const std::vector<double>& rc, const std::vector<double2>& xy )
{
    coeffs<k_matrix> res;
    auto v = z_data.vertices;
    res.x_coeffs.resize( v.size() );
    res.y_coeffs.resize( v.size() );

    for( auto i = 0; i < v.size(); i++ )
    {
        res.x_coeffs[i] = calculate_k_gradients_x_coeff( v[i], rc[i], xy[i], yuy_intrin_extrin );
        res.y_coeffs[i] = calculate_k_gradients_y_coeff( v[i], rc[i], xy[i], yuy_intrin_extrin );
    }

    return res;
}

k_matrix optimizer::calculate_k_gradients_y_coeff( double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin )
{
    k_matrix res;

    auto r = yuy_intrin_extrin.rot.rot;
    double t[3] = { yuy_intrin_extrin.trans.t1, yuy_intrin_extrin.trans.t2, yuy_intrin_extrin.trans.t3 };
    auto d = yuy_intrin_extrin.coeffs;
    auto ppx = (double)yuy_intrin_extrin.k_mat.ppx;
    auto ppy = (double)yuy_intrin_extrin.k_mat.ppy;
    auto fx = (double)yuy_intrin_extrin.k_mat.fx;
    auto fy = (double)yuy_intrin_extrin.k_mat.fy;

    auto x1 = (double)xy.x;
    auto y1 = (double)xy.y;
    auto x = (double)v.x;
    auto y = (double)v.y;
    auto z = (double)v.z;

    auto x2 = x1 * x1;
    auto y2 = y1 * y1;
    auto xy2 = x2 + y2;
    auto x2_y2 = xy2 * xy2;

    res.fx = (fy*(2 * d[2] * x1 + 2 * d[3] * y1 + y1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))
        *(r[0] * x + r[3] * y + r[6] * z + t[0]))
        / (fx*(x*r[2] + y * r[5] + z * r[8] + t[2]));

    res.ppx = (fy*(2 * d[2] * x1 + 2 * d[3] * y1 + y1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))
        *(r[2] * x + r[5] * y + r[8] * z + t[2]))
        / (fx*(x*r[2] + y * r[5] + z * (r[8]) + (t[2])));

    res.fy = ((r[1] * x + r[4] * y + r[7] * z + t[1])*
        (rc + 2 * d[3] * x1 + 6 * d[2] * y1 + y1 * (2 * d[0] * y1 + 4 * d[1] * y1*xy2 + 6 * d[4] * y1*x2_y2)))
        / (x*r[2] + y * r[5] + z * r[8] + t[2]);

    res.ppy = ((r[2] * x + r[5] * y + r[8] * z + t[2])*
        (rc + 2 * d[3] * x1 + 6 * d[2] * y1 + y1 * (2 * d[0] * y1 + 4 * d[1] * y1*xy2 + 6 * d[4] * y1*x2_y2)))
        / (x*r[2] + y * r[5] + z * r[8] + t[2]);

    return res;
}

k_matrix optimizer::calculate_k_gradients_x_coeff( double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin )
{
    k_matrix res;

    auto r = yuy_intrin_extrin.rot.rot;
    double t[3] = { yuy_intrin_extrin.trans.t1, yuy_intrin_extrin.trans.t2, yuy_intrin_extrin.trans.t3 };
    auto d = yuy_intrin_extrin.coeffs;
    auto ppx = (double)yuy_intrin_extrin.k_mat.ppx;
    auto ppy = (double)yuy_intrin_extrin.k_mat.ppy;
    auto fx = (double)yuy_intrin_extrin.k_mat.fx;
    auto fy = (double)yuy_intrin_extrin.k_mat.fy;

    auto x1 = (double)xy.x;
    auto y1 = (double)xy.y;
    auto x = (double)v.x;
    auto y = (double)v.y;
    auto z = (double)v.z;

    auto x2 = x1 * x1;
    auto y2 = y1 * y1;
    auto xy2 = x2 + y2;
    auto x2_y2 = xy2 * xy2;

    res.fx = ((r[0] * x + r[3] * y + r[6] * z + t[0])
        *(rc + 6 * d[3] * x1 + 2 * d[2] * y1 + x1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2)))
        / (x*r[2] + y * r[5] + z * r[8] + t[2]);


    res.ppx = ((r[2] * x + r[5] * y + r[8] * z + t[2] * 1)
        *(rc + 6 * d[3] * x1 + 2 * d[2] * y1 + x1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))
        ) / (x*r[2] + y * (r[5]) + z * (r[8]) + (t[2]));


    res.fy = (fx*(2 * d[2] * x1 + 2 * d[3] * y1 + x1 * (2 * d[0] * y1 + 4 * d[1] * y1*(xy2)+6 * d[4] * y1*x2_y2))*
        (r[1] * x + r[4] * y + r[7] * z + t[1] * 1))
        / (fy*(x*(r[2]) + y * (r[5]) + z * (r[8]) + (t[2])));

    res.ppy = (fx*(2 * d[2] * x1 + 2 * d[3] * y1 + x1 * (2 * d[0] * y1 + 4 * d[1] * y1*(xy2)+6 * d[4] * y1*x2_y2))*(r[2] * x + r[5] * y + r[8] * z + t[2] * 1))
        / (fy*(x*(r[2]) + y * (r[5]) + z * (r[8]) + (t[2])));

    return res;
}

double optimizer::calculate_rotation_x_alpha_coeff( rotation_in_angles rot_angles, double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin )
{
    auto r = yuy_intrin_extrin.rot.rot;
    double t[3] = { yuy_intrin_extrin.trans.t1, yuy_intrin_extrin.trans.t2, yuy_intrin_extrin.trans.t3 };
    auto d = yuy_intrin_extrin.coeffs;
    auto ppx = (double)yuy_intrin_extrin.k_mat.ppx;
    auto ppy = (double)yuy_intrin_extrin.k_mat.ppy;
    auto fx = (double)yuy_intrin_extrin.k_mat.fx;
    auto fy = (double)yuy_intrin_extrin.k_mat.fy;

    auto sin_a = (double)sin( rot_angles.alpha );
    auto sin_b = (double)sin( rot_angles.beta );
    auto sin_g = (double)sin( rot_angles.gamma );

    auto cos_a = (double)cos( rot_angles.alpha );
    auto cos_b = (double)cos( rot_angles.beta );
    auto cos_g = (double)cos( rot_angles.gamma );
    auto x1 = (double)xy.x;
    auto y1 = (double)xy.y;

    auto x2 = x1 * x1;
    auto y2 = y1 * y1;
    auto xy2 = x2 + y2;
    auto x2_y2 = xy2 * xy2;

    auto x = (double)v.x;
    auto y = (double)v.y;
    auto z = (double)v.z;


    auto exp1 = z * (0 * sin_b + 1 * cos_a*cos_b - 0 * cos_b*sin_a)
        + x * (0 * (cos_a*sin_g + cos_g * sin_a*sin_b)
            + 1 * (sin_a*sin_g - cos_a * cos_g*sin_b)
            + 0 * cos_b*cos_g)
        + y * (0 * (cos_a*cos_g - sin_a * sin_b*sin_g)
            + 1 * (cos_g*sin_a + cos_a * sin_b*sin_g)
            - 0 * cos_b*sin_g)
        + 1 * (0 * t[0] + 0 * t[1] + 1 * t[2]);

    auto res = (((x*(0 * (sin_a*sin_g - cos_a * cos_g*sin_b)
        - 1 * (cos_a*sin_g + cos_g * sin_a*sin_b)
        ) + y * (0 * (cos_g*sin_a + cos_a * sin_b*sin_g)
            - 1 * (cos_a*cos_g - sin_a * sin_b*sin_g)
            ) + z * (0 * cos_a*cos_b + 1 * cos_b*sin_a)
        )*(z*(fx*sin_b + ppx * cos_a*cos_b - 0 * cos_b*sin_a)
            + x * (0 * (cos_a*sin_g + cos_g * sin_a*sin_b)
                + ppx * (sin_a*sin_g - cos_a * cos_g*sin_b)
                + fx * cos_b*cos_g)
            + y * (0 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                + ppx * (cos_g*sin_a + cos_a * sin_b*sin_g)
                - fx * cos_b*sin_g)
            + 1 * (fx*t[0] + 0 * t[1] + ppx * t[2])
            ) - (x*(0 * (sin_a*sin_g - cos_a * cos_g*sin_b)
                - ppx * (cos_a*sin_g + cos_g * sin_a*sin_b)
                ) + y * (0 * (cos_g*sin_a + cos_a * sin_b*sin_g)
                    - ppx * (cos_a*cos_g - sin_a * sin_b*sin_g)
                    ) + z * (0 * cos_a*cos_b + ppx * cos_b*sin_a)
                )*(z*(0 * sin_b + 1 * cos_a*cos_b - 0 * cos_b*sin_a)
                    + x * (0 * (cos_a*sin_g + cos_g * sin_a*sin_b)
                        + 1 * (sin_a*sin_g - cos_a * cos_g*sin_b)
                        + 0 * cos_b*cos_g)
                    + y * (0 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                        + 1 * (cos_g*sin_a + cos_a * sin_b*sin_g)
                        - 0 * cos_b*sin_g)
                    + 1 * (0 * t[0] + 0 * t[1] + 1 * t[2])
                    ))
        *(rc + 6 * d[3] * x1 + 2 * d[2] * y1 + x1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))
        ) / (exp1*exp1) + (fx*((x*(0 * (sin_a*sin_g - cos_a * cos_g*sin_b)
            - 1 * (cos_a*sin_g + cos_g * sin_a*sin_b)
            ) + y * (0 * (cos_g*sin_a + cos_a * sin_b*sin_g)
                - 1 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                ) + z * (0 * cos_a*cos_b + 1 * cos_b*sin_a)
            )*(z*(0 * sin_b + ppy * cos_a*cos_b - fy * cos_b*sin_a)
                + x * (fy*(cos_a*sin_g + cos_g * sin_a*sin_b)
                    + ppy * (sin_a*sin_g - cos_a * cos_g*sin_b)
                    + 0 * cos_b*cos_g)
                + y * (fy*(cos_a*cos_g - sin_a * sin_b*sin_g)
                    + ppy * (cos_g*sin_a + cos_a * sin_b*sin_g)
                    - 0 * cos_b*sin_g)
                + 1 * (0 * t[0] + fy * t[1] + ppy * t[2])
                ) - (x*(fy*(sin_a*sin_g - cos_a * cos_g*sin_b)
                    - ppy * (cos_a*sin_g + cos_g * sin_a*sin_b)
                    ) + y * (fy*(cos_g*sin_a + cos_a * sin_b*sin_g)
                        - ppy * (cos_a*cos_g - sin_a * sin_b*sin_g)
                        ) + z * (fy*cos_a*cos_b + ppy * cos_b*sin_a)
                    )*(z*(0 * sin_b + 1 * cos_a*cos_b - 0 * cos_b*sin_a)
                        + x * (0 * (cos_a*sin_g + cos_g * sin_a*sin_b)
                            + 1 * (sin_a*sin_g - cos_a * cos_g*sin_b)
                            + 0 * cos_b*cos_g)
                        + y * (0 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                            + 1 * (cos_g*sin_a + cos_a * sin_b*sin_g)
                            - 0 * cos_b*sin_g)
                        + 1 * (0 * t[0] + 0 * t[1] + 1 * t[2])
                        ))
            *(2 * d[2] * x1 + 2 * d[3] * y1 + x1 * (2 * d[0] * y1 + 4 * d[1] * y1*(xy2)+6 * d[4] * y1*x2_y2))
            ) / (fy*(exp1*exp1));
    return res;
}

double optimizer::calculate_rotation_x_beta_coeff( rotation_in_angles rot_angles, double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin )
{
    auto r = yuy_intrin_extrin.rot.rot;
    double t[3] = { yuy_intrin_extrin.trans.t1, yuy_intrin_extrin.trans.t2, yuy_intrin_extrin.trans.t3 };
    auto d = yuy_intrin_extrin.coeffs;
    auto ppx = (double)yuy_intrin_extrin.k_mat.ppx;
    auto ppy = (double)yuy_intrin_extrin.k_mat.ppy;
    auto fx = (double)yuy_intrin_extrin.k_mat.fx;
    auto fy = (double)yuy_intrin_extrin.k_mat.fy;

    auto sin_a = sin( rot_angles.alpha );
    auto sin_b = sin( rot_angles.beta );
    auto sin_g = sin( rot_angles.gamma );

    auto cos_a = cos( rot_angles.alpha );
    auto cos_b = cos( rot_angles.beta );
    auto cos_g = cos( rot_angles.gamma );
    auto x1 = (double)xy.x;
    auto y1 = (double)xy.y;

    auto x2 = x1 * x1;
    auto y2 = y1 * y1;
    auto xy2 = x2 + y2;
    auto x2_y2 = xy2 * xy2;

    auto x = (double)v.x;
    auto y = (double)v.y;
    auto z = (double)v.z;

    auto exp1 = z * (cos_a*cos_b) +
        x * ((sin_a*sin_g - cos_a * cos_g*sin_b))
        + y * ((cos_g*sin_a + cos_a * sin_b*sin_g))
        + (t[2]);

    auto res = -(((z*(0 * cos_b - 1 * cos_a*sin_b + 0 * sin_a*sin_b)
        - x * (0 * cos_g*sin_b + 1 * cos_a*cos_b*cos_g - 0 * cos_b*cos_g*sin_a)
        + y * (0 * sin_b*sin_g + 1 * cos_a*cos_b*sin_g - 0 * cos_b*sin_a*sin_g)
        )*(z*(fx*sin_b + ppx * cos_a*cos_b - 0 * cos_b*sin_a)
            + x * (0 * (cos_a*sin_g + cos_g * sin_a*sin_b)
                + ppx * (sin_a*sin_g - cos_a * cos_g*sin_b)
                + fx * cos_b*cos_g)
            + y * (0 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                + ppx * (cos_g*sin_a + cos_a * sin_b*sin_g)
                - fx * cos_b*sin_g)
            + 1 * (fx*t[0] + 0 * t[1] + ppx * t[2])
            ) - (z*(fx*cos_b - ppx * cos_a*sin_b + 0 * sin_a*sin_b)
                - x * (fx*cos_g*sin_b + ppx * cos_a*cos_b*cos_g - 0 * cos_b*cos_g*sin_a)
                + y * (fx*sin_b*sin_g + ppx * cos_a*cos_b*sin_g - 0 * cos_b*sin_a*sin_g)
                )*(z*(0 * sin_b + 1 * cos_a*cos_b - 0 * cos_b*sin_a)
                    + x * (0 * (cos_a*sin_g + cos_g * sin_a*sin_b)
                        + 1 * (sin_a*sin_g - cos_a * cos_g*sin_b)
                        + 0 * cos_b*cos_g)
                    + y * (0 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                        + 1 * (cos_g*sin_a + cos_a * sin_b*sin_g)
                        - 0 * cos_b*sin_g)
                    + 1 * (0 * t[0] + 0 * t[1] + 1 * t[2])))
        *(rc + 6 * d[3] * x1 + 2 * d[2] * y1 + x1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))
        ) / (exp1* exp1) - (fx*((z*(0 * cos_b - 1 * cos_a*sin_b + 0 * sin_a*sin_b)
            - x * (0 * cos_g*sin_b + 1 * cos_a*cos_b*cos_g - 0 * cos_b*cos_g*sin_a)
            + y * (0 * sin_b*sin_g + 1 * cos_a*cos_b*sin_g - 0 * cos_b*sin_a*sin_g)
            )*(z*(0 * sin_b + ppy * cos_a*cos_b - fy * cos_b*sin_a)
                + x * (fy*(cos_a*sin_g + cos_g * sin_a*sin_b)
                    + ppy * (sin_a*sin_g - cos_a * cos_g*sin_b)
                    + 0 * cos_b*cos_g)
                + y * (fy*(cos_a*cos_g - sin_a * sin_b*sin_g)
                    + ppy * (cos_g*sin_a + cos_a * sin_b*sin_g)
                    - 0 * cos_b*sin_g)
                + 1 * (0 * t[0] + fy * t[1] + ppy * t[2])
                ) - (z*(0 * cos_b - ppy * cos_a*sin_b + fy * sin_a*sin_b)
                    - x * (0 * cos_g*sin_b + ppy * cos_a*cos_b*cos_g - fy * cos_b*cos_g*sin_a)
                    + y * (0 * sin_b*sin_g + ppy * cos_a*cos_b*sin_g - fy * cos_b*sin_a*sin_g)
                    )*(z*(0 * sin_b + 1 * cos_a*cos_b - 0 * cos_b*sin_a)
                        + x * (0 * (cos_a*sin_g + cos_g * sin_a*sin_b)
                            + 1 * (sin_a*sin_g - cos_a * cos_g*sin_b)
                            + 0 * cos_b*cos_g)
                        + y * (0 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                            + 1 * (cos_g*sin_a + cos_a * sin_b*sin_g)
                            - 0 * cos_b*sin_g)
                        + 1 * (0 * t[0] + 0 * t[1] + 1 * t[2])
                        ))*(2 * d[2] * x1 + 2 * d[3] * y1 + x1 * (2 * d[0] * y1 + 4 * d[1] * y1*(xy2)+6 * d[4] * y1*x2_y2))
            ) / (fy*(exp1*exp1));

    return res;
}

double optimizer::calculate_rotation_x_gamma_coeff( rotation_in_angles rot_angles, double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin )
{
    auto r = yuy_intrin_extrin.rot.rot;
    double t[3] = { yuy_intrin_extrin.trans.t1, yuy_intrin_extrin.trans.t2, yuy_intrin_extrin.trans.t3 };
    auto d = yuy_intrin_extrin.coeffs;
    auto ppx = (double)yuy_intrin_extrin.k_mat.ppx;
    auto ppy = (double)yuy_intrin_extrin.k_mat.ppy;
    auto fx = (double)yuy_intrin_extrin.k_mat.fx;
    auto fy = (double)yuy_intrin_extrin.k_mat.fy;

    auto sin_a = (double)sin( rot_angles.alpha );
    auto sin_b = (double)sin( rot_angles.beta );
    auto sin_g = (double)sin( rot_angles.gamma );

    auto cos_a = (double)cos( rot_angles.alpha );
    auto cos_b = (double)cos( rot_angles.beta );
    auto cos_g = (double)cos( rot_angles.gamma );
    auto x1 = (double)xy.x;
    auto y1 = (double)xy.y;

    auto x2 = x1 * x1;
    auto y2 = y1 * y1;
    auto xy2 = x2 + y2;
    auto x2_y2 = xy2 * xy2;

    auto x = (double)v.x;
    auto y = (double)v.y;
    auto z = (double)v.z;

    auto exp1 = z * cos_a*cos_b +
        x * (sin_a*sin_g - cos_a * cos_g*sin_b) +
        y * (cos_g*sin_a + cos_a * sin_b*sin_g) +
        t[2];

    auto res = (
        ((y*(sin_a*sin_g - cos_a * cos_g*sin_b) - x * (cos_g*sin_a + cos_a * sin_b*sin_g))*
        (z*(fx*sin_b + ppx * cos_a*cos_b) +
            x * (ppx*(sin_a*sin_g - cos_a * cos_g*sin_b) + fx * cos_b*cos_g) +
            y * (ppx*(cos_g*sin_a + cos_a * sin_b*sin_g) - fx * cos_b*sin_g) +
            (fx*t[0] + ppx * t[2])) -
            (y*(ppx* (sin_a*sin_g - cos_a * cos_g*sin_b) + fx * cos_b*cos_g) -
                x * (ppx*(cos_g*sin_a + cos_a * sin_b*sin_g) - fx * cos_b*sin_g))*
                (z*(cos_a*cos_b) + x * (sin_a*sin_g - cos_a * cos_g*sin_b) +
                    y * (cos_g*sin_a + cos_a * sin_b*sin_g) + t[2]))*
                    (rc + 6 * d[3] * x1 + 2 * d[2] * y1 + x1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))
        )
        /
        (exp1* exp1) + (fx*((y*(sin_a*sin_g - cos_a * cos_g*sin_b) -
            x * (cos_g*sin_a + cos_a * sin_b*sin_g))*
            (z*(ppy*cos_a*cos_b - fy * cos_b*sin_a) +
                x * (fy*(cos_a*sin_g + cos_g * sin_a*sin_b) + ppy * (sin_a*sin_g - cos_a * cos_g*sin_b)) + y *
                (fy*(cos_a*cos_g - sin_a * sin_b*sin_g) + ppy *
                (cos_g*sin_a + cos_a * sin_b*sin_g)) +
                    (fy * t[1] + ppy * t[2])) -
                (y*(fy*(cos_a*sin_g + cos_g * sin_a*sin_b) +
                    ppy * (sin_a*sin_g - cos_a * cos_g*sin_b)) - x *
                    (fy*(cos_a*cos_g - sin_a * sin_b*sin_g) + ppy * (cos_g*sin_a + cos_a * sin_b*sin_g)))*
                    (z*cos_a*cos_b + x * ((sin_a*sin_g - cos_a * cos_g*sin_b)) +
                        y * ((cos_g*sin_a + cos_a * sin_b*sin_g)) + t[2]))*(2 * d[2] * x1 + 2 * d[3] * y1 + x1 *
                        (2 * d[0] * y1 + 4 * d[1] * y1*xy2 + 6 * d[4] * y1*x2_y2)) / (fy*exp1*exp1));

    return res;
}

double optimizer::calculate_rotation_y_alpha_coeff( rotation_in_angles rot_angles, double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin )
{
    auto r = yuy_intrin_extrin.rot.rot;
    double t[3] = { yuy_intrin_extrin.trans.t1, yuy_intrin_extrin.trans.t2, yuy_intrin_extrin.trans.t3 };
    auto d = yuy_intrin_extrin.coeffs;
    auto ppx = (double)yuy_intrin_extrin.k_mat.ppx;
    auto ppy = (double)yuy_intrin_extrin.k_mat.ppy;
    auto fx = (double)yuy_intrin_extrin.k_mat.fx;
    auto fy = (double)yuy_intrin_extrin.k_mat.fy;

    auto sin_a = (double)sin( rot_angles.alpha );
    auto sin_b = (double)sin( rot_angles.beta );
    auto sin_g = (double)sin( rot_angles.gamma );

    auto cos_a = (double)cos( rot_angles.alpha );
    auto cos_b = (double)cos( rot_angles.beta );
    auto cos_g = (double)cos( rot_angles.gamma );
    auto x1 = (double)xy.x;
    auto y1 = (double)xy.y;

    /* x1 = 1;
        y1 = 1;*/

    auto x2 = x1 * x1;
    auto y2 = y1 * y1;
    auto xy2 = x2 + y2;
    auto x2_y2 = xy2 * xy2;

    auto x = (double)v.x;
    auto y = (double)v.y;
    auto z = (double)v.z;


    auto exp1 = z * (cos_a*cos_b) + x * ((sin_a*sin_g - cos_a * cos_g*sin_b)) +
        y * ((cos_g*sin_a + cos_a * sin_b*sin_g)) + t[2];

    auto res = (((x*(-(cos_a*sin_g + cos_g * sin_a*sin_b)) + y * (-1 * (cos_a*cos_g - sin_a * sin_b*sin_g)) + z *
        (cos_b*sin_a))*(z*(ppy * cos_a*cos_b - fy * cos_b*sin_a) + x * (fy*(cos_a*sin_g + cos_g * sin_a*sin_b) +
            ppy * (sin_a*sin_g - cos_a * cos_g*sin_b)) + y * (fy*(cos_a*cos_g - sin_a * sin_b*sin_g) +
                ppy * (cos_g*sin_a + cos_a * sin_b*sin_g)) + (fy * t[1] + ppy * t[2])) - (x*(fy*(sin_a*sin_g - cos_a * cos_g*sin_b) -
                    ppy * (cos_a*sin_g + cos_g * sin_a*sin_b)) + y * (fy*(cos_g*sin_a + cos_a * sin_b*sin_g) -
                        ppy * (cos_a*cos_g - sin_a * sin_b*sin_g)) + z * (fy*cos_a*cos_b + ppy * cos_b*sin_a))*
                        (z*(cos_a*cos_b) + x * ((sin_a*sin_g - cos_a * cos_g*sin_b)) + y * ((cos_g*sin_a + cos_a * sin_b*sin_g) - 0 * cos_b*sin_g) + (t[2])))*
        (rc + 2 * d[3] * x1 + 6 * d[2] * y1 + y1 * (2 * d[0] * y1 + 4 * d[1] * y1*(xy2)+6 * d[4] * y1*x2_y2))) /
        (exp1*exp1) + (fy*((x*(-(cos_a*sin_g + cos_g * sin_a*sin_b)) + y * (-(cos_a*cos_g - sin_a * sin_b*sin_g)) +
            z * (cos_b*sin_a))*(z*(fx*sin_b + ppx * cos_a*cos_b) + x * (ppx*(sin_a*sin_g - cos_a * cos_g*sin_b) + fx * cos_b*cos_g) + y *
            (ppx*(cos_g*sin_a + cos_a * sin_b*sin_g) - fx * cos_b*sin_g) + (fx*t[0] + ppx * t[2])) - (x*(-ppx * (cos_a*sin_g + cos_g * sin_a*sin_b)) +
                y * (-ppx * (cos_a*cos_g - sin_a * sin_b*sin_g)) + z * (ppx*cos_b*sin_a))*(z*(cos_a*cos_b - 0 * cos_b*sin_a) + x * ((sin_a*sin_g - cos_a * cos_g*sin_b)) +
                    y * ((cos_g*sin_a + cos_a * sin_b*sin_g)) + (t[2])))*(2 * d[2] * x1 + 2 * d[3] * y1 + y1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))) / (fx*(exp1*exp1));

    return res;
}

double optimizer::calculate_rotation_y_beta_coeff( rotation_in_angles rot_angles, double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin )
{
    auto r = yuy_intrin_extrin.rot.rot;
    double t[3] = { yuy_intrin_extrin.trans.t1, yuy_intrin_extrin.trans.t2, yuy_intrin_extrin.trans.t3 };
    auto d = yuy_intrin_extrin.coeffs;
    auto ppx = (double)yuy_intrin_extrin.k_mat.ppx;
    auto ppy = (double)yuy_intrin_extrin.k_mat.ppy;
    auto fx = (double)yuy_intrin_extrin.k_mat.fx;
    auto fy = (double)yuy_intrin_extrin.k_mat.fy;

    auto sin_a = (double)sin( rot_angles.alpha );
    auto sin_b = (double)sin( rot_angles.beta );
    auto sin_g = (double)sin( rot_angles.gamma );

    auto cos_a = (double)cos( rot_angles.alpha );
    auto cos_b = (double)cos( rot_angles.beta );
    auto cos_g = (double)cos( rot_angles.gamma );
    auto x1 = (double)xy.x;
    auto y1 = (double)xy.y;

    auto x2 = x1 * x1;
    auto y2 = y1 * y1;
    auto xy2 = x2 + y2;
    auto x2_y2 = xy2 * xy2;

    auto x = (double)v.x;
    auto y = (double)v.y;
    auto z = (double)v.z;

    auto exp1 = z * (cos_a*cos_b) + x * ((sin_a*sin_g - cos_a * cos_g*sin_b))
        + y * ((cos_g*sin_a + cos_a * sin_b*sin_g)) + (t[2]);

    auto res = -(((z*(-cos_a * sin_b) - x * (cos_a*cos_b*cos_g)
        + y * (cos_a*cos_b*sin_g))*(z*(ppy * cos_a*cos_b - fy * cos_b*sin_a)
            + x * (fy*(cos_a*sin_g + cos_g * sin_a*sin_b) + ppy * (sin_a*sin_g - cos_a * cos_g*sin_b))
            + y * (fy*(cos_a*cos_g - sin_a * sin_b*sin_g) + ppy * (cos_g*sin_a + cos_a * sin_b*sin_g))
            + (fy * t[1] + ppy * t[2])) - (z*(0 * cos_b - ppy * cos_a*sin_b + fy * sin_a*sin_b)
                - x * (ppy * cos_a*cos_b*cos_g - fy * cos_b*cos_g*sin_a) + y * (ppy * cos_a*cos_b*sin_g - fy * cos_b*sin_a*sin_g))*
                (z*(cos_a*cos_b) + x * ((sin_a*sin_g - cos_a * cos_g*sin_b)) + y * ((cos_g*sin_a + cos_a * sin_b*sin_g)) + t[2]))
        *(rc + 2 * d[3] * x1 + 6 * d[2] * y1 + y1 * (2 * d[0] * y1 + 4 * d[1] * y1*(xy2)+6 * d[4] * y1*x2_y2))) /
        (exp1*exp1) - (fy*((z*(-cos_a * sin_b) - x * (cos_a*cos_b*cos_g) + y * (cos_a*cos_b*sin_g))*(z*(fx*sin_b + ppx * cos_a*cos_b)
            + x * (ppx * (sin_a*sin_g - cos_a * cos_g*sin_b) + fx * cos_b*cos_g) + y * (+ppx * (cos_g*sin_a + cos_a * sin_b*sin_g) - fx * cos_b*sin_g)
            + (fx*t[0] + ppx * t[2])) - (z*(fx*cos_b - ppx * cos_a*sin_b) - x * (fx*cos_g*sin_b + ppx * cos_a*cos_b*cos_g) + y
                * (fx*sin_b*sin_g + ppx * cos_a*cos_b*sin_g))*(z*(cos_a*cos_b) + x * (sin_a*sin_g - cos_a * cos_g*sin_b) + y
                    * (cos_g*sin_a + cos_a * sin_b*sin_g) + t[2]))*(2 * d[2] * x1 + 2 * d[3] * y1 + y1 *
                    (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))) / (fx*(exp1*exp1));

    return res;

}

double optimizer::calculate_rotation_y_gamma_coeff( rotation_in_angles rot_angles, double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin )
{
    auto r = yuy_intrin_extrin.rot.rot;
    double t[3] = { yuy_intrin_extrin.trans.t1, yuy_intrin_extrin.trans.t2, yuy_intrin_extrin.trans.t3 };
    auto d = yuy_intrin_extrin.coeffs;
    auto ppx = (double)yuy_intrin_extrin.k_mat.ppx;
    auto ppy = (double)yuy_intrin_extrin.k_mat.ppy;
    auto fx = (double)yuy_intrin_extrin.k_mat.fx;
    auto fy = (double)yuy_intrin_extrin.k_mat.fy;

    auto sin_a = (double)sin( rot_angles.alpha );
    auto sin_b = (double)sin( rot_angles.beta );
    auto sin_g = (double)sin( rot_angles.gamma );

    auto cos_a = (double)cos( rot_angles.alpha );
    auto cos_b = (double)cos( rot_angles.beta );
    auto cos_g = (double)cos( rot_angles.gamma );
    auto x1 = (double)xy.x;
    auto y1 = (double)xy.y;

    auto x2 = x1 * x1;
    auto y2 = y1 * y1;
    auto xy2 = x2 + y2;
    auto x2_y2 = xy2 * xy2;

    auto x = v.x;
    auto y = v.y;
    auto z = v.z;

    auto exp1 = z * (cos_a*cos_b) + x * (+(sin_a*sin_g - cos_a * cos_g*sin_b))
        + y * ((cos_g*sin_a + cos_a * sin_b*sin_g)) + t[2];

    auto res = (((y*(+(sin_a*sin_g - cos_a * cos_g*sin_b)) - x * ((cos_g*sin_a + cos_a * sin_b*sin_g)))
        *(z*(ppy * cos_a*cos_b - fy * cos_b*sin_a) + x * (fy*(cos_a*sin_g + cos_g * sin_a*sin_b)
            + ppy * (sin_a*sin_g - cos_a * cos_g*sin_b)) + y * (fy*(cos_a*cos_g - sin_a * sin_b*sin_g)
                + ppy * (cos_g*sin_a + cos_a * sin_b*sin_g))
            + (fy * t[1] + ppy * t[2])) - (y*(fy*(cos_a*sin_g + cos_g * sin_a*sin_b) + ppy * (sin_a*sin_g - cos_a * cos_g*sin_b))
                - x * (fy*(cos_a*cos_g - sin_a * sin_b*sin_g) + ppy * (cos_g*sin_a + cos_a * sin_b*sin_g)))*(z*(cos_a*cos_b)
                    + x * ((sin_a*sin_g - cos_a * cos_g*sin_b)) + y * (+(cos_g*sin_a + cos_a * sin_b*sin_g)) + (t[2])))
        *(rc + 2 * d[3] * x1 + 6 * d[2] * y1 + y1 * (2 * d[0] * y1 + 4 * d[1] * y1*(xy2)+6 * d[4] * y1*x2_y2)))
        / (exp1*exp1) + (fy*((y*(+(sin_a*sin_g - cos_a * cos_g*sin_b)) - x * (+(cos_g*sin_a + cos_a * sin_b*sin_g)))
            *(z*(fx*sin_b + ppx * cos_a*cos_b) + x * (ppx * (sin_a*sin_g - cos_a * cos_g*sin_b) + fx * cos_b*cos_g)
                + y * (+ppx * (cos_g*sin_a + cos_a * sin_b*sin_g) - fx * cos_b*sin_g) + (fx*t[0] + ppx * t[2]))
            - (y*(ppx * (sin_a*sin_g - cos_a * cos_g*sin_b) + fx * cos_b*cos_g) - x
                * (+ppx * (cos_g*sin_a + cos_a * sin_b*sin_g) - fx * cos_b*sin_g))*(z*(cos_a*cos_b)
                    + x * ((sin_a*sin_g - cos_a * cos_g*sin_b)) + y * ((cos_g*sin_a + cos_a * sin_b*sin_g)) + (t[2])))
            *(2 * d[2] * x1 + 2 * d[3] * y1 + y1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))
            ) / (fx*(exp1*exp1));

    return res;
}

coeffs<translation> optimizer::calc_translation_coefs( const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const calib & yuy_intrin_extrin, const std::vector<double>& rc, const std::vector<double2>& xy )
{
    coeffs<translation> res;

    auto v = z_data.vertices;
    res.y_coeffs.resize( v.size() );
    res.x_coeffs.resize( v.size() );

    for( auto i = 0; i < rc.size(); i++ )
    {
        res.y_coeffs[i] = calculate_translation_y_coeff( v[i], rc[i], xy[i], yuy_intrin_extrin );
        res.x_coeffs[i] = calculate_translation_x_coeff( v[i], rc[i], xy[i], yuy_intrin_extrin );

    }

    return res;
}


std::pair<calib, double> optimizer::calc_cost_and_grad( const z_frame_data & z_data, const yuy2_frame_data & yuy_data, const calib& curr_calib )
{
    auto uvmap = get_texture_map( z_data.vertices, curr_calib );

#if 0
    std::ofstream f;
    f.open( "uvmap" );
    for( auto i = 0; i < uvmap.size(); i++ )
    {
        f << uvmap[i].x << " " << uvmap[i].y << std::endl;
    }
    f.close();
#endif
    auto cost = calc_cost( z_data, yuy_data, uvmap );
    auto grad = calc_gradients( z_data, yuy_data, uvmap, curr_calib );
    return { grad, cost };
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

void calib::copy_coefs( calib & obj )
{
    obj.width = this->width;
    obj.height = this->height;

    obj.coeffs[0] = this->coeffs[0];
    obj.coeffs[1] = this->coeffs[1];
    obj.coeffs[2] = this->coeffs[2];
    obj.coeffs[3] = this->coeffs[3];
    obj.coeffs[4] = this->coeffs[4];

    obj.model = this->model;
}

calib calib::operator*( double step_size )
{
    calib res;
    res.k_mat.fx = this->k_mat.fx * step_size;
    res.k_mat.fy = this->k_mat.fy * step_size;
    res.k_mat.ppx = this->k_mat.ppx * step_size;
    res.k_mat.ppy = this->k_mat.ppy *step_size;

    res.rot_angles.alpha = this->rot_angles.alpha *step_size;
    res.rot_angles.beta = this->rot_angles.beta *step_size;
    res.rot_angles.gamma = this->rot_angles.gamma *step_size;

    res.trans.t1 = this->trans.t1 *step_size;
    res.trans.t2 = this->trans.t2 * step_size;
    res.trans.t3 = this->trans.t3 *step_size;

    copy_coefs( res );

    return res;
}

calib calib::operator/( double factor )
{
    return (*this)*(1.f / factor);
}

calib calib::operator+( const calib & c )
{
    calib res;
    res.k_mat.fx = this->k_mat.fx + c.k_mat.fx;
    res.k_mat.fy = this->k_mat.fy + c.k_mat.fy;
    res.k_mat.ppx = this->k_mat.ppx + c.k_mat.ppx;
    res.k_mat.ppy = this->k_mat.ppy + c.k_mat.ppy;

    res.rot_angles.alpha = this->rot_angles.alpha + c.rot_angles.alpha;
    res.rot_angles.beta = this->rot_angles.beta + c.rot_angles.beta;
    res.rot_angles.gamma = this->rot_angles.gamma + c.rot_angles.gamma;

    res.trans.t1 = this->trans.t1 + c.trans.t1;
    res.trans.t2 = this->trans.t2 + c.trans.t2;
    res.trans.t3 = this->trans.t3 + c.trans.t3;

    copy_coefs( res );

    return res;
}

calib calib::operator-( const calib & c )
{
    calib res;
    res.k_mat.fx = this->k_mat.fx - c.k_mat.fx;
    res.k_mat.fy = this->k_mat.fy - c.k_mat.fy;
    res.k_mat.ppx = this->k_mat.ppx - c.k_mat.ppx;
    res.k_mat.ppy = this->k_mat.ppy - c.k_mat.ppy;

    res.rot_angles.alpha = this->rot_angles.alpha - c.rot_angles.alpha;
    res.rot_angles.beta = this->rot_angles.beta - c.rot_angles.beta;
    res.rot_angles.gamma = this->rot_angles.gamma - c.rot_angles.gamma;

    res.trans.t1 = this->trans.t1 - c.trans.t1;
    res.trans.t2 = this->trans.t2 - c.trans.t2;
    res.trans.t3 = this->trans.t3 - c.trans.t3;

    copy_coefs( res );

    return res;
}

calib calib::operator/( const calib & c )
{
    calib res;
    res.k_mat.fx = this->k_mat.fx / c.k_mat.fx;
    res.k_mat.fy = this->k_mat.fy / c.k_mat.fy;
    res.k_mat.ppx = this->k_mat.ppx / c.k_mat.ppx;
    res.k_mat.ppy = this->k_mat.ppy / c.k_mat.ppy;

    res.rot_angles.alpha = this->rot_angles.alpha / c.rot_angles.alpha;
    res.rot_angles.beta = this->rot_angles.beta / c.rot_angles.beta;
    res.rot_angles.gamma = this->rot_angles.gamma / c.rot_angles.gamma;

    res.trans.t1 = this->trans.t1 / c.trans.t1;
    res.trans.t2 = this->trans.t2 / c.trans.t2;
    res.trans.t3 = this->trans.t3 / c.trans.t3;

    copy_coefs( res );

    return res;
}

double calib::get_norma()
{
    std::vector<double> grads = { rot_angles.alpha,rot_angles.beta, rot_angles.gamma,
                        trans.t1, trans.t2, trans.t3,
                        k_mat.fx, k_mat.fy, k_mat.ppx, k_mat.ppy };

    double grads_norm = 0;  // TODO meant to have as float??

    for( auto i = 0; i < grads.size(); i++ )
    {
        grads_norm += grads[i] * grads[i];
    }
    grads_norm = sqrt( grads_norm );

    return grads_norm;
}

double calib::sum()
{
    double res = 0;  // TODO meant to have float??
    std::vector<double> grads = { rot_angles.alpha,rot_angles.beta, rot_angles.gamma,
                        trans.t1, trans.t2, trans.t3,
                         k_mat.fx, k_mat.fy, k_mat.ppx, k_mat.ppy };


    for( auto i = 0; i < grads.size(); i++ )
    {
        res += grads[i];
    }

    return res;
}

calib calib::normalize()
{
    std::vector<double> grads = { rot_angles.alpha,rot_angles.beta, rot_angles.gamma,
                        trans.t1, trans.t2, trans.t3,
                        k_mat.fx, k_mat.fy, k_mat.ppx, k_mat.ppy };

    auto norma = get_norma();

    std::vector<double> res_grads( grads.size() );

    for( auto i = 0; i < grads.size(); i++ )
    {
        res_grads[i] = grads[i] / norma;
    }

    calib res;
    res.rot_angles = { res_grads[0], res_grads[1], res_grads[2] };
    res.trans = { res_grads[3], res_grads[4], res_grads[5] };
    res.k_mat = { res_grads[6], res_grads[7], res_grads[8], res_grads[9] };

    return res;
}

calib::calib(rs2_intrinsics_double const & intrin, rs2_extrinsics_double const & extrin)
{
    auto const & r = extrin.rotation;
    auto const & t = extrin.translation;
    auto const & c = intrin.coeffs;

    height = intrin.height;
    width = intrin.width;
    rot = { r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8] };
    trans = { t[0], t[1], t[2] };
    k_mat = { intrin.fx, intrin.fy, intrin.ppx, intrin.ppy };
    coeffs[0] = c[0];
    coeffs[1] = c[1];
    coeffs[2] = c[2];
    coeffs[3] = c[3];
    coeffs[4] = c[4];
    model = intrin.model;
}

librealsense::algo::depth_to_rgb_calibration::calib::calib(rs2_intrinsics const & intrin, rs2_extrinsics const & extrin)
{
    auto const & r = extrin.rotation;
    auto const & t = extrin.translation;
    auto const & c = intrin.coeffs;

    height = intrin.height;
    width = intrin.width;
    rot = { r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8] };
    trans = { t[0], t[1], t[2] };
    k_mat = { intrin.fx, intrin.fy, intrin.ppx, intrin.ppy };
    coeffs[0] = c[0];
    coeffs[1] = c[1];
    coeffs[2] = c[2];
    coeffs[3] = c[3];
    coeffs[4] = c[4];
    model = intrin.model;
}

bool optimizer::is_valid_results()
{
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
