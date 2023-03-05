// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/h/rs_types.h>  // rs2_quaternion
#include "float4.h"

#include <cmath>    // sqrt
#include <cstring>  // memset


namespace rs2 {


struct matrix4
{
    float mat[4][4];

    operator float*() const
    {
        return (float*)&mat;
    } 

    static matrix4 identity()
    {
        matrix4 m;
        for (int i = 0; i < 4; i++)
            m.mat[i][i] = 1.f;
        return m;
    }

    matrix4()
    {
        std::memset(mat, 0, sizeof(mat));
    }

    matrix4(float vals[4][4])
    {
        std::memcpy(mat,vals,sizeof(mat));
    }

    // convert glGetFloatv output to matrix4
    //
    //   float m[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    // into
    //   rs2::matrix4 m = { {0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15} };
    //       0     1     2     3
    //       4     5     6     7
    //       8     9    10    11
    //       12   13    14    15
    matrix4(float vals[16])
    {
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                mat[i][j] = vals[i * 4 + j];
            }
        }
    }

    float& operator()(int i, int j) { return mat[i][j]; }
    const float& operator()(int i, int j) const { return mat[i][j]; }

    //init rotation matrix from quaternion
    matrix4(const rs2_quaternion& q)
    {
        mat[0][0] = 1 - 2*q.y*q.y - 2*q.z*q.z; mat[0][1] = 2*q.x*q.y - 2*q.z*q.w;     mat[0][2] = 2*q.x*q.z + 2*q.y*q.w;     mat[0][3] = 0.0f;
        mat[1][0] = 2*q.x*q.y + 2*q.z*q.w;     mat[1][1] = 1 - 2*q.x*q.x - 2*q.z*q.z; mat[1][2] = 2*q.y*q.z - 2*q.x*q.w;     mat[1][3] = 0.0f;
        mat[2][0] = 2*q.x*q.z - 2*q.y*q.w;     mat[2][1] = 2*q.y*q.z + 2*q.x*q.w;     mat[2][2] = 1 - 2*q.x*q.x - 2*q.y*q.y; mat[2][3] = 0.0f;
        mat[3][0] = 0.0f;                      mat[3][1] = 0.0f;                      mat[3][2] = 0.0f;                      mat[3][3] = 1.0f;
    }

    //init translation matrix from vector
    matrix4(const rs2_vector& t)
    {
        mat[0][0] = 1.0f; mat[0][1] = 0.0f; mat[0][2] = 0.0f; mat[0][3] = t.x;
        mat[1][0] = 0.0f; mat[1][1] = 1.0f; mat[1][2] = 0.0f; mat[1][3] = t.y;
        mat[2][0] = 0.0f; mat[2][1] = 0.0f; mat[2][2] = 1.0f; mat[2][3] = t.z;
        mat[3][0] = 0.0f; mat[3][1] = 0.0f; mat[3][2] = 0.0f; mat[3][3] = 1.0f;
    }

    rs2_quaternion normalize(rs2_quaternion a)
    {
        float norm = sqrtf(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w);
        rs2_quaternion res = a;
        res.x /= norm;
        res.y /= norm;
        res.z /= norm;
        res.w /= norm;
        return res;
    }

    rs2_quaternion to_quaternion()
    {
        float tr[4];
        rs2_quaternion res;
        tr[0] = (mat[0][0] + mat[1][1] + mat[2][2]);
        tr[1] = (mat[0][0] - mat[1][1] - mat[2][2]);
        tr[2] = (-mat[0][0] + mat[1][1] - mat[2][2]);
        tr[3] = (-mat[0][0] - mat[1][1] + mat[2][2]);
        if (tr[0] >= tr[1] && tr[0] >= tr[2] && tr[0] >= tr[3])
        {
            float s = 2 * sqrt(tr[0] + 1);
            res.w = s / 4;
            res.x = (mat[2][1] - mat[1][2]) / s;
            res.y = (mat[0][2] - mat[2][0]) / s;
            res.z = (mat[1][0] - mat[0][1]) / s;
        }
        else if (tr[1] >= tr[2] && tr[1] >= tr[3]) {
            float s = 2 * sqrt(tr[1] + 1);
            res.w = (mat[2][1] - mat[1][2]) / s;
            res.x = s / 4;
            res.y = (mat[1][0] + mat[0][1]) / s;
            res.z = (mat[2][0] + mat[0][2]) / s;
        }
        else if (tr[2] >= tr[3]) {
            float s = 2 * sqrt(tr[2] + 1);
            res.w = (mat[0][2] - mat[2][0]) / s;
            res.x = (mat[1][0] + mat[0][1]) / s;
            res.y = s / 4;
            res.z = (mat[1][2] + mat[2][1]) / s;
        }
        else {
            float s = 2 * sqrt(tr[3] + 1);
            res.w = (mat[1][0] - mat[0][1]) / s;
            res.x = (mat[0][2] + mat[2][0]) / s;
            res.y = (mat[1][2] + mat[2][1]) / s;
            res.z = s / 4;
        }
        return normalize(res);
    }

    void to_column_major(float column_major[16])
    {
        column_major[0] = mat[0][0];
        column_major[1] = mat[1][0];
        column_major[2] = mat[2][0];
        column_major[3] = mat[3][0];
        column_major[4] = mat[0][1];
        column_major[5] = mat[1][1];
        column_major[6] = mat[2][1];
        column_major[7] = mat[3][1];
        column_major[8] = mat[0][2];
        column_major[9] = mat[1][2];
        column_major[10] = mat[2][2];
        column_major[11] = mat[3][2];
        column_major[12] = mat[0][3];
        column_major[13] = mat[1][3];
        column_major[14] = mat[2][3];
        column_major[15] = mat[3][3];
    }
};

inline matrix4 operator*(const matrix4& a, const matrix4& b)
{
    matrix4 res;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++)
            {
                sum += a.mat[i][k] * b.mat[k][j];
            }
            res.mat[i][j] = sum;
        }
    }
    return res;
}

inline float4 operator*(const matrix4& a, const float4& b)
{
    float4 res;
    int i = 0;
    res.x = a(i, 0) * b.x + a(i, 1) * b.y + a(i, 2) * b.z + a(i, 3) * b.w; i++;
    res.y = a(i, 0) * b.x + a(i, 1) * b.y + a(i, 2) * b.z + a(i, 3) * b.w; i++;
    res.z = a(i, 0) * b.x + a(i, 1) * b.y + a(i, 2) * b.z + a(i, 3) * b.w; i++;
    res.w = a(i, 0) * b.x + a(i, 1) * b.y + a(i, 2) * b.z + a(i, 3) * b.w; i++;
    return res;
}


inline matrix4 identity_matrix()
{
    matrix4 data;
    for( int i = 0; i < 4; i++ )
        for( int j = 0; j < 4; j++ )
            data.mat[i][j] = (i == j) ? 1.f : 0.f;
    return data;
}


}  // namespace rs2