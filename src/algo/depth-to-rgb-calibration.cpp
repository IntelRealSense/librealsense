//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "algo/depth-to-rgb-calibration.h"
#include "../include/librealsense2/rsutil.h"
#include <algorithm>
#include <array>

#define AC_LOG_PREFIX "AC1: "
#define AC_LOG(TYPE,MSG) LOG_##TYPE( AC_LOG_PREFIX << MSG )
//#define AC_LOG(TYPE,MSG) LOG_ERROR( AC_LOG_PREFIX << MSG )
//#define AC_LOG(TYPE,MSG) std::cout << (std::string)( to_string() << "-" << #TYPE [0] << "- " << MSG ) << std::endl; //LOG_INFO((std::string)( to_string() << "-" << #TYPE [0] << "- " << MSG ));
//#define AC_LOG_CONTINUE(TYPE,MSG) std::cout << (std::string)( to_string() << "-" << #TYPE [0] << "- " << MSG )
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
    std::vector<double> gauss_convolution(std::vector<T> const& image,
        size_t image_width, size_t image_height,
        size_t mask_width, size_t mask_height,
        std::function< double(std::vector<T> const& sub_image) > convolution_operation
        )
    {
        std::vector<double> res(image.size(), 0);
        std::vector<T> sub_image(mask_width * mask_height, 0);
        size_t ind = 0;
        size_t row_bound[4] = { 0, 1, image_height - 1,image_height - 2 };
        size_t lines[4] = { 2, 1, 2 , 1 };
        size_t first_rows[4] = { 1,1,0,0 };
        size_t last_rows[4] = { 0,0,1,1 };
        // poundaries handling 
        // Extend - The nearest border pixels are conceptually extended as far as necessary to provide values for the convolution.
        // Corner pixels are extended in 90° wedges.Other edge pixels are extended in lines.
        for (auto row = 0; row < 4; row++) {
            for (auto jj = 0; jj < image_width - mask_width + 1; jj++)
            {
                ind = first_rows[row] * mask_width * lines[row]; // skip first 2 lines for padding - start from 3rd line
                for (auto l = first_rows[row] * lines[row]; l < mask_height - last_rows[row] * lines[row]; l++)
                {
                    for (auto k = 0; k < mask_width; k++)
                    {
                        auto p = (l - first_rows[row] * lines[row] + last_rows[row] * (-2 + row_bound[row])) * image_width + jj + k;
                        sub_image[ind++] = (image[p]);
                    }
                }
                // fill first 2 lines to same values as 3rd line
                ind = first_rows[row] * mask_width * lines[row] + last_rows[row] * (mask_width * (mask_height - lines[row]));
                auto ind_pad = last_rows[row] * (mask_width * (mask_height - lines[row] - 1)); // previous line
                for (auto k = 0; k < mask_width * lines[row]; k++)
                {
                    auto idx1 = last_rows[row] * ind;
                    auto idx2 = last_rows[row] * ind_pad + first_rows[row] * ind;
                    sub_image[k + idx1] = sub_image[k % mask_width + idx2];
                }
                auto mid = jj + mask_width / 2 + row_bound[row] * image_width;
                res[mid] = convolution_operation(sub_image);
            }
        }
        // first 2 columns

        size_t column_boundaries[4] = { 0,1, image_width - 1 ,image_width - 2 };
        size_t columns[4] = { 2, 1, 2, 1 };
        size_t left_column[4] = { 1,1,0,0 };
        size_t right_column[4] = { 0,0,1,1 };
        for (auto col = 0; col < 4; col++) {
            for (size_t ii = 0; ii < image_height - mask_height + 1; ii++)
            {
                ind = 0; // skip first 2 columns for padding - start from 3rd column
                for (auto l = 0; l < mask_height; l++)
                {
                    ind += left_column[col] * columns[col];
                    for (auto k = left_column[col] * columns[col]; k < mask_width - right_column[col] * columns[col]; k++)
                    {
                        size_t p = (ii + l) * image_width + k - left_column[col] * columns[col] + right_column[col] * (column_boundaries[col] - 2);
                        sub_image[ind++] = (image[p]);
                    }
                    ind += right_column[col] * columns[col];
                }
                // fill first 2 columns to same values as 3rd column
                ind = columns[col];
                for (auto l = 0; l < mask_height; l++)
                {
                    ind = left_column[col] * columns[col] + right_column[col] * (mask_height - columns[col] - 1) + l * mask_width;
                    for (auto k = 1; k <= columns[col]; k++)
                    {
                        auto idx = left_column[col] * (ind - k) + right_column[col] * (ind + k);
                        sub_image[idx] = sub_image[ind];
                    }
                }
                auto mid = (ii + mask_height / 2) * image_width + column_boundaries[col];
                res[mid] = convolution_operation(sub_image);
            }
        }

        // corners handling
        // 1. image[0] and image[1]
        size_t corners_arr[16] = { 0,1,image_width, image_width + 1,  image_width - 1, image_width - 2, 2 * image_width - 1, 2 * image_width - 2,
            (image_height - 2) * image_width,(image_height - 2) * image_width,  // left corners - line before the last
            (image_height - 1) * image_width,(image_height - 1) * image_width,
            (image_height - 2) * image_width,(image_height - 2) * image_width,  // right corners - line before the last
            (image_height - 1) * image_width,(image_height - 1) * image_width };

        size_t corners_arr_col[16] = { 0,0,0,0,0,0,0,0,0,1,0,1,image_width - 1,image_width - 2,image_width - 1,image_width - 2 };
        size_t left_col[16] = { 1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0 };
        size_t right_col[16] = { 0,0,0,0,1,1 ,1,1,0,0,0,0,1,1,1,1 };
        size_t up_rows[16] = { 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0 };
        size_t down_rows[16] = { 0,0,0,0,0,0 ,0,0,1,1,1,1,1,1,1,1 };
        size_t corner_columns[16] = { 2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1 };
        size_t corner_rows[16] = { 2, 2,1,1 ,2,2,1,1,1,1,2,2,1,1,2,2 };
        for (auto corner = 0; corner < 16; corner++) {
            ind = up_rows[corner] * corner_rows[corner] * mask_width; // starting point in sub-image
            for (auto l = up_rows[corner] * corner_rows[corner]; l < mask_height - down_rows[corner] * corner_rows[corner]; l++)
            {
                ind += left_col[corner] * corner_columns[corner];
                for (auto k = left_col[corner] * corner_columns[corner]; k < mask_width - right_col[corner] * corner_columns[corner]; k++)
                {
                    auto up_row_i = (l - corner_rows[corner] + right_col[corner] * (corner_rows[corner] - 2)) * image_width;
                    auto up_col_i = k - left_col[corner] * corner_columns[corner] + right_col[corner] * (corners_arr[corner] - 2);
                    auto down_row_i = (l - 2) * image_width + corners_arr[corner];
                    auto down_col_i = k - left_col[corner] * corner_columns[corner] + right_col[corner] * (corners_arr_col[corner] - 2);
                    auto col_i = down_rows[corner] * down_col_i + up_rows[corner] * up_col_i;
                    auto row_i = down_rows[corner] * down_row_i + up_rows[corner] * up_row_i;
                    auto p = row_i + col_i;
                    sub_image[ind++] = (image[p]);
                }
                ind += right_col[corner] * corner_columns[corner];
            }
            // fill first 2 columns to same values as 3rd column
            // and first 2 rows to same values as 3rd row
            // 1. rows
            auto l_init = down_rows[corner] * (mask_height - corner_rows[corner]); // pad last rows
            auto l_limit = up_rows[corner] * corner_rows[corner] + down_rows[corner] * mask_height;
            for (auto l = l_init; l < l_limit; l++)
            {
                ind = up_rows[corner] * (corner_rows[corner] * mask_width) + down_rows[corner] * (mask_height - corner_rows[corner] - 1) * mask_width; // start with padding first 2 rows
                auto ind2 = l * mask_width;
                for (auto k = 0; k < mask_width; k++)
                {
                    sub_image[k + ind2] = sub_image[ind++];

                }
            }
            // 2. columns
            for (auto l = 0; l < mask_height; l++)
            {
                ind = l * mask_width + left_col[corner] * corner_columns[corner] + right_col[corner] * (mask_width - 1 - corner_columns[corner]);
                auto ind2 = l * mask_width + right_col[corner] * (mask_width - corner_columns[corner]);
                for (auto k = 0; k < corner_columns[corner]; k++)
                {
                    sub_image[k + ind2] = sub_image[ind];

                }
            }
            auto mid = corners_arr[corner] + corners_arr_col[corner];
            res[mid] = convolution_operation(sub_image);
        }

        // rest of the pixel in the middle
        for (size_t i = 0; i < image_height - mask_height + 1; i++)
        {
            for (size_t j = 0; j < image_width - mask_width + 1; j++)
            {
                ind = 0;
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
                //*(debug + mid) = res[mid];
            }
        }
        return res;
    }
    template<class T>
    std::vector<uint8_t> dilation_convolution(std::vector<T> const& image,
        size_t image_width, size_t image_height,
        size_t mask_width, size_t mask_height,
        std::function< uint8_t(std::vector<T> const& sub_image) > convolution_operation
        )
    {
        std::vector<uint8_t> res(image.size(), 0);
        std::vector<T> sub_image(mask_width * mask_height, 0);
        auto ind = 0;
        size_t arr[2] = { 0, image_height - 1 };

        for (auto arr_i = 0; arr_i < 2; arr_i++) {
            for (auto jj = 0; jj < image_width - mask_width + 1; jj++)
            {
                ind = 0;
                for (auto l = 0; l < mask_height; l++)
                {
                    for (auto k = 0; k < mask_width; k++)
                    {
                        size_t p = (l + arr[arr_i]) * image_width + jj + k;
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
                size_t mid = jj + mask_width / 2 + arr[arr_i] * image_width;
                res[mid] = convolution_operation(sub_image);
            }
        }

    arr[0] = 0;
    arr[1] = image_width - 1;
    for (size_t arr_i = 0; arr_i < 2; arr_i++) {
        for (size_t ii = 0; ii < image_height - mask_height + 1; ii++)
        {
            ind = 0;
            for (size_t l = 0; l < mask_height; l++)
            {
                for (size_t k = 0; k < mask_width; k++)
                {
                    size_t p = (ii + l) * image_width + k+ arr[arr_i];
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
    for (size_t i = 0; i < image_height - mask_height + 1; i++)
    {
        for (size_t j = 0; j < image_width - mask_width + 1; j++)
        {
            ind = 0;
            for (size_t l = 0; l < mask_height; l++)
            {
                for (size_t k = 0; k < mask_width; k++)
                {
                    size_t p = (i + l) * image_width + j + k;
                    sub_image[ind++] = (image[p]);
                }

            }
            size_t mid = (i + mask_height / 2) * image_width + j + mask_width / 2;
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

static rotation extract_rotation_from_angles( const rotation_in_angles & rot_angles )
{
    rotation res;

    auto sin_a = sin( rot_angles.alpha );
    auto sin_b = sin( rot_angles.beta );
    auto sin_g = sin( rot_angles.gamma );

    auto cos_a = cos( rot_angles.alpha );
    auto cos_b = cos( rot_angles.beta );
    auto cos_g = cos( rot_angles.gamma );

    //% function [R] = calcRmatRromAngs(xAlpha,yBeta,zGamma)
    //%     R = [cos( yBeta )*cos( zGamma ), -cos( yBeta )*sin( zGamma ), sin( yBeta ); ...
    //%          cos( xAlpha )*sin( zGamma ) + cos( zGamma )*sin( xAlpha )*sin( yBeta ), cos( xAlpha )*cos( zGamma ) - sin( xAlpha )*sin( yBeta )*sin( zGamma ), -cos( yBeta )*sin( xAlpha ); ...
    //%          sin( xAlpha )*sin( zGamma ) - cos( xAlpha )*cos( zGamma )*sin( yBeta ), cos( zGamma )*sin( xAlpha ) + cos( xAlpha )*sin( yBeta )*sin( zGamma ), cos( xAlpha )*cos( yBeta )];
    //% end
    // -> note the transposing of the coordinates

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

rotation_in_angles extract_angles_from_rotation(const double r[9])
{
    rotation_in_angles res;
    //% function [xAlpha,yBeta,zGamma] = extractAnglesFromRotMat(R)
    //%     xAlpha = atan2(-R(2,3),R(3,3));
    //%     yBeta = asin(R(1,3));
    //%     zGamma = atan2(-R(1,2),R(1,1));
    // -> we transpose the coordinates!
    res.alpha = atan2(-r[7], r[8]);
    res.beta = asin(r[6]);
    res.gamma = atan2(-r[3], r[0]);
    // -> additional code is in the original function that catches some corner
    //    case, but since these have never occurred we were told not to use it 

    // TODO Sanity-check: can be removed
    rotation rot = extract_rotation_from_angles( res );
    double sum = 0;
    for (auto i = 0; i < 9; i++)
    {
        sum += rot.rot[i] - r[i];
    }
    auto epsilon = 0.00001;
    if ((abs(sum)) > epsilon)
        throw "No fit";
    return res;
}

size_t optimizer::optimize( std::function< void( iteration_data_collect const & data ) > cb )
{
    optimaization_params params_orig;
    params_orig.curr_calib = _original_calibration;
    params_orig.curr_calib.p_mat = calc_p_mat(params_orig.curr_calib);

    auto const original_cost_and_grad = calc_cost_and_grad(_z, _yuy, params_orig.curr_calib);

    auto const original_cost = original_cost_and_grad.second;
    AC_LOG(DEBUG, "Original cost = " << original_cost);

    _params_curr = params_orig;
    _params_curr.curr_calib.rot_angles = extract_angles_from_rotation(_params_curr.curr_calib.rot.rot);
    _params_curr.curr_calib.p_mat = calc_p_mat(_params_curr.curr_calib);

    size_t n_iterations = 0;

    for (auto i = 0; i < _params.max_optimization_iters; i++)
    {
        iteration_data_collect data;

        data.iteration = i;

        auto res = calc_cost_and_grad(_z, _yuy, _params_curr.curr_calib, data);
        AC_LOG(DEBUG, "Cost = " << std::fixed << std::setprecision(15) << res.second);

        _params_curr.cost = res.second;
        _params_curr.calib_gradients = res.first;

        data.params = _params_curr;

        if (cb)
            cb(data);

        auto new_params = back_tracking_line_search(_z, _yuy, _params_curr);
        auto norm = (new_params.curr_calib - _params_curr.curr_calib).get_norma();
        if( norm < _params.min_rgb_mat_delta )
        {
            AC_LOG( DEBUG, n_iterations << ": {normal(new-curr)} " << norm << " < " << _params.min_rgb_mat_delta << " {min_rgb_mat_delta}  -->  stopping" );
            _params_curr = new_params;
            break;
        }

        auto delta = abs( new_params.cost - _params_curr.cost );
        if( delta < _params.min_cost_delta )
        {
            AC_LOG( DEBUG, n_iterations << ": Cost = " << new_params.cost << "; delta= " << delta << " < " << _params.min_cost_delta << "  -->  stopping" );
            _params_curr = new_params;
            break;
        }

        _params_curr = new_params;
        ++n_iterations;
        AC_LOG( DEBUG, n_iterations << ": Cost = " << _params_curr.cost << "; delta= " << delta );
    }
    if( !n_iterations )
    {
        AC_LOG( INFO, "Calibration not necessary; nothing done" );
    }
    else
    {
        AC_LOG( INFO, "Calibration finished after " << n_iterations << " iterations; original cost= " << original_cost << "  optimized cost= " << _params_curr.cost );
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
    _original_calibration.p_mat = calc_p_mat( _original_calibration );
    _yuy.width = calibration.width;
    _yuy.height = calibration.height;
    _params.set_rgb_resolution( _yuy.width, _yuy.height );

    _yuy.yuy2_frame = get_luminance_from_yuy2( yuy_data );
    _yuy.yuy2_prev_frame = get_luminance_from_yuy2( prev_yuy_data );

    _yuy.edges = calc_edges( _yuy.yuy2_frame, _yuy.width, _yuy.height );
    _yuy.prev_edges = calc_edges(_yuy.yuy2_prev_frame, _yuy.width, _yuy.height);

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

void optimizer::check_edge_distribution(
    std::vector<double>& sum_weights_per_section,
    double& min_max_ratio,
    bool& is_edge_distributed
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
    if( min_max_ratio < _params.edge_distribution_min_max_ratio)
    {
        is_edge_distributed = false;
        AC_LOG( ERROR, "Edge distribution ratio ({min}" << z_min << "/" << z_max << "{max} = " << min_max_ratio << ") is too small; threshold= " << _params.edge_distribution_min_max_ratio);
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
        if( *it < _params.min_weighted_edge_per_section)
        {
            is_edge_distributed = false;
            break;
        }
    }
    if (!is_edge_distributed) {
        AC_LOG( DEBUG, "check_edge_distribution: weighted edge per section is too low:  ");
        for (auto it = sum_weights_per_section.begin(); it != sum_weights_per_section.end(); ++it)
        {
            AC_LOG( DEBUG, "    " << *it);
        }
        AC_LOG(DEBUG, "threshold is: " << _params.min_weighted_edge_per_section);
        return;
    }
    is_edge_distributed = true;
}

bool optimizer::is_edge_distributed(z_frame_data& z, yuy2_frame_data& yuy)
{
    size_t num_of_sections = _params.num_of_sections_for_edge_distribution_x * _params.num_of_sections_for_edge_distribution_y;

    // depth frame
    AC_LOG( DEBUG, "... checking Z edge distribution" );
    


    sum_per_section(z.sum_weights_per_section, z.section_map, z.weights, num_of_sections);
    //for debug 
    auto it = z.sum_weights_per_section.begin();
    AC_LOG(DEBUG, "... sum_per_section(z), section #0  " << *(it));
    AC_LOG(DEBUG, "... sum_per_section(z), section #1  " << *(it + 2));
    AC_LOG(DEBUG, "... sum_per_section(z), section #2  " << *(it + 1));
    AC_LOG(DEBUG, "... sum_per_section(z), section #3  " << *(it + 3));
    check_edge_distribution(z.sum_weights_per_section, z.min_max_ratio, z.is_edge_distributed);
    // yuy frame
    AC_LOG( DEBUG, "... checking YUY edge distribution" );
    sum_per_section(yuy.sum_weights_per_section, yuy.section_map, yuy.edges_IDT, num_of_sections);
    //for debug 
    it = yuy.sum_weights_per_section.begin();
    AC_LOG(DEBUG, "... sum_per_section(yuy), section #0  " << *(it));
    AC_LOG(DEBUG, "... sum_per_section(yuy), section #1  " << *(it + 2));
    AC_LOG(DEBUG, "... sum_per_section(yuy), section #2  " << *(it + 1));
    AC_LOG(DEBUG, "... sum_per_section(yuy), section #3  " << *(it + 3));
    check_edge_distribution(yuy.sum_weights_per_section, yuy.min_max_ratio, yuy.is_edge_distributed);

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
    auto ix_match = (max_ix + 1) % 3; 
   /* if (ix_match == 0) {
        ix_match = 3;
    }*/
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
            AC_LOG(DEBUG, "is_grad_dir_balanced: gradient direction is not balanced : " << dir_ratio1);
            AC_LOG(DEBUG, "threshold is: " << _params.grad_dir_ratio);
            return false;
        }
        if (min_val_perp < 1e-3)// % Don't devide by zero...
        {
            AC_LOG(DEBUG, "is_grad_dir_balanced: gradient direction is not balanced : " << dir_ratio1);
            AC_LOG(DEBUG, "threshold is: " << _params.grad_dir_ratio);
            dir_ratio2 = DBL_MAX; // = nan 
            return false;
        }
        
        //double min_val = *weights_per_dir.begin() * *ix_check.begin();
        auto ix_check_it = ix_check.begin();
        std::vector<double> filtered_weights_per_dir;
        for (auto it = weights_per_dir.begin(); it< weights_per_dir.end();it++, ix_check_it++)
        {
            auto mult_val = *it * (*ix_check_it);
            if (mult_val>0){
                filtered_weights_per_dir.push_back(*it);
            }
        }
        dir_ratio2 = max_val_perp / *std::min_element(filtered_weights_per_dir.begin(), filtered_weights_per_dir.end());
        if (dir_ratio2 > _params.grad_dir_ratio)
        {
            AC_LOG(DEBUG, "is_grad_dir_balanced: gradient direction is not balanced : " << dir_ratio1);
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
    auto area = yuy.height * yuy.width;
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

void optimizer::gaussian_filter(yuy2_frame_data& yuy)
{
    auto area = yuy.height * yuy.width;

    /* diffIm = abs(im1-im2);
diffIm = imgaussfilt(im1-im2,params.moveGaussSigma);*/
    std::vector<double>  gaussian_kernel = { 0.0029690167439504968, 0.013306209891013651, 0.021938231279714643, 0.013306209891013651, 0.0029690167439504968,
        0.013306209891013651, 0.059634295436180138, 0.098320331348845769, 0.059634295436180138, 0.013306209891013651,
        0.021938231279714643, 0.098320331348845769, 0.16210282163712664, 0.098320331348845769, 0.021938231279714643,
        0.013306209891013651, 0.059634295436180138, 0.098320331348845769, 0.059634295436180138, 0.013306209891013651,
        0.0029690167439504968, 0.013306209891013651, 0.021938231279714643, 0.013306209891013651, 0.0029690167439504968
    };

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
void abs_values(std::vector< double >& vec_in)
{
    //std::vector< double > abs_vec_in = vec_in;
    for (double& val : vec_in)
    {
        if (val < 0)
        {
            val *= -1;
        }
    }
}
void gaussian_dilation_mask(std::vector< double >& gauss_diff, std::vector< uint8_t >& dilation_mask)
{
    auto gauss_it = gauss_diff.begin();
    auto dilation_it = dilation_mask.begin();
    for (auto i = 0; i < gauss_diff.size(); i++, gauss_it++, dilation_it++)
    {
        if (*dilation_it)
        {
            *gauss_it = 0;
        }
    }
}
void move_suspected_mask(std::vector< uint8_t >& move_suspect, std::vector< double >& gauss_diff_masked, double movement_threshold)
{
    for (auto it = gauss_diff_masked.begin(); it != gauss_diff_masked.end(); ++it)
    {
        if (*it > movement_threshold)
        {
            move_suspect.push_back(1);
        }
        else
        {
            move_suspect.push_back(0);
        }
    }
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
%
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
    yuy.gaussian_diff_masked = yuy.gaussian_filtered_image;
    abs_values(yuy.gaussian_diff_masked);
    gaussian_dilation_mask(yuy.gaussian_diff_masked, yuy.dilated_image);
    move_suspected_mask(yuy.move_suspect, yuy.gaussian_diff_masked, _params.move_thresh_pix_val);
    auto sum_move_suspect = 0;
    for (auto it = yuy.move_suspect.begin(); it != yuy.move_suspect.end(); ++it)
    {
        sum_move_suspect += *it;
    }
    if (sum_move_suspect > _params.move_threshold_pix_num)
    {
        AC_LOG(DEBUG, "is_movement_in_images:  # of pixels above threshold " << sum_move_suspect << " allowed #:" << _params.move_threshold_pix_num);
        return true;
    }
    
    return false;
}

bool optimizer::is_scene_valid()
{
    std::vector< byte > section_map_depth( _z.width * _z.height );
    std::vector< byte > section_map_rgb( _yuy.width * _yuy.height );

    size_t const section_w = _params.num_of_sections_for_edge_distribution_x;  //% params.numSectionsH
    size_t const section_h = _params.num_of_sections_for_edge_distribution_y;  //% params.numSectionsH

    // Get a map for each pixel to its corresponding section
    section_per_pixel(_z, section_w, section_h, section_map_depth.data());
    section_per_pixel(_yuy, section_w, section_h, section_map_rgb.data());

    // remove pixels in section map that were removed in weights
    AC_LOG(DEBUG, "... " << _z.supressed_edges.size() << " total edges");
    for (auto i = 0; i < _z.supressed_edges.size(); i++)
    {
        if (_z.supressed_edges[i])
        {
            _z.section_map.push_back(section_map_depth[i]);
        }
    }
    AC_LOG(DEBUG, "... " << _z.section_map.size() << " not suppressed");

    // remove pixels in section map where edges_IDT > 0
    int i = 0;
    AC_LOG(DEBUG, "... " << _z.supressed_edges.size() << " total edges IDT");
    for (auto it = _yuy.edges_IDT.begin(); it != _yuy.edges_IDT.end(); ++it, ++i)
    {
        if (*it > 0)
        {
            _yuy.section_map.push_back(section_map_rgb[i]);
        }
    }
    AC_LOG(DEBUG, "... " << _yuy.section_map.size() << " not suppressed");

    bool res_movement = is_movement_in_images(_yuy);
    bool res_edges = is_edge_distributed(_z, _yuy);
    bool res_gradient = is_grad_dir_balanced(_z);

    return ((!res_movement) && res_edges && res_gradient);
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

rs2_intrinsics_double calib::get_intrinsics() const
{
    return {
        width, height,
        k_mat,
        model, coeffs };
}

rs2_extrinsics_double calib::get_extrinsics() const
{
    auto & r = rot.rot;
    auto & t = trans;
    return {
        { float(r[0]), float(r[1]), float(r[2]), float(r[3]), float(r[4]), float(r[5]), float(r[6]), float(r[7]), float(r[8]) },
        { float(t.t1), float(t.t2), float(t.t3) }
    };
}

p_matrix librealsense::algo::depth_to_rgb_calibration::calib::get_p_matrix() const
{
    return p_mat;
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

    std::ofstream f;
    f.open("interp_y_x");
    f.precision(16);
    for (auto i = 0; i < res.size(); i++)
    {
        f << res[i] << std::endl;
    }
    f.close();
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

optimaization_params optimizer::back_tracking_line_search(const z_frame_data & z_data, const yuy2_frame_data& yuy_data, optimaization_params curr_params)
{
    optimaization_params new_params;

    auto orig = curr_params;
    auto grads_norm = curr_params.calib_gradients.normalize();
    auto normalized_grads = grads_norm / _params.normelize_mat;
    auto normalized_grads_norm = normalized_grads.get_norma();
    auto unit_grad = normalized_grads.normalize();

    auto t = calc_t(curr_params);
    auto step_size = calc_step_size(curr_params);

    curr_params.curr_calib.rot_angles = extract_angles_from_rotation(curr_params.curr_calib.rot.rot);
    auto movement = unit_grad * step_size;
    new_params.curr_calib = curr_params.curr_calib + movement;
    new_params.curr_calib.rot = extract_rotation_from_angles(new_params.curr_calib.rot_angles);
    new_params.curr_calib.p_mat = calc_p_mat(new_params.curr_calib);

    auto uvmap = get_texture_map(z_data.vertices, curr_params.curr_calib);
    curr_params.cost = calc_cost(z_data, yuy_data, uvmap);

    uvmap = get_texture_map(z_data.vertices, new_params.curr_calib);
    new_params.cost = calc_cost(z_data, yuy_data, uvmap);

    auto iter_count = 0;
    AC_LOG(DEBUG, "Current back tracking line search cost = " << std::fixed << std::setprecision(15) << new_params.cost);
    while ((curr_params.cost - new_params.cost) >= step_size * t && abs(step_size) > _params.min_step_size && iter_count++ < _params.max_back_track_iters)
    {
        step_size = _params.tau*step_size;

        new_params.curr_calib = curr_params.curr_calib + unit_grad * step_size;
        new_params.curr_calib.rot = extract_rotation_from_angles(new_params.curr_calib.rot_angles);
        new_params.curr_calib.p_mat = calc_p_mat(new_params.curr_calib);

        uvmap = get_texture_map(z_data.vertices, new_params.curr_calib);
        new_params.cost = calc_cost(z_data, yuy_data, uvmap);

        AC_LOG(DEBUG, "... back tracking line search cost= " << std::fixed << std::setprecision(15) << new_params.cost);
    }

    if (curr_params.cost - new_params.cost >= step_size * t)
    {
        new_params = orig;
    }

    new_params.curr_calib.p_mat = calc_p_mat(new_params.curr_calib);
    new_params.curr_calib.rot = extract_rotation_from_angles(new_params.curr_calib.rot_angles);
    return new_params;
}

p_matrix librealsense::algo::depth_to_rgb_calibration::optimizer::calc_p_mat(calib c)
{
    auto r = c.rot.rot;
    auto t = c.trans;
    auto fx = c.k_mat.fx;
    auto fy = c.k_mat.fy;
    auto ppx = c.k_mat.ppx;
    auto ppy = c.k_mat.ppy;
    return { fx* r[0] + ppx * r[2], fx* r[3] + ppx * r[5], fx* r[6] + ppx * r[8], fx* t.t1 + ppx * t.t3,
             fy* r[1] + ppy * r[2], fy* r[4] + ppy * r[5], fy* r[7] + ppy * r[8], fy* t.t2 + ppy * t.t3,
             r[2]                 , r[5]                 , r[8]                 , t.t3 };
}

double optimizer::calc_step_size(optimaization_params opt_params)
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


double optimizer::calc_cost(
    const z_frame_data & z_data,
    const yuy2_frame_data & yuy_data,
    const std::vector< double2 > & uv,
    iteration_data_collect & data)
{
    double cost = 0;
    size_t N = 0;
    data.d_vals = calc_cost_per_vertex_fn(z_data, yuy_data, uv,
        [&](size_t i, double d_val, double weight, double vertex_cost)
        {
            if( d_val != std::numeric_limits<double>::max() )
            {
                cost += vertex_cost;
                ++N;
            }
        } );
    return N ? cost / N : 0.;
}

calib optimizer::calc_gradients(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const std::vector<double2>& uv,
    const calib& curr_calib, iteration_data_collect& data)
{
    calib res;
    auto interp_IDT_x = biliniar_interp( yuy_data.edges_IDTx, yuy_data.width, yuy_data.height, uv );
    data.d_vals_x = interp_IDT_x;
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
    data.d_vals_y = interp_IDT_y;

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


std::pair<calib, double> optimizer::calc_cost_and_grad(const z_frame_data & z_data, const yuy2_frame_data & yuy_data, const calib& curr_calib, iteration_data_collect& data)
{
    auto uvmap = get_texture_map(z_data.vertices, curr_calib);
    data.uvmap = uvmap;

    auto cost = calc_cost(z_data, yuy_data, uvmap, data);
    auto grad = calc_gradients(z_data, yuy_data, uvmap, curr_calib, data);
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

void calib::copy_coefs( calib & obj ) const
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

calib calib::operator*( double step_size ) const
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

calib calib::operator/( double factor ) const
{
    return (*this)*(1.f / factor);
}

calib calib::operator+( const calib & c ) const
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

calib calib::operator-( const calib & c ) const
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

calib calib::operator/( const calib & c ) const
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
        AC_LOG( DEBUG, "... too big: clipping at limit for iteration " << iteration_number << "= " << max_movement );

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
        new_calib.p_mat = calc_p_mat( new_calib );
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
void write_vector_to_file( std::vector< T > const & v,
    std::string const & dir,
    char const * filename
)
{
    write_to_file( v.data(), v.size() * sizeof( T ), dir, filename );
}

void optimizer::write_data_to( std::string const & dir )
{
    AC_LOG( DEBUG, "... writing data to: " << dir );
    
    try
    {
        write_vector_to_file( _yuy.yuy2_frame, dir, "rgb.raw" );
        write_vector_to_file( _yuy.yuy2_prev_frame, dir, "rgb_prev.raw" );
        write_vector_to_file( _ir.ir_frame, dir, "ir.raw" );
        write_vector_to_file( _z.frame, dir, "depth.raw" );

        write_to_file( &_original_calibration, sizeof( _original_calibration ), dir, "rgb.calib" );
        write_to_file( &_z.intrinsics, sizeof( _z.intrinsics ), dir, "depth.intrinsics" );
        write_to_file( &_z.depth_units, sizeof( _z.depth_units ), dir, "depth.units" );
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
