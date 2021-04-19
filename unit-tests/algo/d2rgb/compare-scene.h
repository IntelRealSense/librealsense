// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

struct scene_stats
{
    size_t n_valid_scene, n_valid_scene_diff;
    size_t n_valid_result, n_valid_result_diff;
    size_t n_converged, n_converged_diff;
    double cost, d_cost;
    double movement, d_movement;
    size_t n_cycles;
    float memory_consumption_peak;
};

void compare_vertices_to_los_data( std::string const & scene_dir,
                                   size_t num_of_vertices,
                                   size_t cycle,
                                   std::string const & time,
                                   algo::convert_norm_vertices_to_los_data const & data )
{

    CHECK( compare_to_bin_file< algo::double3 >(
        data.laser_incident,
        scene_dir,
        bin_file( time + "laserIncidentDirection", cycle, 3, 1, "double_00.bin" ) ) );

    CHECK( compare_to_bin_file< algo::double3 >(
        data.fovex_indicent_direction,
        scene_dir,
        bin_file( time + "fovexIndicentDirection", cycle, 3, num_of_vertices, "double_00" )
            + ".bin",
        num_of_vertices,
        1,
        compare_same_vectors ) );

    CHECK( compare_to_bin_file< algo::double3 >(
        data.mirror_normal_direction,
        scene_dir,
        bin_file( time + "mirrorNormalDirection", cycle, 3, num_of_vertices, "double_00" ) + ".bin",
        num_of_vertices,
        1,
        compare_same_vectors ) );

    CHECK( compare_to_bin_file< double >(
        data.ang_x,
        scene_dir,
        bin_file( time + "angX", cycle, 1, num_of_vertices, "double_00" ) + ".bin",
        num_of_vertices,
        1,
        compare_same_vectors ) );

    CHECK( compare_to_bin_file< double >(
        data.ang_y,
        scene_dir,
        bin_file( time + "angY", cycle, 1, num_of_vertices, "double_00" ) + ".bin",
        num_of_vertices,
        1,
        compare_same_vectors ) );

    CHECK( compare_to_bin_file< double >(
        data.dsm_x_corr,
        scene_dir,
        bin_file( time + "dsmXcorr", cycle, 1, num_of_vertices, "double_00" ) + ".bin",
        num_of_vertices,
        1,
        compare_same_vectors ) );

    CHECK( compare_to_bin_file< double >(
        data.dsm_y_corr,
        scene_dir,
        bin_file( time + "dsmYcorr", cycle, 1, num_of_vertices, "double_00" ) + ".bin",
        num_of_vertices,
        1,
        compare_same_vectors ) );

    CHECK( compare_to_bin_file< double >(
        data.dsm_x,
        scene_dir,
        bin_file( time + "dsmX", cycle, 1, num_of_vertices, "double_00" ) + ".bin",
        num_of_vertices,
        1,
        compare_same_vectors ) );

    CHECK( compare_to_bin_file< double >(
        data.dsm_y,
        scene_dir,
        bin_file( time + "dsmY", cycle, 1, num_of_vertices, "double_00" ) + ".bin",
        num_of_vertices,
        1,
        compare_same_vectors ) );
}

void compare_preprocessing_data( std::string const & scene_dir,
                                 algo::z_frame_data const & depth_data,
                                 algo::ir_frame_data const & ir_data,
                                 algo::yuy2_frame_data const & yuy_data,
                                 scene_metadata const & md )
{
    auto z_w = depth_data.width;
    auto z_h = depth_data.height;

    auto rgb_w = yuy_data.width;
    auto rgb_h = yuy_data.height;

    // smearing
    CHECK( compare_to_bin_file< double >( depth_data.gradient_x,
                                          scene_dir,
                                          "Zx",
                                          z_w,
                                          z_h,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.gradient_y,
                                          scene_dir,
                                          "Zy",
                                          z_w,
                                          z_h,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.gradient_x,
                                          scene_dir,
                                          "Ix",
                                          z_w,
                                          z_h,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.gradient_y,
                                          scene_dir,
                                          "Iy",
                                          z_w,
                                          z_h,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.edges,
                                          scene_dir,
                                          "iEdge",
                                          z_w,
                                          z_h,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.edges,
                                          scene_dir,
                                          "zedge",
                                          z_w,
                                          z_h,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< uint8_t >( depth_data.section_map_depth,
                                           scene_dir,
                                           "sectionMapDepth",
                                           z_w,
                                           z_h,
                                           "uint8_00",
                                           compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.valid_edge_pixels_by_ir,
                                          scene_dir,
                                          "validEdgePixelsByIR",
                                          z_w,
                                          z_h,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.valid_location_rc_x,
                                          scene_dir,
                                          "gridXValid",
                                          1,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.valid_location_rc_y,
                                          scene_dir,
                                          "gridYValid",
                                          1,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.valid_location_rc,
                                          scene_dir,
                                          "locRC",
                                          2,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< uint8_t >( ir_data.valid_section_map,
                                           scene_dir,
                                           "sectionMapValid",
                                           1,
                                           md.n_valid_ir_edges,
                                           "uint8_00",
                                           compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.valid_gradient_x,
                                          scene_dir,
                                          "IxValid",
                                          1,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.valid_gradient_y,
                                          scene_dir,
                                          "IyValid",
                                          1,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.direction_deg,
                                          scene_dir,
                                          "directionInDeg",
                                          1,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.direction_per_pixel,
                                          scene_dir,
                                          "dirPerPixel",
                                          2,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.local_region[0],
                                          scene_dir,
                                          "localRegion",
                                          2,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.local_region[1],
                                          scene_dir,
                                          "localRegion",
                                          2,
                                          md.n_valid_ir_edges,
                                          "double_01",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.local_region[2],
                                          scene_dir,
                                          "localRegion",
                                          2,
                                          md.n_valid_ir_edges,
                                          "double_02",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.local_region[3],
                                          scene_dir,
                                          "localRegion",
                                          2,
                                          md.n_valid_ir_edges,
                                          "double_03",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.local_region_x[0],
                                          scene_dir,
                                          "localRegion_x",
                                          1,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.local_region_x[1],
                                          scene_dir,
                                          "localRegion_x",
                                          1,
                                          md.n_valid_ir_edges,
                                          "double_01",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.local_region_x[2],
                                          scene_dir,
                                          "localRegion_x",
                                          1,
                                          md.n_valid_ir_edges,
                                          "double_02",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.local_region_x[3],
                                          scene_dir,
                                          "localRegion_x",
                                          1,
                                          md.n_valid_ir_edges,
                                          "double_03",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.local_region_y[0],
                                          scene_dir,
                                          "localRegion_y",
                                          1,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.local_region_y[1],
                                          scene_dir,
                                          "localRegion_y",
                                          1,
                                          md.n_valid_ir_edges,
                                          "double_01",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.local_region_y[2],
                                          scene_dir,
                                          "localRegion_y",
                                          1,
                                          md.n_valid_ir_edges,
                                          "double_02",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.local_region_y[3],
                                          scene_dir,
                                          "localRegion_y",
                                          1,
                                          md.n_valid_ir_edges,
                                          "double_03",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.local_edges,
                                          scene_dir,
                                          "localEdges",
                                          4,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< uint8_t >( ir_data.is_supressed,
                                           scene_dir,
                                           "isSupressed",
                                           1,
                                           md.n_valid_ir_edges,
                                           "uint8_00",
                                           compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( ir_data.fraq_step,
                                          scene_dir,
                                          "fraqStep",
                                          1,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.local_rc_subpixel,
                                          scene_dir,
                                          "locRCsub",
                                          2,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.local_x,
                                          scene_dir,
                                          "localZx",
                                          2,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.local_y,
                                          scene_dir,
                                          "localZy",
                                          2,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.gradient,
                                          scene_dir,
                                          "zGrad",
                                          2,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.grad_in_direction,
                                          scene_dir,
                                          "zGradInDirection",
                                          1,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.local_values,
                                          scene_dir,
                                          "localZvalues",
                                          4,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.closest,
                                          scene_dir,
                                          "zValuesForSubEdges",
                                          1,
                                          md.n_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.edge_sub_pixel,
                                          scene_dir,
                                          "edgeSubPixel",
                                          2,
                                          md.n_valid_ir_edges,
                                          "double_00",
                                          compare_same_vectors ) );

    CHECK( compare_to_bin_file< byte >( depth_data.supressed_edges,
                                        scene_dir,
                                        "validEdgePixels",
                                        1,
                                        md.n_valid_ir_edges,
                                        "uint8_00",
                                        compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.grad_in_direction_valid,
                                          scene_dir,
                                          "validzGradInDirection",
                                          1,
                                          md.n_valid_pixels,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.valid_edge_sub_pixel,
                                          scene_dir,
                                          "validedgeSubPixel",
                                          2,
                                          md.n_valid_pixels,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.values_for_subedges,
                                          scene_dir,
                                          "validzValuesForSubEdges",
                                          1,
                                          md.n_valid_pixels,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.valid_direction_per_pixel,
                                          scene_dir,
                                          "validdirPerPixel",
                                          1,
                                          md.n_valid_pixels,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< uint8_t >( depth_data.valid_section_map,
                                           scene_dir,
                                           "validsectionMapDepth",
                                           1,
                                           md.n_valid_pixels,
                                           "uint8_00",
                                           compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.valid_directions,
                                          scene_dir,
                                          "validdirectionIndex",
                                          1,
                                          md.n_valid_pixels,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< algo::k_matrix >(
        depth_data.k_depth_pinv,
        scene_dir,
        bin_file( "k_depth_pinv", 3, 3, "double_00.bin" ) ) );
    // CHECK(compare_to_bin_file< double >(depth_data.valid_edge_sub_pixel_x, scene_dir, "xim", 1,
    // md.n_valid_pixels, "double_00", compare_same_vectors)); CHECK(compare_to_bin_file< double
    // >(depth_data.valid_edge_sub_pixel_y, scene_dir, "yim", 1, md.n_valid_pixels, "double_00",
    // compare_same_vectors));
    CHECK( compare_to_bin_file< double >( depth_data.sub_points,
                                          scene_dir, "subPoints",
                                          3, md.n_valid_pixels,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< algo::double2 >( depth_data.uvmap,
                                                 scene_dir,
                                                 bin_file( "uv", 2, md.n_valid_pixels, "double_00" )
                                                     + ".bin",
                                                 md.n_valid_pixels, 1,
                                                 compare_same_vectors ) );
    CHECK( compare_to_bin_file< byte >( depth_data.is_inside,
                                        scene_dir, "isInside",
                                        1, md.n_valid_pixels,
                                        "uint8_00",
                                        compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.subpixels_x,
                                          scene_dir, "Z_xim",
                                          1, md.n_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.subpixels_y,
                                          scene_dir, "Z_yim",
                                          1, md.n_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.directions,
                                          scene_dir, "directionIndexInside",
                                          1, md.n_edges,
                                          "double_00",
                                          compare_same_vectors ) );

    CHECK( compare_to_bin_file< double >( depth_data.subpixels_x_round,
                                          scene_dir, "round_xim",
                                          1, md.n_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.subpixels_y_round,
                                          scene_dir, "round_yim",
                                          1, md.n_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( depth_data.weights,
                                          scene_dir, "weights",
                                          1, md.n_edges,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< uint8_t >( depth_data.relevant_pixels_image,
                                           scene_dir, "relevantPixelsImage",
                                           z_w, z_h,
                                           "uint8_00",
                                           compare_same_vectors ) );
    CHECK( compare_to_bin_file< algo::double3 >( depth_data.vertices,
                                                 scene_dir,
                                                 bin_file( "vertices", 3, md.n_edges, "double_00" )
                                                     + ".bin",
                                                 md.n_edges, 1,
                                                 compare_same_vectors ) );
    CHECK( compare_to_bin_file< uint8_t >( depth_data.section_map,
                                           scene_dir, "sectionMapDepthInside",
                                           1, md.n_edges,
                                           "uint8_00",
                                           compare_same_vectors ) );

    CHECK( compare_to_bin_file< uint8_t >( yuy_data.debug.lum_frame,
                                           scene_dir, "YUY2_lum",
                                           rgb_w, rgb_h,
                                           "uint8_00",
                                           compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( yuy_data.debug.edges,
                                          scene_dir, "YUY2_edge",
                                          rgb_w, rgb_h,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( yuy_data.edges_IDT,
                                          scene_dir, "YUY2_IDT",
                                          rgb_w, rgb_h,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( yuy_data.edges_IDTx,
                                          scene_dir, "YUY2_IDTx",
                                          rgb_w, rgb_h,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( yuy_data.edges_IDTy,
                                          scene_dir, "YUY2_IDTy",
                                          rgb_w, rgb_h,
                                          "double_00",
                                          compare_same_vectors ) );
}


void compare_scene( std::string const & scene_dir,
                    bool debug_mode = true,
                    scene_stats * stats = nullptr )
{
    TRACE( "Loading " << scene_dir << " ..." );

    memory_profiler profiler;

    camera_params ci = read_camera_params( scene_dir, "camera_params" );
    dsm_params dsm = read_dsm_params( scene_dir, "DSM_params" );
    ci.dsm_params = dsm.params;
    ci.cal_info = dsm.regs;
    ci.cal_regs = dsm.algo_calibration_registers;

    scene_metadata md( scene_dir );

    algo::optimizer::settings settings;
    read_data_from( join( bin_dir( scene_dir ), "settings" ), &settings );

    auto scale = 1.;
    if (read_thermal_data(scene_dir,
        settings.hum_temp, scale))
    {
        ci.rgb.fx *= scale;
        ci.rgb.fy *= scale;

        auto filename = bin_file( "Kthermal_rgb", 9, 1, "double_00" ) + ".bin";
        TRACE( "Comparing " << filename << " ..." );
        CHECK( compare_to_bin_file( algo::k_matrix( ci.rgb ), scene_dir, filename ) );
    }
    else
    {
        TRACE( "No thermal data found" );
    }

    algo::optimizer cal( settings, debug_mode );
    init_algo( cal,
               scene_dir,
               md.rgb_file,
               md.rgb_prev_file,
               md.rgb_prev_valid_file,
               md.ir_file,
               md.z_file,
               ci,
               &profiler );

    auto & z_data = cal.get_z_data();
    auto & ir_data = cal.get_ir_data();
    auto & yuy_data = cal.get_yuy_data();
    auto & depth_data = cal.get_z_data();
    auto & decision_params = cal.get_decision_params();
    auto & svm_features = cal.get_extracted_features();

    auto rgb_h = ci.rgb.height;
    auto rgb_w = ci.rgb.width;
    auto z_h = ci.z.height;
    auto z_w = ci.z.width;
    auto num_of_calib_elements = 22;
    auto num_of_p_matrix_elements = sizeof( algo::p_matrix ) / sizeof( double );
    auto num_of_k_matrix_elements = sizeof( algo::k_matrix ) / sizeof( double );

    if( debug_mode )
        compare_preprocessing_data( scene_dir, z_data, ir_data, yuy_data, md );

    // ---

    algo::input_validity_data data;
    profiler.section( "Checking scene validity" );
    bool const is_scene_valid = cal.is_scene_valid( debug_mode ? &data : nullptr );
    profiler.section_end();

    bool const matlab_scene_valid = md.is_scene_valid;
    CHECK( is_scene_valid == matlab_scene_valid );
    if( debug_mode )
    {
        bool spread
            = read_from< uint8_t >( join( bin_dir( scene_dir ), "DirSpread_1x1_uint8_00.bin" ) );
        CHECK( data.edges_dir_spread == spread );
        bool rgbEdgesSpread = read_from< uint8_t >(
            join( bin_dir( scene_dir ), "rgbEdgesSpread_1x1_uint8_00.bin" ) );
        CHECK( data.rgb_spatial_spread == rgbEdgesSpread );
        bool depthEdgesSpread = read_from< uint8_t >(
            join( bin_dir( scene_dir ), "depthEdgesSpread_1x1_uint8_00.bin" ) );
        CHECK( data.depth_spatial_spread == depthEdgesSpread );
        bool isMovementFromLastSuccess = read_from< uint8_t >(
            join( bin_dir( scene_dir ), "isMovementFromLastSuccess_1x1_uint8_00.bin" ) );
        CHECK( data.is_movement_from_last_success == isMovementFromLastSuccess );
    }
    if( stats )
    {
        stats->n_valid_scene = is_scene_valid;
        stats->n_valid_scene_diff = is_scene_valid != matlab_scene_valid;
    }

    // edge distribution
    CHECK( compare_to_bin_file< double >( z_data.sum_weights_per_section,
                                          scene_dir,
                                          "depthEdgeWeightDistributionPerSectionDepth",
                                          1,
                                          4,
                                          "double_00",
                                          compare_same_vectors ) );

    // CHECK( compare_to_bin_file< byte >( z_data.section_map, scene_dir, "sectionMapDepth_trans",
    // 1, md.n_edges, "uint8_00", compare_same_vectors ) ); CHECK( compare_to_bin_file< byte >(
    // yuy_data.section_map, scene_dir, "sectionMapRgb_trans", 1, rgb_w*rgb_h, "uint8_00",
    // compare_same_vectors ) );

    CHECK( compare_to_bin_file< double >( yuy_data.sum_weights_per_section,
                                          scene_dir,
                                          "edgeWeightDistributionPerSectionRgb",
                                          1,
                                          4,
                                          "double_00",
                                          compare_same_vectors ) );

    // gradient balanced
    // TODO NOHA
    CHECK( compare_to_bin_file< double >( z_data.sum_weights_per_direction,
                                          scene_dir,
                                          "edgeWeightsPerDir",
                                          4,
                                          1,
                                          "double_00",
                                          compare_same_vectors ) );

    // movment check
    // 1. dilation
    if( debug_mode )
        CHECK( compare_to_bin_file< uint8_t >( yuy_data.debug.movement_result.logic_edges,
                                               scene_dir, "logicEdges",
                                               rgb_w, rgb_h,
                                               "uint8_00",
                                               compare_same_vectors ) );
    if( debug_mode )
        CHECK( compare_to_bin_file< double >( yuy_data.debug.movement_result.dilated_image,
                                              scene_dir, "dilatedIm",
                                              rgb_w, rgb_h,
                                              "double_00",
                                              compare_same_vectors ) );

    // 2. gausssian
    if( debug_mode )
        CHECK( compare_to_bin_file< double >( yuy_data.debug.movement_result.yuy_diff,
                                              scene_dir, "diffIm_01",
                                              rgb_w, rgb_h,
                                              "double_00",
                                              compare_same_vectors ) );
    if( debug_mode )
        CHECK( compare_to_bin_file< double >( yuy_data.debug.movement_result.gaussian_filtered_image,
                                              scene_dir, "diffIm",
                                              rgb_w, rgb_h,
                                              "double_00",
                                              compare_same_vectors ) );

    // 3. movement
    if( debug_mode )
        CHECK( compare_to_bin_file< double >( yuy_data.debug.movement_result.gaussian_diff_masked,
                                              scene_dir, "IDiffMasked",
                                              rgb_w, rgb_h,
                                              "double_00",
                                              compare_same_vectors ) );
    if( debug_mode )
        CHECK( compare_to_bin_file< uint8_t >( yuy_data.debug.movement_result.move_suspect,
                                               scene_dir, "ixMoveSuspect",
                                               rgb_w, rgb_h,
                                               "uint8_00",
                                               compare_same_vectors ) );

    //--

    auto cb = [&]( algo::data_collect const & data ) {
        // data.iteration_data_p is 0-based!
        // REQUIRE( data.iteration_data_p < md.n_iterations );

        // REQUIRE( data.cycle <= md.n_cycles );

        if( data.type == algo::k_to_dsm_data )
        {
            std::cout << std::endl
                      << "COMPARING K_TO_DSM DATA " << data.cycle_data_p.cycle << std::endl;
            algo::k_matrix old_k = data.k2dsm_data_p.inputs.old_k;
            algo::k_matrix new_k = data.k2dsm_data_p.inputs.new_k;

            CHECK( compare_to_bin_file< double >(
                std::vector< double >( std::begin( old_k.k_mat.rot ), std::end( old_k.k_mat.rot ) ),
                scene_dir,
                bin_file( "k2dsm_inpus_oldKdepth",
                          data.cycle_data_p.cycle,
                          num_of_k_matrix_elements,
                          1,
                          "double_00.bin" ),
                num_of_k_matrix_elements,
                1,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >(
                std::vector< double >( std::begin( new_k.k_mat.rot ), std::end( new_k.k_mat.rot ) ),
                scene_dir,
                bin_file( "k2dsm_inpus_newKdepth",
                          data.cycle_data_p.cycle,
                          num_of_k_matrix_elements,
                          1,
                          "double_00.bin" ),
                num_of_k_matrix_elements,
                1,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< algo::double3 >( data.k2dsm_data_p.inputs.z.vertices,
                                                         scene_dir,
                                                         bin_file( "k2dsm_inpus_vertices",
                                                                   data.cycle_data_p.cycle,
                                                                   3,
                                                                   md.n_edges,
                                                                   "double_00.bin" ),
                                                         md.n_edges,
                                                         1,
                                                         compare_same_vectors ) );

            CHECK( compare_to_bin_file< algo::double2 >(
                std::vector< algo::double2 >(
                    1,
                    { data.k2dsm_data_p.inputs.previous_dsm_params.h_scale,
                      data.k2dsm_data_p.inputs.previous_dsm_params.v_scale } ),
                scene_dir,
                bin_file( "k2dsm_inpus_acData", data.cycle_data_p.cycle, 2, 1, "double_00.bin" ),
                1,
                1,
                compare_same_vectors ) );


            CHECK( compare_to_bin_file< algo::algo_calibration_registers >(
                data.k2dsm_data_p.inputs.new_dsm_regs,
                scene_dir,
                bin_file( "k2dsm_inpus_dsmRegs",
                          data.cycle_data_p.cycle,
                          4,
                          1,
                          "double_00.bin" ) ) );


            CHECK( compare_to_bin_file< algo::algo_calibration_registers >(
                data.k2dsm_data_p.dsm_regs_orig,
                scene_dir,
                bin_file( "dsmRegsOrig", data.cycle_data_p.cycle, 4, 1, "double_00.bin" ) ) );

            CHECK( compare_to_bin_file< uint8_t >(
                data.k2dsm_data_p.relevant_pixels_image_rot,
                scene_dir,
                bin_file( "relevantPixelnImage_rot", data.cycle_data_p.cycle, z_w, z_h, "uint8_00" )
                    + ".bin",
                z_w,
                z_h,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< algo::los_shift_scaling >(
                data.k2dsm_data_p.dsm_pre_process_data.last_los_error,
                scene_dir,
                bin_file( "dsm_los_error_orig",
                          data.cycle_data_p.cycle,
                          1,
                          4,
                          "double_00.bin" ) ) );

            CHECK( compare_to_bin_file< algo::double3 >(
                data.k2dsm_data_p.dsm_pre_process_data.vertices_orig,
                scene_dir,
                bin_file( "verticesOrig",
                          data.cycle_data_p.cycle,
                          3,
                          md.n_relevant_pixels,
                          "double_00" )
                    + ".bin",
                md.n_relevant_pixels,
                1,
                compare_same_vectors ) );


            compare_vertices_to_los_data( scene_dir,
                                          md.n_relevant_pixels,
                                          data.cycle_data_p.cycle,
                                          "first_",
                                          data.k2dsm_data_p.first_norm_vertices_to_los_data );

            CHECK( compare_to_bin_file< algo::double2 >(
                data.k2dsm_data_p.dsm_pre_process_data.los_orig,
                scene_dir,
                bin_file( "losOrig", data.cycle_data_p.cycle, 2, md.n_relevant_pixels, "double_00" )
                    + ".bin",
                md.n_relevant_pixels,
                1,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >( data.k2dsm_data_p.errL2,
                                                  scene_dir,
                                                  bin_file( "errL2",
                                                            data.cycle_data_p.cycle,
                                                            1,
                                                            data.k2dsm_data_p.errL2.size(),
                                                            "double_00" )
                                                      + ".bin",
                                                  data.k2dsm_data_p.errL2.size(),
                                                  1,
                                                  compare_same_vectors ) );

            CHECK( compare_to_bin_file< algo::double2 >(
                data.k2dsm_data_p.focal_scaling,
                scene_dir,
                bin_file( "focalScaling", data.cycle_data_p.cycle, 2, 1, "double_00.bin" ) ) );

            CHECK( compare_to_bin_file< std::vector< double > >(
                data.k2dsm_data_p.sg_mat,
                scene_dir,
                bin_file( "sgMat",
                          data.cycle_data_p.cycle,
                          data.k2dsm_data_p.sg_mat[0].size(),
                          data.k2dsm_data_p.sg_mat.size(),
                          "double_00" )
                    + ".bin",
                data.k2dsm_data_p.sg_mat.size(),
                data.k2dsm_data_p.sg_mat[0].size(),
                data.k2dsm_data_p.sg_mat.size(),
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >(
                data.k2dsm_data_p.sg_mat_tag_x_sg_mat,
                scene_dir,
                bin_file( "sg_mat_tag_x_sg_mat",
                          data.cycle_data_p.cycle,
                          1,
                          data.k2dsm_data_p.sg_mat_tag_x_sg_mat.size(),
                          "double_00" )
                    + ".bin",
                data.k2dsm_data_p.sg_mat_tag_x_sg_mat.size(),
                1,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >(
                data.k2dsm_data_p.sg_mat_tag_x_err_l2,
                scene_dir,
                bin_file( "sg_mat_tag_x_err_l2",
                          data.cycle_data_p.cycle,
                          1,
                          data.k2dsm_data_p.sg_mat_tag_x_err_l2.size(),
                          "double_00" )
                    + ".bin",
                data.k2dsm_data_p.sg_mat_tag_x_err_l2.size(),
                1,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >( data.k2dsm_data_p.quad_coef,
                                                  scene_dir,
                                                  bin_file( "quadCoef",
                                                            data.cycle_data_p.cycle,
                                                            1,
                                                            data.k2dsm_data_p.quad_coef.size(),
                                                            "double_00" )
                                                      + ".bin",
                                                  data.k2dsm_data_p.quad_coef.size(),
                                                  1,
                                                  compare_same_vectors ) );

            CHECK( compare_to_bin_file< algo::double2 >(
                data.k2dsm_data_p.opt_scaling_1,
                scene_dir,
                bin_file( "optScaling1", data.cycle_data_p.cycle, 1, 2, "double_00.bin" ) ) );

            CHECK( compare_to_bin_file< algo::double2 >(
                data.k2dsm_data_p.opt_scaling,
                scene_dir,
                bin_file( "optScaling", data.cycle_data_p.cycle, 1, 2, "double_00.bin" ) ) );

            CHECK( compare_to_bin_file< algo::double2 >(
                data.k2dsm_data_p.new_los_scaling,
                scene_dir,
                bin_file( "newlosScaling", data.cycle_data_p.cycle, 1, 2, "double_00.bin" ) ) );

            CHECK( compare_to_bin_file< algo::double2 >(
                std::vector< algo::double2 >( 1,
                                              { data.k2dsm_data_p.dsm_params_cand.h_scale,
                                                data.k2dsm_data_p.dsm_params_cand.v_scale } ),
                scene_dir,
                bin_file( "acDataCand", data.cycle_data_p.cycle, 2, 1, "double_00.bin" ),
                1,
                1,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< algo::algo_calibration_registers >(
                data.k2dsm_data_p.dsm_regs_cand,
                scene_dir,
                bin_file( "dsmRegsCand", data.cycle_data_p.cycle, 4, 1, "double_00.bin" ) ) );

            compare_vertices_to_los_data( scene_dir,
                                          md.n_edges,
                                          data.cycle_data_p.cycle,
                                          "second_",
                                          data.k2dsm_data_p.second_norm_vertices_to_los_data );

            CHECK( compare_to_bin_file< algo::double2 >(
                data.k2dsm_data_p.los_orig,
                scene_dir,
                bin_file( "orig_los", data.cycle_data_p.cycle, 2, md.n_edges, "double_00" )
                    + ".bin",
                md.n_edges,
                1,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< algo::double2 >(
                data.k2dsm_data_p.dsm,
                scene_dir,
                bin_file( "dsm", data.cycle_data_p.cycle, 2, md.n_edges, "double_00" ) + ".bin",
                md.n_edges,
                1,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< algo::double3 >(
                data.k2dsm_data_p.vertices,
                scene_dir,
                bin_file( "new_vertices", data.cycle_data_p.cycle, 3, md.n_edges, "double_00.bin" ),
                md.n_edges,
                1,
                compare_same_vectors ) );

            TRACE( "\nSet next cycle data from Matlab:" );
        }
        else if( data.type == algo::cycle_data )
        {
            std::cout << std::endl
                      << "COMPARING CYCLE DATA " << data.cycle_data_p.cycle << std::endl;

            algo::k_matrix k_depth;
            k_depth.k_mat.rot[0] = data.cycle_data_p.new_k_depth.fx;
            k_depth.k_mat.rot[2] = data.cycle_data_p.new_k_depth.ppx;
            k_depth.k_mat.rot[4] = data.cycle_data_p.new_k_depth.fy;
            k_depth.k_mat.rot[5] = data.cycle_data_p.new_k_depth.ppy;

            CHECK( compare_to_bin_file< algo::k_matrix >(
                k_depth,
                scene_dir,
                bin_file( "end_cycle_Kdepth", data.cycle_data_p.cycle, 3, 3, "double_00.bin" ) ) );

            CHECK( compare_to_bin_file< algo::p_matrix >( data.cycle_data_p.new_params.curr_p_mat,
                                                          scene_dir,
                                                          bin_file( "end_cycle_p_matrix",
                                                                    data.cycle_data_p.cycle,
                                                                    num_of_p_matrix_elements,
                                                                    1,
                                                                    "double_00.bin" ) ) );

            try
            {

                /* auto vertices = read_vector_from<algo::double3>(bin_file(bin_dir(scene_dir) +
                 "end_cycle_vertices", data.cycle_data_p.cycle, 3, md.n_edges, "double_00.bin"));

                 algo::p_matrix p_mat;

                 auto p_vec = read_vector_from<double>(bin_file(bin_dir(scene_dir) +
                 "end_cycle_p_matrix", data.cycle_data_p.cycle, num_of_p_matrix_elements, 1,
                     "double_00.bin"));
                 std::copy(p_vec.begin(), p_vec.end(), p_mat.vals);

                 auto dsm_regs_vec = read_vector_from< algo::algo_calibration_registers >(
                     bin_file(bin_dir(scene_dir) + "end_cycle_dsmRegsCand", data.cycle_data_p.cycle,
                 4, 1, "double_00.bin"));

                 algo::algo_calibration_registers dsm_regs =
                 *(algo::algo_calibration_registers*)(dsm_regs_vec.data()); auto ac_data_vec =
                 read_vector_from< algo::double2 >(bin_file(bin_dir(scene_dir) + "end_cycle_acData",
                 data.cycle_data_p.cycle, 2, 1, "double_00.bin"));

                 algo::rs2_dsm_params_double dsm_params;
                 dsm_params.h_scale = ac_data_vec[0].x;
                 dsm_params.v_scale = ac_data_vec[0].y;
                 dsm_params.model = RS2_DSM_CORRECTION_AOT;*/

                // auto Kdepth = read_vector_from<algo::matrix_3x3>(bin_file(bin_dir(scene_dir) +
                // "end_cycle_Kdepth", data.cycle_data_p.cycle, 3, 3, "double_00.bin"));
                // cal.set_cycle_data(vertices, k_depth, p_mat, dsm_regs, dsm_params);
            }
            catch( std::runtime_error & e )
            {
                // if device isn't calibrated, get_extrinsics must error out (according to old
                // comment. Might not be true under new API)
                WARN( e.what() );
            }
        }
        else if( data.type == algo::iteration_data )
        {
            std::cout << std::endl
                      << "COMPARING ITERATION DATA " << data.cycle_data_p.cycle << " "
                      << data.iteration_data_p.iteration + 1 << std::endl;
            CHECK( compare_to_bin_file< algo::p_matrix >(
                data.iteration_data_p.params.curr_p_mat,
                scene_dir,
                bin_file( "p_matrix_iteration",
                          data.cycle_data_p.cycle,
                          data.iteration_data_p.iteration + 1,
                          num_of_p_matrix_elements,
                          1,
                          "double_00.bin" ) ) );

            CHECK( compare_to_bin_file< double >(
                std::vector< double >( std::begin( data.iteration_data_p.c.k_mat.k_mat.rot ),
                                       std::end( data.iteration_data_p.c.k_mat.k_mat.rot ) ),
                scene_dir,
                bin_file( "Krgb_iteration",
                          data.cycle_data_p.cycle,
                          data.iteration_data_p.iteration + 1,
                          9,
                          1,
                          "double_00.bin" ),
                9,
                1,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >(
                std::vector< double >( 1, data.iteration_data_p.params.cost ),
                scene_dir,
                bin_file( "cost_iteration",
                          data.cycle_data_p.cycle,
                          data.iteration_data_p.iteration + 1,
                          1,
                          1,
                          "double_00.bin" ),
                1,
                1,
                compare_same_vectors ) );

            CHECK(
                compare_to_bin_file< algo::double2 >( data.iteration_data_p.uvmap,
                                                      scene_dir,
                                                      bin_file( "uvmap_iteration",
                                                                data.cycle_data_p.cycle,
                                                                data.iteration_data_p.iteration + 1,
                                                                2,
                                                                md.n_edges,
                                                                "double_00.bin" ),
                                                      md.n_edges,
                                                      1,
                                                      compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >( data.iteration_data_p.d_vals,
                                                  scene_dir,
                                                  bin_file( "DVals_iteration",
                                                            data.cycle_data_p.cycle,
                                                            data.iteration_data_p.iteration + 1,
                                                            1,
                                                            md.n_edges,
                                                            "double_00.bin" ),
                                                  md.n_edges,
                                                  1,
                                                  compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >( data.iteration_data_p.d_vals_x,
                                                  scene_dir,
                                                  bin_file( "DxVals_iteration",
                                                            data.cycle_data_p.cycle,
                                                            data.iteration_data_p.iteration + 1,
                                                            1,
                                                            md.n_edges,
                                                            "double_00.bin" ),
                                                  md.n_edges,
                                                  1,
                                                  compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >( data.iteration_data_p.d_vals_y,
                                                  scene_dir,
                                                  bin_file( "DyVals_iteration",
                                                            data.cycle_data_p.cycle,
                                                            data.iteration_data_p.iteration + 1,
                                                            1,
                                                            md.n_edges,
                                                            "double_00.bin" ),
                                                  md.n_edges,
                                                  1,
                                                  compare_same_vectors ) );

            CHECK(
                compare_to_bin_file< algo::double2 >( data.iteration_data_p.xy,
                                                      scene_dir,
                                                      bin_file( "xy_iteration",
                                                                data.cycle_data_p.cycle,
                                                                data.iteration_data_p.iteration + 1,
                                                                2,
                                                                md.n_edges,
                                                                "double_00.bin" ),
                                                      md.n_edges,
                                                      1,
                                                      compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >( data.iteration_data_p.rc,
                                                  scene_dir,
                                                  bin_file( "rc_iteration",
                                                            data.cycle_data_p.cycle,
                                                            data.iteration_data_p.iteration + 1,
                                                            1,
                                                            md.n_edges,
                                                            "double_00.bin" ),
                                                  md.n_edges,
                                                  1,
                                                  compare_same_vectors ) );

            CHECK( compare_to_bin_file< algo::p_matrix >(
                data.iteration_data_p.coeffs_p.x_coeffs,
                scene_dir,
                bin_file( "xCoeff_P_iteration",
                          data.cycle_data_p.cycle,
                          data.iteration_data_p.iteration + 1,
                          num_of_p_matrix_elements,
                          md.n_edges,
                          "double_00.bin" ),
                md.n_edges,
                1,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< algo::p_matrix >(
                data.iteration_data_p.coeffs_p.y_coeffs,
                scene_dir,
                bin_file( "yCoeff_P_iteration",
                          data.cycle_data_p.cycle,
                          data.iteration_data_p.iteration + 1,
                          num_of_p_matrix_elements,
                          md.n_edges,
                          "double_00.bin" ),
                md.n_edges,
                1,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >(
                std::vector< double >(
                    std::begin( data.iteration_data_p.params.calib_gradients.vals ),
                    std::end( data.iteration_data_p.params.calib_gradients.vals ) ),
                scene_dir,
                bin_file( "grad_iteration",
                          data.cycle_data_p.cycle,
                          data.iteration_data_p.iteration + 1,
                          num_of_p_matrix_elements,
                          1,
                          "double_00.bin" ),
                num_of_p_matrix_elements,
                1,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >(
                std::vector< double >( 1, data.iteration_data_p.grads_norma ),
                scene_dir,
                bin_file( "grad_norma_iteration",
                          data.cycle_data_p.cycle,
                          data.iteration_data_p.iteration + 1,
                          1,
                          1,
                          "double_00.bin" ),
                1,
                1,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >(
                std::vector< double >( std::begin( data.iteration_data_p.grads_norm.vals ),
                                       std::end( data.iteration_data_p.grads_norm.vals ) ),
                scene_dir,
                bin_file( "grads_norm_iteration",
                          data.cycle_data_p.cycle,
                          data.iteration_data_p.iteration + 1,
                          num_of_p_matrix_elements,
                          1,
                          "double_00.bin" ),
                num_of_p_matrix_elements,
                1,
                compare_same_vectors ) );


            CHECK( compare_to_bin_file< double >(
                std::vector< double >( std::begin( data.iteration_data_p.normalized_grads.vals ),
                                       std::end( data.iteration_data_p.normalized_grads.vals ) ),
                scene_dir,
                bin_file( "normalized_grads_iteration",
                          data.cycle_data_p.cycle,
                          data.iteration_data_p.iteration + 1,
                          num_of_p_matrix_elements,
                          1,
                          "double_00.bin" ),
                num_of_p_matrix_elements,
                1,
                compare_same_vectors ) );


            CHECK( compare_to_bin_file< double >(
                std::vector< double >( std::begin( data.iteration_data_p.unit_grad.vals ),
                                       std::end( data.iteration_data_p.unit_grad.vals ) ),
                scene_dir,
                bin_file( "unit_grad_iteration",
                          data.cycle_data_p.cycle,
                          data.iteration_data_p.iteration + 1,
                          num_of_p_matrix_elements,
                          1,
                          "double_00.bin" ),
                num_of_p_matrix_elements,
                1,
                compare_same_vectors ) );

            CHECK(
                compare_to_bin_file< double >( std::vector< double >( 1, data.iteration_data_p.t ),
                                               scene_dir,
                                               bin_file( "t_iteration",
                                                         data.cycle_data_p.cycle,
                                                         data.iteration_data_p.iteration + 1,
                                                         1,
                                                         1,
                                                         "double_00.bin" ),
                                               1,
                                               1,
                                               compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >(
                std::vector< double >( 1, data.iteration_data_p.back_tracking_line_search_iters ),
                scene_dir,
                bin_file( "back_tracking_line_iter_count_iteration",
                          data.cycle_data_p.cycle,
                          data.iteration_data_p.iteration + 1,
                          1,
                          1,
                          "double_00.bin" ),
                1,
                1,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >(
                std::vector< double >(
                    std::begin( data.iteration_data_p.next_params.curr_p_mat.vals ),
                    std::end( data.iteration_data_p.next_params.curr_p_mat.vals ) ),
                scene_dir,
                bin_file( "next_p_matrix_iteration",
                          data.cycle_data_p.cycle,
                          data.iteration_data_p.iteration + 1,
                          num_of_p_matrix_elements,
                          1,
                          "double_00.bin" ),
                num_of_p_matrix_elements,
                1,
                compare_same_vectors ) );

            CHECK( compare_to_bin_file< double >(
                std::vector< double >( 1, data.iteration_data_p.next_params.cost ),
                scene_dir,
                bin_file( "next_cost_iteration",
                          data.cycle_data_p.cycle,
                          data.iteration_data_p.iteration + 1,
                          1,
                          1,
                          "double_00.bin" ),
                1,
                1,
                compare_same_vectors ) );
        }
    };

    // Our code doesn't count the first iteration; the Matlab code starts at 1 even if it doesn't do
    // anything...
    // REQUIRE( cal.optimize( cb ) + 1 == md.n_iterations );
    profiler.section( "Optimizing" );
    auto n_cycles = cal.optimize( cb );
    profiler.section_end();


    auto new_calibration = cal.get_calibration();
    auto cost = cal.get_cost();

    auto filename = bin_file( "new_calib", num_of_calib_elements, 1, "double_00" ) + ".bin";
    TRACE( "Comparing " << filename << " ..." );
    algo::calib matlab_calib;
    double matlab_cost = 0;
    CHECK( get_calib_and_cost_from_raw_data( matlab_calib, matlab_cost, scene_dir, filename ) );
    CHECK( compare_calib( new_calibration, cost, matlab_calib, matlab_cost ) );
    new_calibration.copy_coefs( matlab_calib );
    if( stats )
    {
        stats->cost = cost;
        stats->d_cost = cost - matlab_cost;
    }


#if 1
    auto vertices = read_vector_from< algo::double3 >(
        bin_file( join( bin_dir( scene_dir ), "end_vertices" ), 3, md.n_edges, "double_00.bin" ) );

    if( stats )
    {
        auto our_uvmap = algo::get_texture_map( depth_data.vertices,
                                                new_calibration,
                                                new_calibration.calc_p_mat() );
        auto matlab_uvmap
            = algo::get_texture_map( vertices, matlab_calib, matlab_calib.calc_p_mat() );

        CHECK( our_uvmap.size() == matlab_uvmap.size() );
        if( our_uvmap.size() == matlab_uvmap.size() )
            stats->d_movement = cal.calc_correction_in_pixels( our_uvmap, matlab_uvmap );
        else
            stats->d_movement = -1;
    }


    algo::p_matrix p_mat;

    auto p_vec = read_vector_from< double >( bin_file( join( bin_dir( scene_dir ), "end_p_matrix" ),
                                                       num_of_p_matrix_elements,
                                                       1,
                                                       "double_00.bin" ) );

    std::copy( p_vec.begin(), p_vec.end(), p_mat.vals );

    algo::p_matrix p_mat_opt;

    auto p_vec_opt
        = read_vector_from< double >( bin_file( join( bin_dir( scene_dir ), "end_p_matrix_opt" ),
                                                num_of_p_matrix_elements,
                                                1,
                                                "double_00.bin" ) );

    std::copy( p_vec_opt.begin(), p_vec_opt.end(), p_mat_opt.vals );

    cal.set_final_data( vertices, p_mat, p_mat_opt );

    //--

    // Pixel movement is OK, but some sections have negative cost
    profiler.section( "Checking output validity" );
    bool const is_valid_results = cal.is_valid_results();
    profiler.section_end();
    bool const matlab_valid_results = md.is_output_valid;
    CHECK( is_valid_results == matlab_valid_results );
    if( stats )
    {
        stats->n_cycles = n_cycles;
        stats->n_valid_result = is_valid_results;
        stats->n_valid_result_diff = is_valid_results != matlab_valid_results;

        stats->n_converged = is_valid_results && is_scene_valid;
        bool const matlab_converged = matlab_valid_results && matlab_scene_valid;
        stats->n_converged_diff = bool( stats->n_converged ) != matlab_converged;
    }

    double const movement_in_pixels = cal.calc_correction_in_pixels( new_calibration );
    double const matlab_movement_in_pixels = md.correction_in_pixels;
    CHECK( movement_in_pixels == approx( matlab_movement_in_pixels ) );
    if( stats )
    {
        stats->movement = movement_in_pixels;
        // stats->d_movement = movement_in_pixels - matlab_movement_in_pixels;
    }

    CHECK( compare_to_bin_file< double >( z_data.cost_diff_per_section,
                                          scene_dir,
                                          "costDiffPerSection",
                                          4,
                                          1,
                                          "double_00",
                                          compare_same_vectors ) );

    // svm - remove xyMovementFromOrigin because its still not implemented
    auto svm_features_mat = read_vector_from< double >(
        bin_file( join( bin_dir( scene_dir ), "svm_featuresMat" ), 10, 1, "double_00.bin" ) );

    svm_features_mat.erase( svm_features_mat.begin() + 7 );
    auto svm_mat = svm_features;
    svm_mat.erase( svm_mat.begin() + 7 );

    CHECK( compare_same_vectors( svm_features_mat, svm_mat ) );

    // CHECK(compare_to_bin_file< double >(svm_features, scene_dir, "svm_featuresMat", 10, 1,
    // "double_00", compare_same_vectors));
    CHECK( compare_to_bin_file< double >( decision_params.distribution_per_section_depth,
                                          scene_dir,
                                          "svm_edgeWeightDistributionPerSectionDepth",
                                          1,
                                          4,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( decision_params.distribution_per_section_rgb,
                                          scene_dir,
                                          "svm_edgeWeightDistributionPerSectionRgb",
                                          1,
                                          4,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( decision_params.edge_weights_per_dir,
                                          scene_dir,
                                          "svm_edgeWeightsPerDir",
                                          4,
                                          1,
                                          "double_00",
                                          compare_same_vectors ) );
    CHECK( compare_to_bin_file< double >( decision_params.improvement_per_section,
                                          scene_dir,
                                          "svm_improvementPerSection",
                                          4,
                                          1,
                                          "double_00",
                                          compare_same_vectors ) );
#endif

    profiler.stop();

    if( stats )
        stats->memory_consumption_peak = profiler.get_peak() - profiler.get_baseline();
}
