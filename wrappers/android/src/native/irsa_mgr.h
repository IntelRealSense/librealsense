// License: Apache 2.0. See LICENSE file in root directory.

#pragma once

#include <mutex>
#include <memory>
#include "cde_log.h"

namespace irsa {

class IrsaMgrListener {
public:
    virtual void notify(int msg, int arg1, size_t arg2) = 0;
};


class IrsaMgr {
public:
#ifndef __SMART_POINTER__
	static IrsaMgr *getInstance();
#else
	static std::shared_ptr<IrsaMgr> getInstance();
#endif
    IrsaMgr();
    ~IrsaMgr();

	void        notify(int msg, int arg1, size_t arg2);

#ifndef __SMART_POINTER__
	int         setListener(IrsaMgrListener *listener);
#else
	int         setListener(const std::shared_ptr<IrsaMgrListener>& listener);
#endif

    void        setInfo(std::string info);
    std::string& getInfo();

private:
    std::mutex  _lock;
    std::mutex  _listenerLock;

#ifndef __SMART_POINTER__
    IrsaMgrListener* _listener;
    static IrsaMgr *_instance;
#else
    std::shared_ptr<IrsaMgrListener> _listener;
    static std::shared_ptr<IrsaMgr> _instance;
#endif
    std::string _info;
};

}
