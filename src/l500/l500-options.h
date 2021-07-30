// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once
#include "../hw-monitor.h"
#include "l500-device.h"


namespace librealsense
{
    enum l500_control
    {
        confidence = 0,
        post_processing_sharpness = 1,
        pre_processing_sharpness = 2,
        noise_filtering = 3,
        apd = 4,
        laser_gain = 5,
        min_distance = 6,
        invalidation_bypass = 7,
        alternate_ir = 8
    };

    enum l500_command
    {
        get_current = 0,
        get_min = 1,
        get_max = 2,
        get_step = 3,
        get_default = 4
    };

    class l500_options;

    class digital_gain_option : public uvc_xu_option< int >
    {
        typedef uvc_xu_option< int > super;

    public:
        digital_gain_option( uvc_sensor & ep,
                             platform::extension_unit xu,
                             uint8_t id,
                             std::string description,
                             const std::map< float, std::string > & description_per_value,
                             firmware_version fw_version,
                             l500_options * owner )
            : super( ep, xu, id, description, description_per_value )
            , _fw_version( fw_version )
            , _owner( owner )
        {
        }
        void set( float value ) override;
        void set_by_preset( float value );
        option_range get_range() const override;

    private:
        void work_around_for_old_fw();

        firmware_version _fw_version;
        l500_options * _owner;
    };

    class l500_hw_options : public option
    {
    public:
        float query() const override;

        void set(float value) override;

        option_range get_range() const override;

        bool is_enabled() const override { return true; }

        const char * get_description() const override { return _description.c_str(); }

        void enable_recording(std::function<void(const option&)> recording_action) override;

        l500_hw_options( l500_device* l500_dev,
                         hw_monitor* hw_monitor,
                         l500_control type,
                         option * resolution,
                         const std::string & description,
                         firmware_version fw_version,
                         std::shared_ptr< digital_gain_option > digital_gain);

        void set_default( float def );

        // on new FW version query default can fail when we are on 'auto gain' mode
        float query_new_fw_default( bool & success ) const;
        float query_old_fw_default() const;

        float query_current( rs2_sensor_mode mode ) const;

        bool is_read_only() const override { return _is_read_only; }
        void set_read_only( bool read_only );
        void set_manually( bool set );

    private:
        float query_default( bool & success ) const;
        

        l500_control _type;
        l500_device* _l500_dev;
        hw_monitor* _hw_monitor;
        option_range _range;
        uint32_t _width;
        uint32_t _height;
        option* _resolution;
        std::string _description;
        firmware_version _fw_version;
        std::shared_ptr< digital_gain_option > _digital_gain;
        bool _is_read_only;
        bool _was_set_manually;
    };

    class max_usable_range_option : public bool_option
    {
    public:
        max_usable_range_option(l500_device *l500_depth_dev) : bool_option( false ), _l500_depth_dev(l500_depth_dev){};

        void set(float value) override;

        const char * get_description() const override;

    private:
        l500_device *_l500_depth_dev;
    };

    class sensor_mode_option
        : public float_option_with_description< rs2_sensor_mode >
        , public observable_option
    {
    public:
        sensor_mode_option(l500_device *l500_depth_dev, sensor_base* depth_ep, option_range range, std::string description) : float_option_with_description<rs2_sensor_mode>(range, description), _l500_depth_dev(l500_depth_dev),_sensor(depth_ep) {};
        void set(float value) override;
        bool is_read_only() const override
        {
            return _sensor && _sensor->is_opened();
        }

    private:
        l500_device *_l500_depth_dev;
        sensor_base* _sensor;
    };

    class ir_reflectivity_option : public bool_option
    {
    public:
        ir_reflectivity_option(l500_device *l500_depth_dev) : bool_option(false), _l500_depth_dev(l500_depth_dev), _max_usable_range_forced_on(false){};

        void set(float value) override;

        const char * get_description() const override;

    private:
        l500_device *_l500_depth_dev;
        bool _max_usable_range_forced_on;
    };

    class l500_options;

    class l500_preset_option : public float_option_with_description< rs2_l500_visual_preset >
    {
    public:
        l500_preset_option( option_range range, std::string description, l500_options * owner );
        void set( float value ) override;
        void set_value( float value );

    private:
        void verify_max_usable_range_restrictions( rs2_option opt, float value );
        l500_options * _owner;
    };

    class l500_options : public virtual l500_device
    {
    public:
        l500_options(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        std::vector<rs2_option> get_advanced_controls();
        void change_preset( rs2_l500_visual_preset preset );
        void set_preset_value( rs2_l500_visual_preset preset );
        void reset_hw_controls();
        rs2_l500_visual_preset calc_preset_from_controls() const;
        void update_defaults();
        void move_to_custom();
    private:
        void on_set_option(rs2_option opt, float value);
        
        void set_preset_controls_to_defaults();

        
        void set_max_laser();

        void change_gain( rs2_l500_visual_preset preset );
        void change_alt_ir( rs2_l500_visual_preset preset );
        void change_laser_power( rs2_l500_visual_preset preset );
        std::map<rs2_option, std::shared_ptr<cascade_option<l500_hw_options>>> _hw_options;
        std::shared_ptr< digital_gain_option > _digital_gain;
        std::shared_ptr< l500_hw_options > _alt_ir;
        std::shared_ptr< l500_preset_option > _preset;

        template<typename T, class ... Args>
        std::shared_ptr<cascade_option<T>> register_option(rs2_option opt, Args... args)
        {
            auto& depth_sensor = get_synthetic_depth_sensor();

            auto signaled_opt = std::make_shared <cascade_option<T>>(std::forward<Args>(args)...);
            signaled_opt->add_observer([opt, this](float val) {on_set_option(opt, val);});
            depth_sensor.register_option(opt, std::dynamic_pointer_cast<option>(signaled_opt));

            return signaled_opt;
        }
    };

} // namespace librealsense
