//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "optimizer.h"
#include <librealsense2/rsutil.h>
#include <algorithm>
#include <array>
#include "coeffs.h"
#include "cost.h"
#include "debug.h"
#include <math.h>


#define GAUSS_CONV_ROWS 4
#define GAUSS_CONV_COLUMNS 4
#define GAUSS_CONV_CORNERS 16

using namespace librealsense::algo::depth_to_rgb_calibration;
using librealsense::to_string;

template<class T>
std::vector<double> gauss_convolution(std::vector<T> const& image,
    size_t image_width, size_t image_height,
    size_t mask_width, size_t mask_height,
    std::function< double(std::vector<T> const& sub_image) > convolution_operation
    )
{
    // boundaries handling 
    // Extend - The nearest border pixels are conceptually extended as far as necessary to provide values for the convolution.
    // Corner pixels are extended in 90° wedges.Other edge pixels are extended in lines.
    // https://en.wikipedia.org/wiki/Kernel_(image_processing)
    // handling order:
    // 1. rows: 0,1 and image_height-1, image_height-2
    // 2. columns: 0,1 and image_width-1, image_width-2
    // 3. corners: 4 pixels in each corner
    // 4. rest of the pixels

    // 1. rows handling :
    std::vector<double> res(image.size(), 0);
    std::vector<T> sub_image(mask_width * mask_height, 0);
    size_t ind = 0;
    size_t row_bound[GAUSS_CONV_ROWS] = { 0, 1, image_height - 1,image_height - 2 };
    size_t lines[GAUSS_CONV_ROWS] = { 2, 1, 2 , 1 };
    size_t first_rows[GAUSS_CONV_ROWS] = { 1,1,0,0 };
    size_t last_rows[GAUSS_CONV_ROWS] = { 0,0,1,1 };
    
    for (auto row = 0; row < GAUSS_CONV_ROWS; row++) {
        for (auto jj = 0; jj < image_width - mask_width + 1; jj++)
        {
            ind = first_rows[row] * mask_width * lines[row]; // skip first 1/2 rows in sub-image for padding - start from 2nd/3rd line
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

    // 2. columns handling :
    size_t column_boundaries[GAUSS_CONV_COLUMNS] = { 0,1, image_width - 1 ,image_width - 2 };
    size_t columns[GAUSS_CONV_COLUMNS] = { 2, 1, 2, 1 };
    size_t left_column[GAUSS_CONV_COLUMNS] = { 1,1,0,0 };
    size_t right_column[GAUSS_CONV_COLUMNS] = { 0,0,1,1 };
    for (auto col = 0; col < GAUSS_CONV_COLUMNS; col++) {
        for (size_t ii = 0; ii < image_height - mask_height + 1; ii++)
        {
            ind = 0; 
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

    // 3. corners handling
    size_t corners_arr[GAUSS_CONV_CORNERS] = { 0,1,image_width, image_width + 1,  image_width - 1, image_width - 2, 2 * image_width - 1, 2 * image_width - 2,
        (image_height - 2) * image_width,(image_height - 2) * image_width,  // left corners - line before the last
        (image_height - 1) * image_width,(image_height - 1) * image_width,
        (image_height - 2) * image_width,(image_height - 2) * image_width,  // right corners - last row
        (image_height - 1) * image_width,(image_height - 1) * image_width };

    size_t corners_arr_col[GAUSS_CONV_CORNERS] = { 0,0,0,0,0,0,0,0,0,1,0,1,image_width - 1,image_width - 2,image_width - 1,image_width - 2 };
    size_t left_col[GAUSS_CONV_CORNERS] = { 1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0 };
    size_t right_col[GAUSS_CONV_CORNERS] = { 0,0,0,0,1,1 ,1,1,0,0,0,0,1,1,1,1 };
    size_t up_rows[GAUSS_CONV_CORNERS] = { 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0 };
    size_t down_rows[GAUSS_CONV_CORNERS] = { 0,0,0,0,0,0 ,0,0,1,1,1,1,1,1,1,1 };
    size_t corner_columns[GAUSS_CONV_CORNERS] = { 2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1 };
    size_t corner_rows[GAUSS_CONV_CORNERS] = { 2, 2,1,1 ,2,2,1,1,1,1,2,2,1,1,2,2 };
    for (auto corner = 0; corner < GAUSS_CONV_CORNERS; corner++) {
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
        // 3.1. rows
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
        // 3.2. columns
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

    // 4. rest of the pixel in the middle
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
    // poundaries handling 
    // Extend boundary rows and columns to zeros

    // 1. rows
    std::vector<uint8_t> res(image.size(), 0);
    std::vector<T> sub_image(mask_width * mask_height, 0);
    auto ind = 0;
    size_t rows[2] = { 0, image_height - 1 };

    for (auto rows_i = 0; rows_i < 2; rows_i++) {
        for (auto jj = 0; jj < image_width - mask_width + 1; jj++)
        {
            ind = 0;
            for (auto l = 0; l < mask_height; l++)
            {
                for (auto k = 0; k < mask_width; k++)
                {
                    size_t p = (l + rows[rows_i]) * image_width + jj + k;
                    if (rows_i != 0)
                    {
                        p = p - 2 * image_width;
                    }
                    sub_image[ind] = (image[p]);
                    bool cond1 = (l == 2) && (rows_i == 0);
                    bool cond2 = (l == 0) && (rows_i == 1);
                    if (cond1 || cond2) {
                        sub_image[ind] = 0;
                    }
                    ind++;
                }
            }
            size_t mid = jj + mask_width / 2 + rows[rows_i] * image_width;
            res[mid] = convolution_operation(sub_image);
        }
    }

    // 2. columns
    size_t columns[2] = { 0, image_width - 1 };
    for (size_t columns_i = 0; columns_i < 2; columns_i++) {
        for (size_t ii = 0; ii < image_height - mask_height + 1; ii++)
        {
            ind = 0;
            for (size_t l = 0; l < mask_height; l++)
            {
                for (size_t k = 0; k < mask_width; k++)
                {
                    size_t p = (ii + l) * image_width + k + columns[columns_i];
                    if (columns_i != 0)
                    {
                        p = p - 2;
                    }
                    sub_image[ind] = (image[p]);
                    bool cond1 = (k == 2) && (columns_i == 0);
                    bool cond2 = (k == 0) && (columns_i == 1);
                    if (cond1 || cond2) {
                        sub_image[ind] = 0;
                    }
                    ind++;
                }
                auto mid = (ii + mask_height / 2) * image_width + columns[columns_i];
                res[mid] = convolution_operation(sub_image);
            }
        }
    }
    // 3. rest of the pixels
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
    is_edge_distributed = true;

    double z_max = *std::max_element(sum_weights_per_section.begin(), sum_weights_per_section.end());
    double z_min = *std::min_element(sum_weights_per_section.begin(), sum_weights_per_section.end());
    min_max_ratio = z_min / z_max;
    if (min_max_ratio < _params.edge_distribution_min_max_ratio)
    {
        is_edge_distributed = false;
        AC_LOG(ERROR, "Edge distribution ratio ({min}" << z_min << "/" << z_max << "{max} = " << min_max_ratio << ") is too small; threshold= " << _params.edge_distribution_min_max_ratio);
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
    for (auto it = sum_weights_per_section.begin(); it != sum_weights_per_section.end(); ++it)
    {
        if (*it < _params.min_weighted_edge_per_section)
        {
            is_edge_distributed = false;
            break;
        }
    }
    if (!is_edge_distributed) {
        AC_LOG(DEBUG, "check_edge_distribution: weighted edge per section is too low:  ");
        for (auto it = sum_weights_per_section.begin(); it != sum_weights_per_section.end(); ++it)
        {
            AC_LOG(DEBUG, "    " << *it);
        }
        AC_LOG(DEBUG, "threshold is: " << _params.min_weighted_edge_per_section);
        return;
    }
    
}
bool optimizer::is_edge_distributed(z_frame_data& z, yuy2_frame_data& yuy)
{
    size_t num_of_sections = _params.num_of_sections_for_edge_distribution_x * _params.num_of_sections_for_edge_distribution_y;

    // depth frame
    AC_LOG(DEBUG, "... checking Z edge distribution");
    sum_per_section(z.sum_weights_per_section, z.section_map, z.weights, num_of_sections);
    //for debug 
    auto it = z.sum_weights_per_section.begin();
    AC_LOG(DEBUG, "    sum_per_section(z), section #0  " << *(it));
    AC_LOG(DEBUG, "    sum_per_section(z), section #1  " << *(it + 2));
    AC_LOG(DEBUG, "    sum_per_section(z), section #2  " << *(it + 1));
    AC_LOG(DEBUG, "    sum_per_section(z), section #3  " << *(it + 3));
    check_edge_distribution(z.sum_weights_per_section, z.min_max_ratio, z.is_edge_distributed);
    // yuy frame
    AC_LOG(DEBUG, "... checking YUY edge distribution");
    sum_per_section(yuy.sum_weights_per_section, yuy.section_map, yuy.edges_IDT, num_of_sections);
    //for debug 
    it = yuy.sum_weights_per_section.begin();
    AC_LOG(DEBUG, "    sum_per_section(yuy), section #0  " << *(it));
    AC_LOG(DEBUG, "    sum_per_section(yuy), section #1  " << *(it + 2));
    AC_LOG(DEBUG, "    sum_per_section(yuy), section #2  " << *(it + 1));
    AC_LOG(DEBUG, "    sum_per_section(yuy), section #3  " << *(it + 3));
    check_edge_distribution(yuy.sum_weights_per_section, yuy.min_max_ratio, yuy.is_edge_distributed);

    return  (z.is_edge_distributed && yuy.is_edge_distributed);
}

//%function[isBalanced, dirRatio1, perpRatio, dirRatio2, weightsPerDir] = isGradDirBalanced( frame, params )
static bool is_grad_dir_balanced( std::vector< double > const & weights,
                                  std::vector< double > const & directions,
                                  params const & _params,
                                  std::vector< double > * p_weights_per_dir,
                                  double * p_dir_ratio1 )
{
    //%weightsPerDir = sum( frame.weights .* (frame.dirPerPixel == [1:4]) );
    std::vector< double > weights_per_dir( 4, 0. );  // 4 = deg_non is number of directions
    for (auto dir = 0; dir < 4; ++dir)
    {
        for( auto ii = 0; ii < directions.size(); ++ii )
        {
            if( directions[ii] == dir + 1 )  // avoid direction 0
                weights_per_dir[dir] += weights[ii];
        }
    }
    if( p_weights_per_dir )
        *p_weights_per_dir = weights_per_dir;

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
    //%[maxVal, maxIx] = max( weightsPerDir );
    auto max_val = max_element( weights_per_dir.begin(), weights_per_dir.end() );
    auto max_ix = distance( weights_per_dir.begin(), max_val );
    //%ixMatch = mod( maxIx + 2, 4 );
    //%if ixMatch == 0
    //%    ixMatch = 4;
    //%end
    auto ix_match = (max_ix + 1) % 3;

    double dir_ratio1;
    //%if weightsPerDir( ixMatch ) < 1e-3 %Don't devide by zero...
    if (weights_per_dir.at(ix_match) < 1e-3) //%Don't devide by zero...
        dir_ratio1 = 1e6;
    else
        //%dirRatio1 = maxVal / weightsPerDir( ixMatch );
        dir_ratio1 = *max_val / weights_per_dir.at( ix_match );
    if( p_dir_ratio1 )
        *p_dir_ratio1 = dir_ratio1;

    //%if dirRatio1 > params.gradDirRatio
    if( dir_ratio1 > _params.grad_dir_ratio )
    {
        //%ixCheck = true(size(weightsPerDir));
        //%ixCheck([maxIx,ixMatch]) = false;
        //%[maxValPerp,~] = max(weightsPerDir(ixCheck));
        double max_val_perp = DBL_MIN;
        double min_val_perp = DBL_MAX;
        for (auto i = 0; i < 4; ++i)
        {
            if ((i != max_ix) && (i != ix_match))
            {
                if( max_val_perp < weights_per_dir[i] )
                    max_val_perp = weights_per_dir[i];
                if( min_val_perp > weights_per_dir[i] )
                    min_val_perp = weights_per_dir[i];
            }
        }

        //%perpRatio = maxVal/maxValPerp;
        auto perp_ratio = *max_val / max_val_perp;
        if( perp_ratio > _params.grad_dir_ratio_prep )
        {
            AC_LOG( DEBUG,
                    "... gradient direction is not balanced : " << dir_ratio1 << "; threshold is: "
                                                                << _params.grad_dir_ratio );
            return false;
        }
        if( min_val_perp < 1e-3 )  // % Don't devide by zero...
        {
            AC_LOG( DEBUG,
                    "... gradient direction is not balanced : " << dir_ratio1 << "; threshold is: "
                                                                << _params.grad_dir_ratio );
            return false;
        }

        //%dirRatio2 = maxValPerp / min( weightsPerDir( ixCheck ));
        double dir_ratio2 = max_val_perp / min_val_perp;
        //%if dirRatio2 > params.gradDirRatio
        if( dir_ratio2 > _params.grad_dir_ratio )
        {
            AC_LOG( DEBUG,
                    "... gradient direction is not balanced : " << dir_ratio1 << "; threshold is: "
                                                                << _params.grad_dir_ratio );
            return false;
        }
    }
    return true;
}

void optimizer::section_per_pixel(
    frame_data const& f,
    size_t const section_w,
    size_t const section_h,
    byte* const section_map
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

    assert(section_w * section_h <= 256);

    byte* section = section_map;
    for (size_t row = 0; row < f.height; row++)
    {
        size_t const section_y = row * section_h / f.height;  // note not a floating point division!
        for (size_t col = 0; col < f.width; col++)
        {
            size_t const section_x = col * section_w / f.width;  // note not a floating point division!
            //% sectionMap = gridY + gridX*params.numSectionsH;   TODO BUGBUGBUG!!!
            *section++ = byte(section_y + section_x * section_h);
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
    // use this matlab function to get gauss kernel with sigma=1: disp17(fspecial('gaussian',5,1))
    std::vector<double>  gaussian_kernel = { 0.0029690167439504968, 0.013306209891013651, 0.021938231279714643, 0.013306209891013651, 0.0029690167439504968,
        0.013306209891013651, 0.059634295436180138, 0.098320331348845769, 0.059634295436180138, 0.013306209891013651,
        0.021938231279714643, 0.098320331348845769, 0.16210282163712664, 0.098320331348845769, 0.021938231279714643,
        0.013306209891013651, 0.059634295436180138, 0.098320331348845769, 0.059634295436180138, 0.013306209891013651,
        0.0029690167439504968, 0.013306209891013651, 0.021938231279714643, 0.013306209891013651, 0.0029690167439504968
    };

    std::vector<uint8_t>::iterator yuy_iter = yuy.lum_frame.begin();
    std::vector<uint8_t>::iterator yuy_prev_iter = yuy.prev_lum_frame.begin();
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
diffIm = imgaussfilt(double(im1)-double(im2),params.moveGaussSigma);
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
    std::vector< byte > section_map_depth(_z.width * _z.height);
    std::vector< byte > section_map_rgb(_yuy.width * _yuy.height);

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
    _z.section_map = _z.section_map_depth_inside; // NOHA :: taken from preprocessDepth
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
    bool res_gradient = is_grad_dir_balanced( _z.weights,
                                              _z.directions,
                                              _params,
                                              &_z.sum_weights_per_direction,
                                              &_z.dir_ratio1 );

    //return((!res_movement) && res_edges && res_gradient);
    return(!res_movement);
}
