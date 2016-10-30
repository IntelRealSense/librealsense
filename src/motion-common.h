#pragma once
#ifndef LIBREALSENSE_MOTION_COMMON_H
#define LIBREALSENSE_MOTION_COMMON_H


namespace rsimpl
{
 struct IMU_version
    {
        byte ver[4];
        byte size;
        byte CRC32[4];
    };
       
#pragma pack(push, 1)
    struct serial_number
    {
        IMU_version ver;
        byte MM_s_n[6];
        byte DS4_s_n[6];
        byte reserved[235];
    };

    
#pragma pack(pop)
    struct fisheye_intrinsic 
    {
        IMU_version ver;
        float kf[9];
        float distf[5];
        byte reserved[191];
        
        int get_data_size() const
        {
            return sizeof(kf) + sizeof(distf);
        };

        bool has_data() const
        {
            return check_not_all_zeros({(byte*)&kf, ((byte*)&kf) + get_data_size()});
        }

        operator rs_intrinsics () const
        {
            return{ 640, 480, kf[2], kf[5], kf[0], kf[4], RS_DISTORTION_FTHETA, { distf[0], 0, 0, 0, 0 } };
        }
    };


    struct mm_extrinsic
    {
        float rotation[9];
        float translation[3];

        operator rs_extrinsics () const {
            return{ {   rotation[0], rotation[1], rotation[2],
                        rotation[3], rotation[4], rotation[5],
                        rotation[6], rotation[7], rotation[8] },
                      { translation[0], translation[1], translation[2] } };
        }

        int get_data_size() const
        {
            return sizeof(rotation) + sizeof(translation);
        }

        bool has_data() const
        {
            return check_not_all_zeros({ (byte*)&rotation, ((byte*)&rotation) + get_data_size() });
        }
    };
    struct IMU_extrinsic
    {
        IMU_version ver;
        mm_extrinsic  fe_to_imu;
        mm_extrinsic  fe_to_depth;
        mm_extrinsic  rgb_to_imu;
        mm_extrinsic  depth_to_imu;
        byte reserved[55];

        int get_data_size() const
        {
            return sizeof(fe_to_imu) + sizeof(fe_to_depth) + sizeof(rgb_to_imu) + sizeof(depth_to_imu);
        };
    };

    struct MM_intrinsics
    {
        //  Scale X        cross axis        cross axis      Bias X
        //  cross axis     Scale Y           cross axis      Bias Y
        //  cross axis     cross axis        Scale Z         Bias Z

        float val[3][4];
    };

  
    struct IMU_intrinsic
    {
        IMU_version ver;
        MM_intrinsics acc_intrinsic;
        MM_intrinsics gyro_intrinsic;
        float acc_bias_variance[3];
        float acc_noise_variance[3];
        float gyro_bias_variance[3];
        float gyro_noise_variance[3];
        byte reserved[103];

        int get_data_size() const
        {
            return sizeof(acc_intrinsic) + sizeof(gyro_intrinsic) + 
                sizeof(acc_bias_variance) + sizeof(acc_noise_variance) + sizeof(gyro_bias_variance) + sizeof(gyro_noise_variance);
        };

        bool has_data() const
        {
            return check_not_all_zeros({(byte*)&acc_intrinsic, ((byte*)&acc_intrinsic) + get_data_size()});
        }
       
        rs_motion_device_intrinsic convert(const MM_intrinsics& intrin, const float bias_variance[3], const float noises_variance[3]) const
        {
            rs_motion_device_intrinsic res;

            for (auto i = 0; i < 3; i++)
            {
                for (auto j = 0; j < 4; j++)
                {
                    res.data[i][j] = intrin.val[i][j];
                }
            }

            for (auto i = 0; i < 3; i++)
            {
                res.noise_variances[i] = noises_variance[i];
                res.bias_variances[i] = bias_variance[i];
            }
            return res;
        }
        rs_motion_device_intrinsic get_acc_intrinsic() const
        {
            return convert(acc_intrinsic, acc_bias_variance, acc_noise_variance);
        }

        rs_motion_device_intrinsic get_gyro_intrinsic() const
        {
            return convert(gyro_intrinsic, gyro_bias_variance, gyro_noise_variance);
        }
       
        operator rs_motion_intrinsics() const{

            return{ get_acc_intrinsic(), get_gyro_intrinsic() };
        }
    };

    struct calibration
    {
        fisheye_intrinsic fe_intrinsic;
        IMU_extrinsic mm_extrinsic;
        IMU_intrinsic imu_intrinsic;
    };
    struct motion_module_calibration
    {
        serial_number sn;
        calibration calib;
    };
}
#endif
