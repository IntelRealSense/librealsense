#include "advanced-c.h"
#include "../include/r400_advanced_mode/r400_advanced_mode.hpp"
#include "../../include/librealsense/rs2.hpp"

using namespace r400;

template<class T>
int do_advanced_mode(rs2_device* dev, T action)
{
    auto send_receive = [dev](const std::vector<uint8_t>& input)
    {
        rs2_error* e = nullptr;
        auto res = rs2_send_and_receive_raw_data(dev, (void*)input.data(), input.size(), &e);
        if (e)
        {
            rs2_free_error(e);
            throw std::runtime_error("Advanced mode write failed!");
        }
        auto size = rs2_get_raw_data_size(res, &e);
        std::vector<uint8_t> buff(size, 0);
        if (e)
        {
            rs2_free_error(e);
            throw std::runtime_error("Advanced mode write failed!");
        }
        auto ptr = rs2_get_raw_data(res, &e);
        std::copy(ptr, ptr + size, buff.data());
        rs2_delete_raw_data(res);
        return buff;
    };

    advanced_mode advanced(send_receive);
    try
    {
        action(advanced);
        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

int set_depth_control(rs2_device* dev, STDepthControlGroup* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.set(*group); });
}

int get_depth_control(rs2_device* dev, STDepthControlGroup* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.get(group); });
}

int set_rsm(rs2_device* dev, STRsm* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.set(*group); });
}

int get_rsm(rs2_device* dev, STRsm* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.get(group); });
}

int set_rau_support_vector_control(rs2_device* dev, STRauSupportVectorControl* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.set(*group); });
}

int get_rau_support_vector_control(rs2_device* dev, STRauSupportVectorControl* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.get(group); });
}

int set_color_control(rs2_device* dev, STColorControl* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.set(*group); });
}

int get_color_control(rs2_device* dev, STColorControl* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.get(group); });
}

int set_rau_thresholds_control(rs2_device* dev, STRauColorThresholdsControl* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.set(*group); });
}

int get_rau_thresholds_control(rs2_device* dev, STRauColorThresholdsControl* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.get(group); });
}

int set_slo_color_thresholds_control(rs2_device* dev, STSloColorThresholdsControl* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.set(*group); });
}

int get_slo_color_thresholds_control(rs2_device* dev, STSloColorThresholdsControl* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.get(group); });
}

int set_slo_penalty_control(rs2_device* dev, STSloPenaltyControl* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.set(*group); });
}

int get_slo_penalty_control(rs2_device* dev, STSloPenaltyControl* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.get(group); });
}

int set_hdad(rs2_device* dev, STHdad* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.set(*group); });
}

int get_hdad(rs2_device* dev, STHdad* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.get(group); });
}

int set_color_correction(rs2_device* dev, STColorCorrection* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.set(*group); });
}

int get_color_correction(rs2_device* dev, STColorCorrection* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.get(group); });
}

int set_depth_table(rs2_device* dev, STDepthTableControl* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.set(*group); });
}

int get_depth_table(rs2_device* dev, STDepthTableControl* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.get(group); });
}

int set_ae_control(rs2_device* dev, STAEControl* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.set(*group); });
}

int get_ae_control(rs2_device* dev, STAEControl* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.get(group); });
}

int set_census(rs2_device* dev, STCensusRadius* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.set(*group); });
}

int get_census(rs2_device* dev, STCensusRadius* group)
{
    return do_advanced_mode(dev, [=](advanced_mode& advanced) { advanced.get(group); });
}
