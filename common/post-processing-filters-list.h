// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "post-processing-filter.h"
#include <list>
#include <memory>


/*
    Container for all the filters that are defined. These are added into our list and can be iterated
    over, without actual knowledge of any implementation.
*/
struct post_processing_filters_list
{
    typedef std::function<
        std::shared_ptr< post_processing_filter >()
    > create_fn;
    typedef std::list< create_fn > list;

    static list & get()
    {
        static list the_filters;
        return the_filters;
    }

    template< class T >
    static list::iterator register_filter( std::string const & name )
    {
        auto & filters = get();
        return filters.insert(
            filters.end(),
            [name]() -> std::shared_ptr< post_processing_filter >
            {
                try
                {
                    return std::make_shared< T >( name );
                }
                catch( std::exception const& e )
                {
                    LOG( ERROR ) << "Failed to start " << name << ": " << e.what();
                }
                catch( ... )
                {
                    LOG( ERROR ) << "Failed to start " << name << ": unknown exception";
                }
                return std::shared_ptr< T >();
            }
            );
    }
};

