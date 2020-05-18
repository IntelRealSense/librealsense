// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.


void compare_scene( std::string const & scene_dir )
{
    TRACE( "Loading " << scene_dir << " ..." );

    camera_params ci = read_camera_params( scene_dir, "camera_params" );
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
    auto& depth_data = cal.get_depth_data();

    //---
    auto rgb_h = ci.rgb.height;
    auto rgb_w = ci.rgb.width;
    auto z_h = ci.z.height;
    auto z_w = ci.z.width;
    auto num_of_calib_elements = 17;
	auto num_of_p_matrix_elements = sizeof(algo::p_matrix) / sizeof(double);

    CHECK( compare_to_bin_file< double >( yuy_data.edges, scene_dir, "YUY2_edge", rgb_w, rgb_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( yuy_data.edges_IDT, scene_dir, "YUY2_IDT", rgb_w, rgb_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( yuy_data.edges_IDTx, scene_dir, "YUY2_IDTx", rgb_w, rgb_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( yuy_data.edges_IDTy, scene_dir, "YUY2_IDTy", rgb_w, rgb_h, "double_00", compare_same_vectors ) );

    //---
    CHECK( compare_to_bin_file< double >( ir_data.ir_edges, scene_dir, "I_edge", z_w, z_h, "double_00", compare_same_vectors ) );

    //---
    CHECK( compare_to_bin_file< double >( z_data.edges, scene_dir, "Z_edge", z_w, z_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( z_data.supressed_edges, scene_dir, "Z_edgeSupressed", z_w, z_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< byte >( z_data.directions, scene_dir, "Z_dir", z_w, z_h, "uint8_00", compare_same_vectors ) );

    CHECK( compare_to_bin_file< double >( z_data.subpixels_x, scene_dir, "Z_edgeSubPixel", z_w, z_h, "double_01", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( z_data.subpixels_y, scene_dir, "Z_edgeSubPixel", z_w, z_h, "double_00", compare_same_vectors ) );

    CHECK( compare_to_bin_file< double >( z_data.weights, scene_dir, "weightsT", 1, md.n_edges, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( z_data.closest, scene_dir, "Z_valuesForSubEdges", z_w, z_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< algo::double3 >( z_data.vertices, scene_dir, bin_file( "vertices", 3, md.n_edges, "double_00" ) + ".bin", md.n_edges, 1, compare_same_vectors ) );

#if 0
    // smearing
    CHECK( compare_to_bin_file< double >( depth_data.gradient_x, scene_dir, "Zx", z_w, z_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.gradient_y, scene_dir, "Zy", z_w, z_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.gradient_x, scene_dir, "Ix", z_w, z_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.gradient_y, scene_dir, "Iy", z_w, z_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.edges2, scene_dir, "iEdge", z_w, z_h, "double_00", compare_same_vectors ) );
    //CHECK(compare_to_bin_file< double >(depth_data.edges2, scene_dir, "zedge", z_w, z_h, "double_00", compare_same_vectors));
    CHECK( compare_to_bin_file< uint8_t >( depth_data.section_map_depth, scene_dir, "sectionMapDepth", z_w, z_h, "uint8_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< uint8_t >( ir_data.valid_edge_pixels_by_ir, scene_dir, "validEdgePixelsByIR", z_w, z_h, "uint8_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.valid_location_rc_x, scene_dir, "gridXValid", 1, 105794, "double_00" ), 105794, 1, compare_same_vectors ));
    CHECK( compare_to_bin_file< double >( ir_data.valid_location_rc_y, scene_dir, "gridYValid", 1, 105794, "double_00" ), 105794, 1, compare_same_vectors ));
    CHECK( compare_to_bin_file< double >( ir_data.valid_location_rc, scene_dir, "locRC", 2, 105794, "double_00" ), 105794, 2, compare_same_vectors ));
    CHECK( compare_to_bin_file< uint8_t >( ir_data.valid_section_map, scene_dir, "sectionMapValid", 1, 105794, "uint8_00" ), 105794, 1, compare_same_vectors ));
    CHECK( compare_to_bin_file< double >( ir_data.valid_gradient_x, scene_dir, "IxValid", 1, 105794, "double_00" ), 105794, 1, compare_same_vectors ));
    CHECK( compare_to_bin_file< double >( ir_data.valid_gradient_y, scene_dir, "IyValid", 1, 105794, "double_00" ), 105794, 1, compare_same_vectors ));
    CHECK( compare_to_bin_file< double >( ir_data.direction_deg, scene_dir, "directionInDeg", 1, 105794, "double_00" ), 105794, 1, compare_same_vectors ));
    //CHECK(compare_to_bin_file< double >(ir_data.directions, scene_dir, "directionIndex", 1, 105794, "double_00"), 105794, 1, compare_same_vectors)); // it passed, depends on index definition
    CHECK( compare_to_bin_file< double >( ir_data.direction_per_pixel, scene_dir, "dirPerPixel", 2, 105794, "double_00" ), 105794, 2, compare_same_vectors ));
    CHECK( compare_to_bin_file< double >( ir_data.local_region[0], scene_dir, "localRegion", 2, 105794, "double_00" ), 105794, 2, compare_same_vectors ));
    CHECK( compare_to_bin_file< double >( ir_data.local_region[1], scene_dir, "localRegion", 2, 105794, "double_01" ), 105794, 2, compare_same_vectors ));
    CHECK( compare_to_bin_file< double >( ir_data.local_region[2], scene_dir, "localRegion", 2, 105794, "double_02" ), 105794, 2, compare_same_vectors ));
    CHECK( compare_to_bin_file< double >( ir_data.local_region[3], scene_dir, "localRegion", 2, 105794, "double_03" ), 105794, 2, compare_same_vectors ));

    CHECK( compare_to_bin_file< double >( ir_data.local_region_x[0], scene_dir, "localRegion_x", 1, 105794, "double_00" ), 105794, 1, compare_same_vectors ));
    CHECK( compare_to_bin_file< double >( ir_data.local_region_x[1], scene_dir, "localRegion_x", 1, 105794, "double_01" ), 105794, 1, compare_same_vectors ));
    CHECK( compare_to_bin_file< double >( ir_data.local_region_x[2], scene_dir, "localRegion_x", 1, 105794, "double_02" ), 105794, 1, compare_same_vectors ));
    CHECK( compare_to_bin_file< double >( ir_data.local_region_x[3], scene_dir, "localRegion_x", 1, 105794, "double_03" ), 105794, 1, compare_same_vectors ));

    CHECK( compare_to_bin_file< double >( ir_data.local_region_y[0], scene_dir, "localRegion_y", 1, 105794, "double_00" ), 105794, 1, compare_same_vectors ));
    CHECK( compare_to_bin_file< double >( ir_data.local_region_y[1], scene_dir, "localRegion_y", 1, 105794, "double_01" ), 105794, 1, compare_same_vectors ));
    CHECK( compare_to_bin_file< double >( ir_data.local_region_y[2], scene_dir, "localRegion_y", 1, 105794, "double_02" ), 105794, 1, compare_same_vectors ));
    CHECK( compare_to_bin_file< double >( ir_data.local_region_y[3], scene_dir, "localRegion_y", 1, 105794, "double_03" ), 105794, 1, compare_same_vectors ));

    CHECK( compare_to_bin_file< double >( ir_data.local_edges, scene_dir, "localEdges", 4, 105794, "double_00" ), 105794, 4, compare_same_vectors ));
    CHECK( compare_to_bin_file< uint8_t >( ir_data.is_supressed, scene_dir, "isSupressed", 1, 105794, "uint8_00" ), 105794, 1, compare_same_vectors ));
#endif

    // ---
    TRACE( "\nChecking scene validity:" );

    CHECK( cal.is_scene_valid() == md.is_scene_valid );

    // edge distribution
    CHECK( compare_to_bin_file< double >( z_data.sum_weights_per_section, scene_dir, "depthEdgeWeightDistributionPerSectionDepth", 1, 4, "double_00", compare_same_vectors ) );

    CHECK( compare_to_bin_file< byte >( z_data.section_map, scene_dir, "sectionMapDepth_trans", 1, md.n_edges, "uint8_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< byte >( yuy_data.section_map, scene_dir, "sectionMapRgb_trans", 1, rgb_w*rgb_h, "uint8_00", compare_same_vectors ) );

    CHECK( compare_to_bin_file< double >( yuy_data.sum_weights_per_section, scene_dir, "edgeWeightDistributionPerSectionRgb", 1, 4, "double_00", compare_same_vectors ) );

    // gradient balanced
    // TODO NOHA
    //CHECK(compare_to_bin_file< double >(z_data.sum_weights_per_direction, scene_dir, "edgeWeightsPerDir", 1, 4, "double_00", compare_same_vectors));

    // movment check
    // 1. dilation
    CHECK( compare_to_bin_file< uint8_t >( yuy_data.prev_logic_edges, scene_dir, "logicEdges", rgb_w, rgb_h, "uint8_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( yuy_data.dilated_image, scene_dir, "dilatedIm", rgb_w, rgb_h, "double_00", compare_same_vectors ) );

    // 2. gausssian
    CHECK( compare_to_bin_file< double >( yuy_data.yuy_diff, scene_dir, "diffIm_01", rgb_w, rgb_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( yuy_data.gaussian_filtered_image, scene_dir, "diffIm", rgb_w, rgb_h, "double_00", compare_same_vectors ) );

    // 3. movement
    CHECK( compare_to_bin_file< double >( yuy_data.gaussian_diff_masked, scene_dir, "IDiffMasked", rgb_w, rgb_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< uint8_t >( yuy_data.move_suspect, scene_dir, "ixMoveSuspect", rgb_w, rgb_h, "uint8_00", compare_same_vectors ) );


    //--
    TRACE( "\nOptimizing:" );
    auto cb = [&]( algo::iteration_data_collect const & data )
    {
        // data.iteration is 0-based!
       /* REQUIRE( data.iteration < md.n_iterations );*/

		auto file = bin_file("p_matrix_iteration", data.iteration + 1, num_of_p_matrix_elements, 1, "double_00.bin");
		CHECK(compare_to_bin_file< double >(std::vector< double >(std::begin(data.params.curr_p_mat.vals), std::end(data.params.curr_p_mat.vals)),
			scene_dir, file, num_of_p_matrix_elements, 1, compare_same_vectors));

		file = bin_file("cost_iteration", data.iteration + 1, 1, 1, "double_00.bin");
		CHECK(compare_to_bin_file< double >(std::vector< double >(1, data.params.cost),
			scene_dir, file, 1, 1, compare_same_vectors));

        file = bin_file( "uvmap_iteration", data.iteration + 1, 2, md.n_edges, "double_00.bin" );
        CHECK( compare_to_bin_file< algo::double2 >( data.uvmap, scene_dir, file, md.n_edges, 1, compare_same_vectors ) );

        file = bin_file( "DVals_iteration", data.iteration + 1, 1, md.n_edges, "double_00.bin" );
        CHECK( compare_to_bin_file< double >( data.d_vals, scene_dir, file, md.n_edges, 1, compare_same_vectors ) );

        file = bin_file( "DxVals_iteration", data.iteration + 1, 1, md.n_edges, "double_00.bin" );
        CHECK( compare_to_bin_file< double >( data.d_vals_x, scene_dir, file, md.n_edges, 1, compare_same_vectors ) );

        file = bin_file( "DyVals_iteration", data.iteration + 1, 1, md.n_edges, "double_00.bin" );
        CHECK( compare_to_bin_file< double >( data.d_vals_y, scene_dir, file, md.n_edges, 1, compare_same_vectors ) );

        file = bin_file( "xy_iteration", data.iteration + 1, 2, md.n_edges, "double_00.bin" );
        CHECK( compare_to_bin_file< algo::double2 >( data.xy, scene_dir, file, md.n_edges, 1, compare_same_vectors ) );

        file = bin_file( "rc_iteration", data.iteration + 1, 1, md.n_edges, "double_00.bin" );
        CHECK( compare_to_bin_file< double >( data.rc, scene_dir, file, md.n_edges, 1, compare_same_vectors ) );

        file = bin_file( "xCoeff_P", data.iteration + 1, num_of_p_matrix_elements, md.n_edges, "double_00.bin" );
        CHECK( compare_to_bin_file< algo::p_matrix>( data.coeffs_p.x_coeffs, scene_dir, file, md.n_edges, 1, compare_same_vectors ) );

		file = bin_file("grad_iteration", data.iteration + 1, num_of_p_matrix_elements, 1, "double_00.bin");
		CHECK(compare_to_bin_file< double >(std::vector< double >(std::begin(data.params.calib_gradients.vals), std::end(data.params.calib_gradients.vals)),
			scene_dir, file, num_of_p_matrix_elements, 1, compare_same_vectors));

		file = bin_file("grad_norma_iteration", data.iteration + 1, 1, 1, "double_00.bin");
		CHECK(compare_to_bin_file< double >(std::vector< double >(1, data.grads_norma),
			scene_dir, file, 1, 1, compare_same_vectors));

		file = bin_file("grads_norm_iteration", data.iteration + 1, num_of_p_matrix_elements, 1, "double_00.bin");
		CHECK(compare_to_bin_file< double >(std::vector< double >(std::begin(data.grads_norm.vals), std::end(data.grads_norm.vals)),
			scene_dir, file, num_of_p_matrix_elements, 1, compare_same_vectors));


		file = bin_file("normalized_grads_iteration", data.iteration + 1, num_of_p_matrix_elements, 1, "double_00.bin");
		CHECK(compare_to_bin_file< double >(std::vector< double >(std::begin(data.normalized_grads.vals), std::end(data.normalized_grads.vals)),
			scene_dir, file, num_of_p_matrix_elements, 1, compare_same_vectors));


		file = bin_file("unit_grad_iteration", data.iteration + 1, num_of_p_matrix_elements, 1, "double_00.bin");
		CHECK(compare_to_bin_file< double >(std::vector< double >(std::begin(data.unit_grad.vals), std::end(data.unit_grad.vals)),
			scene_dir, file, num_of_p_matrix_elements, 1, compare_same_vectors));

		file = bin_file("t_iteration", data.iteration + 1, 1, 1, "double_00.bin");
		CHECK(compare_to_bin_file< double >(std::vector< double >(1, data.t),
			scene_dir, file, 1, 1, compare_same_vectors));

		file = bin_file("back_tracking_line_iter_count_iteration", data.iteration + 1, 1, 1, "double_00.bin");
		CHECK(compare_to_bin_file< double >(std::vector< double >(1, data.back_tracking_line_search_iters),
			scene_dir, file, 1, 1, compare_same_vectors));

		file = bin_file("next_p_matrix_iteration", data.iteration + 1, num_of_p_matrix_elements, 1, "double_00.bin");
		CHECK(compare_to_bin_file< double >(std::vector< double >(std::begin(data.next_params.curr_p_mat.vals), std::end(data.next_params.curr_p_mat.vals)),
			scene_dir, file, num_of_p_matrix_elements, 1, compare_same_vectors));

		file = bin_file("next_cost_iteration", data.iteration + 1, 1, 1, "double_00.bin");
		CHECK(compare_to_bin_file< double >(std::vector< double >(1, data.next_params.cost),
			scene_dir, file, 1, 1, compare_same_vectors));
       };

    // Our code doesn't count the first iteration; the Matlab code starts at 1 even if it doesn't do anything...
    REQUIRE( cal.optimize( cb ) + 1 == md.n_iterations );

    auto new_calibration = cal.get_calibration();
    auto cost = cal.get_cost();

    CHECK( compare_calib_to_bin_file( new_calibration, cost, scene_dir, "new_calib", num_of_calib_elements, 1, "double_00" ) );


    //--
    TRACE( "\nChecking output validity:" );
    // Pixel movement is OK, but some sections have negative cost
    CHECK( cal.is_valid_results() == md.is_output_valid );

    CHECK( cal.calc_correction_in_pixels() == approx( md.correction_in_pixels ) );

    CHECK( compare_to_bin_file< double >( z_data.cost_diff_per_section, scene_dir, "costDiffPerSection", 4, 1, "double_00", compare_same_vectors ) );
}
