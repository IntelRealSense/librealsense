//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "optimizer.h"
#include "cost.h"
#include "utils.h"
#include "debug.h"

using namespace librealsense::algo::depth_to_rgb_calibration;


// Return the average pixel movement from the calibration
double optimizer::calc_correction_in_pixels( calib const & from_calibration, calib const & to_calibration) const
{
    //%    [uvMap,~,~] = OnlineCalibration.aux.projectVToRGB(frame.vertices,params.rgbPmat,params.Krgb,params.rgbDistort);
    //% [uvMapNew,~,~] = OnlineCalibration.aux.projectVToRGB(frame.vertices,newParams.rgbPmat,newParams.Krgb,newParams.rgbDistort);
    auto old_uvmap = get_texture_map( _z.orig_vertices, from_calibration, from_calibration.calc_p_mat());
    auto new_uvmap = get_texture_map( _z.vertices, to_calibration, to_calibration.calc_p_mat());

    return calc_correction_in_pixels( old_uvmap, new_uvmap );
}


double optimizer::calc_correction_in_pixels( uvmap_t const & old_uvmap, uvmap_t const & new_uvmap ) const
{
    // uvmap is Nx[x,y]

    if( old_uvmap.size() != new_uvmap.size() )
        throw std::runtime_error( to_string()
                                  << "did not expect different uvmap sizes (" << old_uvmap.size()
                                  << " vs " << new_uvmap.size() << ")" );

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
    double xy_movement = calc_correction_in_pixels(_final_calibration);

    // Clip any (average) movement of pixels if it's too big
    AC_LOG( INFO, "    average pixel movement= " << xy_movement );

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

    auto uvmap_old = get_texture_map(_z.orig_vertices, old_calib, old_calib.calc_p_mat());
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


void optimizer::clip_ac_scaling( rs2_dsm_params_double const & ac_data_in,
                                 rs2_dsm_params_double & ac_data_new ) const
{
    if( abs( ac_data_in.h_scale - ac_data_new.h_scale ) > _params.max_global_los_scaling_step )
    {
        double const new_h_scale = ac_data_in.h_scale
                                 + ( ac_data_new.h_scale - ac_data_in.h_scale )
                                       / abs( ac_data_new.h_scale - ac_data_in.h_scale )
                                       * _params.max_global_los_scaling_step;
        AC_LOG( DEBUG,
                "    " << AC_D_PREC << "H scale {new}" << ac_data_new.h_scale
                       << " is not within {step}"
                       << std::string( to_string() << _params.max_global_los_scaling_step )
                       << " of {old}" << ac_data_in.h_scale << "; clipping to {final}"
                       << new_h_scale << " [CLIP-H]" );
        ac_data_new.h_scale = new_h_scale;
    }
    if( abs( ac_data_in.v_scale - ac_data_new.v_scale ) > _params.max_global_los_scaling_step )
    {
        double const new_v_scale = ac_data_in.v_scale
                                 + ( ac_data_new.v_scale - ac_data_in.v_scale )
                                       / abs( ac_data_new.v_scale - ac_data_in.v_scale )
                                       * _params.max_global_los_scaling_step;
        AC_LOG( DEBUG,
                "    " << AC_D_PREC << "V scale {new}" << ac_data_new.v_scale
                       << " is not within {step}"
                       << std::string( to_string() << _params.max_global_los_scaling_step )
                       << " of {old}" << ac_data_in.v_scale << "; clipping to {final}"
                       << new_v_scale << " [CLIP-V]" );
        ac_data_new.v_scale = new_v_scale;
    }
}

std::vector< double > extract_features(decision_params& decision_params)
{
    svm_features features;
    std::vector< double > res;
    auto max_elem = *std::max_element(decision_params.distribution_per_section_depth.begin(), decision_params.distribution_per_section_depth.end());
    auto min_elem = *std::min_element(decision_params.distribution_per_section_depth.begin(), decision_params.distribution_per_section_depth.end());
    features.max_over_min_depth = max_elem / (min_elem + 1e-3);
    res.push_back(features.max_over_min_depth);

    max_elem = *std::max_element(decision_params.distribution_per_section_rgb.begin(), decision_params.distribution_per_section_rgb.end());
    min_elem = *std::min_element(decision_params.distribution_per_section_rgb.begin(), decision_params.distribution_per_section_rgb.end());
    features.max_over_min_rgb = max_elem / (min_elem + 1e-3);
    res.push_back(features.max_over_min_rgb);

    std::vector<double>::iterator it = decision_params.edge_weights_per_dir.begin();
    max_elem = std::max(*it, *(it + 2));
    min_elem = std::min(*it, *(it + 2));
    features.max_over_min_perp = max_elem / (min_elem + 1e-3);
    res.push_back(features.max_over_min_perp);

    max_elem = std::max(*(it + 1), *(it + 3));
    min_elem = std::min(*(it + 1), *(it + 3));
    features.max_over_min_diag = max_elem / (min_elem + 1e-3);
    res.push_back(features.max_over_min_diag);

    features.initial_cost = decision_params.initial_cost;
    features.final_cost = decision_params.new_cost;
    res.push_back(features.initial_cost);
    res.push_back(features.final_cost);

    features.xy_movement = decision_params.xy_movement;
    if (features.xy_movement > 100) { features.xy_movement = 100; }
    features.xy_movement_from_origin = decision_params.xy_movement_from_origin;
    if (features.xy_movement_from_origin > 100) { features.xy_movement_from_origin = 100; }
    res.push_back(features.xy_movement);
    res.push_back(features.xy_movement_from_origin);

    std::vector<double>::iterator  iter = decision_params.improvement_per_section.begin();
    features.positive_improvement_sum = 0;
    features.negative_improvement_sum = 0;
    for (int i = 0; i < decision_params.improvement_per_section.size(); i++)
    {
        if (*(iter + i) > 0)
        {
            features.positive_improvement_sum += *(iter + i);
        }
        else
        {
            features.negative_improvement_sum += *(iter + i);
        }
    }
    res.push_back(features.positive_improvement_sum);
    res.push_back(features.negative_improvement_sum);

    return res;
}
void optimizer::collect_decision_params(z_frame_data& z_data, yuy2_frame_data& yuy_data)
{
     auto uvmap = get_texture_map(_z.orig_vertices,
        _original_calibration,
        _original_calibration.calc_p_mat());

    _decision_params.initial_cost = calc_cost(z_data, yuy_data, uvmap); //1.560848046875000e+04;
    //_decision_params.is_valid = 0;
    _decision_params.xy_movement = calc_correction_in_pixels(_optimaized_calibration); //2.376f; // 
    _decision_params.xy_movement_from_origin = calc_correction_in_pixels(_optimaized_calibration); //2.376f;
    _decision_params.improvement_per_section = _z.cost_diff_per_section; // { -4.4229550, 828.93903, 1424.0482, 2536.4409 }; 
    _decision_params.min_improvement_per_section = *std::min_element(_z.cost_diff_per_section.begin(), _z.cost_diff_per_section.end());// -4.422955036163330;
    _decision_params.max_improvement_per_section = *std::max_element(_z.cost_diff_per_section.begin(), _z.cost_diff_per_section.end()); //2.536440917968750e+03;// 
    //_decision_params.is_valid_1 = 1;
    //_decision_params.moving_pixels = 0;
    _decision_params.min_max_ratio_depth = z_data.min_max_ratio; // 0.762463343108504;
    _decision_params.distribution_per_section_depth = z_data.sum_weights_per_section; //{ 980000, 780000, 1023000, 816000 };
    _decision_params.min_max_ratio_rgb = yuy_data.min_max_ratio; // 0.618130692181835;
    _decision_params.distribution_per_section_rgb = yuy_data.sum_weights_per_section; //{3025208, 2.899468500000000e+06, 4471484, 2.763961500000000e+06};
    _decision_params.dir_ratio_1 = z_data.dir_ratio1;// 2.072327044025157;
    //_decision_params.dir_ratio_2 = z_data.dir_ratio2;
    _decision_params.edge_weights_per_dir = z_data.sum_weights_per_direction;// { 636000, 898000, 1318000, 747000 };
    std::vector<double2> new_uvmap = get_texture_map(_z.vertices, _optimaized_calibration, _optimaized_calibration.calc_p_mat());
    _decision_params.new_cost = calc_cost(z_data, yuy_data, new_uvmap);// 1.677282421875000e+04; 

}
bool svm_rbf_predictor(std::vector< double >& features, svm_model_gaussian& svm_model)
{
    bool res = true;
    std::vector< double > x_norm;
    // Applying the model
    for (auto i = 0; i < features.size(); i++)
    {
        x_norm.push_back((*(features.begin() + i) - *(svm_model.mu.begin() + i)) / (*(svm_model.sigma.begin() + i)));
    }

    // Extracting model parameters
    auto mu = svm_model.mu;
    auto sigma = svm_model.sigma;
    auto x_sv = svm_model.support_vectors;
    auto y_sv = svm_model.support_vectors_labels;
    auto alpha = svm_model.alpha;
    auto bias = svm_model.bias;
    auto gamma = 1 / (svm_model.kernel_param_scale * svm_model.kernel_param_scale);
    int n_samples = 1;// size(featuresMat, 1);
    //labels = zeros(nSamples, 1);
   /* innerProduct = exp(-gamma * sum((xNorm(iSample, :) - xSV). ^ 2, 2));
    score = sum(alpha.*ySV.*innerProduct, 1) + bias;
    labels(iSample) = score > 0; % dealing with the theoretical possibility of score = 0*/
    std::vector< double > inner_product;
    double score = 0;

    for (auto i = 0; i < y_sv.size(); i++)
    {
        double sum_raw = 0;
        for (auto k = 0; k < x_norm.size(); k++)
        {
            double res1 = *(x_norm.begin() + k) - *(x_sv[k].begin() + i);
            sum_raw += res1 * res1;
        }
        double final_sum = exp(-gamma * sum_raw);
        inner_product.push_back(final_sum);
        score += *(alpha.begin() + i) * *(y_sv.begin() + i) * final_sum;
        //score = sum(alpha .* ySV .* innerProduct, 1) + bias;
    }
    score += bias;

    if (score < 0)
    {
        res = false;
    }
    return res;
}
bool optimizer::valid_by_svm(svm_model model)
{
    bool is_valid = true;

    collect_decision_params(_z, _yuy);
    _extracted_features = extract_features(_decision_params);

    double res = 0;
    switch (model)
    {
    case linear:
        for (auto i = 0; i < _svm_model_linear.mu.size(); i++)
        {
            // isValid = (featuresMat-SVMModel.Mu)./SVMModel.Sigma*SVMModel.Beta+SVMModel.Bias > 0;
            auto res1 = (*(_extracted_features.begin() + i) - *(_svm_model_linear.mu.begin() + i)) / (*(_svm_model_linear.sigma.begin() + i));// **(_svm_model_linear.beta.begin() + i) + _svm_model_linear.bias);
            res += res1 * *(_svm_model_linear.beta.begin() + i);
        }
        res += _svm_model_linear.bias;
        if (res < 0)
        {
            AC_LOG( INFO, "Calibration invalid according to SVM linear model" );
            is_valid = false;
        }
        break;

    case gaussian:
        is_valid = svm_rbf_predictor(_extracted_features, _svm_model_gaussian);
        break;

    default:
        AC_LOG(DEBUG, "ERROR : Unknown SVM kernel " << model);
        break;
    }

    return is_valid;
}


bool optimizer::is_valid_results()
{
    if( get_final_data_from_bin )
    {
        _z.vertices = _vertices_from_bin;
        _final_calibration = decompose_p_mat( _p_mat_from_bin );
        _optimaized_calibration = decompose_p_mat( _p_mat_from_bin_opt );
    }


    // Clip any (average) movement of pixels if it's too big
    clip_pixel_movement();

    // Based on (possibly new, clipped) calibration values, see if we've strayed too
    // far away from the camera's original factory calibration -- which we may not have
    if( _factory_calibration.width  &&  _factory_calibration.height )
    {
        double xy_movement = calc_correction_in_pixels(_final_calibration);
        AC_LOG( DEBUG, "    average pixel movement from factory calibration= " << xy_movement );
        if( xy_movement > _params.max_xy_movement_from_origin )
        {
            AC_LOG( ERROR, "Calibration has moved too far from the original factory calibration (" << xy_movement << " pixels)" );
            return false;
        }
    }
    else
    {
        AC_LOG( DEBUG, "    no factory calibration available; skipping distance check" );
    }

    /* %% Check and see that the score didn't increased by a lot in one image section and decreased in the others
     % [c1, costVecOld] = OnlineCalibration.aux.calculateCost( frame.vertices, frame.weights, frame.rgbIDT, params );
     % [c2, costVecNew] = OnlineCalibration.aux.calculateCost( frame.vertices, frame.weights, frame.rgbIDT, newParams );
     % scoreDiffPerVertex = costVecNew - costVecOld;
     % for i = 0:(params.numSectionsH*params.numSectionsV) - 1
     %     scoreDiffPersection( i + 1 ) = nanmean( scoreDiffPerVertex( frame.sectionMapDepth == i ) );
     % end*/
    _z.cost_diff_per_section
        = cost_per_section_diff( _original_calibration, _optimaized_calibration );
    //% validOutputStruct.minImprovementPerSection = min( scoreDiffPersection );
    //% validOutputStruct.maxImprovementPerSection = max( scoreDiffPersection );
    double min_cost_diff
        = *std::min_element( _z.cost_diff_per_section.begin(), _z.cost_diff_per_section.end() );
    double max_cost_diff
        = *std::max_element( _z.cost_diff_per_section.begin(), _z.cost_diff_per_section.end() );
    AC_LOG( DEBUG, "    min cost diff= " << min_cost_diff << "  max= " << max_cost_diff );

    bool res_svm = valid_by_svm( linear );  //(gaussian);
    if( ! res_svm )
        return false;

#if 0
    try
    {
        validate_dsm_params( get_dsm_params() );
    }
    catch( invalid_value_exception const & e )
    {
        AC_LOG( ERROR, "Result DSM parameters are invalid: " << e.what() );
        return false;
    }
#endif

    return true;
}


void librealsense::algo::depth_to_rgb_calibration::validate_dsm_params(
    rs2_dsm_params const & dsm_params )
{
    /*  Considerable values for DSM correction:
        - h/vFactor: 0.98-1.02, representing up to 2% change in FOV.
        - h/vOffset:
            - Under AOT model: (-2)-2, representing up to 2deg FOV tilt
            - Under TOA model: (-125)-125, representing up to approximately
              2deg FOV tilt
        These values are extreme. For more reasonable values take 0.99-1.01
        for h/vFactor and divide the suggested h/vOffset range by 10.

        Update ww30: +/-1.5% limiter both H/V [0.985..1.015] until AC3.
        Update ww33: vFactor for all 60 cocktail 1500h units is in the range
                     of 1.000-1.015; changing to [0.995-1.015]
    */
    std::string error;
    
    if( dsm_params.model != RS2_DSM_CORRECTION_AOT )
        error += to_string() << " {mode}" << +dsm_params.model << " must be AOT";

    if( dsm_params.h_scale < 0.985 || dsm_params.h_scale > 1.015 )
        error += to_string() << " {H-scale}" << dsm_params.h_scale << " exceeds 1.5% change";
    if( dsm_params.v_scale < 0.995 || dsm_params.v_scale > 1.015 )
        error += to_string() << " {V-scale}" << dsm_params.v_scale << " exceeds [-0.5%-1.5%]";

    if( dsm_params.h_offset < -2. || dsm_params.h_offset > 2. )
        error += to_string() << " {H-offset}" << dsm_params.h_offset << " is limited to 2 degrees";
    if( dsm_params.v_offset < -2. || dsm_params.v_offset > 2. )
        error += to_string() << " {V-offset}" << dsm_params.v_offset << " is limited to 2 degrees";

    if( ! error.empty() )
        throw invalid_value_exception( "invalid DSM:" + error + " [LIMIT]" );
}

