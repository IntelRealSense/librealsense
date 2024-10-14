// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <rsutils/type/fourcc.h>
#include <ostream>


namespace rsutils {
namespace type {


std::ostream & operator<<( std::ostream & os, fourcc const & fcc )
{
    os << char( fcc.get() >> 24 );
    os << char( fcc.get() >> 16 );
    os << char( fcc.get() >> 8 );
    os << char( fcc.get() );
    return os;
}


}  // namespace number
}  // namespace rsutils
