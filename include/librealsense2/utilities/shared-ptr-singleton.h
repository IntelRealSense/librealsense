// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <mutex>
#include <memory>


// Implements a global singleton of type T, managed and accessed by a shared_ptr.
// 
// The singleton is instantiated via instance() if not already instantiated. It is deleted when the last shared_ptr
// to it is destroyed.
// 
// I.e., multiple objects alive at the same time will all share this singleton, but when all have been destroyed the
// singleton will also be.
//
template< class T >
class shared_ptr_singleton
{
	std::shared_ptr< T > _ptr;

public:
	template< typename... Args >
	shared_ptr_singleton & instance( Args... args )
	{
		if( ! _ptr )
		{
			static std::mutex _mutex;
			static std::weak_ptr< T > _singleton;

			std::lock_guard< std::mutex > lock( _mutex );
			if( _ptr = _singleton.lock() )
			{
				// The singleton is still alive and we can just use it
			}
			else
			{
				// First instance ever of T, or the singleton died (all references to the singleton were released), so
				// we have to recreate it
				_ptr = std::make_shared< T >( args... );
				_singleton = _ptr;
			}
		}
		return *this;  // for convenience: instance(...)->foo(...);
	}

	T * operator->() const { return _ptr.get(); }

	std::shared_ptr< T > const & get() const { return _ptr; }
	operator std::shared_ptr< T > const & ( ) const { return get(); }

	operator bool() const { return( _ptr.get() != nullptr ); }
	bool operator!() const { return ! _ptr; }
};
