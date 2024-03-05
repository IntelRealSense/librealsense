// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <rsutils/number/float3.h>

#include <ostream>


namespace rsutils {
namespace number {


std::ostream & operator<<( std::ostream & stream, const float3 & elem )
{
    return stream << elem.x << " " << elem.y << " " << elem.z;
}


std::ostream & operator<<( std::ostream & stream, const float4 & elem )
{
    return stream << elem.x << " " << elem.y << " " << elem.z << " " << elem.w;
}


std::ostream & operator<<( std::ostream & stream, const float3x3 & elem )
{
    return stream << elem.x << "\n" << elem.y << "\n" << elem.z;
}


}  // namespace number
}  // namespace rsutils
