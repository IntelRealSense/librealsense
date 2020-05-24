// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.


void compare_scene( std::string const & scene_dir )
{
    TRACE( "Loading " << scene_dir << " ..." );

    camera_params ci = read_camera_params( scene_dir, "ac1x\\camera_params" );
    dsm_params dsm = read_dsm_params(scene_dir, "ac1x\\DSM_params");
    ci.dsm_params = dsm.dsm_params;
    scene_metadata md( scene_dir );

    algo::optimizer cal;

    init_algo( cal, scene_dir,
        md.rgb_file,
        md.rgb_prev_file,
        md.ir_file,
        md.z_file,
        ci );

    auto& z_data = cal.get_z_data();
    auto& ir_data = cal.get_ir_data();
    auto& yuy_data = cal.get_yuy_data();
    auto& depth_data = cal.get_z_data();
    auto& decision_params = cal.get_decision_params();
    auto& svm_features = cal.get_extracted_features();
    //---
    auto rgb_h = ci.rgb.height;
    auto rgb_w = ci.rgb.width;
    auto z_h = ci.z.height;
    auto z_w = ci.z.width;
    auto num_of_calib_elements = 17;
    auto num_of_p_matrix_elements = sizeof(algo::p_matrix) / sizeof(double);

#if 1
    // smearing
    CHECK(compare_to_bin_file< double >(depth_data.gradient_x, scene_dir, "ac1x\\Zx", z_w, z_h, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.gradient_y, scene_dir, "ac1x\\Zy", z_w, z_h, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.gradient_x, scene_dir, "ac1x\\Ix", z_w, z_h, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.gradient_y, scene_dir, "ac1x\\Iy", z_w, z_h, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.edges, scene_dir, "ac1x\\iEdge", z_w, z_h, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.edges, scene_dir, "ac1x\\zedge", z_w, z_h, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< uint8_t >(depth_data.section_map_depth, scene_dir, "ac1x\\sectionMapDepth", z_w, z_h, "uint8_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.valid_edge_pixels_by_ir, scene_dir, "ac1x\\validEdgePixelsByIR", z_w, z_h, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.valid_location_rc_x, scene_dir, "ac1x\\gridXValid", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.valid_location_rc_y, scene_dir, "ac1x\\gridYValid", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.valid_location_rc, scene_dir, "ac1x\\locRC", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< uint8_t >(ir_data.valid_section_map, scene_dir, "ac1x\\sectionMapValid", 1, md.n_valid_ir_edges, "uint8_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.valid_gradient_x, scene_dir, "ac1x\\IxValid", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.valid_gradient_y, scene_dir, "ac1x\\IyValid", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.direction_deg, scene_dir, "ac1x\\directionInDeg", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.direction_per_pixel, scene_dir, "ac1x\\dirPerPixel", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.valid_direction_per_pixel, scene_dir, "ac1x\\validdirPerPixel", 1, md.n_valid_pixels, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region[0], scene_dir, "ac1x\\localRegion", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region[1], scene_dir, "ac1x\\localRegion", 2, md.n_valid_ir_edges, "double_01", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region[2], scene_dir, "ac1x\\localRegion", 2, md.n_valid_ir_edges, "double_02", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region[3], scene_dir, "ac1x\\localRegion", 2, md.n_valid_ir_edges, "double_03", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_x[0], scene_dir, "ac1x\\localRegion_x", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_x[1], scene_dir, "ac1x\\localRegion_x", 1, md.n_valid_ir_edges, "double_01", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_x[2], scene_dir, "ac1x\\localRegion_x", 1, md.n_valid_ir_edges, "double_02", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_x[3], scene_dir, "ac1x\\localRegion_x", 1, md.n_valid_ir_edges, "double_03", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_y[0], scene_dir, "ac1x\\localRegion_y", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_y[1], scene_dir, "ac1x\\localRegion_y", 1, md.n_valid_ir_edges, "double_01", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_y[2], scene_dir, "ac1x\\localRegion_y", 1, md.n_valid_ir_edges, "double_02", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_y[3], scene_dir, "ac1x\\localRegion_y", 1, md.n_valid_ir_edges, "double_03", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_edges, scene_dir, "ac1x\\localEdges", 4, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< uint8_t >(ir_data.is_supressed, scene_dir, "ac1x\\isSupressed", 1, md.n_valid_ir_edges, "uint8_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.fraq_step, scene_dir, "ac1x\\fraqStep", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.local_rc_subpixel, scene_dir, "ac1x\\locRCsub", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.local_x, scene_dir, "ac1x\\localZx", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.local_y, scene_dir, "ac1x\\localZy", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.gradient, scene_dir, "ac1x\\zGrad", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.grad_in_direction, scene_dir, "ac1x\\validzGradInDirection", 1, md.n_valid_pixels, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.local_values, scene_dir, "ac1x\\localZvalues", 4, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.edge_sub_pixel, scene_dir, "ac1x\\edgeSubPixel", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.values_for_subedges, scene_dir, "ac1x\\validzValuesForSubEdges", 1, md.n_valid_pixels, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.supressed_edges, scene_dir, "ac1x\\validEdgePixels", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< uint8_t >(depth_data.valid_section_map, scene_dir, "ac1x\\validsectionMapDepth", 1, md.n_valid_pixels, "uint8_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.valid_directions, scene_dir, "ac1x\\validdirectionIndex", 1, md.n_valid_pixels, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.valid_edge_sub_pixel, scene_dir, "ac1x\\validedgeSubPixel", 2, md.n_valid_pixels, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.valid_edge_sub_pixel_x, scene_dir, "ac1x\\xim", 1, md.n_valid_pixels, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.valid_edge_sub_pixel_y, scene_dir, "ac1x\\yim", 1, md.n_valid_pixels, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double>(depth_data.sub_points, scene_dir, "ac1x\\subPoints", 3, md.n_valid_pixels, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< algo::double2 >(depth_data.uvmap, scene_dir, bin_file("ac1x\\uv", 2, md.n_valid_pixels, "double_00") + ".bin", md.n_valid_pixels, 1, compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.is_inside, scene_dir, "ac1x\\isInside", 1, md.n_valid_pixels, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.subpixels_x, scene_dir, "ac1x\\Z_xim", 1, md.n_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.subpixels_y, scene_dir, "ac1x\\Z_yim", 1, md.n_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.closest, scene_dir, "ac1x\\zValuesForSubEdges", 1, md.n_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.grad_in_direction_inside, scene_dir, "ac1x\\zGradInDirection", 1, md.n_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.directions, scene_dir, "ac1x\\directionIndexInside", 1, md.n_edges, "double_00", compare_same_vectors));
    
    CHECK(compare_to_bin_file< double >(depth_data.subpixels_x_round, scene_dir, "ac1x\\round_xim", 1, md.n_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.subpixels_y_round, scene_dir, "ac1x\\round_yim", 1, md.n_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.weights, scene_dir, "ac1x\\weights", 1, md.n_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< uint8_t >(depth_data.relevant_pixels_image, scene_dir, "ac1x\\relevantPixelsImage", z_w, z_h, "uint8_00", compare_same_vectors));
    CHECK(compare_to_bin_file< algo::double3 >(depth_data.vertices, scene_dir, bin_file("ac1x\\vertices", 3, md.n_edges, "double_00") + ".bin", md.n_edges, 1, compare_same_vectors));
    CHECK(compare_to_bin_file< uint8_t >(depth_data.section_map_depth_inside, scene_dir, "ac1x\\sectionMapDepthInside", 1, md.n_edges, "uint8_00", compare_same_vectors));
#endif

#if 1
    // ---
    TRACE( "\nChecking scene validity:" );

    CHECK( cal.is_scene_valid() == md.is_scene_valid );

    // edge distribution
    CHECK( compare_to_bin_file< double >( z_data.sum_weights_per_section, scene_dir, "ac1x\\depthEdgeWeightDistributionPerSectionDepth", 1, 4, "double_00", compare_same_vectors ) );

    //CHECK( compare_to_bin_file< byte >( z_data.section_map, scene_dir, "sectionMapDepth_trans", 1, md.n_edges, "uint8_00", compare_same_vectors ) );
    //CHECK( compare_to_bin_file< byte >( yuy_data.section_map, scene_dir, "sectionMapRgb_trans", 1, rgb_w*rgb_h, "uint8_00", compare_same_vectors ) );

    CHECK( compare_to_bin_file< double >( yuy_data.sum_weights_per_section, scene_dir, "ac1x\\edgeWeightDistributionPerSectionRgb", 1, 4, "double_00", compare_same_vectors ) );

    // gradient balanced
    // TODO NOHA
    CHECK(compare_to_bin_file< double >(z_data.sum_weights_per_direction, scene_dir, "ac1x\\edgeWeightsPerDir", 4, 1, "double_00", compare_same_vectors));

    // movment check
    // 1. dilation
    CHECK( compare_to_bin_file< uint8_t >( yuy_data.prev_logic_edges, scene_dir, "ac1x\\logicEdges", rgb_w, rgb_h, "uint8_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( yuy_data.dilated_image, scene_dir, "ac1x\\dilatedIm", rgb_w, rgb_h, "double_00", compare_same_vectors ) );

    // 2. gausssian
    CHECK( compare_to_bin_file< double >( yuy_data.yuy_diff, scene_dir, "ac1x\\diffIm_01", rgb_w, rgb_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( yuy_data.gaussian_filtered_image, scene_dir, "ac1x\\diffIm", rgb_w, rgb_h, "double_00", compare_same_vectors ) );

    // 3. movement
    CHECK( compare_to_bin_file< double >( yuy_data.gaussian_diff_masked, scene_dir, "ac1x\\IDiffMasked", rgb_w, rgb_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< uint8_t >( yuy_data.move_suspect, scene_dir, "ac1x\\ixMoveSuspect", rgb_w, rgb_h, "uint8_00", compare_same_vectors ) );

#endif
#if 1
    //--
    TRACE( "\nOptimizing:" );
    auto cb = [&]( algo::iteration_data_collect const & data )
    {
        // data.iteration is 0-based!
        REQUIRE( data.iteration < md.n_iterations );

        auto file = bin_file("ac1x\\p_matrix_iteration", data.iteration + 1, num_of_p_matrix_elements, 1, "double_00.bin");
        CHECK(compare_to_bin_file< double >(std::vector< double >(std::begin(data.params.curr_p_mat.vals), std::end(data.params.curr_p_mat.vals)),
            scene_dir, file, num_of_p_matrix_elements, 1, compare_same_vectors));

        file = bin_file("ac1x\\cost_iteration", data.iteration + 1, 1, 1, "double_00.bin");
        CHECK(compare_to_bin_file< double >(std::vector< double >(1, data.params.cost),
            scene_dir, file, 1, 1, compare_same_vectors));

        file = bin_file( "ac1x\\uvmap_iteration", data.iteration + 1, 2, md.n_edges, "double_00.bin" );
        CHECK( compare_to_bin_file< algo::double2 >( data.uvmap, scene_dir, file, md.n_edges, 1, compare_same_vectors ) );

        file = bin_file( "ac1x\\DVals_iteration", data.iteration + 1, 1, md.n_edges, "double_00.bin" );
        CHECK( compare_to_bin_file< double >( data.d_vals, scene_dir, file, md.n_edges, 1, compare_same_vectors ) );

        file = bin_file( "ac1x\\DxVals_iteration", data.iteration + 1, 1, md.n_edges, "double_00.bin" );
        CHECK( compare_to_bin_file< double >( data.d_vals_x, scene_dir, file, md.n_edges, 1, compare_same_vectors ) );

        file = bin_file( "ac1x\\DyVals_iteration", data.iteration + 1, 1, md.n_edges, "double_00.bin" );
        CHECK( compare_to_bin_file< double >( data.d_vals_y, scene_dir, file, md.n_edges, 1, compare_same_vectors ) );

        file = bin_file( "ac1x\\xy_iteration", data.iteration + 1, 2, md.n_edges, "double_00.bin" );
        CHECK( compare_to_bin_file< algo::double2 >( data.xy, scene_dir, file, md.n_edges, 1, compare_same_vectors ) );

        file = bin_file( "ac1x\\rc_iteration", data.iteration + 1, 1, md.n_edges, "double_00.bin" );
        CHECK( compare_to_bin_file< double >( data.rc, scene_dir, file, md.n_edges, 1, compare_same_vectors ) );

        file = bin_file( "ac1x\\xCoeff_P_iteration", data.iteration + 1, num_of_p_matrix_elements, md.n_edges, "double_00.bin" );
        CHECK( compare_to_bin_file< algo::p_matrix>( data.coeffs_p.x_coeffs, scene_dir, file, md.n_edges, 1, compare_same_vectors ) );

        file = bin_file("ac1x\\yCoeff_P_iteration", data.iteration + 1, num_of_p_matrix_elements, md.n_edges, "double_00.bin");
        CHECK(compare_to_bin_file< algo::p_matrix>(data.coeffs_p.y_coeffs, scene_dir, file, md.n_edges, 1, compare_same_vectors));
        
        file = bin_file("ac1x\\grad_iteration", data.iteration + 1, num_of_p_matrix_elements, 1, "double_00.bin");
        CHECK(compare_to_bin_file< double >(std::vector< double >(std::begin(data.params.calib_gradients.vals), std::end(data.params.calib_gradients.vals)),
            scene_dir, file, num_of_p_matrix_elements, 1, compare_same_vectors));
        
        file = bin_file("ac1x\\grad_norma_iteration", data.iteration + 1, 1, 1, "double_00.bin");
        CHECK(compare_to_bin_file< double >(std::vector< double >(1, data.grads_norma),
            scene_dir, file, 1, 1, compare_same_vectors));
        
        file = bin_file("ac1x\\grads_norm_iteration", data.iteration + 1, num_of_p_matrix_elements, 1, "double_00.bin");
        CHECK(compare_to_bin_file< double >(std::vector< double >(std::begin(data.grads_norm.vals), std::end(data.grads_norm.vals)),
            scene_dir, file, num_of_p_matrix_elements, 1, compare_same_vectors));
        
        
        file = bin_file("ac1x\\normalized_grads_iteration", data.iteration + 1, num_of_p_matrix_elements, 1, "double_00.bin");
        CHECK(compare_to_bin_file< double >(std::vector< double >(std::begin(data.normalized_grads.vals), std::end(data.normalized_grads.vals)),
            scene_dir, file, num_of_p_matrix_elements, 1, compare_same_vectors));
        
        
        file = bin_file("ac1x\\unit_grad_iteration", data.iteration + 1, num_of_p_matrix_elements, 1, "double_00.bin");
        CHECK(compare_to_bin_file< double >(std::vector< double >(std::begin(data.unit_grad.vals), std::end(data.unit_grad.vals)),
            scene_dir, file, num_of_p_matrix_elements, 1, compare_same_vectors));
        
        file = bin_file("ac1x\\t_iteration", data.iteration + 1, 1, 1, "double_00.bin");
        CHECK(compare_to_bin_file< double >(std::vector< double >(1, data.t),
            scene_dir, file, 1, 1, compare_same_vectors));
        
        file = bin_file("ac1x\\back_tracking_line_iter_count_iteration", data.iteration + 1, 1, 1, "double_00.bin");
        CHECK(compare_to_bin_file< double >(std::vector< double >(1, data.back_tracking_line_search_iters),
            scene_dir, file, 1, 1, compare_same_vectors));
        
        file = bin_file("ac1x\\next_p_matrix_iteration", data.iteration + 1, num_of_p_matrix_elements, 1, "double_00.bin");
        CHECK(compare_to_bin_file< double >(std::vector< double >(std::begin(data.next_params.curr_p_mat.vals), std::end(data.next_params.curr_p_mat.vals)),
            scene_dir, file, num_of_p_matrix_elements, 1, compare_same_vectors));
        
        file = bin_file("ac1x\\next_cost_iteration", data.iteration + 1, 1, 1, "double_00.bin");
        CHECK(compare_to_bin_file< double >(std::vector< double >(1, data.next_params.cost),
            scene_dir, file, 1, 1, compare_same_vectors));
       };

    // Our code doesn't count the first iteration; the Matlab code starts at 1 even if it doesn't do anything...
    REQUIRE( cal.optimize( cb ) + 1 == md.n_iterations );

    auto dsm_orig = algo::apply_ac_res_on_dsm_model(dsm.dsm_params, dsm.dsm_regs, algo::ac_to_dsm_dir::inverse);
    CHECK(compare_to_bin_file< algo::DSM_regs >(dsm_orig,
        scene_dir, "ac1x\\dsmRegsOrig_1x4_single_00.bin"));

    auto new_calibration = cal.get_calibration();
    auto cost = cal.get_cost();

    CHECK( compare_calib_to_bin_file( new_calibration, cost, scene_dir, "ac1x\\new_calib", num_of_calib_elements, 1, "double_00" ) );
#endif
#if 0
    //--
    TRACE( "\nChecking output validity:" );
    // Pixel movement is OK, but some sections have negative cost
    CHECK( cal.is_valid_results() == md.is_output_valid );

    CHECK( cal.calc_correction_in_pixels() == approx( md.correction_in_pixels ) );

    CHECK( compare_to_bin_file< double >( z_data.cost_diff_per_section, scene_dir, "costDiffPerSection", 4, 1, "double_00", compare_same_vectors ) );

    //svm
    CHECK(compare_to_bin_file< double >(svm_features, scene_dir, "ac1x\\svm_featuresMat", 10, 1, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(decision_params.distribution_per_section_depth, scene_dir, "ac1x\\svm_edgeWeightDistributionPerSectionDepth", 1, 4, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(decision_params.distribution_per_section_rgb, scene_dir, "ac1x\\svm_edgeWeightDistributionPerSectionRgb", 1, 4, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(decision_params.edge_weights_per_dir, scene_dir, "ac1x\\svm_edgeWeightsPerDir", 4, 1, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(decision_params.improvement_per_section, scene_dir, "ac1x\\svm_improvementPerSection", 4, 1, "double_00", compare_same_vectors));
#endif
}
