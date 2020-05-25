//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "calibration.h"
#include "debug.h"
#include "utils.h"

namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {

    void inv(const double x[9], double y[9])
    {
        double b_x[9];
        int p1;
        int p2;
        int p3;
        double absx11;
        double absx21;
        double absx31;
        int itmp;
        memcpy(&b_x[0], &x[0], 9U * sizeof(double));
        p1 = 0;
        p2 = 3;
        p3 = 6;
        absx11 = std::abs(x[0]);
        absx21 = std::abs(x[1]);
        absx31 = std::abs(x[2]);
        if ((absx21 > absx11) && (absx21 > absx31)) {
            p1 = 3;
            p2 = 0;
            b_x[0] = x[1];
            b_x[1] = x[0];
            b_x[3] = x[4];
            b_x[4] = x[3];
            b_x[6] = x[7];
            b_x[7] = x[6];
        }
        else {
            if (absx31 > absx11) {
                p1 = 6;
                p3 = 0;
                b_x[0] = x[2];
                b_x[2] = x[0];
                b_x[3] = x[5];
                b_x[5] = x[3];
                b_x[6] = x[8];
                b_x[8] = x[6];
            }
        }
    
        b_x[1] /= b_x[0];
        b_x[2] /= b_x[0];
        b_x[4] -= b_x[1] * b_x[3];
        b_x[5] -= b_x[2] * b_x[3];
        b_x[7] -= b_x[1] * b_x[6];
        b_x[8] -= b_x[2] * b_x[6];
        if (std::abs(b_x[5]) > std::abs(b_x[4])) {
            itmp = p2;
            p2 = p3;
            p3 = itmp;
            absx11 = b_x[1];
            b_x[1] = b_x[2];
            b_x[2] = absx11;
            absx11 = b_x[4];
            b_x[4] = b_x[5];
            b_x[5] = absx11;
            absx11 = b_x[7];
            b_x[7] = b_x[8];
            b_x[8] = absx11;
        }
    
        b_x[5] /= b_x[4];
        b_x[8] -= b_x[5] * b_x[7];
        absx11 = (b_x[5] * b_x[1] - b_x[2]) / b_x[8];
        absx21 = -(b_x[1] + b_x[7] * absx11) / b_x[4];
        y[p1] = ((1.0 - b_x[3] * absx21) - b_x[6] * absx11) / b_x[0];
        y[p1 + 1] = absx21;
        y[p1 + 2] = absx11;
        absx11 = -b_x[5] / b_x[8];
        absx21 = (1.0 - b_x[7] * absx11) / b_x[4];
        y[p2] = -(b_x[3] * absx21 + b_x[6] * absx11) / b_x[0];
        y[p2 + 1] = absx21;
        y[p2 + 2] = absx11;
        absx11 = 1.0 / b_x[8];
        absx21 = -b_x[7] * absx11 / b_x[4];
        y[p3] = -(b_x[3] * absx21 + b_x[6] * absx11) / b_x[0];
        y[p3 + 1] = absx21;
        y[p3 + 2] = absx11;
    }

    void transpose(const double x[9], double y[9])
    {
        for (auto i = 0; i < 3; i++)
            for (auto j = 0; j < 3; j++)
                y[i * 3 + j] = x[j * 3 + i];
    }

    void rotate_180(const uint8_t* A, uint8_t* B, uint32_t w, uint32_t h)
    {
        int j;
        int i;

        /* ROT Summary of this function goes here */
        /*    Detailed explanation goes here */
        for (j = 0; j < w; j++) {
            for (i = 0; i < h; i++) {
                B[i + h * j] = A[(h * (w - 1 - j) - i) + h - 1];
            }
        }
    }
    const std::vector<double> interp1(const std::vector<double> ind, const std::vector<double> vals, const std::vector<double> intrp)
    {
        std::vector<double> res(intrp.size(), 0);

        for (auto i = 0; i < intrp.size(); i++)
        {
            auto value = intrp[i];
            auto it = std::lower_bound(ind.begin(), ind.end(), value);
            if (it == ind.begin())
            {
                if (*it == ind.front())
                    res[i] = ind.front();
                else
                    res[i] = std::numeric_limits<double>::max();
            }
            else if (it == ind.end())
            {
                if (*it == ind.back())
                    res[i] = ind.back();
                else
                    res[i] = std::numeric_limits<double>::max();
            }
            else
            {
                auto val1 = *(--it);
                auto ind1 = std::distance(ind.begin(), it);
                auto val2= *(++it);
                auto ind2 = std::distance(ind.begin(), it);

                auto target_val1 = vals[ind1];
                auto target_val2 = vals[ind2];

                res[i] = ((val2 - value) / (val2 - val1))*target_val1 + ((value - val1) / (val2 - val1))*target_val2;
            }
        }
        return res;
    }
}
}
}