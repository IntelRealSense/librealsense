// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_OPTIONS_HPP
#define LIBREALSENSE_RS2_OPTIONS_HPP

#include "rs_types.hpp"
#include "../h/rs_types.h"

#include <memory>


namespace rs2
{    
    class options_list
    {
    public:
        explicit options_list( std::shared_ptr< rs2_options_list > list )
            : _list( std::move( list ) )
        {
        }

        options_list()
            : _list( nullptr )
        {
        }

        options_list & operator=( std::shared_ptr< rs2_options_list > list )
        {
            _list = std::move( list );
            return *this;
        }

        rs2_option operator[]( size_t index ) const
        {
            rs2_error * e = nullptr;
            rs2_option opt = rs2_get_option_from_list( _list.get(), static_cast< int >( index ), &e );
            error::handle( e );
            return opt;
        }

        size_t size() const
        {
            rs2_error * e = nullptr;
            auto size = rs2_get_options_list_size( _list.get(), &e );
            error::handle( e );
            return size;
        }

        rs2_option front() const { return ( *this )[0]; }
        rs2_option back() const { return ( *this )[size() - 1]; }

        class options_list_iterator
        {
            options_list_iterator( const options_list & list, size_t index )
                : _list( list )
                , _index( index )
            {
            }

        public:
            rs2_option operator*() const { return _list[_index]; }
            bool operator!=( const options_list_iterator & other ) const
            {
                return other._index != _index || &other._list != &_list;
            }

            bool operator==( const options_list_iterator & other ) const
            {
                return ! ( *this != other );
            }

            options_list_iterator & operator++()
            {
                _index++;
                return *this;
            }

        private:
            friend options_list;
            const options_list & _list;
            size_t _index;
        };

        options_list_iterator begin() const { return options_list_iterator( *this, 0 ); }
        options_list_iterator end() const { return options_list_iterator( *this, size() ); }

        operator std::shared_ptr< rs2_options_list >() { return _list; };

    private:
        std::shared_ptr< rs2_options_list > _list;
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
        * read option's value
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

            for (auto opt = 0; opt < rs2_get_options_list_size(options_list.get(), &e);opt++)
            {
                res.push_back(rs2_get_option_from_list(options_list.get(), opt, &e));
            }
            return res;
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
