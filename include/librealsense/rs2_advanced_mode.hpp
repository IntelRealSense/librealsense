/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

#ifndef R4XX_ADVANCED_MODE_HPP
#define R4XX_ADVANCED_MODE_HPP

#include <librealsense/rs2.hpp>
#include <librealsense/rs2_advanced_mode.h>

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

        void toggle_advanced_mode(bool enable)
        {
            rs2_error* e = nullptr;
            rs2_toggle_advanced_mode(_dev.get(), enable, &e);
            rs2::error::handle(e);
        }

        bool is_enabled() const
        {
            rs2_error* e = nullptr;
            int enabled;
            rs2_is_enabled(_dev.get(), &enabled, &e);
            rs2::error::handle(e);

            return enabled;
        }

        void set_depth_control(STDepthControlGroup& group)
        {
            rs2_error* e = nullptr;
            rs2_set_depth_control(_dev.get(), &group, &e);
            rs2::error::handle(e);
        }

        STDepthControlGroup get_depth_control(int mode = 0) const
        {
            rs2_error* e = nullptr;
            STDepthControlGroup group{};
            rs2_get_depth_control(_dev.get(), &group, mode, &e);
            rs2::error::handle(e);

            return group;
        }

        void set_rsm(STRsm& group)
        {
            rs2_error* e = nullptr;
            rs2_set_rsm(_dev.get(), &group, &e);
            rs2::error::handle(e);
        }

        STRsm get_rsm(int mode = 0) const
        {
            rs2_error* e = nullptr;
            STRsm group{};
            rs2_get_rsm(_dev.get(), &group, mode, &e);
            rs2::error::handle(e);

            return group;
        }

        void set_rau_support_vector_control(STRauSupportVectorControl& group)
        {
            rs2_error* e = nullptr;
            rs2_set_rau_support_vector_control(_dev.get(), &group, &e);
            rs2::error::handle(e);
        }

        STRauSupportVectorControl get_rau_support_vector_control(int mode = 0) const
        {
            rs2_error* e = nullptr;
            STRauSupportVectorControl group{};
            rs2_get_rau_support_vector_control(_dev.get(), &group, mode, &e);
            rs2::error::handle(e);

            return group;
        }

        void set_color_control(STColorControl& group)
        {
            rs2_error* e = nullptr;
            rs2_set_color_control(_dev.get(),  &group, &e);
            rs2::error::handle(e);
        }

        STColorControl get_color_control(int mode = 0) const
        {
            rs2_error* e = nullptr;
            STColorControl group{};
            rs2_get_color_control(_dev.get(), &group, mode, &e);
            rs2::error::handle(e);

            return group;
        }

        void set_rau_thresholds_control(STRauColorThresholdsControl& group)
        {
            rs2_error* e = nullptr;
            rs2_set_rau_thresholds_control(_dev.get(), &group, &e);
            rs2::error::handle(e);
        }

        STRauColorThresholdsControl get_rau_thresholds_control(int mode = 0) const
        {
            rs2_error* e = nullptr;
            STRauColorThresholdsControl group{};
            rs2_get_rau_thresholds_control(_dev.get(), &group, mode, &e);
            rs2::error::handle(e);

            return group;
        }

        void set_slo_color_thresholds_control(STSloColorThresholdsControl& group)
        {
            rs2_error* e = nullptr;
            rs2_set_slo_color_thresholds_control(_dev.get(), &group, &e);
            rs2::error::handle(e);
        }

        STSloColorThresholdsControl get_slo_color_thresholds_control(int mode = 0) const
        {
            rs2_error* e = nullptr;
            STSloColorThresholdsControl group{};
            rs2_get_slo_color_thresholds_control(_dev.get(), &group, mode, &e);
            rs2::error::handle(e);

            return group;
        }

        void set_slo_penalty_control(STSloPenaltyControl& group)
        {
            rs2_error* e = nullptr;
            rs2_set_slo_penalty_control(_dev.get(), &group, &e);
            rs2::error::handle(e);
        }

        STSloPenaltyControl get_slo_penalty_control(int mode = 0) const
        {
            rs2_error* e = nullptr;
            STSloPenaltyControl group{};
            rs2_get_slo_penalty_control(_dev.get(), &group, mode, &e);
            rs2::error::handle(e);

            return group;
        }

        void set_hdad(STHdad& group)
        {
            rs2_error* e = nullptr;
            rs2_set_hdad(_dev.get(), &group, &e);
            rs2::error::handle(e);
        }

        STHdad get_hdad(int mode = 0) const
        {
            rs2_error* e = nullptr;
            STHdad group{};
            rs2_get_hdad(_dev.get(), &group, mode, &e);
            rs2::error::handle(e);

            return group;
        }

        void set_color_correction(STColorCorrection& group)
        {
            rs2_error* e = nullptr;
            rs2_set_color_correction(_dev.get(), &group, &e);
            rs2::error::handle(e);
        }

        STColorCorrection get_color_correction(int mode = 0) const
        {
            rs2_error* e = nullptr;
            STColorCorrection group{};
            rs2_get_color_correction(_dev.get(), &group, mode, &e);
            rs2::error::handle(e);

            return group;
        }

        void set_depth_table(STDepthTableControl& group)
        {
            rs2_error* e = nullptr;
            rs2_set_depth_table(_dev.get(), &group, &e);
            rs2::error::handle(e);
        }

        STDepthTableControl get_depth_table(int mode = 0) const
        {
            rs2_error* e = nullptr;
            STDepthTableControl group{};
            rs2_get_depth_table(_dev.get(), &group, mode, &e);
            rs2::error::handle(e);

            return group;
        }

        void set_ae_control(STAEControl& group)
        {
            rs2_error* e = nullptr;
            rs2_set_ae_control(_dev.get(), &group, &e);
            rs2::error::handle(e);
        }

        STAEControl get_ae_control(int mode = 0) const
        {
            rs2_error* e = nullptr;
            STAEControl group{};
            rs2_get_ae_control(_dev.get(), &group, mode, &e);
            rs2::error::handle(e);

            return group;
        }

        void set_census(STCensusRadius& group)
        {
            rs2_error* e = nullptr;
            rs2_set_census(_dev.get(), &group, &e);
            rs2::error::handle(e);
        }

        STCensusRadius get_census(int mode = 0) const
        {
            rs2_error* e = nullptr;
            STCensusRadius group{};
            rs2_get_census(_dev.get(), &group, mode, &e);
            rs2::error::handle(e);

            return group;
        }
    };
}
#endif // R4XX_ADVANCED_MODE_HPP
