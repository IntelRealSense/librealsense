// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once


/*
    This encapsulates data needed for a dlib-compatible image that originates
    in a librealsense color frame.

    The data from the color frame is not copied, and so this object should not
    live beyond the lifetime of the frame.
*/
template <
    typename pixel_type,    // The dlib type, e.g. rgb_pixel
    int RS_FORMAT           // The corresponding RS2_FORMAT, e.g. RS2_FORMAT_RGB8
>
class rs_frame_image
{
    void * _data;
    long _nr;
    long _nc;
    rs2::video_frame _frame;  // To keep the frame data alive

public:
    rs_frame_image( rs2::video_frame const & f )
        : _data( const_cast< void * >( f.get_data() ))
        , _nc( f.get_width() )
        , _nr( f.get_height() )
        , _frame( f )
    {
        if( f.get_profile().format() != RS_FORMAT )
        {
            throw std::runtime_error( "unsupported Frame format" );
        }
    }

    rs_frame_image() : _data( nullptr ), _nr( 0 ), _nc( 0 ) {}

    size_t size() const { return static_cast<size_t>(_nr *_nc); }
    void * data() { return _data; }
    void const * data() const { return _data; }

    long nr() const { return _nr; }
    long nc() const { return _nc; }
    long width_step() const { return _nc * sizeof( pixel_type ); }

    pixel_type const * operator[]( unsigned const row ) const
    {
        return reinterpret_cast< const pixel_type * >( _data + width_step() * row );
    }

    rs_frame_image & operator=( const rs_frame_image & ) = default;
};


/*!
    In dlib, an "image" is any object that implements the generic image interface.  In
    particular, this simply means that an image type (let's refer to it as image_type
    from here on) has the following seven global functions defined for it:
        - long        num_rows      (const image_type& img)
        - long        num_columns   (const image_type& img)
        - void        set_image_size(      image_type& img, long rows, long cols)
        - void*       image_data    (      image_type& img)
        - const void* image_data    (const image_type& img)
        - long        width_step    (const image_type& img)
        - void        swap          (      image_type& a, image_type& b)
    And also provides a specialization of the image_traits template that looks like:
        namespace dlib
        {
            template <>
            struct image_traits<image_type>
            {
                typedef the_type_of_pixel_used_in_image_type pixel_type;
            };
        }

    Additionally, an image object must be default constructable.  This means that
    expressions of the form:
        image_type img;
    Must be legal.

    Finally, the type of pixel in image_type must have a pixel_traits specialization.
    That is, pixel_traits<typename image_traits<image_type>::pixel_type> must be one of
    the specializations of pixel_traits.

    (see http://dlib.net/dlib/image_processing/generic_image.h.html)
*/

namespace dlib
{

    // Define the global functions that make rs_frame_image a proper "generic image" according to
    // $(DLIB_DIR)/image_processing/generic_image.h
    // These are all template specializations for existing classes/structs/functions to adapt to the
    // new class rs_frame_image that we defined above.
    template <typename T, int RS>
    struct image_traits< rs_frame_image< T, RS > >
    {
        typedef T pixel_type;
    };

    template <typename T, int RS>
    inline long num_rows( const rs_frame_image<T,RS>& img ) { return img.nr(); }
    template <typename T, int RS>
    inline long num_columns( const rs_frame_image<T,RS>& img ) { return img.nc(); }

    template <typename T, int RS>
    inline void* image_data(
        rs_frame_image<T,RS>& img
    )
    {
        return img.data();
    }

    template <typename T, int RS>
    inline const void* image_data(
        const rs_frame_image<T,RS>& img
    )
    {
        return img.data();
    }

    template <typename T, int RS>
    inline long width_step(
        rs_frame_image< T, RS > const & img
    )
    {
        return img.width_step();
    }
}

