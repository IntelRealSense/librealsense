// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <memory>
#include <map>
#include <string>
#include <functional>
#include <thread>
#include "Dispatcher.h"
#include "Fsm.h"
#include "TrackingManager.h"
#include "Device.h"
#include "TrackingCommon.h"
#include "UsbPlugListener.h"
#include "Event.h"
#include "CompleteTask.h"
#include <list>
#include <mutex>

namespace perc
{
    class Manager : public TrackingManager,
                    public EventHandler,
                    public CompleteQueueHandler,
                    public UsbPlugListener::Owner
    {
    public:

        Manager(Listener*, void* param = 0);
        virtual ~Manager();
        // [interface] TrackingManager
        virtual Handle getHandle() override { return mEvent.handle(); };
        virtual Status handleEvents(bool blocking) override;
        virtual size_t getDeviceList(TrackingDevice** list, unsigned int maxListSize) override;
        virtual Status setHostLogControl(IN const TrackingData::LogControl& logControl) override;
        virtual Status getHostLog(OUT TrackingData::Log* log) override;
        virtual uint64_t version() override;

        // [interface] CompleteQueueHandler
        virtual void addTask(std::shared_ptr<CompleteTask>&) override;
        virtual void removeTasks(void* owner, bool completeTasks) override;

        static std::mutex instanceExistMutex;
        static bool instanceExist;

        bool isInitialized() const override { return mUsbPlugListener->isInitialized(); }

    protected:
        std::thread mThread;
        std::shared_ptr<Dispatcher> mDispatcher;

        // [interface] EventHandler
        virtual void onMessage(const Message &) override {}
        virtual void onTimeout(uintptr_t timerId, const Message &msg) override {}
        virtual void onRead(int fd, void *act) {}
        virtual void onException(int fd, void *act) {}

        // [interface] UsbPlugListener::Owner
        virtual Status onAttach(libusb_device*) override;
        virtual void onDetach(libusb_device*) override;
        virtual libusb_context* context()  override { return mContext; }
        virtual Dispatcher& dispatcher() override { return *mDispatcher.get(); }


        // [FSM] declaration
        Fsm mFsm;
        DECLARE_FSM(main);

        // [States]
        enum {
            IDLE_STATE = FSM_STATE_USER_DEFINED,
            ACTIVE_STATE,
            ERROR_STATE,
        };

        // [Events]
        enum {
            ON_INIT = FSM_EVENT_USER_DEFINED,
            ON_DONE,
            ON_ATTACH,
            ON_DETACH,
            ON_ERROR,
            ON_NEW_TASK,
            ON_REMOVE_TASKS
        };

        // [State] IDLE
        DECLARE_FSM_STATE(IDLE_STATE);
        DECLARE_FSM_STATE_ENTRY(IDLE_STATE);
        DECLARE_FSM_GUARD(IDLE_STATE, ON_INIT);
        DECLARE_FSM_ACTION(IDLE_STATE, ON_INIT);
        
        // [State] ACTIVE
        DECLARE_FSM_STATE(ACTIVE_STATE);
        DECLARE_FSM_STATE_ENTRY(ACTIVE_STATE);
        DECLARE_FSM_STATE_EXIT(ACTIVE_STATE);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_ATTACH);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_DETACH);
        DECLARE_FSM_GUARD(ACTIVE_STATE, ON_NEW_TASK);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_NEW_TASK);
        DECLARE_FSM_GUARD(ACTIVE_STATE, ON_REMOVE_TASKS);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_REMOVE_TASKS);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_DONE);
        DECLARE_FSM_ACTION(ACTIVE_STATE, ON_ERROR);

        // [State] ERROR
        DECLARE_FSM_STATE(ERROR_STATE);
        DECLARE_FSM_ACTION(ERROR_STATE, ON_DONE);

        // private messages
        class MessageON_INIT: public Message {
        public:
            MessageON_INIT(TrackingManager::Listener *l, void *c) : Message(ON_INIT),
                listener(l), context(c) {}
            TrackingManager::Listener *listener;
            void *context;
        };

        class MessageON_LIBUSB_EVENT : public Message {
        public:
            MessageON_LIBUSB_EVENT(bool attach, libusb_device* dev) : Message(attach ? ON_ATTACH : ON_DETACH), device(dev)
            { }
            libusb_device* device;
        };

        class MessageON_NEW_TASK : public Message {
        public:
            MessageON_NEW_TASK(std::shared_ptr<CompleteTask>& newTask) : Message(ON_NEW_TASK), task(newTask)
            { }
            std::shared_ptr<CompleteTask> task;
        };
        
        class MessageON_REMOVE_TASKS : public Message {
        public:
            MessageON_REMOVE_TASKS(void * owner, bool completeTasks) : Message(ON_REMOVE_TASKS), mOwner(owner), mCompleteTasks(completeTasks)
            { }
            void* mOwner;
            bool mCompleteTasks;
        };

        Status processMessage(const Message &, int prio = Dispatcher::PRIORITY_IDLE);

    private:

        Status loadBufferToDevice(libusb_device* device, unsigned char* buffer, size_t size);
        Status loadFileToDevice(libusb_device* device, const char* fileName);
        Status init(TrackingManager::Listener*, void*);
        void done();

        TrackingManager::Listener* mListener;
        std::map<libusb_device*,Device*> mLibUsbDeviceToTrackingDeviceMap;
        libusb_context * mContext;
        
        std::string mFwFileName;
        std::shared_ptr<UsbPlugListener> mUsbPlugListener;
        std::list<std::shared_ptr<CompleteTask>> mCompleteQ;
        std::mutex mCompleteQMutex;
        Event mEvent;
        std::map<Device*, TrackingData::DeviceInfo> mTrackingDeviceInfoMap;
    };
}
