classdef option < int64
    enumeration
        backlight_compensation          ( 0)
        brightness                      ( 1)
        contrast                        ( 2)
        exposure                        ( 3)
        gain                            ( 4)
        gamma                           ( 5)
        hue                             ( 6)
        saturation                      ( 7)
        sharpness                       ( 8)
        white_balance                   ( 9)
        enable_auto_exposure            (10)
        enable_auto_white_balance       (11)
        visual_preset                   (12)
        laser_power                     (13)
        accuracy                        (14)
        motion_range                    (15)
        filter_option                   (16)
        confidence_threshold            (17)
        emitter_enabled                 (18)
        frames_queue_size               (19)
        total_frame_drops               (20)
        auto_exposure_mode              (21)
        power_line_frequency            (22)
        asic_temperature                (23)
        error_polling_enabled           (24)
        projector_temperature           (25)
        output_trigger_enabled          (26)
        motion_module_temperature       (27)
        depth_units                     (28)
        enable_motion_correction        (29)
        auto_exposure_priority          (30)
        color_scheme                    (31)
        histogram_equalization_enabled  (32)
        min_distance                    (33)
        max_distance                    (34)
        texture_source                  (35)
        filter_magnitude                (36)
        filter_smooth_alpha             (37)
        filter_smooth_delta             (38)
        holes_fill                      (39)
        stereo_baseline                 (40)
        auto_exposure_converge_step     (41)
        inter_cam_sync_mode             (42)
        stream_filter                   (43)
        stream_format_filter            (44)
        stream_index_filter             (45)
        emitter_on_off                  (46)
        zero_order_point_x              (47)
        zero_order_point_y              (48)
        lld_temperature                 (49)
        mc_temperature                  (50)
        ma_temperature                  (51)
        apd_temperature                 (52)
        hardware_preset                 (53)
        global_time_enabled             (54)
        enable_mapping                  (55)
        enable_relocalization           (56)
        enable_pose_jumping             (57)
        enable_dynamic_calibration      (58)
        depth_offset                    (59)
        led_power                       (60)
        zero_order_enabled              (61) % Deprecated
        enable_map_preservation         (62)
        freefall_detection_enabled      (63)
        apd_exposure_time               (64)
        post_processing_sharpening      (65)
        pre_processing_sharpening       (66)
        noise_filter_level              (67)
        invalidation_bypass             (68)
        ambient_light_env_level         (69) % Deprecated - Use DIGITAL_GAIN instead
        digital_gain                    (69)
        sensor_mode                     (70)
        emitter_always_on               (71)
        thermal_compensation            (72)
        trigger_camera_accuracy_health  (73)
        reset_camera_accuracy_health    (74)
        host_performance                (75)
        hdr_enabled                     (76)
        sequence_name                   (77)
        sequence_size                   (78)
        sequence_id                     (79)
        humidity_temperature            (80)
        max_usable_range                (81)
        alternate_IR                    (82)
        noise_estimation                (83)
        enable_ir_reclectivity          (84)
        auto_exposure_limit             (85)
        auto_gain_limit                 (86)
        auto_rx_sensitivity             (87)
        transmitter_frequency           (88)
        vertical_binning                (89)
        receiver_sensitivity            (90)
        emitter_frequency               (93)
        depth_auto_exposue_mode         (94)
		safety_preset_active_index      (95)
        safety_mode                     (96)
        count                           (97)
    end
end
