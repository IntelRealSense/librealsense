// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.

/*!
 * @file DescribeParameters.cpp
 * This source file contains the definition of the described types in the IDL file.
 *
 * This file was generated by the tool gen.
 */

#ifdef _WIN32
// Remove linker warning LNK4221 on Visual Studio
namespace {
char dummy;
}  // namespace
#endif  // _WIN32

#include <realdds/topics/ros2/rcl_interfaces/srv/DescribeParameters.h>
#include <fastcdr/Cdr.h>

#include <fastcdr/exceptions/BadParamException.h>
using namespace eprosima::fastcdr::exception;

#include <utility>

rcl_interfaces::srv::DescribeParameters_Request::DescribeParameters_Request()
{
    // m_names com.eprosima.idl.parser.typecode.SequenceTypeCode@2f0a87b3


}

rcl_interfaces::srv::DescribeParameters_Request::~DescribeParameters_Request()
{
}

rcl_interfaces::srv::DescribeParameters_Request::DescribeParameters_Request(
        const DescribeParameters_Request& x)
{
    m_names = x.m_names;
}

rcl_interfaces::srv::DescribeParameters_Request::DescribeParameters_Request(
        DescribeParameters_Request&& x) noexcept 
{
    m_names = std::move(x.m_names);
}

rcl_interfaces::srv::DescribeParameters_Request& rcl_interfaces::srv::DescribeParameters_Request::operator =(
        const DescribeParameters_Request& x)
{

    m_names = x.m_names;

    return *this;
}

rcl_interfaces::srv::DescribeParameters_Request& rcl_interfaces::srv::DescribeParameters_Request::operator =(
        DescribeParameters_Request&& x) noexcept
{

    m_names = std::move(x.m_names);

    return *this;
}

bool rcl_interfaces::srv::DescribeParameters_Request::operator ==(
        const DescribeParameters_Request& x) const
{

    return (m_names == x.m_names);
}

bool rcl_interfaces::srv::DescribeParameters_Request::operator !=(
        const DescribeParameters_Request& x) const
{
    return !(*this == x);
}

size_t rcl_interfaces::srv::DescribeParameters_Request::getMaxCdrSerializedSize(
        size_t current_alignment)
{
    size_t initial_alignment = current_alignment;


    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);


    for(size_t a = 0; a < 100; ++a)
    {
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + 255 + 1;
    }
    return current_alignment - initial_alignment;
}

size_t rcl_interfaces::srv::DescribeParameters_Request::getCdrSerializedSize(
        const rcl_interfaces::srv::DescribeParameters_Request& data,
        size_t current_alignment)
{
    (void)data;
    size_t initial_alignment = current_alignment;


    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);


    for(size_t a = 0; a < data.names().size(); ++a)
    {
        current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) +
            data.names().at(a).size() + 1;
    }
    return current_alignment - initial_alignment;
}

void rcl_interfaces::srv::DescribeParameters_Request::serialize(
        eprosima::fastcdr::Cdr& scdr) const
{

    scdr << m_names;
}

void rcl_interfaces::srv::DescribeParameters_Request::deserialize(
        eprosima::fastcdr::Cdr& dcdr)
{

    dcdr >> m_names;}

/*!
 * @brief This function copies the value in member names
 * @param _names New value to be copied in member names
 */
void rcl_interfaces::srv::DescribeParameters_Request::names(
        const std::vector<std::string>& _names)
{
    m_names = _names;
}

/*!
 * @brief This function moves the value in member names
 * @param _names New value to be moved in member names
 */
void rcl_interfaces::srv::DescribeParameters_Request::names(
        std::vector<std::string>&& _names)
{
    m_names = std::move(_names);
}

/*!
 * @brief This function returns a constant reference to member names
 * @return Constant reference to member names
 */
const std::vector<std::string>& rcl_interfaces::srv::DescribeParameters_Request::names() const
{
    return m_names;
}

/*!
 * @brief This function returns a reference to member names
 * @return Reference to member names
 */
std::vector<std::string>& rcl_interfaces::srv::DescribeParameters_Request::names()
{
    return m_names;
}

size_t rcl_interfaces::srv::DescribeParameters_Request::getKeyMaxCdrSerializedSize(
        size_t current_alignment)
{
    size_t current_align = current_alignment;



    return current_align;
}

bool rcl_interfaces::srv::DescribeParameters_Request::isKeyDefined()
{
    return false;
}

void rcl_interfaces::srv::DescribeParameters_Request::serializeKey(
        eprosima::fastcdr::Cdr& scdr) const
{
    (void) scdr;
     
}

rcl_interfaces::srv::DescribeParameters_Response::DescribeParameters_Response()
{
    // m_descriptors com.eprosima.idl.parser.typecode.SequenceTypeCode@4fcd19b3


}

rcl_interfaces::srv::DescribeParameters_Response::~DescribeParameters_Response()
{
}

rcl_interfaces::srv::DescribeParameters_Response::DescribeParameters_Response(
        const DescribeParameters_Response& x)
{
    m_descriptors = x.m_descriptors;
}

rcl_interfaces::srv::DescribeParameters_Response::DescribeParameters_Response(
        DescribeParameters_Response&& x) noexcept 
{
    m_descriptors = std::move(x.m_descriptors);
}

rcl_interfaces::srv::DescribeParameters_Response& rcl_interfaces::srv::DescribeParameters_Response::operator =(
        const DescribeParameters_Response& x)
{

    m_descriptors = x.m_descriptors;

    return *this;
}

rcl_interfaces::srv::DescribeParameters_Response& rcl_interfaces::srv::DescribeParameters_Response::operator =(
        DescribeParameters_Response&& x) noexcept
{

    m_descriptors = std::move(x.m_descriptors);

    return *this;
}

bool rcl_interfaces::srv::DescribeParameters_Response::operator ==(
        const DescribeParameters_Response& x) const
{

    return (m_descriptors == x.m_descriptors);
}

bool rcl_interfaces::srv::DescribeParameters_Response::operator !=(
        const DescribeParameters_Response& x) const
{
    return !(*this == x);
}

size_t rcl_interfaces::srv::DescribeParameters_Response::getMaxCdrSerializedSize(
        size_t current_alignment)
{
    size_t initial_alignment = current_alignment;


    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);


    for(size_t a = 0; a < 100; ++a)
    {
        current_alignment += rcl_interfaces::msg::ParameterDescriptor::getMaxCdrSerializedSize(current_alignment);}

    return current_alignment - initial_alignment;
}

size_t rcl_interfaces::srv::DescribeParameters_Response::getCdrSerializedSize(
        const rcl_interfaces::srv::DescribeParameters_Response& data,
        size_t current_alignment)
{
    (void)data;
    size_t initial_alignment = current_alignment;


    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);


    for(size_t a = 0; a < data.descriptors().size(); ++a)
    {
        current_alignment += rcl_interfaces::msg::ParameterDescriptor::getCdrSerializedSize(data.descriptors().at(a), current_alignment);}

    return current_alignment - initial_alignment;
}

void rcl_interfaces::srv::DescribeParameters_Response::serialize(
        eprosima::fastcdr::Cdr& scdr) const
{

    scdr << m_descriptors;
}

void rcl_interfaces::srv::DescribeParameters_Response::deserialize(
        eprosima::fastcdr::Cdr& dcdr)
{

    dcdr >> m_descriptors;}

/*!
 * @brief This function copies the value in member descriptors
 * @param _descriptors New value to be copied in member descriptors
 */
void rcl_interfaces::srv::DescribeParameters_Response::descriptors(
        const std::vector<rcl_interfaces::msg::ParameterDescriptor>& _descriptors)
{
    m_descriptors = _descriptors;
}

/*!
 * @brief This function moves the value in member descriptors
 * @param _descriptors New value to be moved in member descriptors
 */
void rcl_interfaces::srv::DescribeParameters_Response::descriptors(
        std::vector<rcl_interfaces::msg::ParameterDescriptor>&& _descriptors)
{
    m_descriptors = std::move(_descriptors);
}

/*!
 * @brief This function returns a constant reference to member descriptors
 * @return Constant reference to member descriptors
 */
const std::vector<rcl_interfaces::msg::ParameterDescriptor>& rcl_interfaces::srv::DescribeParameters_Response::descriptors() const
{
    return m_descriptors;
}

/*!
 * @brief This function returns a reference to member descriptors
 * @return Reference to member descriptors
 */
std::vector<rcl_interfaces::msg::ParameterDescriptor>& rcl_interfaces::srv::DescribeParameters_Response::descriptors()
{
    return m_descriptors;
}

size_t rcl_interfaces::srv::DescribeParameters_Response::getKeyMaxCdrSerializedSize(
        size_t current_alignment)
{
    size_t current_align = current_alignment;



    return current_align;
}

bool rcl_interfaces::srv::DescribeParameters_Response::isKeyDefined()
{
    return false;
}

void rcl_interfaces::srv::DescribeParameters_Response::serializeKey(
        eprosima::fastcdr::Cdr& scdr) const
{
    (void) scdr;
     
}


