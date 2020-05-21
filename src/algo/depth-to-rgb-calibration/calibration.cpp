//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "calibration.h"
#include "debug.h"

using namespace librealsense::algo::depth_to_rgb_calibration;


calib::calib( rs2_intrinsics_double const & intrin, rs2_extrinsics_double const & extrin )
{
    auto const & r = extrin.rotation;
    auto const & t = extrin.translation;
    auto const & c = intrin.coeffs;

    height = intrin.height;
    width = intrin.width;
    rot = { r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8] };
    trans = { t[0], t[1], t[2] };
    k_mat = { intrin.fx, intrin.fy, intrin.ppx, intrin.ppy };
    coeffs[0] = c[0];
    coeffs[1] = c[1];
    coeffs[2] = c[2];
    coeffs[3] = c[3];
    coeffs[4] = c[4];
    model = intrin.model;
}

calib::calib( rs2_intrinsics const & intrin, rs2_extrinsics const & extrin )
{
    auto const & r = extrin.rotation;
    auto const & t = extrin.translation;
    auto const & c = intrin.coeffs;

    height = intrin.height;
    width = intrin.width;
    rot = { r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8] };
    trans = { t[0], t[1], t[2] };
    k_mat = { intrin.fx, intrin.fy, intrin.ppx, intrin.ppy };
    coeffs[0] = c[0];
    coeffs[1] = c[1];
    coeffs[2] = c[2];
    coeffs[3] = c[3];
    coeffs[4] = c[4];
    model = intrin.model;
}

rs2_intrinsics_double calib::get_intrinsics() const
{
    return {
        width, height,
        k_mat,
        model, coeffs };
}

rs2_extrinsics_double calib::get_extrinsics() const
{
    auto & r = rot.rot;
    auto & t = trans;
    return {
        { r[0] , r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8] },
        { t.t1, t.t2, t.t3 }
    };
}

p_matrix const calib::calc_p_mat() const
{
    auto r = rot.rot;
    auto t = trans;
    auto fx = k_mat.fx;
    auto fy = k_mat.fy;
    auto ppx = k_mat.ppx;
    auto ppy = k_mat.ppy;
    p_matrix p_mat = { fx* r[0] + ppx * r[6], fx* r[1] + ppx * r[7], fx* r[2] + ppx * r[8], fx* t.t1 + ppx * t.t3,
              fy* r[3] + ppy * r[6], fy* r[4] + ppy * r[7], fy* r[5] + ppy * r[8], fy* t.t2 + ppy * t.t3,
              r[6]                 , r[7]                 , r[8]                 , t.t3 };

    return p_mat;
}

void calib::copy_coefs( calib & obj ) const
{
    obj.width = this->width;
    obj.height = this->height;

    obj.coeffs[0] = this->coeffs[0];
    obj.coeffs[1] = this->coeffs[1];
    obj.coeffs[2] = this->coeffs[2];
    obj.coeffs[3] = this->coeffs[3];
    obj.coeffs[4] = this->coeffs[4];

    obj.model = this->model;
}

calib calib::operator*( double step_size ) const
{
    calib res;
    res.k_mat.fx = this->k_mat.fx * step_size;
    res.k_mat.fy = this->k_mat.fy * step_size;
    res.k_mat.ppx = this->k_mat.ppx * step_size;
    res.k_mat.ppy = this->k_mat.ppy *step_size;

    for (auto i = 0; i < 9; i++)
        res.rot.rot[i] = this->rot.rot[i] * step_size;

    res.trans.t1 = this->trans.t1 *step_size;
    res.trans.t2 = this->trans.t2 * step_size;
    res.trans.t3 = this->trans.t3 *step_size;

    copy_coefs( res );

    return res;
}

calib calib::operator/( double factor ) const
{
    return (*this)*(1.f / factor);
}

calib calib::operator+( const calib & c ) const
{
    calib res;
    res.k_mat.fx = this->k_mat.fx + c.k_mat.fx;
    res.k_mat.fy = this->k_mat.fy + c.k_mat.fy;
    res.k_mat.ppx = this->k_mat.ppx + c.k_mat.ppx;
    res.k_mat.ppy = this->k_mat.ppy + c.k_mat.ppy;

    for (auto i = 0; i < 9; i++)
        res.rot.rot[i] = this->rot.rot[i] + c.rot.rot[i];

    res.trans.t1 = this->trans.t1 + c.trans.t1;
    res.trans.t2 = this->trans.t2 + c.trans.t2;
    res.trans.t3 = this->trans.t3 + c.trans.t3;

    copy_coefs( res );

    return res;
}

calib calib::operator-( const calib & c ) const
{
    calib res;
    res.k_mat.fx = this->k_mat.fx - c.k_mat.fx;
    res.k_mat.fy = this->k_mat.fy - c.k_mat.fy;
    res.k_mat.ppx = this->k_mat.ppx - c.k_mat.ppx;
    res.k_mat.ppy = this->k_mat.ppy - c.k_mat.ppy;

    for (auto i = 0; i < 9; i++)
        res.rot.rot[i] = this->rot.rot[i] - c.rot.rot[i];

    res.trans.t1 = this->trans.t1 - c.trans.t1;
    res.trans.t2 = this->trans.t2 - c.trans.t2;
    res.trans.t3 = this->trans.t3 - c.trans.t3;

    copy_coefs( res );

    return res;
}

calib calib::operator/( const calib & c ) const
{
    calib res;
    res.k_mat.fx = this->k_mat.fx / c.k_mat.fx;
    res.k_mat.fy = this->k_mat.fy / c.k_mat.fy;
    res.k_mat.ppx = this->k_mat.ppx / c.k_mat.ppx;
    res.k_mat.ppy = this->k_mat.ppy / c.k_mat.ppy;

    for (auto i = 0; i < 9; i++)
        res.rot.rot[i] = this->rot.rot[i] / c.rot.rot[i];


    res.trans.t1 = this->trans.t1 / c.trans.t1;
    res.trans.t2 = this->trans.t2 / c.trans.t2;
    res.trans.t3 = this->trans.t3 / c.trans.t3;

    copy_coefs( res );

    return res;
}


p_matrix p_matrix::operator*(double step_size) const
{
    p_matrix res;

    for (auto i = 0; i < 12; i++)
        res.vals[i] = vals[i] * step_size;

    return res;
}

p_matrix p_matrix::operator/(double factor) const
{
    return (*this)*(1.f / factor);
}

p_matrix p_matrix::operator+(const p_matrix & c) const
{
    p_matrix res;
    for (auto i = 0; i < 12; i++)
        res.vals[i] = vals[i] + c.vals[i];

    return res;
}

p_matrix p_matrix::operator-(const p_matrix & c) const
{
    p_matrix res;
    for (auto i = 0; i < 12; i++)
        res.vals[i] = vals[i] - c.vals[i];

    return res;
}

p_matrix p_matrix::operator*(const p_matrix & c) const
{
    p_matrix res;
    for (auto i = 0; i < 12; i++)
        res.vals[i] = vals[i] * c.vals[i];

    return res;
}

p_matrix p_matrix::operator/(const p_matrix & c) const
{
    p_matrix res;
    for (auto i = 0; i < 12; i++)
        res.vals[i] = c.vals[i] == 0 ? 0 : vals[i] / c.vals[i];

    return res;
}

double p_matrix::get_norma()
{
    double grads_norm = 0;  // TODO meant to have as float??

    for (auto i = 0; i < 12; i++)
        grads_norm += vals[i] * vals[i];

    grads_norm = sqrt(grads_norm);

    return grads_norm;
}

double p_matrix::sum()
{
    double res = 0;  // TODO meant to have float??

    for (auto i = 0; i <12; i++)
    {
        res += vals[i];
    }

    return res;
}

p_matrix p_matrix::normalize()
{
    p_matrix res;

    auto norma = get_norma();

    for (auto i = 0; i < 12; i++)
    {
        res.vals[i] = vals[i] / norma;
    }
    return res;
}

calib librealsense::algo::depth_to_rgb_calibration::decompose(p_matrix mat)
{
    return calib();
}
