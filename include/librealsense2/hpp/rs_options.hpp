// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_OPTIONS_HPP
#define LIBREALSENSE_RS2_OPTIONS_HPP

#include "rs_types.hpp"
#include "../h/rs_types.h"

#include <memory>


namespace rs2
{
    class option_value
    {
        std::shared_ptr< const rs2_option_value > _value;

    public:
        explicit option_value( rs2_option_value const * handle )
            : _value( handle, rs2_delete_option_value )
        {
        }
        option_value( option_value const & ) = default;
        option_value( option_value && ) = default;
        option_value() = default;

        enum invalid_t { invalid };
        option_value( rs2_option option_id, invalid_t )
            : _value( new rs2_option_value{ option_id, false, RS2_OPTION_TYPE_COUNT } ) {}

        option_value( rs2_option option_id, int64_t as_integer )
            : _value( new rs2_option_value{ option_id, true, RS2_OPTION_TYPE_INTEGER } )
        {
            const_cast< rs2_option_value * >( _value.get() )->as_integer = as_integer;
        }
        option_value( rs2_option option_id, float as_float )
            : _value( new rs2_option_value{ option_id, true, RS2_OPTION_TYPE_FLOAT } )
        {
            const_cast< rs2_option_value * >( _value.get() )->as_float = as_float;
        }
        option_value( rs2_option option_id, char const * as_string )
            : _value( new rs2_option_value{ option_id, true, RS2_OPTION_TYPE_STRING } )
        {
            const_cast< rs2_option_value * >( _value.get() )->as_string = as_string;
        }
        option_value( rs2_option option_id, bool as_boolean )
            : _value( new rs2_option_value{ option_id, true, RS2_OPTION_TYPE_BOOLEAN } )
        {
            const_cast<rs2_option_value *>(_value.get())->as_integer = as_boolean;
        }

        option_value & operator=( option_value const & ) = default;
        option_value & operator=( option_value && ) = default;

        rs2_option_value const * operator->() const { return _value.get(); }
        operator rs2_option_value const *() const { return _value.get(); }
    };

    class options_list
    {
    public:
        options_list( options_list const & ) = default;
        options_list( options_list && ) = default;

        explicit options_list( std::shared_ptr< rs2_options_list > list )
            : _list( std::move( list ) )
        {
            rs2_error * e = nullptr;
            _size = rs2_get_options_list_size( _list.get(), &e );
            error::handle( e );
        }

        options_list()
            : _list( nullptr )
            , _size( 0 )
        {
        }

        option_value operator[]( size_t index ) const
        {
            rs2_error * e = nullptr;
            auto value = rs2_get_option_value_from_list( _list.get(), static_cast< int >( index ), &e );
            error::handle( e );
            return option_value( value );
        }

        size_t size() const { return _size; }

        option_value front() const { return ( *this )[0]; }
        option_value back() const { return ( *this )[size() - 1]; }

        class iterator
        {
            iterator( const options_list & list, size_t index )
                : _list( list )
                , _index( index )
            {
            }

        public:
            option_value operator*() const { return _list[_index]; }
            
            bool operator!=( const iterator & other ) const
            {
                return other._index != _index || &other._list != &_list;
            }
            bool operator==( const iterator & other ) const
            {
                return ! ( *this != other );
            }

            iterator & operator++()
            {
                _index++;
                return *this;
            }

        private:
            friend options_list;
            const options_list & _list;
            size_t _index;
        };

        iterator begin() const { return iterator( *this, 0 ); }
        iterator end() const { return iterator( *this, size() ); }

        std::shared_ptr< rs2_options_list > get() const { return _list; };

    private:
        std::shared_ptr< rs2_options_list > _list;
        size_t _size;
    };
    
    class options_changed_callback : public rs2_options_changed_callback
    {
        std::function< void( const options_list & ) > _callback;

    public:
        explicit options_changed_callback( const std::function< void( const options_list & ) > & callback )
            : _callback( callback )
        {
        }

        void on_value_changed( rs2_options_list * list ) override
        {
            std::shared_ptr< rs2_options_list > sptr( list, rs2_delete_options_list );
            options_list opt_list( sptr );
            _callback( opt_list );
        }

        void release() override { delete this; }
    };

    class options
    {
    public:
        /**
        * check if particular option is supported
        * \param[in] option     option id to be checked
        * \return true if option is supported
        */
        bool supports(rs2_option option) const
        {
            rs2_error* e = nullptr;
            auto res = rs2_supports_option(_options, option, &e);
            error::handle(e);
            return res > 0;
        }

        /**
        * get option description
        * \param[in] option     option id to be checked
        * \return human-readable option description
        */
        const char* get_option_description(rs2_option option) const
        {
            rs2_error* e = nullptr;
            auto res = rs2_get_option_description(_options, option, &e);
            error::handle(e);
            return res;
        }

        /**
        * get option name
        * \param[in] option     option id to be checked
        * \return human-readable option name
        */
        const char* get_option_name(rs2_option option) const
        {
            rs2_error* e = nullptr;
            auto res = rs2_get_option_name(_options, option, &e);
            error::handle(e);
            return res;
        }

        /**
        * get option value description (in case specific option value hold special meaning)
        * \param[in] option     option id to be checked
        * \param[in] val      value of the option
        * \return human-readable description of a specific value of an option or null if no special meaning
        */
        const char* get_option_value_description(rs2_option option, float val) const
        {
            rs2_error* e = nullptr;
            auto res = rs2_get_option_value_description(_options, option, val, &e);
            error::handle(e);
            return res;
        }

        /**
        * read option's float value
        * \param[in] option   option id to be queried
        * \return value of the option
        */
        float get_option(rs2_option option) const
        {
            rs2_error* e = nullptr;
            auto res = rs2_get_option(_options, option, &e);
            error::handle(e);
            return res;
        }

        /**
        * read option's value
        * \param[in] option_id   option id to be queried
        * \return                option value
        */
        option_value get_option_value( rs2_option option_id ) const
        {
            rs2_error * e = nullptr;
            auto value = rs2_get_option_value( _options, option_id, &e );
            error::handle( e );
            return option_value( value );
        }

        /**
        * retrieve the available range of values of a supported option
        * \return option  range containing minimum and maximum values, step and default value
        */
        option_range get_option_range(rs2_option option) const
        {
            option_range result;
            rs2_error* e = nullptr;
            rs2_get_option_range(_options, option,
                &result.min, &result.max, &result.step, &result.def, &e);
            error::handle(e);
            return result;
        }

        /**
        * write new value to the option
        * \param[in] option     option id to be queried
        * \param[in] value      new value for the option
        */
        void set_option(rs2_option option, float value) const
        {
            rs2_error* e = nullptr;
            rs2_set_option(_options, option, value, &e);
            error::handle(e);
        }

        /**
        * write new value to the option
        * \param[in] option     option id to be queried
        * \param[in] value      option (id,type,is_valid,new value)
        */
        void set_option_value( option_value const & value ) const
        {
            rs2_error * e = nullptr;
            rs2_set_option_value( _options, value, &e );
            error::handle( e );
        }

        /**
        * check if particular option is read-only
        * \param[in] option     option id to be checked
        * \return true if option is read-only
        */
        bool is_option_read_only(rs2_option option) const
        {
            rs2_error* e = nullptr;
            auto res = rs2_is_option_read_only(_options, option, &e);
            error::handle(e);
            return res > 0;
        }

        /**
         * sets a callback in case an option in this options container value is updated
         * \param[in] callback     the callback function
         */
        void on_options_changed( std::function< void( const options_list & ) > callback ) const
        {
            rs2_error * e = nullptr;
            rs2_set_options_changed_callback_cpp( _options, new options_changed_callback( callback ), &e );
            error::handle( e );
        }

        std::vector<rs2_option> get_supported_options()
        {
            std::vector<rs2_option> res;
            rs2_error* e = nullptr;
            std::shared_ptr< rs2_options_list > options_list( rs2_get_options_list(_options, &e), rs2_delete_options_list);
            error::handle( e );

            for (auto opt = 0; opt < rs2_get_options_list_size(options_list.get(), &e);opt++)
            {
                res.push_back(rs2_get_option_from_list(options_list.get(), opt, &e));
            }
            return res;
        };

        options_list get_supported_option_values()
        {
            rs2_error * e = nullptr;
            std::shared_ptr< rs2_options_list > sptr(
                rs2_get_options_list( _options, &e ),
                rs2_delete_options_list );
            error::handle( e );
            return options_list( sptr );
        };

        options& operator=(const options& other)
        {
            _options = other._options;
            return *this;
        }
        // if operator= is ok, this should be ok too
        options(const options& other) : _options(other._options) {}

        virtual ~options() = default;

    protected:
        explicit options(rs2_options* o = nullptr) : _options(o)
        {
        }

        template<class T>
        options& operator=(const T& dev)
        {
            _options = (rs2_options*)(dev.get());
            return *this;
        }

    private:
        rs2_options* _options;
    };


}
#endif // LIBREALSENSE_RS2_OIPTIONS_HPP
