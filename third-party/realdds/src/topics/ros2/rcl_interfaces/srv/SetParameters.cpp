// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.

/*!
 * @file SetParameters.cpp
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

#include <realdds/topics/ros2/rcl_interfaces/srv/SetParameters.h>
#include <fastcdr/Cdr.h>

#include <fastcdr/exceptions/BadParamException.h>
using namespace eprosima::fastcdr::exception;

#include <utility>

rcl_interfaces::srv::SetParameters_Request::SetParameters_Request()
{
    // m_parameters com.eprosima.idl.parser.typecode.SequenceTypeCode@23a5fd2


}

rcl_interfaces::srv::SetParameters_Request::~SetParameters_Request()
{
}

rcl_interfaces::srv::SetParameters_Request::SetParameters_Request(
        const SetParameters_Request& x)
{
    m_parameters = x.m_parameters;
}

rcl_interfaces::srv::SetParameters_Request::SetParameters_Request(
        SetParameters_Request&& x) noexcept 
{
    m_parameters = std::move(x.m_parameters);
}

rcl_interfaces::srv::SetParameters_Request& rcl_interfaces::srv::SetParameters_Request::operator =(
        const SetParameters_Request& x)
{

    m_parameters = x.m_parameters;

    return *this;
}

rcl_interfaces::srv::SetParameters_Request& rcl_interfaces::srv::SetParameters_Request::operator =(
        SetParameters_Request&& x) noexcept
{

    m_parameters = std::move(x.m_parameters);

    return *this;
}

bool rcl_interfaces::srv::SetParameters_Request::operator ==(
        const SetParameters_Request& x) const
{

    return (m_parameters == x.m_parameters);
}

bool rcl_interfaces::srv::SetParameters_Request::operator !=(
        const SetParameters_Request& x) const
{
    return !(*this == x);
}

size_t rcl_interfaces::srv::SetParameters_Request::getMaxCdrSerializedSize(
        size_t current_alignment)
{
    size_t initial_alignment = current_alignment;


    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);


    for(size_t a = 0; a < 100; ++a)
    {
        current_alignment += rcl_interfaces::msg::Parameter::getMaxCdrSerializedSize(current_alignment);}

    return current_alignment - initial_alignment;
}

size_t rcl_interfaces::srv::SetParameters_Request::getCdrSerializedSize(
        const rcl_interfaces::srv::SetParameters_Request& data,
        size_t current_alignment)
{
    (void)data;
    size_t initial_alignment = current_alignment;


    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);


    for(size_t a = 0; a < data.parameters().size(); ++a)
    {
        current_alignment += rcl_interfaces::msg::Parameter::getCdrSerializedSize(data.parameters().at(a), current_alignment);}

    return current_alignment - initial_alignment;
}

void rcl_interfaces::srv::SetParameters_Request::serialize(
        eprosima::fastcdr::Cdr& scdr) const
{

    scdr << m_parameters;
}

void rcl_interfaces::srv::SetParameters_Request::deserialize(
        eprosima::fastcdr::Cdr& dcdr)
{

    dcdr >> m_parameters;}

/*!
 * @brief This function copies the value in member parameters
 * @param _parameters New value to be copied in member parameters
 */
void rcl_interfaces::srv::SetParameters_Request::parameters(
        const std::vector<rcl_interfaces::msg::Parameter>& _parameters)
{
    m_parameters = _parameters;
}

/*!
 * @brief This function moves the value in member parameters
 * @param _parameters New value to be moved in member parameters
 */
void rcl_interfaces::srv::SetParameters_Request::parameters(
        std::vector<rcl_interfaces::msg::Parameter>&& _parameters)
{
    m_parameters = std::move(_parameters);
}

/*!
 * @brief This function returns a constant reference to member parameters
 * @return Constant reference to member parameters
 */
const std::vector<rcl_interfaces::msg::Parameter>& rcl_interfaces::srv::SetParameters_Request::parameters() const
{
    return m_parameters;
}

/*!
 * @brief This function returns a reference to member parameters
 * @return Reference to member parameters
 */
std::vector<rcl_interfaces::msg::Parameter>& rcl_interfaces::srv::SetParameters_Request::parameters()
{
    return m_parameters;
}

size_t rcl_interfaces::srv::SetParameters_Request::getKeyMaxCdrSerializedSize(
        size_t current_alignment)
{
    size_t current_align = current_alignment;



    return current_align;
}

bool rcl_interfaces::srv::SetParameters_Request::isKeyDefined()
{
    return false;
}

void rcl_interfaces::srv::SetParameters_Request::serializeKey(
        eprosima::fastcdr::Cdr& scdr) const
{
    (void) scdr;
     
}

rcl_interfaces::srv::SetParameters_Response::SetParameters_Response()
{
    // m_results com.eprosima.idl.parser.typecode.SequenceTypeCode@ba8d91c


}

rcl_interfaces::srv::SetParameters_Response::~SetParameters_Response()
{
}

rcl_interfaces::srv::SetParameters_Response::SetParameters_Response(
        const SetParameters_Response& x)
{
    m_results = x.m_results;
}

rcl_interfaces::srv::SetParameters_Response::SetParameters_Response(
        SetParameters_Response&& x) noexcept 
{
    m_results = std::move(x.m_results);
}

rcl_interfaces::srv::SetParameters_Response& rcl_interfaces::srv::SetParameters_Response::operator =(
        const SetParameters_Response& x)
{

    m_results = x.m_results;

    return *this;
}

rcl_interfaces::srv::SetParameters_Response& rcl_interfaces::srv::SetParameters_Response::operator =(
        SetParameters_Response&& x) noexcept
{

    m_results = std::move(x.m_results);

    return *this;
}

bool rcl_interfaces::srv::SetParameters_Response::operator ==(
        const SetParameters_Response& x) const
{

    return (m_results == x.m_results);
}

bool rcl_interfaces::srv::SetParameters_Response::operator !=(
        const SetParameters_Response& x) const
{
    return !(*this == x);
}

size_t rcl_interfaces::srv::SetParameters_Response::getMaxCdrSerializedSize(
        size_t current_alignment)
{
    size_t initial_alignment = current_alignment;


    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);


    for(size_t a = 0; a < 100; ++a)
    {
        current_alignment += rcl_interfaces::msg::SetParametersResult::getMaxCdrSerializedSize(current_alignment);}

    return current_alignment - initial_alignment;
}

size_t rcl_interfaces::srv::SetParameters_Response::getCdrSerializedSize(
        const rcl_interfaces::srv::SetParameters_Response& data,
        size_t current_alignment)
{
    (void)data;
    size_t initial_alignment = current_alignment;


    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);


    for(size_t a = 0; a < data.results().size(); ++a)
    {
        current_alignment += rcl_interfaces::msg::SetParametersResult::getCdrSerializedSize(data.results().at(a), current_alignment);}

    return current_alignment - initial_alignment;
}

void rcl_interfaces::srv::SetParameters_Response::serialize(
        eprosima::fastcdr::Cdr& scdr) const
{

    scdr << m_results;
}

void rcl_interfaces::srv::SetParameters_Response::deserialize(
        eprosima::fastcdr::Cdr& dcdr)
{

    dcdr >> m_results;}

/*!
 * @brief This function copies the value in member results
 * @param _results New value to be copied in member results
 */
void rcl_interfaces::srv::SetParameters_Response::results(
        const std::vector<rcl_interfaces::msg::SetParametersResult>& _results)
{
    m_results = _results;
}

/*!
 * @brief This function moves the value in member results
 * @param _results New value to be moved in member results
 */
void rcl_interfaces::srv::SetParameters_Response::results(
        std::vector<rcl_interfaces::msg::SetParametersResult>&& _results)
{
    m_results = std::move(_results);
}

/*!
 * @brief This function returns a constant reference to member results
 * @return Constant reference to member results
 */
const std::vector<rcl_interfaces::msg::SetParametersResult>& rcl_interfaces::srv::SetParameters_Response::results() const
{
    return m_results;
}

/*!
 * @brief This function returns a reference to member results
 * @return Reference to member results
 */
std::vector<rcl_interfaces::msg::SetParametersResult>& rcl_interfaces::srv::SetParameters_Response::results()
{
    return m_results;
}

size_t rcl_interfaces::srv::SetParameters_Response::getKeyMaxCdrSerializedSize(
        size_t current_alignment)
{
    size_t current_align = current_alignment;



    return current_align;
}

bool rcl_interfaces::srv::SetParameters_Response::isKeyDefined()
{
    return false;
}

void rcl_interfaces::srv::SetParameters_Response::serializeKey(
        eprosima::fastcdr::Cdr& scdr) const
{
    (void) scdr;
     
}


