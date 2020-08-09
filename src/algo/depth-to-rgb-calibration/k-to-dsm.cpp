//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "k-to-dsm.h"
#include "optimizer.h"
#include "debug.h"
#include "utils.h"
#include <math.h>

using namespace librealsense::algo::depth_to_rgb_calibration;


std::ostream &
librealsense::algo::depth_to_rgb_calibration::operator<<( std::ostream & s,
                                                          rs2_dsm_params_double const & self )
{
    s << "[ ";
    switch( self.model )
    {
    case RS2_DSM_CORRECTION_NONE: break;
    case RS2_DSM_CORRECTION_AOT: s << "AoT "; break;
    case RS2_DSM_CORRECTION_TOA: s << "ToA "; break;
    }
    s << "x[" << self.h_scale << " " << self.v_scale << "] ";
    if( self.h_offset  ||  self.v_offset )
        s << "+[" << self.h_offset << " " << self.v_offset;
    if( self.rtd_offset )
        s << " rtd " << self.rtd_offset;
    s << " ]";
    return s;
}


rs2_intrinsics_double rotate_k_mat(const rs2_intrinsics_double& k_mat)
{
    rs2_intrinsics_double res = k_mat;
    res.ppx = k_mat.width - 1 - k_mat.ppx;
    res.ppy = k_mat.height - 1 - k_mat.ppy;

    return res;
}

k_to_DSM::k_to_DSM( const rs2_dsm_params_double & orig_dsm_params,
                    algo_calibration_info const & cal_info,
                    algo_calibration_registers const & cal__regs,
                    const double & max_scaling_step )
    : _regs( cal_info )
    , _dsm_regs( cal__regs )
    , _max_scaling_step( max_scaling_step )
{
}

algo_calibration_registers
k_to_DSM::apply_ac_res_on_dsm_model( const rs2_dsm_params_double & ac_data,
                                     const algo_calibration_registers & dsm_regs,
                                     const ac_to_dsm_dir & type )
{
    algo_calibration_registers res;

    if (type == direct) // convert from original model to modified model
    {
        switch (ac_data.model)
        {
        case RS2_DSM_CORRECTION_NONE:
            res = dsm_regs;
            break;
        case RS2_DSM_CORRECTION_AOT:
            res.EXTLdsmXscale =  (double)dsm_regs.EXTLdsmXscale * ac_data.h_scale ;
            res.EXTLdsmYscale = (double)dsm_regs.EXTLdsmYscale * ac_data.v_scale ;
            res.EXTLdsmXoffset
                =  ( (double)dsm_regs.EXTLdsmXoffset + (double)ac_data.h_offset )
                         / (double)ac_data.h_scale ;
            res.EXTLdsmYoffset
                =  ( (double)dsm_regs.EXTLdsmYoffset + (double)ac_data.v_offset )
                         / (double)ac_data.v_scale ;
            break;
        case RS2_DSM_CORRECTION_TOA:
            res.EXTLdsmXscale =  (double)dsm_regs.EXTLdsmXscale * ac_data.h_scale ;
            res.EXTLdsmYscale =  (double)dsm_regs.EXTLdsmYscale * ac_data.v_scale ;
            res.EXTLdsmXoffset
                =  ( (double)dsm_regs.EXTLdsmXoffset + (double)ac_data.h_offset )
                         / (double)dsm_regs.EXTLdsmXscale ;
            res.EXTLdsmYoffset
                =  ( (double)dsm_regs.EXTLdsmYoffset + (double)ac_data.v_offset )
                         / (double)dsm_regs.EXTLdsmYscale ;
            break;
        default:
            throw std::runtime_error( "invalid model " + std::to_string( ac_data.model ) );
        }
    }
    else if (type == inverse) // revert from modified model to original model
    {
        switch (ac_data.model)
        {
        case RS2_DSM_CORRECTION_NONE:
            res = dsm_regs;
            break;
        case RS2_DSM_CORRECTION_AOT:
            res.EXTLdsmXscale = (double)dsm_regs.EXTLdsmXscale / (double)ac_data.h_scale;
            res.EXTLdsmYscale = (double)dsm_regs.EXTLdsmYscale / (double)ac_data.v_scale;
            res.EXTLdsmXoffset = (double)dsm_regs.EXTLdsmXoffset* (double)ac_data.h_scale - (double)ac_data.h_offset;
            res.EXTLdsmYoffset = (double)dsm_regs.EXTLdsmYoffset* (double)ac_data.v_scale - (double)ac_data.v_offset;
            break;
        case RS2_DSM_CORRECTION_TOA:
            res.EXTLdsmXscale = (double)dsm_regs.EXTLdsmXscale / (double)ac_data.h_scale;
            res.EXTLdsmYscale = (double)dsm_regs.EXTLdsmYscale / (double)ac_data.v_scale;
            res.EXTLdsmXoffset = (double)dsm_regs.EXTLdsmXoffset - (double)ac_data.h_offset / (double)res.EXTLdsmXscale;
            res.EXTLdsmYoffset = (double)dsm_regs.EXTLdsmYoffset - (double)ac_data.v_offset / (double)res.EXTLdsmYscale;
            break;
        default:
            throw std::runtime_error( "invalid valid model " + std::to_string( ac_data.model ) );
            break;
        }
    }
    return res;
}

los_shift_scaling
k_to_DSM::convert_ac_data_to_los_error( const algo_calibration_registers & dsm_regs,
                                        const rs2_dsm_params_double & ac_data )
{
    los_shift_scaling res;
    switch (ac_data.model)
    {
    case RS2_DSM_CORRECTION_NONE:
        res.los_scaling_x = 1;
        res.los_scaling_y = 1;
        res.los_shift_x = 0;
        res.los_shift_y = 0;
        break;
    case RS2_DSM_CORRECTION_AOT:
        res.los_scaling_x = 1/ (double)ac_data.h_scale;
        res.los_scaling_y = 1/ (double)ac_data.v_scale;
        res.los_shift_x = -(double)ac_data.h_offset*res.los_scaling_x;
        res.los_shift_y = -(double)ac_data.v_offset*res.los_scaling_y;
        break;
    case RS2_DSM_CORRECTION_TOA:
        res.los_scaling_x = 1 / (double)ac_data.h_scale;
        res.los_scaling_y = 1 / (double)ac_data.v_scale;

        auto dsm_orig = apply_ac_res_on_dsm_model(ac_data, dsm_regs, inverse);

        res.los_shift_x = -(double)ac_data.h_offset / (double)dsm_orig.EXTLdsmXscale - (double)dsm_orig.EXTLdsmXoffset*(1 - res.los_scaling_x);
        res.los_shift_y = -(double)ac_data.v_offset / (double)dsm_orig.EXTLdsmYscale - (double)dsm_orig.EXTLdsmYoffset*(1 - res.los_scaling_y);
        break;
    }
    return res;
}

pre_process_data k_to_DSM::pre_processing
(
    const algo_calibration_info& regs,
    const rs2_dsm_params_double& ac_data,
    const algo_calibration_registers& algo_calibration_registers,
    const rs2_intrinsics_double& orig_k_raw,
    const std::vector<uint8_t>& relevant_pixels_image,
    data_collect* data
)
{
    pre_process_data res;
    res.orig_k = orig_k_raw;
    res.relevant_pixels_image_rot = relevant_pixels_image;
    res.last_los_error = convert_ac_data_to_los_error(algo_calibration_registers, ac_data);

    res.vertices_orig = calc_relevant_vertices(relevant_pixels_image, orig_k_raw);
    auto dsm_res_orig = apply_ac_res_on_dsm_model(ac_data, algo_calibration_registers, inverse);

    convert_norm_vertices_to_los_data* vertices_to_los_data = nullptr;
    if (data)
        vertices_to_los_data = &data->k2dsm_data_p.first_norm_vertices_to_los_data;

    res.los_orig = convert_norm_vertices_to_los(regs, dsm_res_orig, res.vertices_orig, vertices_to_los_data);
    return res;
}

rs2_dsm_params_double k_to_DSM::convert_new_k_to_DSM
(
    const rs2_intrinsics_double& old_k,
    const rs2_intrinsics_double& new_k,
    const z_frame_data& z,
    std::vector<double3> & new_vertices,
    rs2_dsm_params_double const & previous_dsm_params,
    algo_calibration_registers& new_dsm_regs,
    data_collect* data
)
{
    if (data)
    {
        data->k2dsm_data_p.inputs.old_k = old_k;
        data->k2dsm_data_p.inputs.new_k = new_k;
        data->k2dsm_data_p.inputs.z = z;
        data->k2dsm_data_p.inputs.new_vertices = new_vertices;
        data->k2dsm_data_p.inputs.previous_dsm_params = previous_dsm_params;
        data->k2dsm_data_p.inputs.new_dsm_regs = new_dsm_regs;
    }


    auto w = old_k.width;
    auto h = old_k.height;

    auto old_k_raw = rotate_k_mat(old_k);
    auto new_k_raw = rotate_k_mat(new_k);

    auto dsm_orig = apply_ac_res_on_dsm_model( previous_dsm_params, new_dsm_regs, inverse);

    std::vector<uint8_t> relevant_pixels_image_rot(z.relevant_pixels_image.size(), 0);
    rotate_180(z.relevant_pixels_image.data(), relevant_pixels_image_rot.data(), w, h);

    _pre_process_data = pre_processing(_regs, previous_dsm_params, new_dsm_regs, old_k_raw, relevant_pixels_image_rot, data);

    auto new_los_scaling = convert_k_to_los_error(_regs, new_dsm_regs, new_k_raw, data);
    double2 los_shift = { 0 };
    auto ac_data_cand = convert_los_error_to_ac_data( previous_dsm_params, new_dsm_regs, los_shift, new_los_scaling);
    auto dsm_regs_cand = apply_ac_res_on_dsm_model(ac_data_cand, dsm_orig, direct);

    auto sc_vertices = new_vertices;

    for (auto i = 0; i < sc_vertices.size(); i++)
    {
        sc_vertices[i].x *= -1;
        sc_vertices[i].y *= -1;
    }

    convert_norm_vertices_to_los_data* vertices_to_los_data = nullptr;
    if (data)
        vertices_to_los_data = &data->k2dsm_data_p.second_norm_vertices_to_los_data;
    auto los_orig = convert_norm_vertices_to_los(_regs, new_dsm_regs, sc_vertices, vertices_to_los_data);
    new_vertices = convert_los_to_norm_vertices(_regs, dsm_regs_cand, los_orig, data);

    if (data)
    {
        data->k2dsm_data_p.dsm_regs_orig = dsm_orig;
        data->k2dsm_data_p.relevant_pixels_image_rot = relevant_pixels_image_rot;
        data->k2dsm_data_p.new_los_scaling = new_los_scaling;
        data->k2dsm_data_p.dsm_regs_cand = dsm_regs_cand;
        data->k2dsm_data_p.los_orig = los_orig;
    }

    for (auto i = 0; i < new_vertices.size(); i++)
    {
        new_vertices[i].x = new_vertices[i].x / new_vertices[i].z * sc_vertices[i].z;
        new_vertices[i].y = new_vertices[i].y / new_vertices[i].z * sc_vertices[i].z;
        new_vertices[i].z = new_vertices[i].z / new_vertices[i].z * sc_vertices[i].z;

        new_vertices[i].x *= -1;
        new_vertices[i].y *= -1;
    }

    AC_LOG( DEBUG,
            "    new DSM params: " << AC_D_PREC << ac_data_cand << "; vertices are changed" );
    new_dsm_regs = dsm_regs_cand;
    return ac_data_cand;
}

const pre_process_data& k_to_DSM::get_pre_process_data() const
{
    return _pre_process_data;
}

double2 k_to_DSM::convert_k_to_los_error
(
    algo_calibration_info const & regs,
    algo_calibration_registers const &dsm_regs,
    rs2_intrinsics_double const & new_k_raw,
    data_collect* data
)
{
    double2 focal_scaling;

    focal_scaling.x = new_k_raw.fx / _pre_process_data.orig_k.fx;
    focal_scaling.y = new_k_raw.fy / _pre_process_data.orig_k.fy;

    double coarse_grid[5] = { -1, -0.5, 0, 0.5, 1 };
    double fine_grid[5] = { -1, -0.5, 0, 0.5, 1 };

    double coarse_grid_x[5];
    double coarse_grid_y[5];

    for (auto i = 0; i < 5; i++)
    {
        coarse_grid_x[i] = coarse_grid[i] * _max_scaling_step + (double)_pre_process_data.last_los_error.los_scaling_x;
        coarse_grid_y[i] = coarse_grid[i] * _max_scaling_step + (double)_pre_process_data.last_los_error.los_scaling_y;
        fine_grid[i] = fine_grid[i] * 0.6* _max_scaling_step;
    }

    double grid_y[SIZE_OF_GRID_X];
    double grid_x[SIZE_OF_GRID_X];
    ndgrid_my(coarse_grid_y, coarse_grid_x, grid_y, grid_x);

    auto opt_scaling = run_scaling_optimization_step(regs, dsm_regs, grid_x, grid_y, focal_scaling, data);

    if (data)
    {
        data->k2dsm_data_p.focal_scaling = focal_scaling;
        data->k2dsm_data_p.opt_scaling = opt_scaling;
    }
        

    double fine_grid_x[SIZE_OF_GRID_X] = { 0 };
    double fine_grid_y[SIZE_OF_GRID_X] = { 0 };

    for (auto i = 0; i < 5; i++)
    {
        fine_grid_x[i] = fine_grid[i] + opt_scaling.x;
        fine_grid_y[i] = fine_grid[i] + opt_scaling.y;
    }

    ndgrid_my(fine_grid_y, fine_grid_x, grid_y, grid_x);
    auto los_scaling = run_scaling_optimization_step(regs, dsm_regs, grid_x, grid_y, focal_scaling);

    auto max_step_with_margin = 1.01*_max_scaling_step;

    los_scaling.x
        = std::min( std::max( los_scaling.x, _pre_process_data.last_los_error.los_scaling_x
                                                 - max_step_with_margin ),
                    _pre_process_data.last_los_error.los_scaling_x + max_step_with_margin );
    los_scaling.y
        = std::min( std::max( los_scaling.y, _pre_process_data.last_los_error.los_scaling_y
                                                 - max_step_with_margin ),
                    _pre_process_data.last_los_error.los_scaling_y + max_step_with_margin );

    return los_scaling;
}

rs2_dsm_params_double k_to_DSM::convert_los_error_to_ac_data
(
    const rs2_dsm_params_double& ac_data,
    const algo_calibration_registers& dsm_regs,
    double2 los_shift, 
    double2 los_scaling
)
{
    rs2_dsm_params_double ac_data_out = ac_data;
    ac_data_out.model = ac_data.model;
    switch (ac_data_out.model)
    {
    case RS2_DSM_CORRECTION_NONE:
        ac_data_out.h_scale = 1;
        ac_data_out.v_scale = 1;
        ac_data_out.h_offset = 0;
        ac_data_out.v_offset = 0;
        break;
    case RS2_DSM_CORRECTION_AOT:
        ac_data_out.h_scale =  1 / los_scaling.x ;
        ac_data_out.v_scale = 1 / los_scaling.y ;
        ac_data_out.h_offset =  -los_shift.x / los_scaling.x;
        ac_data_out.v_offset =  -los_shift.y / los_scaling.y;
        break;
    case RS2_DSM_CORRECTION_TOA:
        ac_data_out.h_scale =  1 / los_scaling.x;
        ac_data_out.v_scale =  1 / los_scaling.y;
        _dsm_regs = apply_ac_res_on_dsm_model(ac_data, dsm_regs, inverse); // the one used for assessing LOS error
        ac_data_out.h_offset
            =  -( _dsm_regs.EXTLdsmXoffset * ( 1 - los_scaling.x ) + los_shift.x )
                     * _dsm_regs.EXTLdsmXscale ;
        ac_data_out.v_offset
            =  -( _dsm_regs.EXTLdsmYoffset * ( 1 - los_scaling.y ) + los_shift.y )
                     * _dsm_regs.EXTLdsmYscale ;
        break;
        
    }
    return ac_data_out;
}

double2 k_to_DSM::run_scaling_optimization_step
(
    algo_calibration_info const & regs,
    algo_calibration_registers const &dsm_regs,
    double scaling_grid_x[SIZE_OF_GRID_X],
    double scaling_grid_y[SIZE_OF_GRID_X],
    double2 focal_scaling,
    data_collect* data)
{
    auto opt_k = optimize_k_under_los_error(regs, dsm_regs, scaling_grid_x, scaling_grid_y);

    double fx_scaling_on_grid[SIZE_OF_GRID_X];
    double fy_scaling_on_grid[SIZE_OF_GRID_X];

    for (auto i = 0; i < SIZE_OF_GRID_X; i++)
    {
        fx_scaling_on_grid[i] = opt_k[i].mat[0][0] / _pre_process_data.orig_k.fx;
        fy_scaling_on_grid[i] = opt_k[i].mat[1][1] / _pre_process_data.orig_k.fy;
    }
    double err_l2[SIZE_OF_GRID_X];
    for (auto i = 0; i < SIZE_OF_GRID_X; i++)
    {
        err_l2[i] = std::sqrt(std::pow(fx_scaling_on_grid[i] - focal_scaling.x, 2) + std::pow(fy_scaling_on_grid[i] - focal_scaling.y, 2));
    }
    
    double sg_mat[SIZE_OF_GRID_X][SIZE_OF_GRID_Y];
    
    for (auto i = 0; i < SIZE_OF_GRID_X; i++)
    {
        sg_mat[i][0] = scaling_grid_x[i] * scaling_grid_x[i];
        sg_mat[i][1] = scaling_grid_y[i] * scaling_grid_y[i];
        sg_mat[i][2] = scaling_grid_x[i] * scaling_grid_y[i];
        sg_mat[i][3] = scaling_grid_x[i];
        sg_mat[i][4] = scaling_grid_y[i];
        sg_mat[i][5] = 1;
    }

    double sg_mat_tag_x_sg_mat[36] = { 0 };
    double sg_mat_tag_x_err_l2[6] = { 0 };

    for (auto i = 0; i < 6; i++)
    {
        for (auto j = 0; j < 6; j++)
        {
            for (auto l = 0; l < 25; l++)
            {
                sg_mat_tag_x_sg_mat[i * 6 + j] += sg_mat[l][i] * sg_mat[l][j];
            }
            
        }
    }

    for (auto j = 0; j < 6; j++)
    {
        for (auto l = 0; l < 25; l++)
        {
            sg_mat_tag_x_err_l2[j] += sg_mat[l][j] * err_l2[l];
        }
    }

    double quad_coef[6];
    direct_inv_6x6(sg_mat_tag_x_sg_mat, sg_mat_tag_x_err_l2, quad_coef);

   

    double A[4] = { quad_coef[0], quad_coef[2] / 2, quad_coef[2] / 2, quad_coef[1] };
    double B[2] = { quad_coef[3] , quad_coef[4] };
    double opt_scaling[2];

    direct_inv_2x2(A, B, opt_scaling);
    opt_scaling[0] = -opt_scaling[0] / 2;
    opt_scaling[1] = -opt_scaling[1] / 2;

    if (data)
    {
        data->k2dsm_data_p.errL2 = std::vector<double>(std::begin(err_l2), std::end(err_l2));

        data->k2dsm_data_p.sg_mat.resize(SIZE_OF_GRID_X);
        for (auto i = 0; i < SIZE_OF_GRID_X; i++)
        {
            data->k2dsm_data_p.sg_mat[i].resize(SIZE_OF_GRID_Y);
            for (auto j = 0; j < SIZE_OF_GRID_Y; j++)
            {
                data->k2dsm_data_p.sg_mat[i][j] = sg_mat[i][j];
            }
        }

        data->k2dsm_data_p.sg_mat_tag_x_sg_mat = std::vector<double>(std::begin(sg_mat_tag_x_sg_mat), std::end(sg_mat_tag_x_sg_mat));
        data->k2dsm_data_p.sg_mat_tag_x_err_l2 = std::vector<double>(std::begin(sg_mat_tag_x_err_l2), std::end(sg_mat_tag_x_err_l2));
        data->k2dsm_data_p.quad_coef = std::vector<double>(std::begin(quad_coef), std::end(quad_coef));
        data->k2dsm_data_p.opt_scaling_1 = { opt_scaling[0], opt_scaling[1] };
    }
    // sanity check

    double min_x, min_y, max_x, max_y;
    min_x = max_x = scaling_grid_x[0];
    min_y = max_y = scaling_grid_y[0];

    for (auto i = 0; i < SIZE_OF_GRID_X; i++)
    {
        if (min_x > scaling_grid_x[i])
            min_x = scaling_grid_x[i];

        if (min_y > scaling_grid_y[i])
            min_y = scaling_grid_y[i];

        if (max_x < scaling_grid_x[i])
            max_x = scaling_grid_x[i];

        if (max_y < scaling_grid_y[i])
            max_y = scaling_grid_y[i];
    }

    auto is_pos_def = (quad_coef[0] + quad_coef[1]) > 0 && (quad_coef[0] * quad_coef[1] - quad_coef[2] * quad_coef[2] / 4) > 0;
    auto is_with_in_lims = (opt_scaling[0] > min_x) && (opt_scaling[0] < max_x) && (opt_scaling[1] > min_y) && (opt_scaling[1] < max_y);

    if (!is_pos_def || !is_with_in_lims)
    {
        double min_err = err_l2[0];
        int ind_min = 0;
        for (auto i = 0; i < SIZE_OF_GRID_X; i++)
        {
            if (min_err > err_l2[i])
            {
                min_err = err_l2[i];
                ind_min = i;
            }
        }
        opt_scaling[0] = scaling_grid_x[ind_min];
        opt_scaling[1] = scaling_grid_y[ind_min];
    }
    return { opt_scaling[0], opt_scaling[1] };
}

std::vector<double3x3> k_to_DSM::optimize_k_under_los_error
(
    algo_calibration_info const & regs,
    algo_calibration_registers const &dsm_regs,
    double scaling_grid_x[SIZE_OF_GRID_X],
    double scaling_grid_y[SIZE_OF_GRID_X]
)
{
    auto orig_k = _pre_process_data.orig_k;

    std::vector<double3x3> res(SIZE_OF_GRID_X, { 0 });

    for (auto i = 0; i < SIZE_OF_GRID_X; i++)
    {
        std::vector<double2> los(_pre_process_data.los_orig.size());
        std::vector<double3> updated_pixels(_pre_process_data.los_orig.size());

        for (auto j = 0; j < updated_pixels.size(); j++)
        {
            los[j].x = scaling_grid_x[i] * _pre_process_data.los_orig[j].x;
            los[j].y = scaling_grid_y[i] * _pre_process_data.los_orig[j].y;
        } 
        auto updated_vertices = convert_los_to_norm_vertices(regs, dsm_regs, los);
       
        for (auto i = 0; i < updated_vertices.size(); i++)
        {
            updated_pixels[i].x = updated_vertices[i].x*orig_k.fx + updated_vertices[i].z*orig_k.ppx;
            updated_pixels[i].y = updated_vertices[i].y*orig_k.fy + updated_vertices[i].z*orig_k.ppy;
            updated_pixels[i].z = updated_vertices[i].z;
        }

        std::vector<double2> v1(updated_pixels.size());
        std::vector<double2> v2(updated_pixels.size());
        std::vector<double> p1(updated_pixels.size());
        std::vector<double> p2(updated_pixels.size());

        for (auto i = 0; i < v1.size(); i++)
        {
            v1[i].x = _pre_process_data.vertices_orig[i].x;
            v1[i].y = 0;

            v2[i].x = 0;
            v2[i].y = _pre_process_data.vertices_orig[i].y;

            p1[i] = updated_pixels[i].x - orig_k.ppx*_pre_process_data.vertices_orig[i].z;
            p2[i] = updated_pixels[i].y - orig_k.ppy*_pre_process_data.vertices_orig[i].z;
        }
        v1.insert(v1.end(), v2.begin(), v2.end());
        p1.insert(p1.end(), p2.begin(), p2.end());

        double vtag_x_v[4] = { 0 };
        double vtag_x_p[2] = { 0 };

        for (auto i = 0; i < v1.size(); i++)
        {
            vtag_x_v[0] += v1[i].x*v1[i].x;
            vtag_x_v[1] += v1[i].x*v1[i].y;
            vtag_x_v[2] += v1[i].y*v1[i].x;
            vtag_x_v[3] += v1[i].y*v1[i].y;

            vtag_x_p[0] += v1[i].x*p1[i];
            vtag_x_p[1] += v1[i].y*p1[i];
        }

        double inv[2];

        direct_inv_2x2(vtag_x_v, vtag_x_p, inv);

        double3x3 mat = { inv[0],0.0,orig_k.ppx ,
                            0.0,inv[1],orig_k.ppy ,
                            0.0,0.0,1.0 };

        res[i] = mat;
    }
    return res;
}

std::vector<double3> k_to_DSM::convert_los_to_norm_vertices
(
    algo_calibration_info const & regs,
    algo_calibration_registers const &dsm_regs,
    std::vector<double2> los,
    data_collect* data
)
{
    std::vector<double3> fove_x_indicent_direction(los.size());
    std::vector<double3> fove_y_indicent_direction(los.size());

    auto laser_incident = laser_incident_direction({ (double)regs.FRMWlaserangleH , (double)regs.FRMWlaserangleV + 180 });

    std::vector<double2> dsm(los.size());

    for (auto i = 0; i < los.size(); i++)
    {
        dsm[i].x = (los[i].x + (double)dsm_regs.EXTLdsmXoffset)*(double)dsm_regs.EXTLdsmXscale - (double)2047;
        dsm[i].y = (los[i].y + (double)dsm_regs.EXTLdsmYoffset)*(double)dsm_regs.EXTLdsmYscale - (double)2047;

        auto dsm_x = dsm[i].x;
        auto dsm_y = dsm[i].y;

        auto dsm_x_corr_coarse = dsm_x + (dsm_x / 2047)*(double)regs.FRMWpolyVars[0] +
            std::pow(dsm_x / 2047, 2)*(double)regs.FRMWpolyVars[1] +
            std::pow(dsm_x / 2047, 3)*(double)regs.FRMWpolyVars[2];

        auto dsm_y_corr_coarse = dsm_y + (dsm_x / 2047)*(double)regs.FRMWpitchFixFactor;

        auto dsm_x_corr = dsm_x_corr_coarse + (dsm_x_corr_coarse / 2047)*(double)regs.FRMWundistAngHorz[0] +
            std::pow(dsm_x_corr_coarse / 2047, 2)*(double)regs.FRMWundistAngHorz[1] +
            std::pow(dsm_x_corr_coarse / 2047, 3)*(double)regs.FRMWundistAngHorz[2] +
            std::pow(dsm_x_corr_coarse / 2047, 4)*(double)regs.FRMWundistAngHorz[3];

        auto dsm_y_corr = dsm_y_corr_coarse;

        auto mode = 1;
        auto ang_x = dsm_x_corr * ((double)regs.FRMWxfov[mode] * 0.25 / 2047);
        auto ang_y = dsm_y_corr * ((double)regs.FRMWyfov[mode] * 0.25 / 2047);
        
        auto mirror_normal_direction = laser_incident_direction({ ang_x ,ang_y });
        
        fove_x_indicent_direction[i] = laser_incident - mirror_normal_direction*(2 * (mirror_normal_direction*laser_incident));
    }

    if (data)
        data->k2dsm_data_p.dsm = dsm;

    for (auto i = 0; i < fove_x_indicent_direction.size(); i++)
    {
        fove_x_indicent_direction[i].x /= sqrt(fove_x_indicent_direction[i].get_norm());
        fove_x_indicent_direction[i].y /= sqrt(fove_x_indicent_direction[i].get_norm());
        fove_x_indicent_direction[i].z /= sqrt(fove_x_indicent_direction[i].get_norm());
    }

    auto outbound_direction = fove_x_indicent_direction;
    if (regs.FRMWfovexExistenceFlag)
    {
        std::fill(outbound_direction.begin(), outbound_direction.end(), double3{ 0,0,0 });
        for (auto i = 0; i < fove_x_indicent_direction.size(); i++)
        {
            auto ang_pre_exp = rad_to_deg(std::acos(fove_x_indicent_direction[i].z));
            auto ang_post_exp = ang_pre_exp + ang_pre_exp * (double)regs.FRMWfovexNominal[0] +
                std::pow(ang_pre_exp, 2) * (double)regs.FRMWfovexNominal[1] +
                std::pow(ang_pre_exp, 3) * (double)regs.FRMWfovexNominal[2] +
                std::pow(ang_pre_exp, 4) * (double)regs.FRMWfovexNominal[3];

            outbound_direction[i].z = std::cos(deg_to_rad(ang_post_exp));
            auto xy_norm = fove_x_indicent_direction[i].x*fove_x_indicent_direction[i].x + 
                fove_x_indicent_direction[i].y*fove_x_indicent_direction[i].y;

            auto xy_factor = std::sqrt((1 - outbound_direction[i].z*outbound_direction[i].z) / xy_norm);
            outbound_direction[i].x = fove_x_indicent_direction[i].x*xy_factor;
            outbound_direction[i].y = fove_x_indicent_direction[i].y*xy_factor;
        };

    }
    for (auto i = 0; i < outbound_direction.size(); i++)
    {
        outbound_direction[i].x /= outbound_direction[i].z;
        outbound_direction[i].y /= outbound_direction[i].z;
        outbound_direction[i].z /= outbound_direction[i].z;
    }
    return outbound_direction;
}

std::vector<double3> k_to_DSM::calc_relevant_vertices
(
    const std::vector<uint8_t>& relevant_pixels_image, 
    const rs2_intrinsics_double & k
)
{
    std::vector<double3> res;

    double k_arr[9] = { k.fx, 0, 0,
                        0, k.fy, 0,
                        k.ppx , k.ppy, 1 };

    double k_arr_inv[9] = { 0 };

    inv(k_arr, k_arr_inv);

    double k_t[9] = { 0 };

    transpose(k_arr_inv, k_t);

    for (auto i = 0; i < k.height; i++)
    {
        for (auto j = 0; j < k.width; j++)
        {
            if (relevant_pixels_image[i*k.width + j])
            {
                double3 ver;
                double3 pix = { (double)j, (double)i, (double)1 };

                ver.x = pix.x*k_t[0] + pix.y*k_t[1] + pix.z*k_t[2];
                ver.y = pix.x*k_t[3] + pix.y*k_t[4] + pix.z*k_t[5];
                ver.z = pix.x*k_t[6] + pix.y*k_t[7] + pix.z*k_t[8];

                res.push_back(ver);
            }
        }
    }
    return res;
}

std::vector<double2> k_to_DSM::convert_norm_vertices_to_los
(
    algo_calibration_info const &regs,
    algo_calibration_registers const &dsm_regs,
    std::vector< double3 > const & vertices,
    convert_norm_vertices_to_los_data* data
)
{
    const size_t angle = 45;
    auto directions = transform_to_direction(vertices);

    auto fovex_indicent_direction = directions;
    if (_regs.FRMWfovexExistenceFlag)
    {
        std::fill(fovex_indicent_direction.begin(), fovex_indicent_direction.end(), double3{ 0,0,0 });
        std::vector<double> ang_post_exp(fovex_indicent_direction.size(), 0);
        for (auto i = 0; i < ang_post_exp.size(); i++)
        {
            ang_post_exp[i] = std::acos(directions[i].z)* (double)180. / (double)M_PI;
        }

        std::vector<double> ang_grid(angle, 0);
        std::vector<double> ang_out_on_grid(angle, 0);

        for (auto i = 0; i < angle; i++)
        {
            ang_grid[i] = i;
            auto fovex_nominal = std::pow(i,1)* (double)regs.FRMWfovexNominal[0] +
                std::pow(i, 2)* (double)regs.FRMWfovexNominal[1] +
                std::pow(i, 3)* (double)regs.FRMWfovexNominal[2] +
                std::pow(i, 4)* (double)regs.FRMWfovexNominal[3];

            ang_out_on_grid[i] = (double)i + fovex_nominal;
        }
        std::vector<double> ang_pre_exp = interp1(ang_out_on_grid, ang_grid, ang_post_exp);
        //std::vector<double> xy_norm(directions.size(), 0);
        for (auto i = 0; i < fovex_indicent_direction.size(); i++)
        {
            fovex_indicent_direction[i].z = std::cos(ang_pre_exp[i] * M_PI / (double)180.);
            auto xy_norm = directions[i].x*directions[i].x + directions[i].y*directions[i].y;
            auto xy_factor = sqrt(((double)1 - (fovex_indicent_direction[i].z* fovex_indicent_direction[i].z)) / xy_norm);
            fovex_indicent_direction[i].x = directions[i].x*xy_factor;
            fovex_indicent_direction[i].y = directions[i].y*xy_factor;
        }
    }

    auto laser_incident = laser_incident_direction({ (double)_regs.FRMWlaserangleH , (double)_regs.FRMWlaserangleV + (double)180 });

    std::vector<double3> mirror_normal_direction(fovex_indicent_direction.size());
    std::vector<double> dsm_x_corr(fovex_indicent_direction.size());
    std::vector<double> dsm_y_corr(fovex_indicent_direction.size());
    std::vector<double> ang_x(fovex_indicent_direction.size());
    std::vector<double> ang_y(fovex_indicent_direction.size());

    for (auto i = 0; i < fovex_indicent_direction.size(); i++)
    {
        mirror_normal_direction[i].x = fovex_indicent_direction[i].x - laser_incident[0];
        mirror_normal_direction[i].y = fovex_indicent_direction[i].y - laser_incident[1];
        mirror_normal_direction[i].z = fovex_indicent_direction[i].z - laser_incident[2];

        auto norm = sqrt(mirror_normal_direction[i].x*mirror_normal_direction[i].x +
            mirror_normal_direction[i].y* mirror_normal_direction[i].y +
            mirror_normal_direction[i].z* mirror_normal_direction[i].z);

        mirror_normal_direction[i].x /= norm;
        mirror_normal_direction[i].y /= norm;
        mirror_normal_direction[i].z /= norm;

        ang_x[i] = std::atan(mirror_normal_direction[i].x / mirror_normal_direction[i].z)* (double)180. / (double)M_PI;
        ang_y[i] = std::asin(mirror_normal_direction[i].y)* (double)180. / (double)M_PI;
        
        int mirror_mode = 1/*_regs.frmw.mirrorMovmentMode*/;

        dsm_x_corr[i] = ang_x[i] / (double)(regs.FRMWxfov[mirror_mode - 1] * (double)0.25 / (double)2047);
        dsm_y_corr[i] = ang_y[i] / (double)(regs.FRMWyfov[mirror_mode - 1] * (double)0.25 / (double)2047);
    }

    std::vector<double> dsm_grid(421);

    auto ind = 0;
    for (auto i = -2100; i <= 2100; i += 10)
    {
        dsm_grid[ind++] = i;
    }

    std::vector<double> dsm_x_coarse_on_grid(dsm_grid.size());
    std::vector<double> dsm_x_corr_on_grid(dsm_grid.size());
    for (auto i = 0; i < dsm_grid.size(); i ++)
    {
        double rot[3] = { std::pow((dsm_grid[i] / (double)2047), 1), std::pow((dsm_grid[i] / 2047), 2), std::pow((dsm_grid[i] / 2047), 3) };
        dsm_x_coarse_on_grid[i] = dsm_grid[i] + rot[0] * (double)_regs.FRMWpolyVars[0] + rot[1] * (double)_regs.FRMWpolyVars[1] + rot[2] * (double)_regs.FRMWpolyVars[2];
        auto val = dsm_x_coarse_on_grid[i] / (double)2047;
        double vals2[4] = { std::pow(val , 1),std::pow(val , 2), std::pow(val , 3) , std::pow(val , 4) };
        dsm_x_corr_on_grid[i] = dsm_x_coarse_on_grid[i] + vals2[0] * (double)_regs.FRMWundistAngHorz[0] +
            vals2[1] * (double)_regs.FRMWundistAngHorz[1] +
            vals2[2] * (double)_regs.FRMWundistAngHorz[2] +
            vals2[3] * (double)_regs.FRMWundistAngHorz[3];
    }

    auto dsm_x = interp1(dsm_x_corr_on_grid, dsm_grid, dsm_x_corr);
    std::vector<double> dsm_y(dsm_x.size());

    for (auto i = 0; i < dsm_y.size(); i++)
    {
        dsm_y[i] = dsm_y_corr[i] - (dsm_x[i] / (double)2047)*(double)_regs.FRMWpitchFixFactor;
    }
   
    std::vector<double2> res(dsm_x.size());

    if (data)
    {
        data->laser_incident = laser_incident;
        data->fovex_indicent_direction = fovex_indicent_direction;
        data->mirror_normal_direction = mirror_normal_direction;
        data->ang_x = ang_x;
        data->ang_y = ang_y;
        data->dsm_x_corr = dsm_x_corr;
        data->dsm_y_corr = dsm_y_corr;
        data->dsm_y = dsm_y;
        data->dsm_x = dsm_x;
        data->dsm_y = dsm_y;
    }
    for (auto i = 0; i < res.size(); i++)
    {
        res[i].x = (dsm_x[i] + 2047.) / (double)dsm_regs.EXTLdsmXscale - (double)dsm_regs.EXTLdsmXoffset;
        res[i].y = (dsm_y[i] + 2047.) / (double)dsm_regs.EXTLdsmYscale - (double)dsm_regs.EXTLdsmYoffset;
    }
    return res;
}

double3 k_to_DSM::laser_incident_direction(double2 angle_rad)
{
    double2 angle_deg = { angle_rad.x * (double)M_PI / (double)180., (angle_rad.y)* (double)M_PI / (double)180. };

    double3 laser_incident_direction = { std::cos(angle_deg.y)*std::sin(angle_deg.x),
                                        std::sin(angle_deg.y),
                                        std::cos(angle_deg.y)*std::cos(angle_deg.x) };

    return laser_incident_direction;
}

std::vector< double3 > k_to_DSM::transform_to_direction( std::vector< double3 > const & vec )
{
    std::vector< double3 > res( vec.size() );

    for( auto i = 0; i < vec.size(); i++ )
    {
        auto norm = sqrt( vec[i].x * vec[i].x + vec[i].y * vec[i].y + vec[i].z * vec[i].z );
        res[i] = { vec[i].x / norm, vec[i].y / norm, vec[i].z / norm };
    }
    return res;
}
