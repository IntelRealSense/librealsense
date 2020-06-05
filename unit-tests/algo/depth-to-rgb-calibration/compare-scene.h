// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.


void compare_scene( std::string const & scene_dir )
{
    TRACE( "Loading " << scene_dir << " ..." );

    camera_params ci = read_camera_params( scene_dir, "camera_params" );
    dsm_params dsm = read_dsm_params(scene_dir, "DSM_params");
    ci.dsm_params = dsm.dsm_params;
    ci.cal_info = dsm.regs;
    ci.cal_regs = dsm.algo_calibration_registers;

    scene_metadata md( scene_dir );

    algo::optimizer cal;

    /*std::vector<double> in = { 1,2,3,4 };
    std::vector<double>out(4);

    algo::direct_inv(in,out,2);*/

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
    
    auto rgb_h = ci.rgb.height;
    auto rgb_w = ci.rgb.width;
    auto z_h = ci.z.height;
    auto z_w = ci.z.width;
    auto num_of_calib_elements = 17;
    auto num_of_p_matrix_elements = sizeof(algo::p_matrix) / sizeof(double);


    //---
    CHECK( compare_to_bin_file< double >( yuy_data.edges, scene_dir, "YUY2_edge", rgb_w, rgb_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( yuy_data.edges_IDT, scene_dir, "YUY2_IDT", rgb_w, rgb_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( yuy_data.edges_IDTx, scene_dir, "YUY2_IDTx", rgb_w, rgb_h, "double_00", compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( yuy_data.edges_IDTy, scene_dir, "YUY2_IDTy", rgb_w, rgb_h, "double_00", compare_same_vectors ) );

    // smearing
    CHECK(compare_to_bin_file< double >(depth_data.gradient_x, scene_dir, "Zx", z_w, z_h, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.gradient_y, scene_dir, "Zy", z_w, z_h, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.gradient_x, scene_dir, "Ix", z_w, z_h, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.gradient_y, scene_dir, "Iy", z_w, z_h, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.edges, scene_dir, "iEdge", z_w, z_h, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.edges, scene_dir, "zedge", z_w, z_h, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< uint8_t >(depth_data.section_map_depth, scene_dir, "sectionMapDepth", z_w, z_h, "uint8_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.valid_edge_pixels_by_ir, scene_dir, "validEdgePixelsByIR", z_w, z_h, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.valid_location_rc_x, scene_dir, "gridXValid", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.valid_location_rc_y, scene_dir, "gridYValid", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.valid_location_rc, scene_dir, "locRC", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< uint8_t >(ir_data.valid_section_map, scene_dir, "sectionMapValid", 1, md.n_valid_ir_edges, "uint8_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.valid_gradient_x, scene_dir, "IxValid", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.valid_gradient_y, scene_dir, "IyValid", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.direction_deg, scene_dir, "directionInDeg", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.direction_per_pixel, scene_dir, "dirPerPixel", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region[0], scene_dir, "localRegion", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region[1], scene_dir, "localRegion", 2, md.n_valid_ir_edges, "double_01", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region[2], scene_dir, "localRegion", 2, md.n_valid_ir_edges, "double_02", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region[3], scene_dir, "localRegion", 2, md.n_valid_ir_edges, "double_03", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_x[0], scene_dir, "localRegion_x", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_x[1], scene_dir, "localRegion_x", 1, md.n_valid_ir_edges, "double_01", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_x[2], scene_dir, "localRegion_x", 1, md.n_valid_ir_edges, "double_02", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_x[3], scene_dir, "localRegion_x", 1, md.n_valid_ir_edges, "double_03", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_y[0], scene_dir, "localRegion_y", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_y[1], scene_dir, "localRegion_y", 1, md.n_valid_ir_edges, "double_01", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_y[2], scene_dir, "localRegion_y", 1, md.n_valid_ir_edges, "double_02", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_region_y[3], scene_dir, "localRegion_y", 1, md.n_valid_ir_edges, "double_03", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.local_edges, scene_dir, "localEdges", 4, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< uint8_t >(ir_data.is_supressed, scene_dir, "isSupressed", 1, md.n_valid_ir_edges, "uint8_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(ir_data.fraq_step, scene_dir, "fraqStep", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.local_rc_subpixel, scene_dir, "locRCsub", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.local_x, scene_dir, "localZx", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.local_y, scene_dir, "localZy", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.gradient, scene_dir, "zGrad", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.grad_in_direction, scene_dir, "zGradInDirection", 1, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.local_values, scene_dir, "localZvalues", 4, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.closest, scene_dir, "zValuesForSubEdges", 1, md.n_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.edge_sub_pixel, scene_dir, "edgeSubPixel", 2, md.n_valid_ir_edges, "double_00", compare_same_vectors));
    
    CHECK(compare_to_bin_file< byte >(depth_data.supressed_edges, scene_dir, "validEdgePixels", 1, md.n_valid_ir_edges, "uint8_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.grad_in_direction_valid, scene_dir, "validzGradInDirection", 1, md.n_valid_pixels, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >( depth_data.valid_edge_sub_pixel, scene_dir, "validedgeSubPixel", 2, md.n_valid_pixels, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >( depth_data.values_for_subedges, scene_dir, "validzValuesForSubEdges", 1, md.n_valid_pixels, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.valid_direction_per_pixel, scene_dir, "validdirPerPixel", 1, md.n_valid_pixels, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< uint8_t >(depth_data.valid_section_map, scene_dir, "validsectionMapDepth", 1, md.n_valid_pixels, "uint8_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.valid_directions, scene_dir, "validdirectionIndex", 1, md.n_valid_pixels, "double_00", compare_same_vectors));
    
    //CHECK(compare_to_bin_file< double >(depth_data.valid_edge_sub_pixel_x, scene_dir, "xim", 1, md.n_valid_pixels, "double_00", compare_same_vectors));
    //CHECK(compare_to_bin_file< double >(depth_data.valid_edge_sub_pixel_y, scene_dir, "yim", 1, md.n_valid_pixels, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double>(depth_data.sub_points, scene_dir, "subPoints", 3, md.n_valid_pixels, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< algo::double2 >(depth_data.uvmap, scene_dir, bin_file("uv", 2, md.n_valid_pixels, "double_00") + ".bin", md.n_valid_pixels, 1, compare_same_vectors));
    CHECK(compare_to_bin_file< byte >(depth_data.is_inside, scene_dir, "isInside", 1, md.n_valid_pixels, "uint8_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.subpixels_x, scene_dir, "Z_xim", 1, md.n_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.subpixels_y, scene_dir, "Z_yim", 1, md.n_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.directions, scene_dir, "directionIndexInside", 1, md.n_edges, "double_00", compare_same_vectors));
    
    CHECK(compare_to_bin_file< double >(depth_data.subpixels_x_round, scene_dir, "round_xim", 1, md.n_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.subpixels_y_round, scene_dir, "round_yim", 1, md.n_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(depth_data.weights, scene_dir, "weights", 1, md.n_edges, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< uint8_t >(depth_data.relevant_pixels_image, scene_dir, "relevantPixelsImage", z_w, z_h, "uint8_00", compare_same_vectors));
    CHECK(compare_to_bin_file< algo::double3 >(depth_data.vertices, scene_dir, bin_file("vertices", 3, md.n_edges, "double_00") + ".bin", md.n_edges, 1, compare_same_vectors));
    CHECK(compare_to_bin_file< uint8_t >(depth_data.section_map_depth_inside, scene_dir, "sectionMapDepthInside", 1, md.n_edges, "uint8_00", compare_same_vectors));


    // ---
    TRACE( "\nChecking scene validity:" );

    CHECK( cal.is_scene_valid() == md.is_scene_valid );

    // edge distribution
    CHECK( compare_to_bin_file< double >( z_data.sum_weights_per_section, scene_dir, "depthEdgeWeightDistributionPerSectionDepth", 1, 4, "double_00", compare_same_vectors ) );

    //CHECK( compare_to_bin_file< byte >( z_data.section_map, scene_dir, "sectionMapDepth_trans", 1, md.n_edges, "uint8_00", compare_same_vectors ) );
    //CHECK( compare_to_bin_file< byte >( yuy_data.section_map, scene_dir, "sectionMapRgb_trans", 1, rgb_w*rgb_h, "uint8_00", compare_same_vectors ) );

    CHECK( compare_to_bin_file< double >( yuy_data.sum_weights_per_section, scene_dir, "edgeWeightDistributionPerSectionRgb", 1, 4, "double_00", compare_same_vectors ) );

    // gradient balanced
    // TODO NOHA
    CHECK(compare_to_bin_file< double >(z_data.sum_weights_per_direction, scene_dir, "edgeWeightsPerDir", 4, 1, "double_00", compare_same_vectors));

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
        //REQUIRE( data.iteration < md.n_iterations );

        //REQUIRE( data.cycle <= md.n_cycles );

        if (data.type == algo::cycle_data)
        {
            CHECK(compare_to_bin_file< algo::algo_calibration_registers >(
                data.cycle_data_p.dsm_regs_orig,
                scene_dir, 
                bin_file("dsmRegsOrig", data.cycle, 4, 1, "double_00.bin")));

            CHECK(compare_to_bin_file< uint8_t >(
                data.cycle_data_p.relevant_pixels_image_rot,
                scene_dir,
                bin_file("relevantPixelnImage_rot", data.cycle, z_w, z_h, "uint8_00") + ".bin",
                z_w, z_h, 
                compare_same_vectors));

            CHECK(compare_to_bin_file< algo::los_shift_scaling >(
                data.cycle_data_p.dsm_pre_process_data.last_los_error,
                scene_dir, 
                bin_file("dsm_los_error_orig", data.cycle, 1, 4, "double_00.bin")));

            CHECK(compare_to_bin_file< algo::double3 >(
                data.cycle_data_p.dsm_pre_process_data.vertices_orig,
                scene_dir, 
                bin_file("verticesOrig", data.cycle, 3, md.n_relevant_pixels, "double_00") + ".bin",
                md.n_relevant_pixels, 1, compare_same_vectors));

            CHECK(compare_to_bin_file< algo::double2 >(
                data.cycle_data_p.dsm_pre_process_data.los_orig, 
                scene_dir,
                bin_file("losOrig", data.cycle, 2, md.n_relevant_pixels, "double_00") + ".bin",
                md.n_relevant_pixels, 1, 
                compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                data.cycle_data_p.errL2,
                scene_dir,
                bin_file("errL2", data.cycle, 1, data.cycle_data_p.errL2.size(), "double_00") + ".bin",
                data.cycle_data_p.errL2.size(), 1, compare_same_vectors));

            CHECK(compare_to_bin_file< algo::double2 >(
                data.cycle_data_p.focal_scaling,
                scene_dir,
                bin_file("focalScaling", data.cycle, 2, 1, "double_00.bin")));

            CHECK(compare_to_bin_file< std::vector<double>>(
                data.cycle_data_p.sg_mat,
                scene_dir,
                bin_file("sgMat", data.cycle, data.cycle_data_p.sg_mat[0].size(), data.cycle_data_p.sg_mat.size(), "double_00") + ".bin",
                data.cycle_data_p.sg_mat.size(), data.cycle_data_p.sg_mat[0].size(),
                data.cycle_data_p.sg_mat.size(),
                compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                    data.cycle_data_p.sg_mat_tag_x_sg_mat,
                    scene_dir,
                    bin_file("sg_mat_tag_x_sg_mat", data.cycle, 1, data.cycle_data_p.sg_mat_tag_x_sg_mat.size(), "double_00") + ".bin",
                    data.cycle_data_p.sg_mat_tag_x_sg_mat.size(), 1, compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                data.cycle_data_p.sg_mat_tag_x_err_l2,
                scene_dir,
                bin_file("sg_mat_tag_x_err_l2", data.cycle, 1, data.cycle_data_p.sg_mat_tag_x_err_l2.size(), "double_00") + ".bin",
                data.cycle_data_p.sg_mat_tag_x_err_l2.size(), 1, compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                data.cycle_data_p.quad_coef,
                scene_dir,
                bin_file("quadCoef", data.cycle, 1, data.cycle_data_p.quad_coef.size(), "double_00") + ".bin",
                data.cycle_data_p.quad_coef.size(), 1, compare_same_vectors));

            CHECK(compare_to_bin_file< algo::double2 >(
                data.cycle_data_p.opt_scaling,
                scene_dir,
                bin_file("optScaling", data.cycle, 1, 2, "double_00.bin")));

            CHECK(compare_to_bin_file< algo::double2 >(
                data.cycle_data_p.new_los_scaling,
                scene_dir,
                bin_file("newlosScaling", data.cycle, 1, 2, "double_00.bin")));

            CHECK(compare_to_bin_file< algo::double2 >(
                std::vector< algo::double2 >(1, { data.cycle_data_p.dsm_params_cand.h_scale, data.cycle_data_p.dsm_params_cand.v_scale }),
                scene_dir,
                bin_file("acDataCand", data.cycle, 2, 1, "double_00.bin"),
                1, 1,
                compare_same_vectors));

           /* CHECK(compare_to_bin_file< algo::algo_calibration_registers >(
                data.cycle_data_p.dsm_regs_cand,
                scene_dir,
                bin_file("dsmRegsCand", data.cycle, 4, 1, "double_00.bin")));*/

            CHECK(compare_to_bin_file< algo::double2 >(
                data.cycle_data_p.los_orig, 
                scene_dir,
                bin_file("orig_los", data.cycle, 2, md.n_edges, "double_00") + ".bin",
                md.n_edges, 1, 
                compare_same_vectors));

            CHECK(compare_to_bin_file< algo::double2 >(
                data.cycle_data_p.dsm, 
                scene_dir,
                bin_file("dsm", data.cycle, 2, md.n_edges, "double_00") + ".bin",
                md.n_edges, 1, 
                compare_same_vectors));

            CHECK(compare_to_bin_file< algo::double3 >(
                data.cycle_data_p.vertices,
                scene_dir,
                bin_file("new_vertices_cycle", data.cycle, 3, md.n_edges, "double_00.bin"),
                md.n_edges, 1,
                compare_same_vectors));

            TRACE("\nSet next cycle data from Matlab:");

            try
            {
                auto vertices = read_vector_from<algo::double3>(bin_file(bin_dir(scene_dir) + "end_cycle_vertices", data.cycle, 3, md.n_edges, "double_00.bin"));
                algo::p_matrix p_mat;
               
                auto p_vec = read_vector_from<double>(bin_file(bin_dir(scene_dir) + "end_cycle_p_matrix",
                    data.cycle,
                    num_of_p_matrix_elements, 1,
                    "double_00.bin"));
                std::copy(p_vec.begin(), p_vec.end(), p_mat.vals);
                cal.set_cycle_data(vertices, p_mat);
            }
            catch (std::runtime_error &e) {
                // if device isn't calibrated, get_extrinsics must error out (according to old comment. Might not be true under new API)
                WARN(e.what());
            }

           
        }

        else
        {
            CHECK(compare_to_bin_file< double >(
                std::vector< double >(std::begin(data.params.curr_p_mat.vals),
                    std::end(data.params.curr_p_mat.vals)),
                scene_dir,
                bin_file("p_matrix_iteration",
                    data.cycle,
                    data.iteration + 1,
                    num_of_p_matrix_elements,
                    1,
                    "double_00.bin"),
                num_of_p_matrix_elements, 1,
                compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                std::vector< double >(1, data.params.cost),
                scene_dir,
                bin_file("cost_iteration", data.cycle, data.iteration + 1, 1, 1, "double_00.bin"),
                1, 1,
                compare_same_vectors));

            CHECK(compare_to_bin_file< algo::double2 >(
                data.uvmap,
                scene_dir,
                bin_file("uvmap_iteration", data.cycle, data.iteration + 1, 2, md.n_edges, "double_00.bin"),
                md.n_edges, 1,
                compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                data.d_vals,
                scene_dir,
                bin_file("DVals_iteration", data.cycle, data.iteration + 1, 1, md.n_edges, "double_00.bin"),
                md.n_edges, 1,
                compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                data.d_vals_x,
                scene_dir,
                bin_file("DxVals_iteration", data.cycle, data.iteration + 1, 1, md.n_edges, "double_00.bin"),
                md.n_edges, 1,
                compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                data.d_vals_y,
                scene_dir,
                bin_file("DyVals_iteration", data.cycle, data.iteration + 1, 1, md.n_edges, "double_00.bin"),
                md.n_edges, 1,
                compare_same_vectors));

            CHECK(compare_to_bin_file< algo::double2 >(
                data.xy,
                scene_dir,
                bin_file("xy_iteration", data.cycle, data.iteration + 1, 2, md.n_edges, "double_00.bin"),
                md.n_edges, 1,
                compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                data.rc,
                scene_dir,
                bin_file("rc_iteration", data.cycle, data.iteration + 1, 1, md.n_edges, "double_00.bin"),
                md.n_edges, 1,
                compare_same_vectors));

            CHECK(compare_to_bin_file< algo::p_matrix >(data.coeffs_p.x_coeffs,
                scene_dir,
                bin_file("xCoeff_P_iteration",
                    data.cycle,
                    data.iteration + 1,
                    num_of_p_matrix_elements, md.n_edges,
                    "double_00.bin"),
                md.n_edges, 1,
                compare_same_vectors));

            CHECK(compare_to_bin_file< algo::p_matrix >(data.coeffs_p.y_coeffs,
                scene_dir,
                bin_file("yCoeff_P_iteration",
                    data.cycle,
                    data.iteration + 1,
                    num_of_p_matrix_elements, md.n_edges,
                    "double_00.bin"),
                md.n_edges, 1,
                compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                std::vector< double >(std::begin(data.params.calib_gradients.vals),
                    std::end(data.params.calib_gradients.vals)),
                scene_dir,
                bin_file("grad_iteration",
                    data.cycle,
                    data.iteration + 1,
                    num_of_p_matrix_elements, 1,
                    "double_00.bin"),
                num_of_p_matrix_elements, 1,
                compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                std::vector< double >(1, data.grads_norma),
                scene_dir,
                bin_file("grad_norma_iteration", data.cycle, data.iteration + 1, 1, 1, "double_00.bin"),
                1, 1,
                compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                std::vector< double >(std::begin(data.grads_norm.vals),
                    std::end(data.grads_norm.vals)),
                scene_dir,
                bin_file("grads_norm_iteration",
                    data.cycle,
                    data.iteration + 1,
                    num_of_p_matrix_elements, 1,
                    "double_00.bin"),
                num_of_p_matrix_elements, 1,
                compare_same_vectors));


            CHECK(compare_to_bin_file< double >(
                std::vector< double >(std::begin(data.normalized_grads.vals),
                    std::end(data.normalized_grads.vals)),
                scene_dir,
                bin_file("normalized_grads_iteration",
                    data.cycle,
                    data.iteration + 1,
                    num_of_p_matrix_elements, 1,
                    "double_00.bin"),
                num_of_p_matrix_elements, 1,
                compare_same_vectors));


            CHECK(
                compare_to_bin_file< double >(std::vector< double >(std::begin(data.unit_grad.vals),
                    std::end(data.unit_grad.vals)),
                    scene_dir,
                    bin_file("unit_grad_iteration",
                        data.cycle,
                        data.iteration + 1,
                        num_of_p_matrix_elements, 1,
                        "double_00.bin"),
                    num_of_p_matrix_elements, 1,
                    compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                std::vector< double >(1, data.t),
                scene_dir,
                bin_file("t_iteration", data.cycle, data.iteration + 1, 1, 1, "double_00.bin"),
                1, 1,
                compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                std::vector< double >(1, data.back_tracking_line_search_iters),
                scene_dir,
                bin_file("back_tracking_line_iter_count_iteration",
                    data.cycle,
                    data.iteration + 1,
                    1, 1,
                    "double_00.bin"),
                1, 1,
                compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                std::vector< double >(std::begin(data.next_params.curr_p_mat.vals),
                    std::end(data.next_params.curr_p_mat.vals)),
                scene_dir,
                bin_file("next_p_matrix_iteration",
                    data.cycle,
                    data.iteration + 1,
                    num_of_p_matrix_elements, 1,
                    "double_00.bin"),
                num_of_p_matrix_elements, 1,
                compare_same_vectors));

            CHECK(compare_to_bin_file< double >(
                std::vector< double >(1, data.next_params.cost),
                scene_dir,
                bin_file("next_cost_iteration", data.cycle, data.iteration + 1, 1, 1, "double_00.bin"),
                1, 1,
                compare_same_vectors));
        }
       
    };

    // Our code doesn't count the first iteration; the Matlab code starts at 1 even if it doesn't do anything...
    //REQUIRE( cal.optimize( cb ) + 1 == md.n_iterations );
    cal.optimize( cb );

    
    auto new_calibration = cal.get_calibration();
    auto cost = cal.get_cost();

    CHECK( compare_calib_to_bin_file( new_calibration, cost, scene_dir, "new_calib", num_of_calib_elements, 1, "double_00" ) );


#if 1
    //--
    TRACE( "\nChecking output validity:" );
    // Pixel movement is OK, but some sections have negative cost
    CHECK( cal.is_valid_results() == md.is_output_valid );

    CHECK( cal.calc_correction_in_pixels() == approx( md.correction_in_pixels ) );

    CHECK( compare_to_bin_file< double >( z_data.cost_diff_per_section, scene_dir, "costDiffPerSection", 4, 1, "double_00", compare_same_vectors ) );

    //svm
    CHECK(compare_to_bin_file< double >(svm_features, scene_dir, "svm_featuresMat", 10, 1, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(decision_params.distribution_per_section_depth, scene_dir, "svm_edgeWeightDistributionPerSectionDepth", 1, 4, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(decision_params.distribution_per_section_rgb, scene_dir, "svm_edgeWeightDistributionPerSectionRgb", 1, 4, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(decision_params.edge_weights_per_dir, scene_dir, "svm_edgeWeightsPerDir", 4, 1, "double_00", compare_same_vectors));
    CHECK(compare_to_bin_file< double >(decision_params.improvement_per_section, scene_dir, "svm_improvementPerSection", 4, 1, "double_00", compare_same_vectors));
#endif
}
