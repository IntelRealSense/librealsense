// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "Log.h"
#include "Dispatcher.h"


namespace perc {
// -[Defines]------------------------------------------------------------------
/// C++ helpers macros that transform static FSM functions interface to class methods.
// Put these macros into header file (protected/private) section.
#define ACTION(Class, State, Event)    Class::sAction_s ## State ## _e ## Event
#define GUARD(Class, State, Event)     Class::sGuard_s  ## State ## _e ## Event
#define ENTRY(Class, State)            Class::sEntry_s  ## State
#define EXIT(Class, State)             Class::sExit_s   ## State

#define DECLARE_FSM_ACTION(State, Event) \
    static void sAction_s##State##_e##Event(Fsm *, const Message &); \
    void Action_s##State##_e##Event(const Message &)

#define DECLARE_FSM_GUARD(State, Event) \
    static bool sGuard_s##State##_e##Event(Fsm *, const Message &); \
    bool Guard_s##State##_e##Event(const Message &)

#define DECLARE_FSM_STATE_ENTRY(State) \
    static void sEntry_s##State(Fsm *); \
    void Entry_s##State()

#define DECLARE_FSM_STATE_EXIT(State) \
    static void sExit_s##State(Fsm *); \
    void Exit_s##State()

// Put these macros into source file and define body.
#define DEFINE_FSM_ACTION(Class, State, Event, Msg) \
    void ACTION(Class, State, Event)(Fsm *pContext, const Message &Msg) \
    { Class* pThis = reinterpret_cast<Class*> (pContext->owner()); pThis->Action_s ## State ## _e ## Event(Msg); } \
    void Class::Action_s ## State ## _e ## Event(const Message &Msg)

#define DEFINE_FSM_GUARD(Class, State, Event, Msg) \
    bool GUARD(Class, State, Event)(Fsm *pContext, const Message &Msg) \
    { Class* pThis = reinterpret_cast<Class*> (pContext->owner()); return pThis->Guard_s##State##_e##Event(Msg); } \
    bool Class::Guard_s##State##_e##Event(const Message &Msg)

#define DEFINE_FSM_STATE_ENTRY(Class, State) \
    void ENTRY(Class, State)(Fsm *pContext) \
    { Class* pThis = reinterpret_cast<Class*> (pContext->owner()); pThis->Entry_s##State(); } \
    void Class::Entry_s##State()

#define DEFINE_FSM_STATE_EXIT(Class, State) \
    void EXIT(Class, State)(Fsm *pContext) \
    { Class* pThis = reinterpret_cast<Class*> (pContext->owner()); pThis->Exit_s##State(); } \
    void Class::Exit_s##State()


// -[FSM Event]----------------------------------------------------------------
///
/// Event types definitions.
///
/// @note:  event types - this is a user defined type exclude timeout event type.
///         User must define application specific events like this (offset from USER_DEFINED):  
///             - #define EVENT_TYPE_0      (0 + FSM_EVENT_USER_DEFINED)
///             - #define EVENT_TYPE_1      (1 + FSM_EVENT_USER_DEFINED)
///             - ...
///
#define FSM_EVENT_NONE                  ((char)-1)
#define FSM_EVENT_TIMEOUT               0
#define FSM_EVENT_USER_DEFINED          1


// -[FSM Transition]-----------------------------------------------------------
///
/// Internal FSM transition type definitions.
///
/// @note:  Only for Infra::Fsm internal use.
///
#define FSM_TRANSITION_NONE_NAME            "NONE"
#define FSM_TRANSITION_NONE                 ((char)-1)
#define FSM_TRANSITION                      0
#define FSM_TRANSITION_AFTER                1
#define FSM_TRANSITION_INTERNAL             2
#define FSM_TRANSITION_INTERNAL_AFTER       3
#define FSM_TRANSITION_UNCONDITIONAL        4

// Define max timeout of after transition.
#define FSM_TRANSITION_AFTER_MAX_TIMEOUT    ((unsigned long)~0)

// Guard and Action function prototypes.
class Fsm;
class FsmEvent;
typedef bool (*FsmTransitionGuard) (Fsm *, const Message &);
typedef void (*FsmTransitionAction)(Fsm *, const Message &);


// -[Infra:FSM State]----------------------------------------------------------
///
/// FSM state type definitions.
///
/// @note:  !!! WARNING - Forbidden defining state with typeid - (-1)
///         User must define application specific states like this(offset from USER_DEFINED):
///             - #define STATE_0      (0 + FSM_STATE_USER_DEFINED)
///             - #define STATE_1      (1 + FSM_STATE_USER_DEFINED)
///             - ...
///
#define FSM_STATE_FINAL_NAME                "FINAL"
#define FSM_STATE_FINAL                     ((char)-1)    ///< !!! Don't use this define for user defined states
#define FSM_STATE_SAME                      0
#define FSM_STATE_USER_DEFINED              1

// State entry/exit functions prototypes.
typedef void (*FsmStateEntry)(Fsm *pFsmContext);
typedef void (*FsmStateExit) (Fsm *pFsmContext);


// -[FSM Definition]-----------------------------------------------------------
/// Transition definition
typedef struct FsmTransition_T
{
    const char              *Name;
    char                    Type;
    char                    EventType;
    char                    NewState;
    FsmTransitionGuard      Guard;
    FsmTransitionAction     Action;
    unsigned long           TimeOut;
} FsmTransition;

/// Default NONE transition definition.
extern const FsmTransition s_FsmTransitionNone;


/// State definition
typedef struct FsmState_T
{
    const char              *Name;
    char                    Type;
    FsmStateEntry           Entry;
    FsmStateExit            Exit;
    FsmTransition           *TransitionList;
} FsmState;

/// Final state default definitions
extern const FsmState s_FsmStateFinal;


/// Internal use: FSM loggings definition macros
# define FSM_NAME(_name)    _name, 

/// Transition list definition macros
#define TRANSITION_INTERNAL(_event, _guard, _action) \
    { FSM_NAME (#_event) FSM_TRANSITION_INTERNAL, _event, FSM_STATE_SAME, _guard, _action, FSM_TRANSITION_AFTER_MAX_TIMEOUT },

#define TRANSITION_INTERNAL_AFTER(_guard, _action, _timeout) \
    { FSM_NAME ("INTERNAL_AFTER") FSM_TRANSITION_INTERNAL_AFTER, FSM_EVENT_TIMEOUT, FSM_STATE_SAME, _guard, _action, _timeout },

#define TRANSITION(_event, _guard, _action, _new_state_type) \
    { FSM_NAME (#_event) FSM_TRANSITION, _event, _new_state_type, _guard, _action, FSM_TRANSITION_AFTER_MAX_TIMEOUT },

#define TRANSITION_AFTER(_guard, _action, _new_state_type, _timeout) \
    { FSM_NAME ("AFTER") FSM_TRANSITION_AFTER, FSM_EVENT_TIMEOUT, _new_state_type, _guard, _action, _timeout },


/// State definition macros
#define DECLARE_FSM_STATE(_state) \
    static const FsmState s_Fsm##_s##_state; \
    static const FsmTransition s_Fsm##_s##_state##_##TransitionList[]

#define DEFINE_FSM_STATE_BEGIN(Class, _state) \
    const FsmTransition Class::s_Fsm##_s##_state##_##TransitionList[] = {

#define DEFINE_FSM_STATE_END(Class, _state, _entry, _exit) \
    { FSM_NAME (FSM_TRANSITION_NONE_NAME) FSM_TRANSITION_NONE, FSM_EVENT_NONE, FSM_STATE_FINAL, 0, 0, FSM_TRANSITION_AFTER_MAX_TIMEOUT } }; \
    const FsmState Class::s_Fsm##_s##_state = \
    { FSM_NAME (#_state) _state, _entry, _exit, (FsmTransition *)Class::s_Fsm##_s##_state##_##TransitionList };


/// Macros for defining FSM.
#define FSM(_fsm)           s_Fsm##_##_fsm

#define DEFINE_FSM_BEGIN(Class, _fsm) \
    const FsmState * const Class::FSM(_fsm)[] = {

#define STATE(Class, _state) \
        &Class::s_Fsm##_s##_state,

#define DEFINE_FSM_END() \
        &s_FsmStateFinal \
    };

#define DECLARE_FSM(_fsm) \
    static const FsmState * const FSM(_fsm)[];


// ----------------------------------------------------------------------------
///
/// @class  Fsm
///
/// @brief  FSM context class.
///
// ----------------------------------------------------------------------------
/// FSM engine handle event return codes.
#define FSM_CONTEXT_STATUS_ERROR                            (-1)
#define FSM_CONTEXT_STATUS_OK                               0
#define FSM_CONTEXT_STATUS_TRANSITION_NOT_FOUND             1
#define FSM_CONTEXT_STATUS_UNCOND_TRANSITION_NOT_FOUND      2
#define FSM_CONTEXT_STATUS_EVENT_NOT_HANDLED                3
#define FSM_CONTEXT_STATUS_STATE_FINAL                      4
#define FSM_CONTEXT_STATUS_STATE_NOT_FOUND                  5

#define FSM_CONTEXT_DEFAULT_NAME                            "Fsm"
#define FSM_CONTEXT_TIMER_LIST_LEN                          8

class Fsm : public EventHandler
{
public:

    Fsm ();
    virtual ~Fsm ();

    /// Explicit init function.
    int init (const FsmState * const *pFsm,
              void                   *Owner = 0,
              Dispatcher             *pDispatcher = 0,
              const char             *Name = FSM_CONTEXT_DEFAULT_NAME);

    ///
    /// FSM context main function - handle user events and run defined state machine.
    /// Before calling this function user MUST call Init() function.
    /// User MUST call this function from well defined (single) context.
    ///
    /// @param      pEvent          Event to proceed.
    ///
    /// @return     See Infra FSM engine handle event return codes.
    ///             In case of successful event handling function returns - FSM_CONTEXT_STATUS_OK.
    ///             In another case function returns error code that user MUST check and process.
    ///
    /// @note       May enter recursively !!!
    ///
    int fireEvent(const Message &);

    int done();

    // = Get/Set methods 
    //
    /// Get User param method.
    void *owner() const { return m_Owner; }
    void  owner(void *pOwner) { m_Owner = pOwner; }

    // = Self event mechanism
    //
    // Call to <selfEvent> function will enable recursive message processing without
    // main dispatcher loop. User SHOULD call once to this function in <FSM:Entry> function,
    // <FSM:Internal> and <FSM:InternalAfter> transitions only.
    void selfEvent(const Message &selfEvent)
    { 
        ASSERT(!m_SelfEvent);
        m_SelfEvent = new Message(selfEvent);
        ASSERT(m_SelfEvent);
    }

    /// Utility
    int getCurrentState (void) const
    { 
        if (m_pFsm)
            return m_pFsm[m_CurrStateId]->Type;
        return FSM_STATE_FINAL;
    }

    static const char *statusName (int Status);

protected:

    // = Interface: EventHandler
    //
    void onMessage(const Message &msg) { fireEvent (msg); }
    void onTimeout(uintptr_t timerId, const Message &msg) { fireEvent (msg); }

    // Get the dispatcher associated with this handler
    Dispatcher *dispatcher() const { return m_Dispatcher; }

private:
    /// FSM context parameters.
    const FsmState * const  *m_pFsm;
    void                    *m_Owner;
    Dispatcher              *m_Dispatcher;
    int                     m_CurrStateId;
    const char              *m_Name;

    /// Self event
    Message                 *m_SelfEvent;
    void resetSelfEvent() { m_SelfEvent = 0; }
    int processSelfEvent()
    {
        int ret = FSM_CONTEXT_STATUS_OK;
        if (m_SelfEvent)
        {
            Message *savedSelfEvent = m_SelfEvent;
            ret = fireEvent (*m_SelfEvent);
            delete savedSelfEvent;
        }
        return ret;
    }

    // = Internal functions
    //
    int InitNewState(int StateType);
    int DoneCurrState();
    static const char *TransitionType(int Transition);
    void logRetCode (int retCode, const FsmState *state, const Message &);

    ///
    /// This function returns id of transition that wait for this event type in OUT parameter.
    /// Checks guard function of transition. In case that we reached last state's defined transition 
    /// this function returns error - INFRA_FSM_TRANSITION_NONE.
    ///
    int FindTransition(int /*OUT*/ *, const Message &);

    /// Init after transition list: schedule timer via engine object
    int ScheduleAfterTransitions();

    /// Cancel all timers and reset after transition list
    int CancelAfterTransitions();

    bool CallTransitionGuard(const FsmTransition *, const Message &);
    void CallTransitionAction(const FsmTransition *, const Message &);
    void CallStateEntry();
    void CallStateExit();
};


// ----------------------------------------------------------------------------
} // namespace perc
