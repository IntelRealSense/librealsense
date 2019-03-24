// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "TrackingCommon.h"

namespace perc {
// ----------------------------------------------------------------------------
///
/// @class  Message
///
/// @brief  Declare a base class for all messages that can be posted to Dispatcher.
///         User MUST manage allocation policy if needed by defining copy constructor
//          and appropriate destructor. <Message> and its derives will be copied internally and
///         SHOULD be freed after dispatching callback by Dispatcher.
///
// ----------------------------------------------------------------------------
class Message
{
public:
    Message(int type, int param = 0) : Type(type), Param(param), Result(-1) {}
    virtual ~Message() {}
    int Type;
    int Param;
    mutable int Result;

    // replace <std::is_base_of> mechanism that generate compilation error
    // if derived class was not inherited from <Message> class
    static inline void IS_DERIVED_FROM_Message_() {}
#define IS_DERIVED_FROM_Message IS_DERIVED_FROM_Message_()
};

// ----------------------------------------------------------------------------
///
/// @class  EventHandler
///
/// @brief  This base class defines the interface for receiving the
///         results of sync and asynchronous operations.
///         Subclasses of this class will fill in appropriate methods.
/// @note
///
// ----------------------------------------------------------------------------
class EventHandler
{
public:
    virtual ~EventHandler() {}

    // = Completion callbacks
    virtual void onMessage(const Message &) {}
    virtual void onEvent(Handle fd, unsigned long mask, void *act) {}
    virtual void onTimeout(uintptr_t timerId, const Message &) {}
    virtual void onExit() {}

protected:
    // Hide the constructor
    EventHandler() {}
};


// ----------------------------------------------------------------------------
} // namespace perc
