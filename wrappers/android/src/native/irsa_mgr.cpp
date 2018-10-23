// License: Apache 2.0. See LICENSE file in root directory.

#define LOG_TAG "IRSA_NATIVE_MGR"

#include <sstream>

#include "irsa_mgr.h"
#include "irsa_event.h"
#include "jni_buildversion.h"

namespace irsa {

#ifndef __SMART_POINTER__
IrsaMgr* IrsaMgr::_instance;
#else
std::shared_ptr<IrsaMgr> IrsaMgr::_instance;
#endif

#ifndef __SMART_POINTER__
IrsaMgr* IrsaMgr::getInstance() {
#else
std::shared_ptr<IrsaMgr> IrsaMgr::getInstance() {
#endif
    if (!_instance) {
#ifndef __SMART_POINTER__
        _instance = new IrsaMgr();
#else
        _instance = std::make_shared<IrsaMgr>();
#endif
    }
    return _instance;
}


IrsaMgr::IrsaMgr() {
}

IrsaMgr::~IrsaMgr() {
    LOGV("destructor");
#ifndef __SMART_POINTER__
    delete _listener;
    _listener = nullptr;
#else
    _listener.reset();
#endif
}


#ifndef __SMART_POINTER__
int IrsaMgr::setListener(IrsaMgrListener *listener) {
#else
int IrsaMgr::setListener(const std::shared_ptr<IrsaMgrListener> &listener) {
#endif
    std::lock_guard<std::mutex> lock(_lock);
	_listener = listener;
    return 0;
}


void IrsaMgr::setInfo(std::string info) {
    _info = info;
}


std::string& IrsaMgr::getInfo() {
    return _info;
}


void IrsaMgr::notify(int msg, int arg1, size_t arg2) {
    std::lock_guard<std::mutex> lock(_listenerLock);

	if (_listener != nullptr) {
		_listener->notify(msg, arg1, arg2);
    } else {
        LOGV("_listener is null");
    }
}

}
