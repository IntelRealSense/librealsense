#pragma once

#include <librealsense2/rs.hpp>
#include <string>


namespace rs2
{
    class subdevice_model;
    class option_model;

    class processing_block_model
    {
    public:
        processing_block_model( subdevice_model* owner,
            const std::string& name,
            std::shared_ptr<rs2::filter> block,
            std::function<rs2::frame( rs2::frame )> invoker,
            std::string& error_message,
            bool enabled = true );
        virtual ~processing_block_model() = default;

        const std::string& get_name() const { return _name; }

        option_model& get_option( rs2_option opt );

        rs2::frame invoke( rs2::frame f ) const { return _invoker( f ); }

        void save_to_config_file();

        std::vector<rs2_option> get_option_list()
        {
            return _block->get_supported_options();
        }

        void populate_options( const std::string& opt_base_label,
            subdevice_model* model,
            bool* options_invalidated,
            std::string& error_message );

        std::shared_ptr<rs2::filter> get_block() { return _block; }

        void enable( bool e = true )
        {
            processing_block_enable_disable( _enabled = e );
        }
        bool is_enabled() const { return _enabled; }

        bool visible = true;

        // Callback when our state changes
        // NOTE: actual may not be same as is_enabled()! The latter is this particular pb,
        // while the former takes into account global "Post-Processing"...
        virtual void processing_block_enable_disable( bool actual ) {}

    protected:
        bool _enabled = true;
        std::shared_ptr<rs2::filter> _block;
        std::map<int, option_model> options_metadata;
        std::string _name;
        std::string _full_name;
        std::function<rs2::frame( rs2::frame )> _invoker;
        subdevice_model* _owner;
    };
}
