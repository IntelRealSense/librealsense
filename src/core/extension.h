// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "../../include/librealsense/rs2.h" //TODO: Ziv, remove relative\file
#include <memory>
#include <functional>
namespace librealsense
{
	template<typename T, typename P>
	bool Is(std::shared_ptr<P> ptr)
	{
		return std::dynamic_pointer_cast<T>(ptr) != nullptr;
	}

    template<typename T, typename P>
    bool Is(P* ptr)
    {
        return dynamic_cast<T*>(ptr) != nullptr;
    }

	class extendable_interface
    {
    public:
        virtual bool extend_to(rs2_extension_type extension_type, void** ptr) = 0;
        virtual ~extendable_interface() = default;
    };

    /**
     * Extensions' snapshot implementations are expected to derive from this class in addition to the actual extension
     * Extensions' interfaces are no expected to derive from this class (//TODO Ziv, what if they do? - test that it works)
     */
    //template <typename T>
    class extension_snapshot  //TODO: : Ziv, : public T, public std::enable_shared_from_this ?
    {
    public:
        virtual void update(std::shared_ptr<extension_snapshot> ext) = 0;
        virtual ~extension_snapshot() = default;
    };

    template <typename T>
    class extension_snapshot_base : public extension_snapshot
    {
    public:
        void update(std::shared_ptr<extension_snapshot> ext) override
        {
            auto api = dynamic_cast<T*>(ext.get());
            if(api == nullptr)
            {
                throw std::runtime_error(typeid(T).name());
            }
            update_self(api);
        }
    protected:
        virtual void update_self(T* info_api) = 0;
    };

    /**
 * Deriving classes are expected to return an extension_snapshot
 * We need this since Sensors will derive from multiple extensions and
 * @tparam T The interface that should be recorded
 */
    template <typename T>
    class recordable
    {
    public:
        virtual void create_snapshot(std::shared_ptr<T>& snapshot) = 0;
        virtual void create_recordable(std::shared_ptr<T>& recordable, std::function<void(std::shared_ptr<extension_snapshot>)> record_action) = 0;
        virtual ~recordable() = default;

    };

}