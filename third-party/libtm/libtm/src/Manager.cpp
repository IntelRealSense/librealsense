// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#define LOG_TAG "TrackingManager"
#include "Utils.h"
#include <stdio.h>
#include <stdlib.h>
#include "Manager.h"
#include <chrono>
#include "Version.h"
#include <vector>
#include "fw.h"
#include "Common.h"

using namespace std::chrono;
using namespace std;

#define DEFAULT_CHUNKSZ 1024*32
#define EP_OUT 1
#define TIMEOUT_MS 3000

namespace perc {
// -[factory]------------------------------------------------------------------    

bool Manager::instanceExist = false;
std::mutex Manager::instanceExistMutex;

TrackingManager* TrackingManager::CreateInstance(Listener* lis, void* param)
{
    std::lock_guard<std::mutex> lock(Manager::instanceExistMutex);
    try
    {
        if (Manager::instanceExist)
        {
            LOGW("Manager instance already exist");
            return nullptr;
        }
        Manager::instanceExist = true;
        return new Manager(lis, param);
    }
    catch (std::exception& e)
    {
        Manager::instanceExist = false;
        LOGE(e.what());
        return nullptr;
    }
}

void TrackingManager::ReleaseInstance(TrackingManager*& manager)
{
    std::lock_guard<std::mutex> lock(Manager::instanceExistMutex);
    try
    {
        if (manager)
        {
            Manager::instanceExist = false;
            delete manager;
            manager = nullptr;
        }
    }
    catch (std::exception& e)
    {
        LOGE(e.what());
    }
}

// -[interface]----------------------------------------------------------------
Manager::Manager(Listener* lis, void* param) : mDispatcher(new Dispatcher()), mListener(nullptr), mContext(nullptr), mFwFileName(""), mLibUsbDeviceToTrackingDeviceMap(), mEvent(), mCompleteQMutex(), mCompleteQ(), mTrackingDeviceInfoMap()
{
    setHostLogControl({ LogVerbosityLevel::Error, LogOutputMode::LogOutputModeScreen, true });

    // start running context 
    mThread = std::thread([this] {
        mFsm.init(FSM(main), this, mDispatcher.get(), LOG_TAG);
        while (this->mDispatcher->handleEvents() >= 0);
        mFsm.done();
    });
    if (init(lis, param) != Status::SUCCESS)
    {
        throw std::runtime_error("Failed to init manager");
    }
}

Manager::~Manager()
{
    done();
}

Status Manager::onAttach(libusb_device * device)
{
    MessageON_LIBUSB_EVENT msg(true, device);
    mFsm.fireEvent(msg);
    if (msg.Result == 0)
        return Status::SUCCESS;
    return Status::INIT_FAILED;
}

void Manager::onDetach(libusb_device * device)
{
    mFsm.fireEvent(MessageON_LIBUSB_EVENT(false, device));
}

void Manager::addTask(std::shared_ptr<CompleteTask>& newTask)
{
    //mDispatcher->postMessage(&mFsm, MessageON_NEW_TASK(newTask));
    lock_guard<mutex> lock(mCompleteQMutex);
    mCompleteQ.push_back(newTask);
    mEvent.signal();
}

Status Manager::init(Listener *listener, void *param)
{
    return processMessage(MessageON_INIT(listener, param));
}

void Manager::removeTasks(void * owner, bool completeTasks)
{
    mFsm.fireEvent(MessageON_REMOVE_TASKS(owner, completeTasks));
}

// ----------------------------------------------------------------------------
Status Manager::handleEvents(bool blocking)
{
    shared_ptr<CompleteTask> task = nullptr;
    size_t count;

    {
        lock_guard<mutex> lock(mCompleteQMutex);
        count = mCompleteQ.size();
        mEvent.reset();
    }
    
    LOGV("Manager::handleEvents is about to handle %d tasks", count);
    while (true)
    {
        task = nullptr;
        {
            lock_guard<mutex> lock(mCompleteQMutex);
            if ( count > 0 && mCompleteQ.size() > 0 )
            {
                task = mCompleteQ.front();
                mCompleteQ.pop_front();
                count--;
            }
        }
        if (task)
            task->complete();
        else
            break;
    }
    return Status::SUCCESS;
}


// ----------------------------------------------------------------------------
void Manager::done()
{
    processMessage(Message(ON_DONE));

    if (mThread.joinable()) {
        try {
            mDispatcher->endEventsLoop();
            mThread.join();
        }
        catch (...) {
            LOGE("mThread.join() exception");
        }
    }

    for (auto item : mLibUsbDeviceToTrackingDeviceMap)
    {
        delete item.second;
        libusb_unref_device(item.first);
    }
}

size_t Manager::getDeviceList(TrackingDevice ** list, unsigned int maxListSize)
{
    // TODO: move to sendmessage and dispatcher context
    auto count = maxListSize > mLibUsbDeviceToTrackingDeviceMap.size() ? mLibUsbDeviceToTrackingDeviceMap.size() : maxListSize;

    size_t i = 0;
    for (auto item : mLibUsbDeviceToTrackingDeviceMap)
    {
        *list++ = item.second;
        i--;
        if (i == count)
            break;
    }
    return count;
}

Status Manager::setHostLogControl(IN const TrackingData::LogControl& logControl)
{
    //LOGF("Set Host log Control, output to %s, verbosity = 0x%X, Rollover mode = 0x%X", (logControl.outputMode == LogOutputModeScreen)?"Screen":"Buffer", logControl.verbosity, logControl.rolloverMode);
    __perc_Log_Set_Configuration(LogSourceHost, logControl.outputMode, logControl.verbosity, logControl.rolloverMode);
    return Status::SUCCESS;
}

Status Manager::getHostLog(OUT TrackingData::Log* log)
{
    __perc_Log_Get_Log((void*)log);
    return Status::SUCCESS;
}

// -[impl]---------------------------------------------------------------------
Status Manager::processMessage(const Message &msg, int prio)
{
    msg.Result = toUnderlying(Status::COMMON_ERROR);

    int ret = mDispatcher->sendMessage(&mFsm, msg, prio);
    if (ret < 0) {
        LOGE("mDispatcher->sendMessage ret %d", ret);
        return Status::COMMON_ERROR;
    }
    return (Status)msg.Result;
}



Status Manager::loadFileToDevice(libusb_device * device, const char * fileName)
{
    Status status = Status::SUCCESS;

    if (!fileName || !device)
    {
        LOGE("Error while loading file %s to device 0x%p - bad input", fileName, device);
        return Status::INIT_FAILED;
    }

    FILE* file;
    auto res = fopen_s(&file, fileName, "rb");
    if (res != 0)
    {
        LOGE("Error while loading file %s to device 0x%p - can't open file", fileName, device);
        return Status::INIT_FAILED;
    }

    unsigned int size;

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    unsigned char* txBuf;

    if (!(txBuf = (unsigned char*)malloc(size)))
    {
        LOGE("Error while loading file %s (size %d) to device 0x%p - memory allocation error", fileName, size, device);
        status = Status::INIT_FAILED;
        goto cleanup;
    }

    if (fread(txBuf, 1, size, file) != size)
    {
        LOGE("Error while loading file %s (size %d) to device 0x%p - read file error", fileName, size, device);
        status = Status::INIT_FAILED;
        goto cleanup;
    }

    status = loadBufferToDevice(device, txBuf, size);

cleanup: 
    fclose(file);
    if (txBuf)
    {
        free(txBuf);
    }
    return status;
}


Status Manager::loadBufferToDevice(libusb_device * device, unsigned char * buffer, size_t size)
{
    LOGV("loading image to device 0x%p", device);

    if (!device || !buffer || size == 0)
    {
        LOGE("Error while loading image - device 0x%p, buffer 0x%p, size %d", device, buffer, size);
        return Status::INIT_FAILED;
    }

    libusb_device_handle * devHandle;
    auto start = high_resolution_clock::now();
    uint64_t duration = 0;
    int bytesWritten = 0;

    auto rc = libusb_open(device, &devHandle);
    if (rc != 0)
    {
        LOGE("Error while loading image - libusb_open failed (0x%X), will try again later", rc);
        return Status::INIT_FAILED;
    }

    rc = libusb_claim_interface(devHandle, INTERFACE_INDEX);
    if (rc != 0)
    {
        LOGE("Error while loading image - libusb_claim_interface failed (0x%X)", rc);
        libusb_close(devHandle);
        return Status::INIT_FAILED;
    }

    rc = libusb_bulk_transfer(devHandle, EP_OUT, buffer, (int)size, &bytesWritten, TIMEOUT_MS);
    if (rc != 0 || size != bytesWritten)
    {
        LOGE("Error while loading image - libusb_bulk_transfer failed. LIBUSB_ERROR_CODE: %d (%s)", rc, libusb_error_name(rc));
        libusb_release_interface(devHandle, INTERFACE_INDEX);
        libusb_close(devHandle);
        return Status::INIT_FAILED;
    }

    //W\A for pipe error bug
    rc = libusb_bulk_transfer(devHandle, EP_OUT, buffer, 0, &bytesWritten, TIMEOUT_MS);
    // status check ignored according to Movidius sample code
    //if (rc != 0 || 0 != bytesWritten)
    //{
    //    LOGE("Error while loading image - libusb_bulk_transfer failed. LIBUSB_ERROR_CODE: %d (%s)", rc, libusb_error_name(rc));
    //    libusb_release_interface(devHandle, INTERFACE_INDEX);
    //    libusb_close(devHandle);
    //    return Status::INIT_FAILED;
    //}

    duration = duration_cast<milliseconds>(high_resolution_clock::now() - start).count();
    LOGD("USB Device FW Load finish - FW image loaded in %d mSec", duration);

    libusb_release_interface(devHandle, INTERFACE_INDEX);
    libusb_close(devHandle);
    return Status::SUCCESS;
}

uint64_t Manager::version()
{
    return LIBTM_VERSION;
}
// -[FSM]----------------------------------------------------------------------
//
// [FSM State::ANY]

// [FSM State::IDLE]
DEFINE_FSM_STATE_ENTRY(Manager, IDLE_STATE)
{
}

DEFINE_FSM_GUARD(Manager, IDLE_STATE, ON_INIT, msg)
{
    return 0 == libusb_init(&mContext);
}

DEFINE_FSM_ACTION(Manager, IDLE_STATE, ON_INIT, msg)
{
    auto initMsg = dynamic_cast<const MessageON_INIT&>(msg);
    mListener = initMsg.listener;
    char* t = (char*)initMsg.context;
    if (t)
    {
        string tmp(t);
        mFwFileName = tmp;
    }
    msg.Result = toUnderlying(Status::SUCCESS);
}

// [FSM State::ACTIVE]
DEFINE_FSM_STATE_ENTRY(Manager, ACTIVE_STATE)
{
    mUsbPlugListener = make_shared<UsbPlugListener>(*this);
}

DEFINE_FSM_STATE_EXIT(Manager, ACTIVE_STATE)
{
}

DEFINE_FSM_GUARD(Manager, ACTIVE_STATE, ON_REMOVE_TASKS, msg)
{
    auto m = dynamic_cast<const MessageON_REMOVE_TASKS&>(msg);
    if (!m.mOwner)
        return false;
    return true;
}

DEFINE_FSM_ACTION(Manager, ACTIVE_STATE, ON_REMOVE_TASKS, msg)
{
    lock_guard<mutex> lock(mCompleteQMutex);
    uint32_t nonCompleteTasks = 0;

    auto it = mCompleteQ.begin();
    auto m = dynamic_cast<const MessageON_REMOVE_TASKS&>(msg);

    while (it != mCompleteQ.end())
    {
        if ((*it)->getOwner() == m.mOwner)
        {
            auto itTmp = it;
            it++;

            /* Check if asked to complete all pending tasks (upon exiting from ASYNC state) or this is a mustComplete tasks (status / error / localization / fw update tasks) */
            if ((m.mCompleteTasks == true) || (itTmp->get()->mustComplete() == true))
            {
                itTmp->get()->complete();
            }
            else
            {
                nonCompleteTasks++;
            }

            mCompleteQ.erase(itTmp);
            continue;
        }
        it++;
    }

    if (nonCompleteTasks > 0)
    {
        /* If there are non completed tasks in this queue, this means that some of the callbacks (onVideo, onPose, onGyro, onAccelerometer) have a high latency */
        /* Which prevent the processor from processing all tasks in this queue while on ACTIVE state, which means loosing callbacks data                        */
        LOGE("Stopping Manager - Cleaned %d non complete callbacks (onVideoFrame, onPoseFrame, onGyroFrame, onAccelerometerFrame, onControllerframe) - latency is too high!", nonCompleteTasks);
    }

}

DEFINE_FSM_GUARD(Manager, ACTIVE_STATE, ON_NEW_TASK, msg)
{
    auto m = dynamic_cast<const MessageON_NEW_TASK&>(msg);
    if (!m.task)
        return false;
    return true;
}

DEFINE_FSM_ACTION(Manager, ACTIVE_STATE, ON_NEW_TASK, msg)
{
/*    lock_guard<mutex> lock(mCompleteQMutex);
    auto m = dynamic_cast<const MessageON_NEW_TASK&>(msg);
    mCompleteQ.push_back(m.task);
    mEvent.signal();*/
}

DEFINE_FSM_ACTION(Manager, ACTIVE_STATE, ON_ATTACH, msg)
{
    Status status = Status::SUCCESS;
    msg.Result = toUnderlying(Status::ERROR_USB_TRANSFER);
    
    auto m = dynamic_cast<const MessageON_LIBUSB_EVENT&>(msg);
    libusb_device_descriptor desc = { 0 };

    auto rc = libusb_get_device_descriptor(m.device, &desc);
    if (rc)
    {
        LOGE("Error while getting device descriptor. LIBUSB_ERROR_CODE: %d (%s)", rc, libusb_error_name(rc));
        return;
    }

    /* USB Device Firmware Upgrade (DFU) */
    if (mUsbPlugListener->identifyDFUDevice(&desc) == true)
    {
        libusb_ref_device(m.device);

        // load firmware here
        if (mFwFileName != "")
        {
            LOGD("USB Device FW Load start - loading external FW image from %s", mFwFileName.c_str());
            status = loadFileToDevice(m.device, mFwFileName.c_str());
        }
        else
        {
            LOGD("USB Device FW Load start - loading internal FW image \"%s\"", FW_VERSION);
            auto size = sizeof(target_hex);
            status = loadBufferToDevice(m.device, (unsigned char*)target_hex, size);
        }
    }

    if (status != Status::SUCCESS)
    {
        msg.Result = toUnderlying(Status::INIT_FAILED);
        return;
    }

    /* Real device arrived - do something */
    if (mUsbPlugListener->identifyDevice(&desc) == true)
    {
        /* To debug libusb issues, enable logs using libusb_set_debug(mContext, LIBUSB_LOG_LEVEL_INFO) */

        auto device = new Device(m.device, mDispatcher.get(), this, this);
        if (device->IsDeviceReady() == false)
        {
            delete device;
            msg.Result = toUnderlying(Status::INIT_FAILED);
            return;
        }

        libusb_ref_device(m.device);
        mLibUsbDeviceToTrackingDeviceMap[m.device] = device;

        shared_ptr<CompleteTask> ptr;
        TrackingData::DeviceInfo deviceInfo;
        device->GetDeviceInfo(deviceInfo);
        mTrackingDeviceInfoMap[device] = deviceInfo;

        ptr = make_shared<UsbCompleteTask>(mListener, device, &deviceInfo, ATTACH, this);
        this->addTask(ptr);

        if (deviceInfo.status.hw != Status::SUCCESS)
        {
            ptr = make_shared<ErrorTask>(mListener, device, deviceInfo.status.hw, this);
            this->addTask(ptr);
        }
        else if (deviceInfo.status.host != Status::SUCCESS)
        {
            ptr = make_shared<ErrorTask>(mListener, device, deviceInfo.status.host, this);
            this->addTask(ptr);
        }

        mDispatcher->registerHandler(device);
    }
    //libusb_ref_device(m.device);
    msg.Result = toUnderlying(Status::SUCCESS);
}

DEFINE_FSM_ACTION(Manager, ACTIVE_STATE, ON_DETACH, msg)
{
    msg.Result = toUnderlying(Status::ERROR_USB_TRANSFER);
    libusb_device_descriptor desc = { 0 };
    auto m = dynamic_cast<const MessageON_LIBUSB_EVENT&>(msg);
    auto rc = libusb_get_device_descriptor(m.device, &desc);
    if (rc)
    {
        LOGE("Error while getting device descriptor. LIBUSB_ERROR_CODE: %d (%s)", rc, libusb_error_name(rc));
        return;
    }

    if (mUsbPlugListener->identifyDevice(&desc) == true)
    {
        auto device = mLibUsbDeviceToTrackingDeviceMap[m.device];
        mLibUsbDeviceToTrackingDeviceMap.erase(m.device);
        mDispatcher->removeHandler(device);

        auto deviceInfo = mTrackingDeviceInfoMap[device];
        mTrackingDeviceInfoMap.erase(device);

        // notify listener on device erased
        shared_ptr<CompleteTask> ptr = make_shared<UsbCompleteTask>(mListener, device, &deviceInfo, DETACH, this);
        this->addTask(ptr);
        delete device;
        libusb_unref_device(m.device);
    }
    else if (mUsbPlugListener->identifyDFUDevice(&desc) == true)
    {
        libusb_unref_device(m.device);
    }

    //libusb_unref_device(m.device);
    msg.Result = toUnderlying(Status::SUCCESS);
}

DEFINE_FSM_ACTION(Manager, ACTIVE_STATE, ON_DONE, msg)
{
    msg.Result = toUnderlying(Status::SUCCESS);
}

DEFINE_FSM_ACTION(Manager, ACTIVE_STATE, ON_ERROR, msg)
{
    msg.Result = toUnderlying(Status::SUCCESS);
}

// [FSM State::ERROR]
DEFINE_FSM_ACTION(Manager, ERROR_STATE, ON_DONE, msg)
{
}
 
// -[Definition]---------------------------------------------------------------
#define FSM_ERROR_TIMEOUT               1000

// [FSM State::IDLE]
DEFINE_FSM_STATE_BEGIN(Manager, IDLE_STATE)
    TRANSITION(ON_INIT, GUARD(Manager, IDLE_STATE, ON_INIT), ACTION(Manager, IDLE_STATE, ON_INIT), ACTIVE_STATE)
DEFINE_FSM_STATE_END(Manager, IDLE_STATE, ENTRY(Manager, IDLE_STATE), 0)

// [FSM State::ACTIVE]
DEFINE_FSM_STATE_BEGIN(Manager, ACTIVE_STATE)
    TRANSITION(ON_DONE, 0, ACTION(Manager, ACTIVE_STATE, ON_DONE), IDLE_STATE)
    TRANSITION_INTERNAL(ON_ATTACH, 0, ACTION(Manager, ACTIVE_STATE, ON_ATTACH))
    TRANSITION_INTERNAL(ON_DETACH, 0, ACTION(Manager, ACTIVE_STATE, ON_DETACH))
    TRANSITION_INTERNAL(ON_NEW_TASK, GUARD(Manager, ACTIVE_STATE, ON_NEW_TASK), ACTION(Manager, ACTIVE_STATE, ON_NEW_TASK))
    TRANSITION_INTERNAL(ON_REMOVE_TASKS, GUARD(Manager, ACTIVE_STATE, ON_REMOVE_TASKS), ACTION(Manager, ACTIVE_STATE, ON_REMOVE_TASKS))
    TRANSITION(ON_ERROR, 0, ACTION(Manager, ACTIVE_STATE, ON_ERROR), ERROR_STATE)
DEFINE_FSM_STATE_END(Manager, ACTIVE_STATE, ENTRY(Manager, ACTIVE_STATE), EXIT(Manager, ACTIVE_STATE))

// [FSM State::ERROR] 
DEFINE_FSM_STATE_BEGIN(Manager, ERROR_STATE)
    TRANSITION(ON_DONE, 0, ACTION(Manager, ERROR_STATE, ON_DONE), IDLE_STATE)
DEFINE_FSM_STATE_END(Manager, ERROR_STATE, 0, 0)

// -[FSM: Definition]----------------------------------------------------------
DEFINE_FSM_BEGIN(Manager, main)
    STATE(Manager, IDLE_STATE)
    STATE(Manager, ACTIVE_STATE)
    STATE(Manager, ERROR_STATE)
DEFINE_FSM_END()

// ----------------------------------------------------------------------------
} // namespace tracking
