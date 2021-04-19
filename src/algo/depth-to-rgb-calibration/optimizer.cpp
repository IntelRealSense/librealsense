//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "optimizer.h"
#include <librealsense2/rsutil.h>
#include <algorithm>
#include <array>
#include "coeffs.h"
#include "cost.h"
#include "uvmap.h"
#include "k-to-dsm.h"
#include "debug.h"
#include "utils.h"

using namespace librealsense::algo::depth_to_rgb_calibration;


namespace
{
    std::vector<double> calc_intensity( std::vector<double> const & image1, std::vector<double> const & image2 )
    {
        std::vector<double> res( image1.size(), 0 );

        for(size_t i = 0; i < image1.size(); i++ )
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

        for (size_t i = 0; i < image_height - mask_height + 1; i++)
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


std::string optimizer::settings::to_string() const
{
    return librealsense::to_string()
        << '[' << ( is_manual_trigger ? "MANUAL" : "AUTO" ) << ' ' << hum_temp << "degC"
        << " digital gain="
        << (digital_gain == RS2_DIGITAL_GAIN_HIGH ? "high/long"
            : digital_gain == RS2_DIGITAL_GAIN_LOW ? "low/short"
                                                        : "??" )
        << " receiver gain=" << receiver_gain << ']';
}


optimizer::optimizer( settings const & s, bool debug_mode )
    : _settings( s )
    , _debug_mode( debug_mode )
{
    AC_LOG( DEBUG, "Optimizer settings are " << _settings.to_string() );
    if( _settings.is_manual_trigger )
        adjust_params_to_manual_mode();
    else
        adjust_params_to_auto_mode();
}

static std::vector< double > get_direction_deg(
    std::vector<double> const & gradient_x,
    std::vector<double> const & gradient_y
)
{
    std::vector<double> res( gradient_x.size(), deg_none );

    for(size_t i = 0; i < gradient_x.size(); i++ )
    {
        int closest = -1;
        auto angle = atan2( gradient_y[i], gradient_x[i] )* 180.f / M_PI;
        angle = angle < 0 ? 180 + angle : angle;
        auto dir = fmod( angle, 180 );


        res[i] = dir;
    }
    return res;
}
static std::vector< double > get_direction_deg2(
    std::vector<double> const& gradient_x,
    std::vector<double> const& gradient_y
    )
{
    std::vector<double> res(gradient_x.size(), deg_none);

    for (size_t i = 0; i < gradient_x.size(); i++)
    {
        int closest = -1;
        auto angle = atan2(gradient_y[i], gradient_x[i])*180.f  / M_PI;
        angle = angle < 0 ?  360+angle : angle;
        auto dir = fmod(angle, 360);


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

void zero_margin( std::vector< double > & gradient, size_t margin, size_t width, size_t height )
{
    auto it = gradient.begin();
    for(size_t m = 0; m < margin; ++m )
    {
        for(size_t i = 0; i < width; i++ )
        {
            // zero mask of 2nd row, and row before the last
            *( it + m * width + i ) = 0;
            *( it + width * ( height - m - 1 ) + i ) = 0;
        }
        for(size_t i = 0; i < height; i++ )
        {
            // zero mask of 2nd column, and column before the last
            *( it + i * width + m ) = 0;
            *( it + i * width + ( width - m - 1 ) ) = 0;
        }
    }
}

template < class T >
void sample_by_mask( std::vector< T > & filtered,
                     std::vector< T > const & origin,
                     std::vector< byte > const & valid_edge_by_ir,
                     size_t const width,
                     size_t const height )
{
    //%function [values] = sampleByMask(I,binMask)
    //%    % Extract values from image I using the binMask with the order being
    //%    % row and then column
    //%    I = I';
    //%    values = I(binMask');
    //end
    for(size_t x = 0; x < origin.size(); ++x )
        if( valid_edge_by_ir[x] )
            filtered.push_back( origin[x] );
}

template < class T >
void depth_filter( std::vector< T > & filtered,
                   std::vector< T > const & origin,
                   std::vector< byte > const & valid_edge_by_ir,
                   size_t const width,
                   size_t const height )
{
    // origin and valid_edge_by_ir are of same size
    for(size_t j = 0; j < width; j++ )
    {
        for(size_t i = 0; i < height; i++ )
        {
            auto idx = i * width + j;
            if( valid_edge_by_ir[idx] )
            {
                filtered.push_back( origin[idx] );
            }
        }
    }
}

void grid_xy(
    std::vector<double>& gridx,
    std::vector<double>& gridy,
    size_t width,
    size_t height)
{
    for (size_t i = 1; i <= height; i++)
    {
        for (size_t j = 1; j <= width; j++)
        {
            gridx.push_back( double( j ) );
            gridy.push_back( double( i ) );
        }
    }
}

template<class T>
std::vector< double > interpolation( std::vector< T > const & grid_points,
                                     std::vector< double > const x[],
                                     std::vector< double > const y[],
                                     size_t dim,
                                     size_t valid_size,
                                     size_t valid_width )
{
    std::vector<double> local_interp;
    local_interp.reserve( valid_size * dim );
    auto iedge_it = grid_points.begin();
    std::vector<double>::const_iterator loc_reg_x[4];
    std::vector<double>::const_iterator loc_reg_y[4];
    for( auto i = 0; i < dim; i++ )
    {
        loc_reg_x[i] = x[i].begin();
        loc_reg_y[i] = y[i].begin();
    }
    for (size_t i = 0; i < valid_size; i++)
    {
        for (size_t k = 0; k < dim; k++)
        {
            auto idx = *(loc_reg_x[k] + i) - 1;
            auto idy = *(loc_reg_y[k] + i) - 1;
            //assert(_ir.width * idy + idx <= _ir.width * _ir.height);
            auto val = *( iedge_it + size_t( valid_width * idy + idx ) );  // find value in iEdge
            local_interp.push_back(val);
        }
    }
    return local_interp;
}
std::vector<uint8_t> is_suppressed(std::vector<double> const & local_edges, size_t valid_size)
{
    std::vector<uint8_t> is_supressed;
    auto loc_edg_it = local_edges.begin();
    for (size_t i = 0; i < valid_size; i++)
    {
        //isSupressed = localEdges(:,3) >= localEdges(:,2) & localEdges(:,3) >= localEdges(:,4);
        auto vec2 = *(loc_edg_it + 1);
        auto vec3 = *(loc_edg_it + 2);
        auto vec4 = *(loc_edg_it + 3);
        loc_edg_it += 4;
        bool res = (vec3 >= vec2) && (vec3 >= vec4);
        is_supressed.push_back(res);
    }
    return is_supressed;
}

static std::vector< double > depth_mean( std::vector< double > const & local_x,
                                         std::vector< double > const & local_y )
{
    std::vector<double> res;
    res.reserve( local_x.size() );
    size_t size = local_x.size() / 2;
    auto itx = local_x.begin();
    auto ity = local_y.begin();
    for (size_t i = 0; i < size; i++, ity += 2, itx += 2)
    {
        double valy = (*ity + *(ity + 1)) / 2;
        double valx = (*itx + *(itx + 1)) / 2;
        res.push_back(valy);
        res.push_back(valx);
    }

    return res;
}

static std::vector< double > sum_gradient_depth( std::vector< double > const & gradient,
                                                 std::vector< double > const & direction_per_pixel )
{
    std::vector<double> res;
    size_t size = direction_per_pixel.size() / 2;
    res.reserve( size );
    auto it_dir = direction_per_pixel.begin();
    auto it_grad = gradient.begin();
    for (size_t i = 0; i < size; i++, it_dir+=2, it_grad+=2)
    {
        // normalize : res = val/sqrt(row_sum)
        auto rorm_dir1 = *it_dir / sqrt(abs(*it_dir) + abs(*(it_dir + 1)));
        auto rorm_dir2 = *(it_dir+1) / sqrt(abs(*it_dir) + abs(*(it_dir + 1)));
        auto val = abs(*it_grad * rorm_dir1 + *(it_grad + 1) * rorm_dir2);
        res.push_back(val);
    }
    return res;
}


std::vector< byte > find_valid_depth_edges( std::vector< double > const & grad_in_direction,
                                            std::vector< byte > const & is_supressed,
                                            std::vector< double > const & values_for_subedges,
                                            std::vector< double > const & ir_local_edges,
                                            const params & p )
{
    std::vector< byte > res;
    res.reserve( grad_in_direction.size() );
    //%validEdgePixels = zGradInDirection > params.gradZTh & isSupressed & zValuesForSubEdges > 0;
    if (p.use_enhanced_preprocessing)
    {
        for (size_t i = 0; i < grad_in_direction.size(); i++)
        {
            bool cond1 = (grad_in_direction[i] > p.grad_z_low_th  && ir_local_edges[i * 4 + 2] > p.grad_ir_high_th) ||
                         (grad_in_direction[i] > p.grad_z_high_th && ir_local_edges[i * 4 + 2] > p.grad_ir_low_th);

            bool cond2 = is_supressed[i];
            bool cond3 = values_for_subedges[i] > 0;
            res.push_back(cond1 && cond2 && cond3);
        }
    }
    else
    {
        for (size_t i = 0; i < grad_in_direction.size(); i++)
        {
            bool cond1 = grad_in_direction[i] > p.grad_z_threshold;
            bool cond2 = is_supressed[i];
            bool cond3 = values_for_subedges[i] > 0;
            res.push_back(cond1 && cond2 && cond3);
        }
    }
   
    return res;
}


static std::vector< double > find_local_values_min( std::vector< double > const & local_values )
{
    std::vector<double> res;
    size_t size = local_values.size() / 4;
    res.reserve( size );
    auto it = local_values.begin();
    for (size_t i = 0; i < size; i++)
    {
        auto val1 = *it;
        auto val2 = *(it + 1);
        auto val3 = *(it + 2);
        auto val4 = *(it + 3);
        it += 4;
        double res_val = std::min(val1, std::min(val2, std::min(val3, val4)));
        res.push_back(res_val);
    }
    return res;
}


void optimizer::set_z_data( std::vector< z_t > && depth_data,
                            rs2_intrinsics_double const & depth_intrinsics,
                            rs2_dsm_params const & dsm_params,
                            algo_calibration_info const & cal_info,
                            algo_calibration_registers const & cal_regs, float depth_units )
{
    _original_dsm_params = dsm_params;
    _k_to_DSM = std::make_shared<k_to_DSM>(dsm_params, cal_info, cal_regs, _params.max_scaling_step);

    /*[zEdge,Zx,Zy] = OnlineCalibration.aux.edgeSobelXY(uint16(frame.z),2); % Added the second input - margin to zero out
    [iEdge,Ix,Iy] = OnlineCalibration.aux.edgeSobelXY(uint16(frame.i),2); % Added the second input - margin to zero out
    validEdgePixelsByIR = iEdge>params.gradITh; */
    _params.set_depth_resolution(depth_intrinsics.width, depth_intrinsics.height, _settings.digital_gain);
    _z.width = depth_intrinsics.width;
    _z.height = depth_intrinsics.height;
    _z.orig_intrinsics = depth_intrinsics;
    _z.orig_dsm_params = dsm_params;
    _z.depth_units = depth_units;

    _z.frame = std::move(depth_data);

    auto z_gradient_x = calc_vertical_gradient(_z.frame, depth_intrinsics.width, depth_intrinsics.height);
    auto z_gradient_y = calc_horizontal_gradient( _z.frame, depth_intrinsics.width, depth_intrinsics.height );
    auto ir_gradient_x = calc_vertical_gradient( _ir.ir_frame, depth_intrinsics.width, depth_intrinsics.height );
    auto ir_gradient_y = calc_horizontal_gradient( _ir.ir_frame, depth_intrinsics.width, depth_intrinsics.height );

    // set margin of 2 pixels to 0
    zero_margin( z_gradient_x, 2, _z.width, _z.height );
    zero_margin( z_gradient_y, 2, _z.width, _z.height );
    zero_margin( ir_gradient_x, 2, _z.width, _z.height );
    zero_margin( ir_gradient_y, 2, _z.width, _z.height );

    auto ir_edges = calc_intensity( ir_gradient_x, ir_gradient_y );
    std::vector< byte > valid_edge_pixels_by_ir;
    {
        auto z_edges = calc_intensity( z_gradient_x, z_gradient_y );
        valid_edge_pixels_by_ir.reserve( ir_edges.size() );
        
        for( auto ir = ir_edges.begin(), z = z_edges.begin();
             ir < ir_edges.end() && z < z_edges.end();
             ir++, z++ )
        {
            bool valid_edge;
            if( _params.use_enhanced_preprocessing )
            {
                valid_edge = ( *ir > _params.grad_ir_high_th && *z > _params.grad_z_low_th )
                          || ( *ir > _params.grad_ir_low_th && *z > _params.grad_z_high_th );
            }
            else
            {
                valid_edge = ( *ir > _params.grad_ir_threshold );
            }
            valid_edge_pixels_by_ir.push_back( valid_edge );
        }

        if( _debug_mode )
            _z.edges = std::move( z_edges );
    }
        

    /*sz = size(frame.i);
    [gridX,gridY] = meshgrid(1:sz(2),1:sz(1)); % gridX/Y contains the indices of the pixels
    sectionMapDepth = OnlineCalibration.aux.sectionPerPixel(params);
    */
    // Get a map for each pixel to its corresponding section
    std::vector< byte > section_map_depth;
    section_map_depth.resize(_z.width * _z.height);
    size_t const section_w = _params.num_of_sections_for_edge_distribution_x;  //% params.numSectionsH
    size_t const section_h = _params.num_of_sections_for_edge_distribution_y;  //% params.numSectionsH
    section_per_pixel(_z, section_w, section_h, section_map_depth.data());

    //%locRC = [sampleByMask( gridY, validEdgePixelsByIR ), sampleByMask( gridX, validEdgePixelsByIR )];
    //%sectionMapValid = sampleByMask( sectionMapDepth, validEdgePixelsByIR );
    //%IxValid = sampleByMask( Ix, validEdgePixelsByIR );
    //%IyValid = sampleByMask( Iy, validEdgePixelsByIR );

    std::vector< double > valid_location_rc_x;
    std::vector< double > valid_location_rc_y;
    {
        std::vector< double > grid_x, grid_y;
        grid_xy( grid_x, grid_y, _z.width, _z.height );
        sample_by_mask( valid_location_rc_x, grid_x, valid_edge_pixels_by_ir, _z.width, _z.height );
        sample_by_mask( valid_location_rc_y, grid_y, valid_edge_pixels_by_ir, _z.width, _z.height );
    }
    std::vector< byte > valid_section_map;
    std::vector< double > valid_gradient_x;
    std::vector< double > valid_gradient_y;

    sample_by_mask( valid_section_map,  section_map_depth, valid_edge_pixels_by_ir, _z.width, _z.height );
    sample_by_mask( valid_gradient_x, ir_gradient_x, valid_edge_pixels_by_ir, _z.width, _z.height );
    sample_by_mask( valid_gradient_y, ir_gradient_y, valid_edge_pixels_by_ir, _z.width, _z.height );

    auto itx = valid_location_rc_x.begin();
    auto ity = valid_location_rc_y.begin();

    std::vector< double > valid_location_rc;
    for (size_t i = 0; i < valid_location_rc_x.size(); i++)
    {
        auto x = *(itx + i);
        auto y = *(ity + i);
        valid_location_rc.push_back(y);
        valid_location_rc.push_back(x);
    }

    /*
    directionInDeg = atan2d(IyValid,IxValid);
    directionInDeg(directionInDeg<0) = directionInDeg(directionInDeg<0) + 360;
    [~,directionIndex] = min(abs(directionInDeg - [0:45:315]),[],2); % Quantize the direction to 4 directions (don't care about the sign)
    */

    if( _debug_mode )
        _ir.direction_deg = get_direction_deg2( valid_gradient_x, valid_gradient_y );

    std::vector< direction > dirs = get_direction2( valid_gradient_x, valid_gradient_y );

    /*dirsVec = [0,1; 1,1; 1,0; 1,-1]; % These are the 4 directions
    dirsVec = [dirsVec;-dirsVec];
    if 1
        % Take the right direction
        dirPerPixel = dirsVec(directionIndex,:);
        localRegion = locRC + dirPerPixel.*reshape(vec(-2:1),1,1,[]);
        localEdges = squeeze(interp2(iEdge,localRegion(:,2,:),localRegion(:,1,:)));
        isSupressed = localEdges(:,3) >= localEdges(:,2) & localEdges(:,3) >= localEdges(:,4);

        fraqStep = (-0.5*(localEdges(:,4)-localEdges(:,2))./(localEdges(:,4)+localEdges(:,2)-2*localEdges(:,3))); % The step we need to move to reach the subpixel gradient i nthe gradient direction
        fraqStep((localEdges(:,4)+localEdges(:,2)-2*localEdges(:,3))==0) = 0;

        locRCsub = locRC + fraqStep.*dirPerPixel;*/
    double directions[8][2] = { {0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1} };

    std::vector< double > direction_per_pixel;
    std::vector< double > direction_per_pixel_x;  //used later when finding valid direction per pixel

    for(size_t i = 0; i < dirs.size(); i++ )
    {
        int idx = dirs[i];
        direction_per_pixel.push_back(directions[idx][0]);
        direction_per_pixel.push_back(directions[idx][1]);
        direction_per_pixel_x.push_back(directions[idx][0]);
    }
    double vec[4] = { -2,-1,0,1 }; // one pixel along gradient direction, 2 pixels against gradient direction

    auto loc_it = valid_location_rc.begin();
    auto dir_pp_it = direction_per_pixel.begin();

    std::vector< double > local_region[4];
    for( auto k = 0; k < 4; k++ )
    {
        local_region[k].reserve( direction_per_pixel.size() );
        for(size_t i = 0; i < direction_per_pixel.size(); i++ )
        {
            double val = *( loc_it + i ) + *( dir_pp_it + i ) * vec[k];
            local_region[k].push_back( val );
        }
    }
    std::vector< double > local_region_x[4];
    std::vector< double > local_region_y[4];
    for( auto k = 0; k < 4; k++ )
    {
        local_region_x[k].reserve( 2 * valid_location_rc_x.size() );
        local_region_y[k].reserve( 2 * valid_location_rc_x.size() );
        for(size_t i = 0; i < 2 * valid_location_rc_x.size(); i++ )
        {
            local_region_y[k].push_back( *( local_region[k].begin() + i ) );
            i++;
            local_region_x[k].push_back( *( local_region[k].begin() + i ) );
        }
    }
    // interpolation 
    _ir.local_edges = interpolation(ir_edges,local_region_x, local_region_y, 4,  valid_location_rc_x.size(), _ir.width);

    // is suppressed
    _ir.is_supressed = is_suppressed(_ir.local_edges,  valid_location_rc_x.size());


    /*fraqStep = (-0.5*(localEdges(:,4)-localEdges(:,2))./(localEdges(:,4)+localEdges(:,2)-2*localEdges(:,3))); % The step we need to move to reach the subpixel gradient i nthe gradient direction
       fraqStep((localEdges(:,4)+localEdges(:,2)-2*localEdges(:,3))==0) = 0;

       locRCsub = locRC + fraqStep.*dirPerPixel;

       % Calculate the Z gradient for thresholding
       localZx = squeeze(interp2(Zx,localRegion(:,2,2:3),localRegion(:,1,2:3)));
       localZy = squeeze(interp2(Zy,localRegion(:,2,2:3),localRegion(:,1,2:3)));
       zGrad = [mean(localZy,2) ,mean(localZx,2)];
       zGradInDirection = abs(sum(zGrad.*normr(dirPerPixel),2));
       % Take the z value of the closest part of the edge
       localZvalues = squeeze(interp2(frame.z,localRegion(:,2,:),localRegion(:,1,:)));

       zValuesForSubEdges = min(localZvalues,[],2);
       edgeSubPixel = fliplr(locRCsub);% From Row-Col to XY*/

    std::vector< double > ::iterator loc_edg_it = _ir.local_edges.begin();
    auto valid_loc_rc = valid_location_rc.begin(); // locRC
    auto dir_per_pixel_it = direction_per_pixel.begin(); // dirPerPixel

    std::vector< double > edge_sub_pixel_x;
    std::vector< double > edge_sub_pixel_y;
    std::vector< double > fraq_step;          // debug only
    std::vector< double > local_rc_subpixel;  // debug only
    std::vector< double > edge_sub_pixel;     // debug only

    for(size_t i = 0; i < valid_location_rc_x.size(); i++ )
    {
        double vec2 = *(loc_edg_it + 1);
        double vec3 = *(loc_edg_it + 2);
        double vec4 = *(loc_edg_it + 3);
        loc_edg_it += 4;

        // The step we need to move to reach the subpixel gradient in the gradient direction
        //%fraqStep = (-0.5*(localEdges(:,4) - localEdges(:,2)). / (localEdges(:,4) + localEdges(:,2) - 2 * localEdges(:,3)));
        //%fraqStep( (localEdges(:,4) + localEdges(:,2) - 2 * localEdges(:,3)) == 0 ) = 0;
        double const denom = vec4 + vec2 - 2 * vec3;
        double const res = ( denom == 0 ) ? 0 : ( -0.5 * ( vec4 - vec2 ) / denom );

        auto valx = *valid_loc_rc + *dir_per_pixel_it * res;
        valid_loc_rc++;
        dir_per_pixel_it++;
        auto valy = *valid_loc_rc + *dir_per_pixel_it * res;
        valid_loc_rc++;
        dir_per_pixel_it++;

        if( _debug_mode )
        {
            fraq_step.push_back( res );
            local_rc_subpixel.push_back( valx );
            local_rc_subpixel.push_back( valy );
            edge_sub_pixel.push_back( valy );
            edge_sub_pixel.push_back( valx );
        }
        edge_sub_pixel_x.push_back(valy);
        edge_sub_pixel_y.push_back(valx);
    }
    _z.edge_sub_pixel = std::move( edge_sub_pixel );
    _z.local_rc_subpixel = std::move( local_rc_subpixel );
    _ir.fraq_step = std::move( fraq_step );

    std::vector< double > grad_in_direction;
    {
        std::vector< double > local_x;
        std::vector< double > local_y;
        std::vector< double > gradient;

        local_x = interpolation( z_gradient_x,
                                 local_region_x + 1,
                                 local_region_y + 1,
                                 2,
                                 valid_location_rc_x.size(),
                                 _z.width );
        local_y = interpolation( z_gradient_y,
                                 local_region_x + 1,
                                 local_region_y + 1,
                                 2,
                                 valid_location_rc_x.size(),
                                 _z.width );
        gradient = depth_mean( local_x, local_y );
        grad_in_direction = sum_gradient_depth( gradient, direction_per_pixel );
        if( _debug_mode )
        {
            _z.local_x = std::move( local_x );
            _z.local_y = std::move( local_y );
            _z.gradient = std::move( gradient );
        }
    }
    std::vector< double > values_for_subedges;
    {
        std::vector< double > local_values = interpolation( _z.frame,
                                                            local_region_x,
                                                            local_region_y,
                                                            4,
                                                            valid_location_rc_x.size(),
                                                            _z.width );
        values_for_subedges = find_local_values_min( local_values );
        if( _debug_mode )
            _z.local_values = std::move( local_values );
    }

    //_params.alpha;
    /* validEdgePixels = zGradInDirection > params.gradZTh & isSupressed & zValuesForSubEdges > 0;

   zGradInDirection = zGradInDirection(validEdgePixels);
   edgeSubPixel = edgeSubPixel(validEdgePixels,:);
   zValuesForSubEdges = zValuesForSubEdges(validEdgePixels);
   dirPerPixel = dirPerPixel(validEdgePixels);
   sectionMapDepth = sectionMapValid(validEdgePixels);
   directionIndex = directionIndex(validEdgePixels);
   directionIndex(directionIndex>4) = directionIndex(directionIndex>4)-4;% Like taking abosoulte value on the direction
   */
    _z.supressed_edges = find_valid_depth_edges(  grad_in_direction,
                                                 _ir.is_supressed,
                                                 values_for_subedges,
                                                 _ir.local_edges,
                                                 _params);

    if( _debug_mode )
    {
        depth_filter( _z.grad_in_direction_valid,
                      grad_in_direction,
                      _z.supressed_edges,
                      1,
                      _z.supressed_edges.size() );
    }

    std::vector< double > valid_edge_sub_pixel_x;
    std::vector< double > valid_edge_sub_pixel_y;
    depth_filter( valid_edge_sub_pixel_x,
                  edge_sub_pixel_x,
                  _z.supressed_edges,
                  1,
                  _z.supressed_edges.size() );
    depth_filter( valid_edge_sub_pixel_y,
                  edge_sub_pixel_y,
                  _z.supressed_edges,
                  1,
                  _z.supressed_edges.size() );

    {
        std::vector< double > valid_values_for_subedges;
        depth_filter( valid_values_for_subedges,  // out
                      values_for_subedges,        // in
                      _z.supressed_edges,
                      1,
                      _z.supressed_edges.size() );
        values_for_subedges = valid_values_for_subedges;
    }

    /* weights = min(max(zGradInDirection - params.gradZTh,0),params.gradZMax - params.gradZTh);
    if params.constantWeights
        weights(:) = params.constantWeightsValue;
    end
    xim = edgeSubPixel(:,1)-1;
    yim = edgeSubPixel(:,2)-1;

    subPoints = [xim,yim,ones(size(yim))];
    vertices = subPoints*(pinv(params.Kdepth)').*zValuesForSubEdges/single(params.zMaxSubMM);

    [uv,~,~] =
    OnlineCalibration.aux.projectVToRGB(vertices,params.rgbPmat,params.Krgb,params.rgbDistort);
    isInside = OnlineCalibration.aux.isInsideImage(uv,params.rgbRes);

    xim = xim(isInside);
    yim = yim(isInside);
    zValuesForSubEdges = zValuesForSubEdges(isInside);
    zGradInDirection = zGradInDirection(isInside);
    directionIndex = directionIndex(isInside);
    weights = weights(isInside);
    vertices = vertices(isInside,:);
    sectionMapDepth = sectionMapDepth(isInside);*/
    k_matrix k = depth_intrinsics;
    matrix_3x3 k_depth_pinv = { 0 };
    pinv_3x3( k.as_3x3().rot, k_depth_pinv.rot );
    _z.k_depth_pinv = k_depth_pinv;

    std::vector< byte > is_inside;
    {
        std::vector< double3 > vertices_all;
        {
            std::vector< double > sub_points;
            sub_points.reserve( valid_edge_sub_pixel_x.size() * 3 );
            {
                std::vector< double > valid_edge_sub_pixel;
                if( _debug_mode )
                    valid_edge_sub_pixel.reserve( valid_edge_sub_pixel_x.size() );
                for(size_t i = 0; i < valid_edge_sub_pixel_x.size(); i++ )
                {
                    if( _debug_mode )
                    {
                        valid_edge_sub_pixel.push_back( *( valid_edge_sub_pixel_x.begin() + i ) );
                        valid_edge_sub_pixel.push_back( *( valid_edge_sub_pixel_y.begin() + i ) );
                    }
                    // subPoints : subPoints = [xim,yim,ones(size(yim))];
                    sub_points.push_back( *( valid_edge_sub_pixel_x.begin() + i ) - 1 );
                    sub_points.push_back( *( valid_edge_sub_pixel_y.begin() + i ) - 1 );
                    sub_points.push_back( 1 );
                }
                if( _debug_mode )
                    _z.valid_edge_sub_pixel = std::move( valid_edge_sub_pixel );
            }

            vertices_all.reserve( valid_edge_sub_pixel_x.size() );
            for(size_t i = 0; i < sub_points.size(); i += 3 )
            {
                //% vertices = subPoints * pinv(params.Kdepth)' .* zValuesForSubEdges /
                //params.zMaxSubMM;
                double sub_points_mult[3];
                double x = sub_points[i];
                double y = sub_points[i + 1];
                double z = sub_points[i + 2];
                for( auto jj = 0; jj < 3; jj++ )
                {
                    sub_points_mult[jj] = x * k_depth_pinv.rot[3 * jj + 0]
                                        + y * k_depth_pinv.rot[3 * jj + 1]
                                        + z * k_depth_pinv.rot[3 * jj + 2];
                }
                auto z_value_for_subedge = values_for_subedges[i / 3];
                auto val1 = sub_points_mult[0] * z_value_for_subedge / _params.max_sub_mm_z;
                auto val2 = sub_points_mult[1] * z_value_for_subedge / _params.max_sub_mm_z;
                auto val3 = sub_points_mult[2] * z_value_for_subedge / _params.max_sub_mm_z;
                vertices_all.push_back( { val1, val2, val3 } );
            }
            if( _debug_mode )
                _z.sub_points = std::move( sub_points );
        }
        std::vector< double2 > uvmap = get_texture_map( vertices_all,
                                                        _original_calibration,
                                                        _original_calibration.calc_p_mat() );
        for(size_t i = 0; i < uvmap.size(); i++ )
        {
            //%isInside = xy(:,1) >= 0 & ...
            //%           xy(:,1) <= res(2) - 1 & ...
            //%           xy(:,2) >= 0 & ...
            //%           xy(:,2) <= res(1) - 1;
            bool cond_x = ( uvmap[i].x >= 0 ) && ( uvmap[i].x <= _yuy.width - 1 );
            bool cond_y = ( uvmap[i].y >= 0 ) && ( uvmap[i].y <= _yuy.height - 1 );
            is_inside.push_back( cond_x && cond_y );
        }
        depth_filter( _z.vertices, vertices_all, is_inside, 1, is_inside.size() );
        if( _debug_mode )
            _z.uvmap = std::move( uvmap );
    }

    if( _debug_mode )
        depth_filter( _z.valid_direction_per_pixel,
                      direction_per_pixel_x,
                      _z.supressed_edges,
                      1,
                      _z.supressed_edges.size() );

    {
        std::vector< byte > z_valid_section_map;
        depth_filter( z_valid_section_map,
                      valid_section_map,
                      _z.supressed_edges,
                      1,
                      _z.supressed_edges.size() );
        depth_filter( _z.section_map, z_valid_section_map, is_inside, 1, is_inside.size() );
        if( _debug_mode )
            _z.valid_section_map = std::move( z_valid_section_map );
    }

    {
        std::vector< double > valid_directions;
        {
            std::vector< double > edited_ir_directions;
            edited_ir_directions.reserve( dirs.size() );
            for(size_t i = 0; i < dirs.size(); i++ )
            {
                auto val = double( *( dirs.begin() + i ) );
                val = val + 1;  // +1 to align with matlab
                val = val > 4 ? val - 4 : val;
                edited_ir_directions.push_back( val );
            }
            depth_filter( valid_directions,
                          edited_ir_directions,
                          _z.supressed_edges,
                          1,
                          _z.supressed_edges.size() );
        }
        depth_filter( _z.directions, valid_directions, is_inside, 1, is_inside.size() );
        if( _debug_mode )
            _z.valid_directions = std::move( valid_directions );
    }

    /*xim = xim(isInside);
    yim = yim(isInside); 
    zValuesForSubEdges = zValuesForSubEdges(isInside);
    zGradInDirection = zGradInDirection(isInside);
    directionIndex = directionIndex(isInside);
    weights = weights(isInside);
    vertices = vertices(isInside,:);
    sectionMapDepth = sectionMapDepth(isInside);*/
    //std::vector<double> weights;

    {
        std::vector< double > valid_weights;
        valid_weights.reserve( is_inside.size() );
        for(size_t i = 0; i < is_inside.size(); i++ )
            valid_weights.push_back( _params.constant_weights );
        depth_filter( _z.weights, valid_weights, is_inside, 1, is_inside.size() );
    }

    transform(valid_edge_sub_pixel_x.begin(), valid_edge_sub_pixel_x.end(), valid_edge_sub_pixel_x.begin(), bind2nd(std::plus<double>(), -1.0));
    transform(valid_edge_sub_pixel_y.begin(), valid_edge_sub_pixel_y.end(), valid_edge_sub_pixel_y.begin(), bind2nd(std::plus<double>(), -1.0));
    depth_filter(_z.subpixels_x, valid_edge_sub_pixel_x, is_inside, 1, is_inside.size());
    depth_filter(_z.subpixels_y, valid_edge_sub_pixel_y, is_inside, 1, is_inside.size());

    depth_filter(_z.closest, values_for_subedges, is_inside, 1, is_inside.size());

    _z.relevant_pixels_image.resize(_z.width * _z.height, 0);
    std::vector< double > sub_pixel_x = _z.subpixels_x;
    std::vector< double > sub_pixel_y = _z.subpixels_y;

    transform(_z.subpixels_x.begin(), _z.subpixels_x.end(), sub_pixel_x.begin(), [](double x) {return round(x + 1); });
    transform(_z.subpixels_y.begin(), _z.subpixels_y.end(), sub_pixel_y.begin(), [](double x) {return round(x + 1); });

    for (size_t i = 0; i < sub_pixel_x.size(); i++)
    {
        auto x = sub_pixel_x[i];
        auto y = sub_pixel_y[i];

        _z.relevant_pixels_image[size_t( ( y - 1 ) * _z.width + x - 1 )] = 1;
    }

    if( _debug_mode )
    {
        _z.gradient_x = std::move( z_gradient_x );
        _z.gradient_y = std::move( z_gradient_y );
        _ir.gradient_x = std::move( ir_gradient_x );
        _ir.gradient_y = std::move( ir_gradient_y );
        _ir.edges = std::move( ir_edges );
        _ir.valid_edge_pixels_by_ir = std::move( valid_edge_pixels_by_ir );
        _z.section_map_depth = std::move( section_map_depth );
        _ir.valid_section_map = std::move( valid_section_map );
        _ir.valid_gradient_x = std::move( valid_gradient_x );
        _ir.valid_gradient_y = std::move( valid_gradient_y );
        _ir.valid_location_rc_x = std::move( valid_location_rc_x );
        _ir.valid_location_rc_y = std::move( valid_location_rc_y );
        _ir.valid_location_rc = std::move( valid_location_rc );
        _ir.directions = std::move( dirs );
        _ir.direction_per_pixel = std::move( direction_per_pixel );
        _z.grad_in_direction = std::move( grad_in_direction );
        _z.values_for_subedges = std::move( values_for_subedges );

        for (auto i = 0; i < 4; i++)
        {
            _ir.local_region[i] = std::move( local_region[i] );
            _ir.local_region_x[i] = std::move( local_region_x[i] );
            _ir.local_region_y[i] = std::move( local_region_y[i] );
        }

        _z.is_inside = std::move( is_inside );
        _z.subpixels_x_round = std::move( sub_pixel_x );
        _z.subpixels_y_round = std::move( sub_pixel_y );
    }
}


void optimizer::set_yuy_data(
    std::vector< yuy_t > && yuy_data,
    std::vector< yuy_t > && prev_yuy_data,
    std::vector< yuy_t > && last_successful_yuy_data,
    calib const & calibration
)
{
    _original_calibration = calibration;
    _original_calibration.model = RS2_DISTORTION_BROWN_CONRADY; //change to ac model
    _yuy.width = calibration.width;
    _yuy.height = calibration.height;
    _params.set_rgb_resolution( _yuy.width, _yuy.height );

    _yuy.orig_frame = std::move( yuy_data );
    _yuy.prev_frame = std::move( prev_yuy_data );
    _yuy.last_successful_frame = std::move( last_successful_yuy_data );

    std::vector< uint8_t > lum_frame;
    std::vector< uint8_t > prev_lum_frame;
    std::vector< uint8_t > last_successful_lum_frame;

    lum_frame = get_luminance_from_yuy2( _yuy.orig_frame );
    prev_lum_frame = get_luminance_from_yuy2( _yuy.prev_frame );

    auto edges = calc_edges( lum_frame, _yuy.width, _yuy.height );

    {
        auto prev_edges = calc_edges( prev_lum_frame, _yuy.width, _yuy.height );

        _yuy.movement_from_prev_frame
            = is_movement_in_images( { prev_edges, prev_lum_frame },
                                     { edges, lum_frame },
                                     _debug_mode ? &_yuy.debug.movement_result : nullptr,
                                     _params.move_thresh_pix_val,
                                     _params.move_threshold_pix_num,
                                     _yuy.width, _yuy.height );
    }

    AC_LOG( DEBUG,
            "    previous calibration image "
                << ( last_successful_yuy_data.empty() ? "was NOT supplied" : "supplied" ) );
    if( ! _settings.is_manual_trigger && ! _yuy.last_successful_frame.empty() )
    {
        last_successful_lum_frame = get_luminance_from_yuy2( _yuy.last_successful_frame );
        auto last_successful_edges = calc_edges( last_successful_lum_frame, _yuy.width, _yuy.height );

        _yuy.movement_from_last_success = is_movement_in_images(
            { last_successful_edges, last_successful_lum_frame },
            { edges, lum_frame },
            _debug_mode ? &_yuy.debug.movement_prev_valid_result : nullptr,
            _params.move_last_success_thresh_pix_val,
            _params.move_last_success_thresh_pix_num,
            _yuy.width, _yuy.height );
    }
    else
        _yuy.movement_from_last_success = true;

    _yuy.edges_IDT = blur_edges( edges, _yuy.width, _yuy.height );
    _yuy.edges_IDTx = calc_vertical_gradient( _yuy.edges_IDT, _yuy.width, _yuy.height );
    _yuy.edges_IDTy = calc_horizontal_gradient( _yuy.edges_IDT, _yuy.width, _yuy.height );

    // Get a map for each pixel to its corresponding section
    std::vector< byte > section_map_rgb( _yuy.width * _yuy.height );
    section_per_pixel( _yuy,
                       _params.num_of_sections_for_edge_distribution_x,  //% params.numSectionsH
                       _params.num_of_sections_for_edge_distribution_y,  //% params.numSectionsV
                       section_map_rgb.data() );

    // remove pixels in section map where rgbEdge <= params.gradRgbTh (see preprocessRGB)
    double const gradRgbTh = 15. * 1280 / _yuy.width;
    int i = 0;
    _yuy.section_map_edges.reserve( edges.size() );
    for( auto it = edges.begin(); it != edges.end(); ++it, ++i )
    {
        if( *it > gradRgbTh )
            _yuy.section_map_edges.push_back( section_map_rgb[i] );
    }
    _yuy.section_map_edges.shrink_to_fit();
    AC_LOG( DEBUG, "    " << _yuy.section_map_edges.size() << " pixels with a relevant edge" );

    if( _debug_mode )
    {
        _yuy.debug.lum_frame = std::move( lum_frame );
        _yuy.debug.prev_lum_frame = std::move( prev_lum_frame );
        _yuy.debug.last_successful_lum_frame = std::move( last_successful_lum_frame );

        _yuy.debug.edges = std::move( edges );
    }
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
}

calib optimizer::decompose_p_mat(p_matrix p)
{
    auto calib = decompose(p, _original_calibration);
    return calib;
}


rs2_intrinsics_double optimizer::get_new_z_intrinsics_from_new_calib(const rs2_intrinsics_double& orig, const calib & new_c, const calib & orig_c)
{
    rs2_intrinsics_double res;
    res = orig;
    res.fx = res.fx / new_c.k_mat.get_fx()*orig_c.k_mat.get_fx();
    res.fy = res.fy / new_c.k_mat.get_fy()*orig_c.k_mat.get_fy();

    return res;
}

std::vector< direction > optimizer::get_direction( std::vector<double> gradient_x, std::vector<double> gradient_y )
{
    std::vector<direction> res( gradient_x.size(), deg_none );

    std::map<int, direction> angle_dir_map = { {0, deg_0}, {45,deg_45} , {90,deg_90}, {135,deg_135} };

    for(size_t i = 0; i < gradient_x.size(); i++ )
    {
        int closest = -1;
        auto angle = atan2( gradient_y[i], gradient_x[i] )* 180.f / M_PI;
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
std::vector< direction > optimizer::get_direction2(std::vector<double> gradient_x, std::vector<double> gradient_y)
{
    std::vector<direction> res(gradient_x.size(), deg_none);
    
    std::map<int, direction> angle_dir_map = { {0, deg_0}, {45,deg_45} , {90,deg_90}, {135,deg_135} , { 180,deg_180 }, { 225,deg_225 }, { 270,deg_270 }, { 315,deg_315 } };



    for (size_t i = 0; i < gradient_x.size(); i++)
    {
        int closest = -1;
        auto angle = atan2(gradient_y[i], gradient_x[i]) * 180.f / M_PI;
        angle = angle < 0 ? 360 + angle : angle;
        auto dir = fmod(angle, 360);

        for (auto d : angle_dir_map)
        {
            closest = closest == -1 || abs(dir - d.first) < abs(dir - closest) ? d.first : closest;
        }
        res[i] = angle_dir_map[closest];
    }
    return res;
}
//std::vector< uint16_t > optimizer::get_closest_edges(
//    z_frame_data const & z_data,
//    ir_frame_data const & ir_data,
//    size_t width, size_t height )
//{
//    std::vector< uint16_t > z_closest;
//    z_closest.reserve( z_data.edges.size() );
//
//    for( auto i = 0; i < int(height); i++ )
//    {
//        for( auto j = 0; j < int(width); j++ )
//        {
//            auto idx = i * width + j;
//
//            auto edge = z_data.edges[idx];
//
//            //if (edge == 0)  continue;
//
//            auto edge_prev_idx = get_prev_index( z_data.valid_directions[idx], i, j, width, height );
//
//            auto edge_next_idx = get_next_index( z_data.valid_directions[idx], i, j, width, height );
//
//            auto edge_minus_idx = edge_prev_idx.second * width + edge_prev_idx.first;
//
//            auto edge_plus_idx = edge_next_idx.second * width + edge_next_idx.first;
//
//            auto z_edge_plus = z_data.edges[edge_plus_idx];
//            auto z_edge = z_data.edges[idx];
//            auto z_edge_minus = z_data.edges[edge_minus_idx];
//
//           
//            if (z_data.supressed_edges[idx])
//            {
//                z_closest.push_back(std::min(z_data.frame[edge_minus_idx], z_data.frame[edge_plus_idx]));
//            }
//            else
//            {
//                z_closest.push_back(0);
//            }
//        }
//    }
//    return z_closest;
//}

/* Given pixel coordinates and depth in an image with no distortion or inverse distortion coefficients, compute the corresponding point in 3D space relative to the same camera */
static void deproject_pixel_to_point(double point[3], const struct rs2_intrinsics_double * intrin, const double pixel[2], double depth)
{
    double x = (double)(pixel[0] - intrin->ppx) / intrin->fx;
    double y = (double)(pixel[1] - intrin->ppy) / intrin->fy;

    point[0] = depth * x;
    point[1] = depth * y;
    point[2] = depth;
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

std::vector<double> optimizer::blur_edges(std::vector<double> const & edges, size_t image_width, size_t image_height)
{
    std::vector<double> res = edges;

    for(size_t i = 0; i < image_height; i++ )
        for(size_t j = 0; j < image_width; j++ )
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

    for(size_t i = 0; i < image_height; i++ )
        for(size_t j = 0; j < image_width; j++ )
            res[i*image_width + j] = _params.alpha * edges[i*image_width + j] + (1 - _params.alpha) * res[i*image_width + j];
    return res;
}


std::vector< byte > optimizer::get_luminance_from_yuy2( std::vector< yuy_t > const & yuy2_imagh )
{
    std::vector<byte> res( yuy2_imagh.size(), 0 );
    auto yuy2 = (uint8_t*)yuy2_imagh.data();
    for(size_t i = 0; i < res.size(); i++ )
        res[i] = yuy2[i * 2];

    return res;
}

std::vector< uint8_t > optimizer::get_logic_edges( std::vector< double > const & edges )
{
    std::vector<uint8_t> logic_edges( edges.size(), 0 );
    auto max = std::max_element( edges.begin(), edges.end() );
    auto thresh = *max * _params.edge_thresh4_logic_lum;

    for(size_t i = 0; i < edges.size(); i++ )
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
    if( section_map.size() != weights.size() )
        throw std::runtime_error( to_string()
                                  << "unexpected size for section_map (" << section_map.size()
                                  << ") vs weights (" << weights.size() << ")" );
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

//std::vector<double> optimizer::calculate_weights(z_frame_data& z_data)
//{
//    std::vector<double> res;
//
//    for (auto i = 0; i < z_data.supressed_edges.size(); i++)
//    {
//        if (z_data.supressed_edges[i])
//            z_data.weights.push_back(
//                get_min(get_max(z_data.supressed_edges[i] - _params.grad_z_min, (double)0),
//                    _params.grad_z_max - _params.grad_z_min));
//    }
//
//    return res;
//}

void deproject_sub_pixel(
    std::vector<double3>& points,
    const rs2_intrinsics_double& intrin,
    std::vector< byte > const & valid_edges,
    const double* x,
    const double* y,
    const double* depth, double depth_units
)
{
    auto ptr = (double*)points.data();
    byte const * valid_edge = valid_edges.data();
    for (size_t i = 0; i < valid_edges.size(); ++i)
    {
        if (!valid_edge[i])
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

static p_matrix calc_p_gradients(const z_frame_data & z_data, 
    const std::vector<double3>& new_vertices,
    const yuy2_frame_data & yuy_data, 
    std::vector<double> interp_IDT_x, 
    std::vector<double> interp_IDT_y,
    const calib & cal,
    const p_matrix & p_mat,
    const std::vector<double>& rc, 
    const std::vector<double2>& xy,
    data_collect * data = nullptr)
{
    auto coefs = calc_p_coefs(z_data, new_vertices, yuy_data, cal, p_mat, rc, xy);
    auto w = z_data.weights;

    if (data)
        data->iteration_data_p.coeffs_p = coefs;

    p_matrix sums = { 0 };
    auto sum_of_valids = 0;

    for (size_t i = 0; i < coefs.x_coeffs.size(); i++)
    {
        if (interp_IDT_x[i] == std::numeric_limits<double>::max() || interp_IDT_y[i] == std::numeric_limits<double>::max())
            continue;

        sum_of_valids++;

        for (auto j = 0; j < 12; j++)
        {
            sums.vals[j] += w[i] * (interp_IDT_x[i] * coefs.x_coeffs[i].vals[j] + interp_IDT_y[i] * coefs.y_coeffs[i].vals[j]);
        }
        
    }

    p_matrix averages = { 0 };
    for (auto i = 0; i < 8; i++) //zero the last line of P grad?
    {
        averages.vals[i] = (double)sums.vals[i] / (double)sum_of_valids;
    }

    return averages;
}

static
std::pair< std::vector<double2>, std::vector<double>> calc_rc(
    const z_frame_data & z_data,
    const std::vector<double3>& new_vertices,
    const yuy2_frame_data & yuy_data,
    const calib & cal,
    const p_matrix & p_mat
)
{
    auto v = new_vertices;

    std::vector<double2> f1( z_data.vertices.size() );
    std::vector<double> r2( z_data.vertices.size() );
    std::vector<double> rc( z_data.vertices.size() );

    auto yuy_intrin = cal.get_intrinsics();
    auto yuy_extrin = cal.get_extrinsics();

    auto fx = (double)yuy_intrin.fx;
    auto fy = (double)yuy_intrin.fy;
    auto ppx = (double)yuy_intrin.ppx;
    auto ppy = (double)yuy_intrin.ppy;

   /* double mat[3][4] = {
        fx*(double)r[0] + ppx * (double)r[2], fx*(double)r[3] + ppx * (double)r[5], fx*(double)r[6] + ppx * (double)r[8], fx*(double)t[0] + ppx * (double)t[2],
        fy*(double)r[1] + ppy * (double)r[2], fy*(double)r[4] + ppy * (double)r[5], fy*(double)r[7] + ppy * (double)r[8], fy*(double)t[1] + ppy * (double)t[2],
        r[2], r[5], r[8], t[2] };
*/
    auto mat = p_mat.vals;
    for(size_t i = 0; i < z_data.vertices.size(); ++i )
    {
        double x = v[i].x;
        double y = v[i].y;
        double z = v[i].z;

        double x1 = (double)mat[0] * (double)x + (double)mat[1] * (double)y + (double)mat[2] * (double)z + (double)mat[3];
        double y1 = (double)mat[4] * (double)x + (double)mat[5] * (double)y + (double)mat[6] * (double)z + (double)mat[7];
        double z1 = (double)mat[8] * (double)x + (double)mat[9] * (double)y + (double)mat[10] * (double)z + (double)mat[11];

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

static p_matrix calc_gradients(
    const z_frame_data& z_data,
    const std::vector<double3>& new_vertices,
    const yuy2_frame_data& yuy_data,
    const std::vector<double2>& uv,
    const calib & cal,
    const p_matrix & p_mat,
    data_collect * data = nullptr
)
{
    p_matrix res;
    auto interp_IDT_x = biliniar_interp( yuy_data.edges_IDTx, yuy_data.width, yuy_data.height, uv );      
    auto interp_IDT_y = biliniar_interp( yuy_data.edges_IDTy, yuy_data.width, yuy_data.height, uv );

    auto rc = calc_rc( z_data, new_vertices, yuy_data, cal, p_mat);

    if (data)
    {
        data->iteration_data_p.d_vals_x = interp_IDT_x;
        data->iteration_data_p.d_vals_y = interp_IDT_y;
        data->iteration_data_p.xy = rc.first;
        data->iteration_data_p.rc = rc.second;
    }
        
    res = calc_p_gradients( z_data, new_vertices, yuy_data, interp_IDT_x, interp_IDT_y, cal, p_mat, rc.second, rc.first, data );
    return res;
}

std::pair<double, p_matrix> calc_cost_and_grad(
    const z_frame_data & z_data,
    const std::vector<double3>& new_vertices,
    const yuy2_frame_data & yuy_data,
    const calib & cal,
    const p_matrix & p_mat,
    data_collect * data = nullptr
)
{
    auto uvmap = get_texture_map(new_vertices, cal, p_mat);
    if( data )
        data->iteration_data_p.uvmap = uvmap;

    auto cost = calc_cost(z_data, yuy_data, uvmap, data ? &data->iteration_data_p.d_vals : nullptr );
    auto grad = calc_gradients(z_data, new_vertices, yuy_data, uvmap, cal, p_mat, data);
    return { cost, grad };
}

params::params()
{
    normalize_mat.vals[0] = 0.353692440000000;
    normalize_mat.vals[1] = 0.266197740000000;
    normalize_mat.vals[2] = 1.00926010000000;
    normalize_mat.vals[3] = 0.000673204490000000;
    normalize_mat.vals[4] = 0.355085250000000;
    normalize_mat.vals[5] = 0.266275050000000;
    normalize_mat.vals[6] = 1.01145800000000;
    normalize_mat.vals[7] = 0.000675013750000000;
    normalize_mat.vals[8] = 414.205570000000;
    normalize_mat.vals[9] = 313.341060000000;
    normalize_mat.vals[10] = 1187.34590000000;
    normalize_mat.vals[11] = 0.791570250000000;

    // NOTE: until we know the resolution, the current state is just the default!
    // We need to get the depth and rgb resolutions to make final decisions!
}
svm_model_linear::svm_model_linear()
{
}
svm_model_gaussian::svm_model_gaussian()
{
}
void params::set_depth_resolution( size_t width, size_t height, rs2_digital_gain digital_gain)
{
    AC_LOG( DEBUG, "    depth resolution= " << width << "x" << height );
    // Some parameters are resolution-dependent
    bool const XGA = ( width == 1024 && height == 768 );
    bool const VGA = ( width == 640 && height == 480 );

    if( ! XGA && ! VGA )
    {
        throw std::runtime_error( to_string() << width << "x" << height
                                              << " this resolution is not supported" );
    }

    if( XGA )
    {
        AC_LOG( DEBUG, "    changing IR threshold: " << grad_ir_threshold << " -> " << 2.5 << "  (because of resolution)" );
        grad_ir_threshold = 2.5;
    }
    if (use_enhanced_preprocessing)
    {
        if (digital_gain == RS2_DIGITAL_GAIN_HIGH)
        {
            if (VGA)
            {
                grad_ir_low_th = 1.5;
                grad_ir_high_th = 3.5;
                grad_z_low_th = 0;
                grad_z_high_th = 100;
            }
            else if (XGA)
            {
                grad_ir_low_th = 1;
                grad_ir_high_th = 2.5;
                grad_z_low_th = 0;
                grad_z_high_th = 80;
            }
        }
        else
        {
            if (VGA)
            {
                grad_ir_low_th = std::numeric_limits<double>::max();
                grad_ir_high_th = 3.5;
                grad_z_low_th = 0;
                grad_z_high_th = std::numeric_limits<double>::max();
            }
            else if (XGA)
            {
                grad_ir_low_th = std::numeric_limits<double>::max();
                grad_ir_high_th = 2.5;
                grad_z_low_th = 0;
                grad_z_high_th = std::numeric_limits<double>::max();
            }
        }
    }

    min_weighted_edge_per_section_depth = 50. * ( 480 * 640 ) / ( width * height );
}

void params::set_rgb_resolution( size_t width, size_t height )
{
    AC_LOG( DEBUG, "    RGB resolution= " << width << "x" << height );
    auto area = width * height;
    size_t const hd_area = 1920 * 1080;
    move_threshold_pix_num = 3e-5 * area;
    move_last_success_thresh_pix_num = 0.1 * area;
    max_xy_movement_per_calibration[0] = 10. * area / hd_area;
    max_xy_movement_per_calibration[1] = max_xy_movement_per_calibration[2] = 2. * area / hd_area;
    max_xy_movement_from_origin = 20. * area / hd_area;
    min_weighted_edge_per_section_rgb = 0.05 * hd_area / area;

}

calib const & optimizer::get_calibration() const
{
    return _final_calibration;
}

rs2_dsm_params const & optimizer::get_dsm_params() const
{
    return _final_dsm_params;
}

double optimizer::get_cost() const
{
    return _params_curr.cost;
}



template< typename T >
void write_obj( std::fstream & f, T const & o )
{
    f.write( (char const *)&o, sizeof( o ) );
}

void write_matlab_camera_params_file(
    rs2_intrinsics const & _intr_depth,
    calib const & rgb_calibration,
    float _depth_units,
    std::string const & dir,
    char const * filename
)
{
    std::string path = dir + filename;
    std::fstream f( path, std::ios::out | std::ios::binary );
    if( !f )
        throw std::runtime_error( "failed to open file:\n" + path );


    //depth intrinsics
    write_obj( f, (double)_intr_depth.width );
    write_obj( f, (double)_intr_depth.height );
    write_obj( f, (double)1/_depth_units );

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
        write_obj( f, (double)_extr.rotation[i] );
    }

    f.close();
}

void optimizer::write_data_to( std::string const & dir )
{
    // NOTE: it is expected that dir ends with a path separator or this won't work!
    AC_LOG( DEBUG, "    writing data to: " << dir );
    
    try
    {
        write_vector_to_file( _yuy.orig_frame, dir, "rgb.raw" );
        write_vector_to_file( _yuy.prev_frame, dir, "rgb_prev.raw" );
        write_vector_to_file( _yuy.last_successful_frame, dir, "rgb_last_successful.raw");
        write_vector_to_file( _ir.ir_frame, dir, "ir.raw" );
        write_vector_to_file( _z.frame, dir, "depth.raw" );

        write_to_file( &_original_dsm_params, sizeof( _original_dsm_params ), dir, "dsm.params" );
        write_to_file( &_original_calibration, sizeof( _original_calibration ), dir, "rgb.calib" );
        auto & cal_info = _k_to_DSM->get_calibration_info();
        auto & cal_regs = _k_to_DSM->get_calibration_registers();
        write_to_file( &cal_info, sizeof( cal_info ), dir, "cal.info" );
        write_to_file( &cal_regs, sizeof( cal_regs ), dir, "cal.registers" );
        write_to_file( &_z.orig_intrinsics, sizeof( _z.orig_intrinsics), dir, "depth.intrinsics" );
        write_to_file( &_z.depth_units, sizeof( _z.depth_units ), dir, "depth.units" );
        write_to_file( &_settings, sizeof( _settings ), dir, "settings" );

        // This file is meant for matlab -- it packages all the information needed
        write_matlab_camera_params_file( _z.orig_intrinsics,
                                         _original_calibration,
                                         _z.depth_units,
                                         dir,
                                         "camera_params"
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

optimization_params optimizer::back_tracking_line_search( optimization_params const & curr_params,
                                                          const std::vector<double3>& new_vertices,
                                                          data_collect * data ) const
{
    optimization_params new_params;

    // was gradStruct.P ./ norm( gradStruct.P(:) )   -> vector norm
    // now gradStruct.P ./ norm( gradStruct.P )      -> matrix 2-norm
    auto grads_over_norm
        = curr_params.calib_gradients.normalize( curr_params.calib_gradients.matrix_norm() );
    //%grad = gradStruct.P ./ norm(gradStruct.P) ./ params.rgbPmatNormalizationMat;
    auto grad = grads_over_norm / _params.normalize_mat;

    //%unitGrad = grad ./ norm(grad);     <-   was ./ norm(grad(:)')
    auto grad_norm = grad.matrix_norm();
    auto unit_grad = grad.normalize( grad_norm );

    //%t = -params.controlParam * grad(:)' * unitGrad(:);
    // -> dot product of grad and unitGrad
    auto t_vals = ( grad * -_params.control_param ) * unit_grad;
    auto t = t_vals.sum();

    //%stepSize = params.maxStepSize * norm(grad) / norm(unitGrad);
    auto step_size = _params.max_step_size * grad_norm / unit_grad.matrix_norm();


    auto movement = unit_grad * step_size;
    new_params.curr_p_mat = curr_params.curr_p_mat + movement;
    
    calib old_calib = decompose( curr_params.curr_p_mat, _original_calibration );
    auto uvmap_old = get_texture_map(new_vertices, old_calib, curr_params.curr_p_mat );
    //curr_params.cost = calc_cost( z_data, yuy_data, uvmap_old );

    calib new_calib = decompose( new_params.curr_p_mat, _original_calibration );
    auto uvmap_new = get_texture_map(new_vertices, new_calib, new_params.curr_p_mat );
    new_params.cost = calc_cost( _z, _yuy, uvmap_new );

    auto diff = calc_cost_per_vertex_diff( _z, _yuy, uvmap_old, uvmap_new );

    auto iter_count = 0;
    while( diff >= step_size * t
           && abs( step_size ) > _params.min_step_size
           && iter_count++ < _params.max_back_track_iters )
    {
        //AC_LOG( DEBUG, "    back tracking line search cost= " << AC_D_PREC << new_params.cost );
        step_size = _params.tau * step_size;

        new_params.curr_p_mat = curr_params.curr_p_mat + unit_grad * step_size;
        
        new_calib = decompose( new_params.curr_p_mat, _original_calibration );
        uvmap_new = get_texture_map(new_vertices, new_calib, new_params.curr_p_mat);
        new_params.cost = calc_cost( _z, _yuy, uvmap_new);
        diff = calc_cost_per_vertex_diff( _z, _yuy, uvmap_old, uvmap_new );
    }

    if(diff >= step_size * t )
    {
        new_params = curr_params;
    }

    if (data)
    {
        data->iteration_data_p.grads_norma = curr_params.calib_gradients.get_norma();
        data->iteration_data_p.grads_norm = grads_over_norm;
        data->iteration_data_p.normalized_grads = grad;
        data->iteration_data_p.unit_grad = unit_grad;
        data->iteration_data_p.back_tracking_line_search_iters = iter_count;
        data->iteration_data_p.t = t;
    }
    return new_params;
}

void optimizer::set_final_data(const std::vector<double3>& vertices,
    const p_matrix& p_mat,
    const p_matrix& p_mat_opt)
{
    _vertices_from_bin = vertices;
    _p_mat_from_bin = p_mat;
    _p_mat_from_bin_opt = p_mat_opt;
}

void optimizer::set_cycle_data(const std::vector<double3>& vertices, 
    const rs2_intrinsics_double& k_depth,
    const p_matrix& p_mat, 
    const algo_calibration_registers& dsm_regs_cand,
    const rs2_dsm_params_double& dsm_params_cand)
{
    _vertices_from_bin = vertices;
    _k_dapth_from_bin = k_depth;
    _p_mat_from_bin = p_mat;
    _dsm_regs_cand_from_bin = dsm_regs_cand;
    _dsm_params_cand_from_bin = dsm_params_cand;
}

void optimizer::adjust_params_to_apd_gain()
{
    if(_settings.digital_gain == RS2_DIGITAL_GAIN_HIGH) // long preset
        _params.saturation_value = 230;
    else if(_settings.digital_gain == RS2_DIGITAL_GAIN_LOW) // short preset
        _params.saturation_value = 250;
    else
        throw std::runtime_error(to_string() << _settings.digital_gain << " invalid digital gain value");
}

void optimizer::adjust_params_to_manual_mode()
{
    _params.max_global_los_scaling_step = 0.005;
    _params.pix_per_section_depth_th = 0.01;
    _params.pix_per_section_rgb_th = 0.01;
    _params.min_section_with_enough_edges = 2;
    _params.edges_per_direction_ratio_th = 0.004;
    _params.minimal_full_directions = 2;

    const static double newvals[N_BASIC_DIRECTIONS] = { 0.09,0.09,0.09,0.09 };
    std::copy(std::begin(newvals), std::end(newvals), std::begin(_params.dir_std_th));
    _params.saturation_ratio_th = 0.15;
    adjust_params_to_apd_gain();
}

void optimizer::adjust_params_to_auto_mode()
{
    _params.max_global_los_scaling_step = 0.004;
    _params.pix_per_section_depth_th = 0.01;
    _params.pix_per_section_rgb_th = 0.02;
    _params.min_section_with_enough_edges = 2;
    _params.edges_per_direction_ratio_th = 0.004;
    _params.minimal_full_directions = 2;

    const static double newvals[N_BASIC_DIRECTIONS] = { 0.09,0.09,0.09,0.09 };
    std::copy(std::begin(newvals), std::end(newvals), std::begin(_params.dir_std_th));
    _params.saturation_ratio_th = 0.05;
    adjust_params_to_apd_gain();
}

size_t optimizer::optimize_p
(
    const optimization_params& params_curr,
    const std::vector<double3>& new_vertices,
    optimization_params& params_new,
    calib& optimaized_calibration,
    calib& new_rgb_calib_for_k_to_dsm,
    rs2_intrinsics_double& new_z_k,
    std::function<void(data_collect const&data)> cb,
    data_collect& data )
{
    // The params_curr that we get contains a cost and p_matrix that do not match:
    // The cost is the optimal cost calculated in the previous cycle, but we don't use it here.
    // The p_matrix is the optimal p_matrix that has been modified (see the end of this function).
    // Between the previous cycle and now we have new vertices so we recalculate a new cost based
    // on the modified p_matrix and the new vertices
    size_t n_iterations = 0;
    auto curr = params_curr;
    while (1)
    {

        auto res = calc_cost_and_grad(_z, new_vertices, _yuy, new_rgb_calib_for_k_to_dsm, curr.curr_p_mat, &data);
        curr.cost = res.first;
        curr.calib_gradients = res.second;
        AC_LOG( DEBUG, std::setw( 3 ) << std::right << n_iterations << std::left
                           << " cost= " << AC_D_PREC << curr.cost );

        data.iteration_data_p.iteration = n_iterations;
        if( _debug_mode )
        {
            
            data.iteration_data_p.params = curr;
            data.iteration_data_p.c = new_rgb_calib_for_k_to_dsm;
            data.iteration_data_p.iteration = n_iterations;
        }

        params_new = back_tracking_line_search( curr, new_vertices, &data );
        
        if( _debug_mode )
            data.iteration_data_p.next_params = params_new;

        data.type = _debug_mode ? iteration_data : general_data;
        if( cb )
            cb(data);


        auto norm = (params_new.curr_p_mat - curr.curr_p_mat).get_norma();
        if (norm < _params.min_rgb_mat_delta)
        {
            AC_LOG(DEBUG, "    {normal(new-curr)} " << norm << " < " << _params.min_rgb_mat_delta << " {min_rgb_mat_delta}  -->  stopping");
            break;
        }

        auto delta = params_new.cost - curr.cost;
        //AC_LOG( DEBUG, "    delta= " << AC_D_PREC << delta );
        delta = abs(delta);
        if (delta < _params.min_cost_delta)
        {
            AC_LOG(DEBUG, "    delta < " << _params.min_cost_delta << "  -->  stopping");
            break;
        }

        if (++n_iterations >= _params.max_optimization_iters)
        {
            AC_LOG(DEBUG, "    exceeding max iterations  -->  stopping");
            break;
        }

        curr = params_new;
        new_rgb_calib_for_k_to_dsm = decompose_p_mat(params_new.curr_p_mat);
    }

    AC_LOG( DEBUG,
            "    optimize_p finished after " << n_iterations << " iterations; cost "
                         << AC_D_PREC << params_curr.cost << "  -->  " << params_new.cost );
    new_rgb_calib_for_k_to_dsm = optimaized_calibration = decompose_p_mat(params_new.curr_p_mat);

    new_rgb_calib_for_k_to_dsm.k_mat.k_mat.rot[1] = 0; //sheer

    new_z_k = get_new_z_intrinsics_from_new_calib(_z.orig_intrinsics, new_rgb_calib_for_k_to_dsm, _original_calibration);
    new_rgb_calib_for_k_to_dsm.k_mat.k_mat.rot[0] = _original_calibration.k_mat.get_fx();
    new_rgb_calib_for_k_to_dsm.k_mat.k_mat.rot[4] = _original_calibration.k_mat.get_fy();
    params_new.curr_p_mat = new_rgb_calib_for_k_to_dsm.calc_p_mat();

    return n_iterations;
}

size_t optimizer::optimize( std::function< void( data_collect const & data ) > cb )
{
    optimization_params params_orig;
    params_orig.curr_p_mat = _original_calibration.calc_p_mat();
    _params_curr = params_orig;

    data_collect data;

    auto cycle = 1;
    data.cycle_data_p.cycle = cycle;

    auto res = calc_cost_and_grad( _z,
                                   _z.vertices,
                                   _yuy,
                                   decompose( _params_curr.curr_p_mat, _original_calibration ),
                                   _params_curr.curr_p_mat,
                                   _debug_mode? &data: nullptr );

    _params_curr.cost = res.first;
    _params_curr.calib_gradients = res.second;
    params_orig.cost = res.first;

    optimization_params new_params;
    calib new_calib = _original_calibration;
    calib new_k_to_dsm_calib = _original_calibration;
    rs2_intrinsics_double new_k_depth;
    algo_calibration_registers new_dsm_regs = _k_to_DSM->get_calibration_registers();
    auto new_vertices = _z.vertices;

    double last_cost = _params_curr.cost;
    
    auto n_iterations = optimize_p( _params_curr,
                                    new_vertices,
                                    new_params,
                                    _optimaized_calibration,
                                    new_calib,
                                    new_k_depth,
                                    cb,
                                    data );

    _z.orig_vertices = _z.vertices;
    rs2_dsm_params_double new_dsm_params = _z.orig_dsm_params;

    while (cycle-1 < _params.max_K2DSM_iters)
    {
        std::vector<double3> cand_vertices = _z.vertices;
        auto dsm_regs_cand = new_dsm_regs;

        optimization_params params_candidate = new_params;
        calib calib_candidate = new_calib;
        calib calib_k_to_dsm_candidate = new_calib;
        calib optimaized_calib_candidate = _optimaized_calibration;
        rs2_intrinsics_double k_depth_candidate = new_k_depth;

        ++cycle;
        data.cycle_data_p.cycle = cycle;

        if (_debug_mode)
        {
            data.cycle_data_p.new_calib = new_calib;
            data.cycle_data_p.new_k_depth = new_k_depth;
            data.cycle_data_p.new_params = new_params;
            data.cycle_data_p.new_dsm_params = new_dsm_params;
            data.cycle_data_p.new_dsm_regs = new_dsm_regs;
            data.cycle_data_p.new_vertices = new_vertices;
            data.cycle_data_p.optimaized_calib_candidate = optimaized_calib_candidate;

        }
       
        if( cb )
        {
            data.type = _debug_mode ? cycle_data : general_data;
            cb( data );
        }

        AC_LOG(INFO, "CYCLE " << data.cycle_data_p.cycle << " started with: cost = " << AC_D_PREC << new_params.cost);


        if (get_cycle_data_from_bin)
        {
            new_params.curr_p_mat = _p_mat_from_bin;
            new_calib = decompose(_p_mat_from_bin, _original_calibration);
            new_k_depth = _k_dapth_from_bin;
            new_dsm_regs = _dsm_regs_cand_from_bin;
            new_dsm_params = _dsm_params_cand_from_bin;
        }

        auto dsm_candidate = _k_to_DSM->convert_new_k_to_DSM( _z.orig_intrinsics,
                                                              new_k_depth,
                                                              _z,
                                                              cand_vertices,
                                                              new_dsm_params,
                                                              dsm_regs_cand,
                                                              _debug_mode ? &data : nullptr );
        //data.type = cycle_data;

        //this calib is now candidate to be the optimaized we can confirm it only after running more optimize_p
        calib_k_to_dsm_candidate = calib_candidate;

        if (_debug_mode)
        {
            data.k2dsm_data_p.dsm_params_cand = dsm_candidate;
            data.k2dsm_data_p.vertices = cand_vertices;
            data.k2dsm_data_p.dsm_pre_process_data = _k_to_DSM->get_pre_process_data();

            if( cb )
            {
                 data.type = k_to_dsm_data;
                 cb(data);
            }
        }
     

        optimize_p( new_params,
                    cand_vertices,
                    params_candidate,
                    optimaized_calib_candidate,
                    calib_candidate,
                    k_depth_candidate,
                    cb,
                    data );

        if( params_candidate.cost < last_cost )
        {
            if (cycle == 2)
            {// No change at all (probably very good starting point)
                AC_LOG(DEBUG, "No change required (probably very good starting point)");
                new_k_to_dsm_calib = new_calib;
                _optimaized_calibration = _original_calibration;
            }

            AC_LOG( DEBUG, "    cost is a regression; stopping -- not using last cycle" );
            break;
        }

        new_params = params_candidate;
        _params_curr = new_params;
        new_calib = calib_candidate;
        new_k_to_dsm_calib = calib_k_to_dsm_candidate;
        new_k_depth = k_depth_candidate;
        new_dsm_params = dsm_candidate;
        new_dsm_regs = dsm_regs_cand;
        last_cost = new_params.cost;
        new_vertices = cand_vertices;
        _z.vertices = new_vertices;
        _optimaized_calibration = optimaized_calib_candidate;
    }
   
    AC_LOG( INFO,
            "Calibration converged; cost " << AC_D_PREC << params_orig.cost << "  -->  "
                                           << new_params.cost );

    _final_dsm_params = _z.orig_dsm_params;
    clip_ac_scaling( _z.orig_dsm_params, new_dsm_params );
    new_dsm_params.copy_to( _final_dsm_params );
    _final_dsm_params.temp_x2 = byte( _settings.hum_temp * 2 );

    _final_calibration = new_k_to_dsm_calib;

    // The actual valid cycles - we starting from 1 and the last cycle is only for verification
    return cycle - 2;
}
