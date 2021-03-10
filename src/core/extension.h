// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "../../include/librealsense2/h/rs_types.h"
#include <memory>
#include <functional>

/*! Preprocessor Macro to define mapping between rs2_extension and their respective interface (and vice versa) */
#define MAP_EXTENSION(E, T)                                      \
    template<> struct ExtensionToType<E> {                       \
        using type = T;                                          \
        static constexpr const char* to_string() { return #T; }; \
    };                                                           \
    template<> struct TypeToExtension<T> {                       \
        static constexpr rs2_extension value = E;                \
        static constexpr const char* to_string() { return #T; }; \
    }                                                            \

namespace librealsense
{
    class extendable_interface
    {
    public:
        virtual bool extend_to(rs2_extension extension_type, void** ptr) = 0;
        virtual ~extendable_interface() = default;
    };

    /**
     * Extensions' snapshots implementations are expected to derive from this class in addition to the actual extensions'
     * interfaces. Extensions are not expected to derive from this class.
     */
    class extension_snapshot
    {
    public:
        virtual void update(std::shared_ptr<extension_snapshot> ext) = 0;
        virtual ~extension_snapshot() = default;
    };

    /**
     * Deriving classes are expected to return an extension_snapshot
     * We need this since Sensors will derive from multiple extensions and C++ does not allow function overloads by return type
     * @tparam T The interface that should be recorded
     */
    template <typename T>
    class recordable
    {
    public:
        /**
        * Create a snapshot of the deriving extension.
        * A snapshot of T is a reflection of the state and memory of T at the time of the call
        */
        virtual void create_snapshot(std::shared_ptr<T>& snapshot) const = 0;

        /**
        * Instruct the derived class to begin notifying on changes
        * Derived class should call the recording_function with a reference of themselves
        */
        virtual void enable_recording(std::function<void(const T&)> recording_function) = 0;

        virtual ~recordable() = default;

    };

    /*
     * Helper functions
     *
     */

    template <typename To>
    inline bool try_extend(std::shared_ptr<extension_snapshot> from, void** ext)
    {
        if (from == nullptr)
        {
            return false;
        }

        auto casted = std::dynamic_pointer_cast<To>(from);
        if (casted == nullptr)
        {
            return false;
        }
        *ext = casted.get();
        return true;
    }

    template<typename T, typename P>
    inline std::shared_ptr<T> As(std::shared_ptr<P> ptr)
    {
        return std::dynamic_pointer_cast<T>(ptr);
    }

    template<typename T, typename P>
    inline T* As(P* ptr)
    {
        return dynamic_cast<T*>(ptr);
    }

    template<typename T, typename P>
    inline bool Is(std::shared_ptr<P> ptr)
    {
        return As<T>(ptr) != nullptr;
    }

    template<typename T, typename P>
    inline bool Is(P* ptr)
    {
        return As<T>(ptr) != nullptr;
    }
    //Creating Helper functions to map rs2_extension enums to actual interface
    template<rs2_extension> struct ExtensionToType;
    template<typename T> struct TypeToExtension;
}
