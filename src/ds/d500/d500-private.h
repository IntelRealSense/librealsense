// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds/ds-private.h"
#include <src/core/notification.h>

namespace librealsense
{
    namespace platform {
        struct uvc_device_info;
    }

    namespace ds
    {
        const uint16_t D555E_PID = 0x0B56;
        const uint16_t D555E_RECOVERY_PID = 0x0ADE;

        namespace xu_id
        {
        }

        // d500 Devices supported by the current version
        static const std::set<std::uint16_t> rs500_sku_pid = {
            D555E_PID
        };

        static const std::set<std::uint16_t> d500_multi_sensors_pid = {
            D555E_PID
        };

        static const std::set<std::uint16_t> d500_hid_sensors_pid = {
            D555E_PID
        };

        static const std::set<std::uint16_t> d500_hid_bmi_085_pid = {
            D555E_PID
        };

        static const std::map< std::uint16_t, std::string > rs500_sku_names = {
            { ds::D555E_PID,          "Intel RealSense D555e" },
            { ds::D555E_RECOVERY_PID, "Intel RealSense D555e Recovery" }
        };

        bool d500_try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result);

        namespace d500_gvd_offsets 
        {
            constexpr size_t version_offset = 0;
            constexpr size_t payload_size_offset = 0x2;
            constexpr size_t crc32_offset = 0x4;
            constexpr size_t optical_module_serial_offset = 0x54;
            constexpr size_t mb_module_serial_offset = 0x7a;
            constexpr size_t fw_version_offset = 0xba;
        }  // namespace d500_gvd_offsets

        struct d500_gvd_parsed_fields
        {
            uint8_t gvd_version[2];
            uint16_t payload_size;
            uint32_t crc32; 
            std::string optical_module_sn;
            std::string mb_module_sn;
            std::string fw_version;
        };

        enum class d500_calibration_table_id
        {
            depth_eeprom_toc_id = 0xb0,
            module_info_id = 0x1b1,
            rgb_lens_shading_id = 0xb2,
            left_lens_shading_id = 0x1b3,
            right_lens_shading_id = 0x2b3,
            depth_calibration_id = 0xb4,
            left_x_lut_id = 0xb5,
            left_y_lut_id = 0xb6,
            right_x_lut_id = 0xb7,
            right_y_lut_id = 0xb8,
            rgb_calibration_id = 0xb9,
            rgb_lut_id = 0xba,
            imu_calibration_id = 0xbb,
            max_id = -1
        };

        struct d500_undist_configuration
        {
            uint32_t     fx;
            uint32_t     fy;
            uint32_t     x0;
            uint32_t     y0;
            uint32_t     x_shift_in;
            uint32_t     y_shift_in;
            uint32_t     x_scale_in;
            uint32_t     y_scale_in;
        };

        // Calibration implemented according to version 3.1
        struct mini_intrinsics
        {
            uint16_t    image_width;    /**< Width of the image in pixels */
            uint16_t    image_height;   /**< Height of the image in pixels */
            float       ppx;            /**< Horizontal coordinate of the principal point of the image, as a pixel offset from the left edge */
            float       ppy;            /**< Vertical coordinate of the principal point of the image, as a pixel offset from the top edge */
            float       fx;             /**< Focal length of the image plane, as a multiple of pixel width */
            float       fy;             /**< Focal length of the image plane, as a multiple of pixel height */
        };

        struct single_sensor_coef_table
        {
            mini_intrinsics           base_instrinsics;
            uint32_t                  distortion_non_parametric;
            rs2_distortion            distortion_model;          /**< Distortion model of the image */
            float                     distortion_coeffs[13];     /**< Distortion coefficients. Order for Brown-Conrady: [k1, k2, p1, p2, k3]. Order for F-Theta Fish-eye: [k1, k2, k3, k4, 0]. Other models are subject to their own interpretations */
            uint8_t                   reserved[4];
            float                     radial_distortion_lut_range_degs;
            float                     radial_distortion_lut_focal_length;
            d500_undist_configuration undist_config;
            float3x3                  rotation_matrix;
        };

        struct d500_coefficients_table
        {
            table_header              header;
            single_sensor_coef_table  left_coefficients_table;
            single_sensor_coef_table  right_coefficients_table;
            float                     baseline;                   //  the baseline between the cameras in mm units
            uint8_t                   translation_dir;
            uint8_t                   realignement_essential;     // 1/0 - indicates whether the vertical alignement
                                                                  // is required to avoiid overflow in the REC buffer
            int16_t                   vertical_shift;             // in pixels
            mini_intrinsics           rectified_intrinsics;
            uint8_t                   reserved[148];
        };

        struct d500_rgb_calibration_table
        {
            table_header              header;
            single_sensor_coef_table  rgb_coefficients_table;
            float3                    translation_rect;           // Translation vector for rectification
            mini_intrinsics           rectified_intrinsics;
            uint8_t                   reserved[48];
        };

        rs2_intrinsics get_d500_intrinsic_by_resolution(const std::vector<uint8_t>& raw_data, d500_calibration_table_id table_id, 
            uint32_t width, uint32_t height, bool is_symmetrization_enabled = false);
        rs2_intrinsics get_d500_depth_intrinsic_by_resolution(const std::vector<uint8_t>& raw_data, 
            uint32_t width, uint32_t height, bool is_symmetrization_enabled = false);
        rs2_intrinsics get_d500_color_intrinsic_by_resolution(const std::vector<uint8_t>& raw_data, uint32_t width, uint32_t height);
        pose get_d500_color_stream_extrinsic(const std::vector<uint8_t>& raw_data);



        enum class d500_calib_location
        {
            d500_calib_eeprom          = 0,
            d500_calib_flash_memory    = 1,
            d500_calib_ram_memory      = 2
        };

        enum class d500_calib_type
        {
            d500_calib_dynamic = 0,
            d500_calib_gold    = 1
        };

        enum d500_notification_types { // enam literal names received from HKR team [RSDEV-643]
            EHU_IDX_START                      = 0U,
            EHU_IDX_CAN_FD_INT1                = 0U,
            EHU_IDX_CAN_FD_MEM_CORR_ERR        = 1U,
            EHU_IDX_CAN_FD_NON_FATAL_ERR       = 2U,
            EHU_IDX_CCU_ERR                    = 3U,
            EHU_IDX_TOP_CONFIG_NOC_MISSION_ERR = 4U,
            EHU_IDX_TOP_CONFIG_NOC_LATENT_ERR  = 5U,
            EHU_IDX_TOP_CONFIG_NOC_WDT_ERR     = 6U,
            EHU_IDX_CPU_WDT_INTR_0             = 7U,
            EHU_IDX_CPU_WDT_INTR_1             = 8U,
            EHU_IDX_CPU_WDT_INTR_2             = 9U,
            EHU_IDX_CPU_WDT_INTR_3             = 10U,
            EHU_IDX_CPU_WDT_SYS_RST_0          = 11U,
            EHU_IDX_CPU_WDT_SYS_RST_1          = 12U,
            EHU_IDX_CPU_WDT_SYS_RST_2          = 13U,
            EHU_IDX_CPU_WDT_SYS_RST_3          = 14U,
            EHU_IDX_CPU_MEM_CORR_ERR           = 15U,
            EHU_IDX_CPU_MEM_NON_FATAL_ERR      = 16U,
            EHU_IDX_CPU_MEM_FATAL_ERR          = 17U,
            EHU_IDX_CPU_NOC_MISSION_ERR        = 18U,
            EHU_IDX_CPU_NOC_LATENT_ERR         = 19U,
            EHU_IDX_CPU_EXTERRIRQ              = 20U,
            EHU_IDX_CPU_INTERRIRQ              = 21U,
            EHU_IDX_GIC_ECC_FATAL              = 22U,
            EHU_IDX_TOP_DATA_NOC_MISSION_ERR   = 23U,
            EHU_IDX_TOP_DATA_NOC_LATENT_ERR    = 24U,
            EHU_IDX_TOP_DATA_NOC_WDT_ERR       = 25U,
            EHU_IDX_LPDDR_INTERNAL_FATAL_ERR   = 26U,
            EHU_IDX_LPDDR_DFI_ECC_CORR_ERR     = 27U,
            EHU_IDX_DSP_RST_REQ                = 28U,
            EHU_IDX_DSP_PFATAL_ERR             = 29U,
            EHU_IDX_DSP_MEM_CORR_ERR           = 30U,
            EHU_IDX_DSP_MEM_NON_FATAL_ERR      = 31U,
            EHU_IDX_DSP_MEM_FATAL_ERR          = 32U,
            EHU_IDX_EHU_WDT0_ERR               = 33U,
            EHU_IDX_EHU_WDT1_ERR               = 34U,
            EHU_IDX_EHU_WDT2_ERR               = 35U,
            EHU_IDX_EHU_WDT3_ERR               = 36U,
            EHU_IDX_EHU_WDT4_ERR               = 37U,
            EHU_IDX_EHU_WDT5_ERR               = 38U,
            EHU_IDX_EHU_WDT6_ERR               = 39U,
            EHU_IDX_ETH_SBD_SFTY_CE_INTR       = 40U,
            EHU_IDX_ETH_SBD_SFTY_UE_INTR       = 41U,
            EHU_IDX_ENC0_MEM_NON_FATAL_ERR     = 42U,
            EHU_IDX_ENC1_MEM_NON_FATAL_ERR     = 43U,
            EHU_IDX_HIF_UC_PFATAL_ERR          = 44U,
            EHU_IDX_HIF_UC_WDT_ERR             = 45U,
            EHU_IDX_HIF_MEM_NON_FATAL_ERR      = 46U,
            EHU_IDX_HIF_NOC_MISSION_ERR        = 47U,
            EHU_IDX_HIF_NOC_LATENT_ERR         = 48U,
            EHU_IDX_HOM_MEM_CORR_ERR           = 49U,
            EHU_IDX_HOM_MEM_NON_FATAL_ERR      = 50U,
            EHU_IDX_HOM_MEM_FATAL_ERR          = 51U,
            EHU_IDX_IPU_IS_MEM_CORR_ERR        = 52U,
            EHU_IDX_IPU_IS_MEM_NON_FATAL_ERR   = 53U,
            EHU_IDX_IPU_IS_MEM_FATAL_ERR       = 54U,
            EHU_IDX_IPU_IS_NOC_MISSION_ERR     = 55U,
            EHU_IDX_IPU_IS_NOC_LATENT_ERR      = 56U,
            EHU_IDX_IPU_PS_MEM_CORR_ERR        = 57U,
            EHU_IDX_IPU_PS_MEM_NON_FATAL_ERR   = 58U,
            EHU_IDX_IPU_PS_MEM_FATAL_ERR       = 59U,
            EHU_IDX_IPU_PS_NOC_MISSION_ERR     = 60U,
            EHU_IDX_IPU_PS_NOC_LATENT_ERR      = 61U,
            EHU_IDX_IPU_IS_UCNT_WDT            = 62U,
            EHU_IDX_IPU_PS_UCNT_WDT            = 63U,
            EHU_IDX_IPU_IS_DPHYRX_CORR_ERR     = 64U,
            EHU_IDX_IPU_IS_DPHYRX_UNCORR_ERR   = 65U,
            EHU_IDX_IPU_IS_UC_PFATAL_ERR       = 66U,
            EHU_IDX_IPU_PS_UC_PFATAL_ERR       = 67U,
            EHU_IDX_MGCI_MEM_CORR_ERR          = 68U,
            EHU_IDX_MGCI_MEM_NON_FATAL_ERR     = 69U,
            EHU_IDX_MGCI_MEM_FATAL_ERR         = 70U,
            EHU_IDX_MGCI_NOC_MISSION_ERR       = 71U,
            EHU_IDX_MGCI_NOC_LATENT_ERR        = 72U,
            EHU_IDX_MGCI_IRQ                   = 73U,
            EHU_IDX_MGCV_MEM_CORR_ERR          = 74U,
            EHU_IDX_MGCV_MEM_NON_FATAL_ERR     = 75U,
            EHU_IDX_MGCV_MEM_FATAL_ERR         = 76U,
            EHU_IDX_MGCV_NOC_MISSION_ERR       = 77U,
            EHU_IDX_MGCV_NOC_LATENT_ERR        = 78U,
            EHU_IDX_MGCV_IRQ                   = 79U,
            EHU_IDX_HIF_MIPI_CSI_DIAG_INTER0   = 80U,
            EHU_IDX_HIF_MIPI_CSI_DIAG_INTER1   = 81U,
            EHU_IDX_OPT_ECC_CORR_INT           = 82U,
            EHU_IDX_OPT_ECC_UNCORR_INT         = 83U,
            EHU_IDX_PCIE_SS_CFG_SF_CORR        = 84U,
            EHU_IDX_PCIE_SS_CFG_SF_UNCORR      = 85U,
            EHU_IDX_PCIE_ETH_MEM_CORR_ERR      = 86U,
            EHU_IDX_PCIE_ETH_MEM_NON_FATAL_ERR = 87U,
            EHU_IDX_PCIE_ETH_MEM_FATAL_ERR     = 88U,
            EHU_IDX_PCIE_ETH_NOC_MISSION_ERR   = 89U,
            EHU_IDX_PCIE_ETH_NOC_LATENT_ERR    = 90U,
            EHU_IDX_SPARE1                     = 91U,
            EHU_IDX_CCU_PVT_INTR_1             = 92U,
            EHU_IDX_CCU_PVT_INTR_2             = 93U,
            EHU_IDX_CPU_PVT_INTR_1             = 94U,
            EHU_IDX_CPU_PVT_INTR_2             = 95U,
            EHU_IDX_HIF_PVT_INTR_1             = 96U,
            EHU_IDX_HIF_PVT_INTR_2             = 97U,
            EHU_IDX_HOM_PVT_INTR_1             = 98U,
            EHU_IDX_HOM_PVT_INTR_2             = 99U,
            EHU_IDX_IPU_PVT_INTR_1             = 100U,
            EHU_IDX_IPU_PVT_INTR_2             = 101U,
            EHU_IDX_NCL_PVT_INTR_1             = 102U,
            EHU_IDX_NCL_PVT_INTR_2             = 103U,
            EHU_IDX_PVT_NCL_TOP_0_INTR_1       = 104U,
            EHU_IDX_PVT_NCL_TOP_0_INTR_2       = 105U,
            EHU_IDX_PVT_NCL_TOP_1_INTR_1       = 106U,
            EHU_IDX_SPARE2                     = 107U,
            EHU_IDX_PVT_NCL_TOP_1_INTR_2       = 108U,
            EHU_IDX_SDIO_MEM_CORR_ERR          = 109U,
            EHU_IDX_SIO_MEM_NON_FATAL_ERR      = 110U,
            EHU_IDX_SDP_MEM_CORR_ERR           = 111U,
            EHU_IDX_SDP_MEM_NON_FATAL_ERR      = 112U,
            EHU_IDX_SDP_MEM_FATAL_ERR          = 113U,
            EHU_IDX_SDP_NOC_MISSION_ERR        = 114U,
            EHU_IDX_SDP_NOC_LATENT_ERR         = 115U,
            EHU_IDX_SDP_IRQ                    = 116U,
            EHU_IDX_USB_MEM_NON_FATAL_ERR      = 117U,
            EHU_IDX_USB_MEM_FATAL_ERR          = 118U,
            EHU_IDX_PCIE_SS_APP_PARITY_ERR     = 119U,
            EHU_IDX_OPT_BOOT_ERR               = 120U,
            EHU_IDX_OPT_PARITY_ERR             = 121U,
            EHU_IDX_EHU_LOCK                   = 122U,
            EHU_IDX_SW_OS_EXCEPTION            = 123U,
            EHU_IDX_SF_OS_EXCEPTION            = 124U,
            EHU_IDX_FLASH_DATA_CRC_ERR         = 125U,
            EHU_IDX_FRAME_DELAY_ERR            = 126U,
            EHU_IDX_DSP_UP_CHECKSUM_ERR        = 127U,
        }; 

        const std::map< int, std::string > d500_fw_error_report = { // Received from HKR team [RSDEV-643]
            { EHU_IDX_START, "NO ERROR" },
            { EHU_IDX_CAN_FD_INT1, "CAN FD INT1 ERROR" },
            { EHU_IDX_CAN_FD_MEM_CORR_ERR, "CAN FD MEM CORR ERROR" },
            { EHU_IDX_CAN_FD_NON_FATAL_ERR, "CAN FD NON FATAL ERROR" },
            { EHU_IDX_CCU_ERR, "CCU ERROR" },
            { EHU_IDX_TOP_CONFIG_NOC_MISSION_ERR, "TOP CONFIG NOC MISSION ERROR" },
            { EHU_IDX_TOP_CONFIG_NOC_LATENT_ERR, "TOP CONFIG NOC LATENT ERROR" },
            { EHU_IDX_TOP_CONFIG_NOC_WDT_ERR, "TOP CONFIG NOC WDT ERROR" },
            { EHU_IDX_CPU_WDT_INTR_0, "CPU WDT INTR 0 ERROR" },
            { EHU_IDX_CPU_WDT_INTR_1, "CPU WDT INTR 1 ERROR" },
            { EHU_IDX_CPU_WDT_INTR_2, "CPU WDT INTR 2 ERROR" },
            { EHU_IDX_CPU_WDT_INTR_3, "CPU WDT INTR 3 ERROR" },
            { EHU_IDX_CPU_WDT_SYS_RST_0, "CPU WDT SYS RST 0 ERROR" },
            { EHU_IDX_CPU_WDT_SYS_RST_1, "CPU WDT SYS RST 1 ERROR" },
            { EHU_IDX_CPU_WDT_SYS_RST_2, "CPU WDT SYS RST 2 ERROR" },
            { EHU_IDX_CPU_WDT_SYS_RST_3, "CPU WDT SYS RST 3 ERROR" },
            { EHU_IDX_CPU_MEM_CORR_ERR, "CPU MEM CORR ERROR" },
            { EHU_IDX_CPU_MEM_NON_FATAL_ERR, "CPU MEM NON FATAL ERROR" },
            { EHU_IDX_CPU_MEM_FATAL_ERR, "CPU MEM FATAL ERROR" },
            { EHU_IDX_CPU_NOC_MISSION_ERR, "CPU NOC MISSION ERROR" },
            { EHU_IDX_CPU_NOC_LATENT_ERR, "CPU NOC LATENT ERROR" },
            { EHU_IDX_CPU_EXTERRIRQ, "CPU EXTERRIRQ ERROR" },
            { EHU_IDX_CPU_INTERRIRQ, "CPU INTERRIRQ ERROR" },
            { EHU_IDX_GIC_ECC_FATAL, "GIC ECC FATAL ERROR" },
            { EHU_IDX_TOP_DATA_NOC_MISSION_ERR, "TOP DATA NOC MISSION ERROR" },
            { EHU_IDX_TOP_DATA_NOC_LATENT_ERR, "TOP DATA NOC LATENT ERROR" },
            { EHU_IDX_TOP_DATA_NOC_WDT_ERR, "TOP DATA NOC WDT ERROR" },
            { EHU_IDX_LPDDR_INTERNAL_FATAL_ERR, "LPDDR INTERNAL FATAL ERROR" },
            { EHU_IDX_LPDDR_DFI_ECC_CORR_ERR, "LPDDR DFI ECC CORR ERROR" },
            { EHU_IDX_DSP_PFATAL_ERR, "DSP PFATAL ERROR" },
            { EHU_IDX_DSP_MEM_CORR_ERR, "DSP MEM CORR ERROR" },
            { EHU_IDX_DSP_MEM_NON_FATAL_ERR, "DSP MEM NON FATAL ERROR" },
            { EHU_IDX_DSP_MEM_FATAL_ERR, "DSP MEM FATAL ERROR" },
            { EHU_IDX_EHU_WDT0_ERR, "EHU WDT0 ERROR" },
            { EHU_IDX_EHU_WDT1_ERR, "EHU WDT1 ERROR" },
            { EHU_IDX_EHU_WDT2_ERR, "EHU WDT2 ERROR" },
            { EHU_IDX_EHU_WDT3_ERR, "EHU WDT3 ERROR" },
            { EHU_IDX_EHU_WDT4_ERR, "EHU WDT4 ERROR" },
            { EHU_IDX_EHU_WDT5_ERR, "EHU WDT5 ERROR" },
            { EHU_IDX_EHU_WDT6_ERR, "EHU WDT6 ERROR" },
            { EHU_IDX_ETH_SBD_SFTY_CE_INTR, "ETH SBD SFTY CE INTR ERROR" },
            { EHU_IDX_ETH_SBD_SFTY_UE_INTR, "ETH SBD SFTY UE INTR ERROR" },
            { EHU_IDX_ENC0_MEM_NON_FATAL_ERR, "ENC0 MEM NON FATAL ERROR" },
            { EHU_IDX_ENC1_MEM_NON_FATAL_ERR, "ENC1 MEM NON FATAL ERROR" },
            { EHU_IDX_HIF_UC_PFATAL_ERR, "HIF UC PFATAL ERROR" },
            { EHU_IDX_HIF_UC_WDT_ERR, "HIF UC WDT ERROR" },
            { EHU_IDX_HIF_MEM_NON_FATAL_ERR, "HIF MEM NON FATAL ERROR" },
            { EHU_IDX_HIF_NOC_MISSION_ERR, "HIF NOC MISSION ERROR" },
            { EHU_IDX_HIF_NOC_LATENT_ERR, "HIF NOC LATENT ERROR" },
            { EHU_IDX_HOM_MEM_CORR_ERR, "HOM MEM CORR ERROR" },
            { EHU_IDX_HOM_MEM_NON_FATAL_ERR, "HOM MEM NON FATAL ERROR" },
            { EHU_IDX_HOM_MEM_FATAL_ERR, "HOM MEM FATAL ERROR" },
            { EHU_IDX_IPU_IS_MEM_CORR_ERR, "IPU IS MEM CORR ERROR" },
            { EHU_IDX_IPU_IS_MEM_NON_FATAL_ERR, "IPU IS MEM NON FATAL ERROR" },
            { EHU_IDX_IPU_IS_MEM_FATAL_ERR, "IPU IS MEM FATAL ERROR" },
            { EHU_IDX_IPU_IS_NOC_MISSION_ERR, "IPU IS NOC MISSION ERROR" },
            { EHU_IDX_IPU_IS_NOC_LATENT_ERR, "IPU IS NOC LATENT ERROR" },
            { EHU_IDX_IPU_PS_MEM_CORR_ERR, "IPU PS MEM CORR ERROR" },
            { EHU_IDX_IPU_PS_MEM_NON_FATAL_ERR, "IPU PS MEM NON FATAL ERROR" },
            { EHU_IDX_IPU_PS_MEM_FATAL_ERR, "IPU PS MEM FATAL ERROR" },
            { EHU_IDX_IPU_PS_NOC_MISSION_ERR, "IPU PS NOC MISSION ERROR" },
            { EHU_IDX_IPU_PS_NOC_LATENT_ERR, "IPU PS NOC LATENT ERROR" },
            { EHU_IDX_IPU_IS_UCNT_WDT, "IPU IS UCNT WDT ERROR" },
            { EHU_IDX_IPU_PS_UCNT_WDT, "IPU PS UCNT WDT ERROR" },
            { EHU_IDX_IPU_IS_DPHYRX_CORR_ERR, "IPU IS DPHYRX CORR ERROR" },
            { EHU_IDX_IPU_IS_DPHYRX_UNCORR_ERR, "IPU IS DPHYRX UNCORR ERROR" },
            { EHU_IDX_IPU_IS_UC_PFATAL_ERR, "IPU IS UC PFATAL ERROR" },
            { EHU_IDX_IPU_PS_UC_PFATAL_ERR, "IPU PS UC PFATAL ERROR" },
            { EHU_IDX_MGCI_MEM_CORR_ERR, "MGCI MEM CORR ERROR" },
            { EHU_IDX_MGCI_MEM_NON_FATAL_ERR, "MGCI MEM NON FATAL ERROR" },
            { EHU_IDX_MGCI_MEM_FATAL_ERR, "MGCI MEM FATAL ERROR" },
            { EHU_IDX_MGCI_NOC_MISSION_ERR, "MGCI NOC MISSION ERROR" },
            { EHU_IDX_MGCI_NOC_LATENT_ERR, "MGCI NOC LATENT ERROR" },
            { EHU_IDX_MGCI_IRQ, "MGCI IRQ ERROR" },
            { EHU_IDX_MGCV_MEM_CORR_ERR, "MGCV MEM CORR ERROR" },
            { EHU_IDX_MGCV_MEM_NON_FATAL_ERR, "MGCV MEM NON FATAL ERROR" },
            { EHU_IDX_MGCV_MEM_FATAL_ERR, "MGCV MEM FATAL ERROR" },
            { EHU_IDX_MGCV_NOC_MISSION_ERR, "MGCV NOC MISSION ERROR" },
            { EHU_IDX_MGCV_NOC_LATENT_ERR, "MGCV NOC LATENT ERROR" },
            { EHU_IDX_MGCV_IRQ, "MGCV IRQ ERROR" },
            { EHU_IDX_HIF_MIPI_CSI_DIAG_INTER0, "HIF MIPI CSI DIAG INTER0 ERROR" },
            { EHU_IDX_HIF_MIPI_CSI_DIAG_INTER1, "HIF MIPI CSI DIAG INTER1 ERROR" },
            { EHU_IDX_OPT_ECC_CORR_INT, "OPT ECC CORR INT ERROR" },
            { EHU_IDX_OPT_ECC_UNCORR_INT, "OPT ECC UNCORR INT ERROR" },
            { EHU_IDX_PCIE_SS_CFG_SF_CORR, "PCIE SS CFG SF CORR ERROR" },
            { EHU_IDX_PCIE_SS_CFG_SF_UNCORR, "PCIE SS CFG SF UNCORR ERROR" },
            { EHU_IDX_PCIE_ETH_MEM_CORR_ERR, "PCIE ETH MEM CORR ERROR" },
            { EHU_IDX_PCIE_ETH_MEM_NON_FATAL_ERR, "PCIE ETH MEM NON FATAL ERROR" },
            { EHU_IDX_PCIE_ETH_MEM_FATAL_ERR, "PCIE ETH MEM FATAL ERROR" },
            { EHU_IDX_PCIE_ETH_NOC_MISSION_ERR, "PCIE ETH NOC MISSION ERROR" },
            { EHU_IDX_PCIE_ETH_NOC_LATENT_ERR, "PCIE ETH NOC LATENT ERROR" },
            { EHU_IDX_SPARE1, "SPARE1 ERROR" },
            { EHU_IDX_CCU_PVT_INTR_1, "CCU PVT INTR 1 ERROR" },
            { EHU_IDX_CCU_PVT_INTR_2, "CCU PVT INTR 2 ERROR" },
            { EHU_IDX_CPU_PVT_INTR_1, "CPU PVT INTR 1 ERROR" },
            { EHU_IDX_CPU_PVT_INTR_2, "CPU PVT INTR 2 ERROR" },
            { EHU_IDX_HIF_PVT_INTR_1, "HIF PVT INTR 1 ERROR" },
            { EHU_IDX_HIF_PVT_INTR_2, "HIF PVT INTR 2 ERROR" },
            { EHU_IDX_HOM_PVT_INTR_1, "HOM PVT INTR 1 ERROR" },
            { EHU_IDX_HOM_PVT_INTR_2, "HOM PVT INTR 2 ERROR" },
            { EHU_IDX_IPU_PVT_INTR_1, "IPU PVT INTR 1 ERROR" },
            { EHU_IDX_IPU_PVT_INTR_2, "IPU PVT INTR 2 ERROR" },
            { EHU_IDX_NCL_PVT_INTR_1, "NCL PVT INTR 1 ERROR" },
            { EHU_IDX_NCL_PVT_INTR_2, "NCL PVT INTR 2 ERROR" },
            { EHU_IDX_PVT_NCL_TOP_0_INTR_1, "PVT NCL TOP 0 INTR 1 ERROR" },
            { EHU_IDX_PVT_NCL_TOP_0_INTR_2, "PVT NCL TOP 0 INTR 2 ERROR" },
            { EHU_IDX_PVT_NCL_TOP_1_INTR_1, "PVT NCL TOP 1 INTR 1 ERROR" },
            { EHU_IDX_SPARE2, "SPARE2 ERROR" },
            { EHU_IDX_PVT_NCL_TOP_1_INTR_2, "PVT NCL TOP 1 INTR 2 ERROR" },
            { EHU_IDX_SDIO_MEM_CORR_ERR, "SDIO MEM CORR ERROR" },
            { EHU_IDX_SIO_MEM_NON_FATAL_ERR, "SIO MEM NON FATAL ERROR" },
            { EHU_IDX_SDP_MEM_CORR_ERR, "SDP MEM CORR ERROR" },
            { EHU_IDX_SDP_MEM_NON_FATAL_ERR, "SDP MEM NON FATAL ERROR" },
            { EHU_IDX_SDP_MEM_FATAL_ERR, "SDP MEM FATAL ERROR" },
            { EHU_IDX_SDP_NOC_MISSION_ERR, "SDP NOC MISSION ERROR" },
            { EHU_IDX_SDP_NOC_LATENT_ERR, "SDP NOC LATENT ERROR" },
            { EHU_IDX_SDP_IRQ, "SDP IRQ ERROR" },
            { EHU_IDX_USB_MEM_NON_FATAL_ERR, "USB MEM NON FATAL ERROR" },
            { EHU_IDX_USB_MEM_FATAL_ERR, "USB MEM FATAL ERROR" },
            { EHU_IDX_PCIE_SS_APP_PARITY_ERR, "PCIE SS APP PARITY ERROR" },
            { EHU_IDX_OPT_BOOT_ERR, "OPT BOOT ERROR" },
            { EHU_IDX_OPT_PARITY_ERR, "OPT PARITY ERROR" },
            { EHU_IDX_EHU_LOCK, "EHU LOCK ERROR" },
            { EHU_IDX_SW_OS_EXCEPTION, "SW OS EXCEPTION ERROR" },
            { EHU_IDX_FRAME_DELAY_ERR, "FRAME DELAY ERROR" },
            { EHU_IDX_SF_OS_EXCEPTION, "SF OS EXCEPTION ERROR" },
            { EHU_IDX_FLASH_DATA_CRC_ERR, "FLASH DATA CRC ERROR" },
            { EHU_IDX_DSP_UP_CHECKSUM_ERR, "DSP UP CHECKSUM ERROR" },
        };

    } // namespace ds
} // namespace librealsense