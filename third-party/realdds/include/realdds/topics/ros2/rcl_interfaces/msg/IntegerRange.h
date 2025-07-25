// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.

/*!
 * @file IntegerRange.h
 * This header file contains the declaration of the described types in the IDL file.
 *
 * This file was generated by the tool gen.
 */

#ifndef _FAST_DDS_GENERATED_RCL_INTERFACES_MSG_INTEGERRANGE_H_
#define _FAST_DDS_GENERATED_RCL_INTERFACES_MSG_INTEGERRANGE_H_


//#include <fastrtps/utils/fixed_size_string.hpp>

#include <stdint.h>
#include <array>
#include <string>
#include <vector>
#include <map>
#include <bitset>

#if defined(_WIN32)
#if defined(EPROSIMA_USER_DLL_EXPORT)
#define eProsima_user_DllExport __declspec( dllexport )
#else
#define eProsima_user_DllExport
#endif  // EPROSIMA_USER_DLL_EXPORT
#else
#define eProsima_user_DllExport
#endif  // _WIN32

#if defined(_WIN32)
#if defined(EPROSIMA_USER_DLL_EXPORT)
#if defined(IntegerRange_SOURCE)
#define IntegerRange_DllAPI __declspec( dllexport )
#else
#define IntegerRange_DllAPI __declspec( dllimport )
#endif // IntegerRange_SOURCE
#else
#define IntegerRange_DllAPI
#endif  // EPROSIMA_USER_DLL_EXPORT
#else
#define IntegerRange_DllAPI
#endif // _WIN32

namespace eprosima {
namespace fastcdr {
class Cdr;
} // namespace fastcdr
} // namespace eprosima


namespace rcl_interfaces {
    namespace msg {
        /*!
         * @brief This class represents the structure IntegerRange defined by the user in the IDL file.
         * @ingroup INTEGERRANGE
         */
        class IntegerRange
        {
        public:

            /*!
             * @brief Default constructor.
             */
            eProsima_user_DllExport IntegerRange();

            /*!
             * @brief Default destructor.
             */
            eProsima_user_DllExport ~IntegerRange();

            /*!
             * @brief Copy constructor.
             * @param x Reference to the object rcl_interfaces::msg::IntegerRange that will be copied.
             */
            eProsima_user_DllExport IntegerRange(
                    const IntegerRange& x);

            /*!
             * @brief Move constructor.
             * @param x Reference to the object rcl_interfaces::msg::IntegerRange that will be copied.
             */
            eProsima_user_DllExport IntegerRange(
                    IntegerRange&& x) noexcept;

            /*!
             * @brief Copy assignment.
             * @param x Reference to the object rcl_interfaces::msg::IntegerRange that will be copied.
             */
            eProsima_user_DllExport IntegerRange& operator =(
                    const IntegerRange& x);

            /*!
             * @brief Move assignment.
             * @param x Reference to the object rcl_interfaces::msg::IntegerRange that will be copied.
             */
            eProsima_user_DllExport IntegerRange& operator =(
                    IntegerRange&& x) noexcept;

            /*!
             * @brief Comparison operator.
             * @param x rcl_interfaces::msg::IntegerRange object to compare.
             */
            eProsima_user_DllExport bool operator ==(
                    const IntegerRange& x) const;

            /*!
             * @brief Comparison operator.
             * @param x rcl_interfaces::msg::IntegerRange object to compare.
             */
            eProsima_user_DllExport bool operator !=(
                    const IntegerRange& x) const;

            /*!
             * @brief This function sets a value in member from_value
             * @param _from_value New value for member from_value
             */
            eProsima_user_DllExport void from_value(
                    int64_t _from_value);

            /*!
             * @brief This function returns the value of member from_value
             * @return Value of member from_value
             */
            eProsima_user_DllExport int64_t from_value() const;

            /*!
             * @brief This function returns a reference to member from_value
             * @return Reference to member from_value
             */
            eProsima_user_DllExport int64_t& from_value();

            /*!
             * @brief This function sets a value in member to_value
             * @param _to_value New value for member to_value
             */
            eProsima_user_DllExport void to_value(
                    int64_t _to_value);

            /*!
             * @brief This function returns the value of member to_value
             * @return Value of member to_value
             */
            eProsima_user_DllExport int64_t to_value() const;

            /*!
             * @brief This function returns a reference to member to_value
             * @return Reference to member to_value
             */
            eProsima_user_DllExport int64_t& to_value();

            /*!
             * @brief This function sets a value in member step
             * @param _step New value for member step
             */
            eProsima_user_DllExport void step(
                    uint64_t _step);

            /*!
             * @brief This function returns the value of member step
             * @return Value of member step
             */
            eProsima_user_DllExport uint64_t step() const;

            /*!
             * @brief This function returns a reference to member step
             * @return Reference to member step
             */
            eProsima_user_DllExport uint64_t& step();


            /*!
             * @brief This function returns the maximum serialized size of an object
             * depending on the buffer alignment.
             * @param current_alignment Buffer alignment.
             * @return Maximum serialized size.
             */
            eProsima_user_DllExport static size_t getMaxCdrSerializedSize(
                    size_t current_alignment = 0);

            /*!
             * @brief This function returns the serialized size of a data depending on the buffer alignment.
             * @param data Data which is calculated its serialized size.
             * @param current_alignment Buffer alignment.
             * @return Serialized size.
             */
            eProsima_user_DllExport static size_t getCdrSerializedSize(
                    const rcl_interfaces::msg::IntegerRange& data,
                    size_t current_alignment = 0);


            /*!
             * @brief This function serializes an object using CDR serialization.
             * @param cdr CDR serialization object.
             */
            eProsima_user_DllExport void serialize(
                    eprosima::fastcdr::Cdr& cdr) const;

            /*!
             * @brief This function deserializes an object using CDR serialization.
             * @param cdr CDR serialization object.
             */
            eProsima_user_DllExport void deserialize(
                    eprosima::fastcdr::Cdr& cdr);



            /*!
             * @brief This function returns the maximum serialized size of the Key of an object
             * depending on the buffer alignment.
             * @param current_alignment Buffer alignment.
             * @return Maximum serialized size.
             */
            eProsima_user_DllExport static size_t getKeyMaxCdrSerializedSize(
                    size_t current_alignment = 0);

            /*!
             * @brief This function tells you if the Key has been defined for this type
             */
            eProsima_user_DllExport static bool isKeyDefined();

            /*!
             * @brief This function serializes the key members of an object using CDR serialization.
             * @param cdr CDR serialization object.
             */
            eProsima_user_DllExport void serializeKey(
                    eprosima::fastcdr::Cdr& cdr) const;

        private:

            int64_t m_from_value;
            int64_t m_to_value;
            uint64_t m_step;
        };
    } // namespace msg
} // namespace rcl_interfaces

#endif // _FAST_DDS_GENERATED_RCL_INTERFACES_MSG_INTEGERRANGE_H_
