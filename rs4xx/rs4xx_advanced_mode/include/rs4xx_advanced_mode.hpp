/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#ifndef R4XX_ADVANCED_MODE_HPP
#define R4XX_ADVANCED_MODE_HPP

#include <librealsense/rs2.hpp>
#include "rs4xx_advanced_mode.h"

namespace rs4xx
{
    class advanced_mode : public rs2::device
    {
    public:
        advanced_mode(rs2::device d)
                : rs2::device(d.get())
        {
            rs2_error* e = nullptr;
            if(rs2_is_device(_dev.get(), RS2_EXTENSION_TYPE_ADVANCED_MODE, &e) == 0 && !e)
            {
                _dev = nullptr;
            }
            rs2::error::handle(e);
        }

        void go_to_advanced_mode()
        {
            if (rs2_go_to_advanced_mode(_dev.get()))
                throw std::runtime_error("go_to_advanced_mode failed!");
        }

        bool is_enabled()
        {
            int enabled;
            if (rs2_is_enabled(_dev.get(), &enabled))
                throw std::runtime_error("is_enabled failed!");

            return enabled;
        }

        void set_depth_control(STDepthControlGroup& group, int mode = 0) const
        {
            if (rs2_set_depth_control(_dev.get(), &group, mode))
                    throw std::runtime_error("set_depth_control failed!");
        }

        STDepthControlGroup get_depth_control(int mode = 0) const
        {
            STDepthControlGroup group{};
            if (rs2_get_depth_control(_dev.get(), &group, mode))
                throw std::runtime_error("get_depth_control failed!");

            return group;
        }

        void set_rsm(STRsm& group, int mode = 0) const
        {
            if (rs2_set_rsm(_dev.get(), &group, mode))
                    throw std::runtime_error("set_rsm failed!");
        }

        STRsm get_rsm(int mode = 0) const
        {
            STRsm group{};
            if (rs2_get_rsm(_dev.get(), &group, mode))
                throw std::runtime_error("get_rsm failed!");

            return group;
        }

        void set_rau_support_vector_control(STRauSupportVectorControl& group, int mode = 0) const
        {
            if (rs2_set_rau_support_vector_control(_dev.get(), &group, mode))
                    throw std::runtime_error("set_rau_support_vector_control failed!");
        }

        STRauSupportVectorControl get_rau_support_vector_control(int mode = 0) const
        {
            STRauSupportVectorControl group{};
            if (rs2_get_rau_support_vector_control(_dev.get(), &group, mode))
                throw std::runtime_error("get_rau_support_vector_control failed!");

            return group;
        }

        void set_color_control(STColorControl& group, int mode = 0) const
        {
            if (rs2_set_color_control(_dev.get(),  &group, mode))
                    throw std::runtime_error("set_color_control failed!");
        }

        STColorControl get_color_control(int mode = 0) const
        {
            STColorControl group{};
            if (rs2_get_color_control(_dev.get(), &group, mode))
                throw std::runtime_error("get_color_control failed!");

            return group;
        }

        void set_rau_thresholds_control(STRauColorThresholdsControl& group, int mode = 0) const
        {
            if (rs2_set_rau_thresholds_control(_dev.get(), &group, mode))
                    throw std::runtime_error("set_rau_thresholds_control failed!");
        }

        STRauColorThresholdsControl get_rau_thresholds_control(int mode = 0) const
        {
            STRauColorThresholdsControl group{};
            if (rs2_get_rau_thresholds_control(_dev.get(), &group, mode))
                throw std::runtime_error("get_rau_thresholds_control failed!");

            return group;
        }

        void set_slo_color_thresholds_control(STSloColorThresholdsControl& group, int mode = 0) const
        {
            if (rs2_set_slo_color_thresholds_control(_dev.get(), &group, mode))
                    throw std::runtime_error("set_slo_color_thresholds_control failed!");
        }

        STSloColorThresholdsControl get_slo_color_thresholds_control(int mode = 0) const
        {
            STSloColorThresholdsControl group{};
            if (rs2_get_slo_color_thresholds_control(_dev.get(), &group, mode))
                throw std::runtime_error("get_slo_color_thresholds_control failed!");

            return group;
        }

        void set_slo_penalty_control(STSloPenaltyControl& group, int mode = 0) const
        {
            if (rs2_set_slo_penalty_control(_dev.get(), &group, mode))
                    throw std::runtime_error("set_slo_penalty_control failed!");
        }

        STSloPenaltyControl get_slo_penalty_control(int mode = 0) const
        {
            STSloPenaltyControl group{};
            if (rs2_get_slo_penalty_control(_dev.get(), &group, mode))
                throw std::runtime_error("get_slo_penalty_control failed!");

            return group;
        }

        void set_hdad(STHdad& group, int mode = 0) const
        {
            if (rs2_set_hdad(_dev.get(), &group, mode))
                    throw std::runtime_error("set_hdad failed!");
        }

        STHdad get_hdad(int mode = 0) const
        {
            STHdad group{};
            if (rs2_get_hdad(_dev.get(), &group, mode))
                throw std::runtime_error("get_hdad failed!");

            return group;
        }

        void set_color_correction(STColorCorrection& group, int mode = 0) const
        {
            if (rs2_set_color_correction(_dev.get(), &group, mode))
                    throw std::runtime_error("set_color_correction failed!");
        }

        STColorCorrection get_color_correction(int mode = 0) const
        {
            STColorCorrection group{};
            if (rs2_get_color_correction(_dev.get(), &group, mode))
                throw std::runtime_error("get_color_correction failed!");

            return group;
        }

        void set_depth_table(STDepthTableControl& group, int mode = 0) const
        {
            if (rs2_set_depth_table(_dev.get(), &group, mode))
                    throw std::runtime_error("set_depth_table failed!");
        }

        STDepthTableControl get_depth_table(int mode = 0) const
        {
            STDepthTableControl group{};
            if (rs2_get_depth_table(_dev.get(), &group, mode))
                throw std::runtime_error("get_depth_table failed!");

            return group;
        }

        void set_ae_control(STAEControl& group, int mode = 0) const
        {
            if (rs2_set_ae_control(_dev.get(), &group, mode))
                    throw std::runtime_error("set_ae_control failed!");
        }

        STAEControl get_ae_control(int mode = 0) const
        {
            STAEControl group{};
            if (rs2_get_ae_control(_dev.get(), &group, mode))
                throw std::runtime_error("get_ae_control failed!");

            return group;
        }

        void set_census(STCensusRadius& group, int mode = 0) const
        {
            if (rs2_set_census(_dev.get(), &group, mode))
                    throw std::runtime_error("set_census failed!");
        }

        STCensusRadius get_census(int mode = 0) const
        {
            STCensusRadius group{};
            if (rs2_get_census(_dev.get(), &group, mode))
                throw std::runtime_error("get_census failed!");

            return group;
        }
    };
}
#endif // R4XX_ADVANCED_MODE_HPP
