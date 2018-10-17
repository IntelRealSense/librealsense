// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#define LOG_TAG "Fsm"

#include "Fsm.h"

namespace perc {
// -[statics]------------------------------------------------------------------
// Default NONE transition definition.
const FsmTransition s_FsmTransitionNone =
{
    FSM_TRANSITION_NONE_NAME,
    FSM_TRANSITION_NONE,
    FSM_EVENT_NONE,
    FSM_STATE_FINAL,
    0,
    0,
    FSM_TRANSITION_AFTER_MAX_TIMEOUT
};

// Default FINAL state definition.
const FsmState s_FsmStateFinal =
{
    FSM_STATE_FINAL_NAME,
    FSM_STATE_FINAL,
    0,
    0,
    (FsmTransition *)&s_FsmTransitionNone
};


// ----------------------------------------------------------------------------
Fsm::Fsm() :
        m_pFsm(0),
        m_CurrStateId(FSM_STATE_FINAL),
        m_Owner(NULL),
        m_Dispatcher(NULL),
        m_Name(""),
        m_SelfEvent(NULL)
{
}

// ----------------------------------------------------------------------------
Fsm::~Fsm()
{
    done();
}

// ----------------------------------------------------------------------------
int
Fsm::init (const FsmState * const *pFsm,
           void                   *Owner,
           Dispatcher             *pDispatcher,
           const char             *Name)
{
    // User MUST define FSM definition!!!
    ASSERT (pFsm);

    // Save context parameters
    m_pFsm          = pFsm;
    m_Owner         = Owner;
    m_Dispatcher    = pDispatcher;
    m_Name          = Name;

    // Check if Dispatcher was set
    if (!dispatcher ())
    {
        LOGW("engine not found, can't schedule after transitions!");
    }

    // Init Initial state and call to initial state entry function.
    // Initial state always first into the states list!
    resetSelfEvent();
    int ret_code = InitNewState (m_pFsm[0]->Type);
    if (FSM_CONTEXT_STATUS_OK != ret_code)
    {
        ASSERT(!m_SelfEvent);
        logRetCode(ret_code, m_pFsm[m_CurrStateId], Message(FSM_EVENT_NONE));
        return ret_code;
    }
    // Process self event if exist
    return processSelfEvent();
}

// ----------------------------------------------------------------------------
int
Fsm::done ()
{
    if (!m_pFsm) return FSM_CONTEXT_STATUS_ERROR;

    CancelAfterTransitions ();

    // Reset FSM context parameters.
    m_pFsm          = 0;
    m_Owner         = 0;
    m_Dispatcher    = 0;

    return FSM_CONTEXT_STATUS_OK;
}

// ----------------------------------------------------------------------------
int
Fsm::fireEvent (const Message &pEvent)
{
    ASSERT (m_pFsm);

    resetSelfEvent();

    //  Find transition that wait for this event
    const FsmState *state = m_pFsm[m_CurrStateId];
    int transition_id = FSM_TRANSITION_NONE;
    int ret_code = FSM_CONTEXT_STATUS_ERROR;
    const FsmTransition *transition = 0;

    if  ((ret_code = FindTransition (&transition_id, pEvent)) != FSM_CONTEXT_STATUS_OK)
        goto exit_;
    else
        transition = &state->TransitionList[transition_id];

    ASSERT (transition);

    // Check if transition internal - in this case only call to transition action and return
    if (transition->NewState == FSM_STATE_SAME)
    {
        CallTransitionAction (transition, pEvent);
        ret_code = FSM_CONTEXT_STATUS_OK;
        goto exit_;
    }

    // Transition is not internal - we must to do some work...
    // Call to state exit function.
    DoneCurrState ();

    // Call to transition action.
    CallTransitionAction (transition, pEvent);

    // Go to new state, that may be final...
    ret_code = InitNewState (transition->NewState);

exit_:
    if (FSM_CONTEXT_STATUS_OK != ret_code)
    {
        ASSERT(!m_SelfEvent);
        logRetCode(ret_code, state, pEvent);
        return ret_code;
    }
    // Process self event if exist
    return processSelfEvent();
}

// -[Private functions]--------------------------------------------------------
int
Fsm::InitNewState (int StateType)
{
    // Check if new state - final state
    if (StateType == FSM_STATE_FINAL) 
        return FSM_CONTEXT_STATUS_STATE_FINAL;

    // Find new state definition
    for (int state_id = 0; m_pFsm[state_id]->Type != FSM_STATE_FINAL; state_id++)
    {
        if (m_pFsm[state_id]->Type == StateType) 
        {
            // Remember new state index.
            m_CurrStateId = state_id;

            CallStateEntry ();

            // Schedule After transitions timers via engine object.
            ScheduleAfterTransitions ();

            return FSM_CONTEXT_STATUS_OK;
        }
    }
    return FSM_CONTEXT_STATUS_STATE_NOT_FOUND;
}

// ----------------------------------------------------------------------------
int
Fsm::DoneCurrState ()
{
    // Cancel after transitions timers
    CancelAfterTransitions ();

    // Call to old state exit function
    CallStateExit ();

    return FSM_CONTEXT_STATUS_OK;
}

// ----------------------------------------------------------------------------
int
Fsm::FindTransition (int *pTransitionId, const Message &pEvent)
{
    const FsmTransition *transition_list = m_pFsm[m_CurrStateId]->TransitionList;
    int ret_code = FSM_CONTEXT_STATUS_TRANSITION_NOT_FOUND;

    // = Find transition that wait for this event type
    //
    if (pEvent.Type == FSM_EVENT_TIMEOUT)
    {
        int tid = pEvent.Param;

        ASSERT (transition_list[tid].Type == FSM_TRANSITION_AFTER ||
                transition_list[tid].Type == FSM_TRANSITION_INTERNAL_AFTER);

        // Check transition Guard function
        if (CallTransitionGuard (&transition_list[tid], pEvent))
        {
            // Save found transition
            *pTransitionId = tid;
            ret_code = FSM_CONTEXT_STATUS_OK;
        }
        else
            ret_code = FSM_CONTEXT_STATUS_EVENT_NOT_HANDLED;
    }
    else
    {
        for (int i = 0; transition_list[i].Type != FSM_TRANSITION_NONE; i++)
        {
            // Check if this transition wait for current event.
            if (transition_list[i].EventType == pEvent.Type)
            {
                ret_code = FSM_CONTEXT_STATUS_EVENT_NOT_HANDLED;

                if (CallTransitionGuard (&transition_list[i], pEvent))
                {
                    // Save found transition
                    *pTransitionId = i;
                    return FSM_CONTEXT_STATUS_OK;
                }
            }
        }
    }
    return ret_code;
}

// ----------------------------------------------------------------------------
int
Fsm::ScheduleAfterTransitions ()
{
    if (!dispatcher ()) return -1;

    const FsmState *state = m_pFsm[m_CurrStateId];
    const FsmTransition *transition_list = state->TransitionList;

    // Find after transitions and schedule timers for them.
    for (int tid = 0; transition_list[tid].Type != FSM_TRANSITION_NONE; tid++)
    {
        // Check if this after transition
        if (transition_list[tid].TimeOut != FSM_TRANSITION_AFTER_MAX_TIMEOUT)
        {
            uintptr_t timerId = dispatcher ()->scheduleTimer(this, ms2ns(transition_list[tid].TimeOut), Message(FSM_EVENT_TIMEOUT, tid));
            if (!timerId)
            {
                LOGE("[%s]:invalid timer id, can't schedule more!", state->Name);
            }
        }
    }
    return 0;
}

// ----------------------------------------------------------------------------
int Fsm::CancelAfterTransitions()
{
    if (!dispatcher()) return -1;

    // Cancel timer for all after transitions that was scheduled.
    dispatcher()->removeHandler(this, Dispatcher::TIMERS_MASK);
    return 0;
}

// ----------------------------------------------------------------------------
bool Fsm::CallTransitionGuard(const FsmTransition *pTransition, const Message &pEvent)
{
    bool ret = true;

    if (pTransition->Guard)
        ret = pTransition->Guard(this, pEvent);

    LOGV("[%s]:%s[%s]:guard%s %d",
         m_pFsm[m_CurrStateId]->Name,
         TransitionType (pTransition->Type),
         pTransition->Name,
         pTransition->Guard ? "()": "(none)",
         ret);
    return ret;
}

// ----------------------------------------------------------------------------
void Fsm::CallTransitionAction(const FsmTransition *pTransition, const Message &pEvent)
{
    LOGV("[%s]:%s[%s]:action%s",
         m_pFsm[m_CurrStateId]->Name,
         TransitionType (pTransition->Type),
         pTransition->Name,
         pTransition->Action ? "()": "(none)");
    if (pTransition->Action)
        pTransition->Action(this, pEvent);
}

// ----------------------------------------------------------------------------
void Fsm::CallStateEntry()
{
    const FsmState *state = m_pFsm[m_CurrStateId];
    LOGV("[%s]:entry%s",
          state->Name,
          state->Entry ? "()": "(none)");
    if (state->Entry)
        state->Entry(this);
}

// ----------------------------------------------------------------------------
void Fsm::CallStateExit()
{
    const FsmState *state = m_pFsm[m_CurrStateId];
    LOGV("[%s]:exit%s",
          state->Name,
          state->Exit ? "()": "(none)");
    if (state->Exit)
        state->Exit(this);
}

// ----------------------------------------------------------------------------
// A utility static method that translates FSM status to a string.
const char*
Fsm::statusName (int Status)
{
    switch (Status)
    {
        case FSM_CONTEXT_STATUS_ERROR:                  return "Error";
        case FSM_CONTEXT_STATUS_OK:                     return "OK";
        case FSM_CONTEXT_STATUS_TRANSITION_NOT_FOUND:   return "Transition not found";
        case FSM_CONTEXT_STATUS_EVENT_NOT_HANDLED:      return "Event not handled";
        case FSM_CONTEXT_STATUS_STATE_FINAL:            return "State final";
        case FSM_CONTEXT_STATUS_STATE_NOT_FOUND:        return "State not found";
    }
    return "Unknown status";
}

// ----------------------------------------------------------------------------
// A utility static method that translates FSM transition to a string.
const char*
Fsm::TransitionType (int Transition)
{
    switch (Transition)
    {
        case FSM_TRANSITION_NONE:               return "TN";
        case FSM_TRANSITION:                    return "T";
        case FSM_TRANSITION_AFTER:              return "TA";
        case FSM_TRANSITION_INTERNAL:           return "TI";
        case FSM_TRANSITION_INTERNAL_AFTER:     return "TIA";
    }
    return "T?";
}

// ----------------------------------------------------------------------------
// Debug print of FSM status
void
Fsm::logRetCode (int retCode, const FsmState *state, const Message &pEvent)
{
    if (FSM_CONTEXT_STATUS_STATE_FINAL == retCode)
    {
        LOGD("final state reached");
    }
    else
    {
        if (FSM_CONTEXT_STATUS_EVENT_NOT_HANDLED == retCode)
        {
            LOGW("[%s]:event[%d] not handled", state->Name, pEvent.Type);
        }
        else if (FSM_CONTEXT_STATUS_TRANSITION_NOT_FOUND == retCode)
        {
            LOGW("[%s]:no appropriate transition for this event[%d]",
                 state->Name, pEvent.Type);
        }
        else if (FSM_CONTEXT_STATUS_STATE_NOT_FOUND == retCode)
        {
            LOGW("[%s]:no appropriate state found for this event[%d]",
                 state->Name, pEvent.Type);
        }
        else
        {
            LOGE("[%s]:undefined status error - %d, event[%d]",
                 state->Name, retCode, pEvent.Type);
        }
    }
}


// ----------------------------------------------------------------------------
} // namespace perc
