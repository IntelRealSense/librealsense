#pragma once

#include "Core.h"
#include "Engine.h"

#include <exception>
#include <vector>
#include <map>

#include "RealSenseNative.h"
#include "RealSensePlugin.h"

#pragma warning(disable: 4101) // unreferenced local variable

DECLARE_LOG_CATEGORY_EXTERN(LogRealSense, Log, All);
#define REALSENSE_ERR(Format, ...) UE_LOG(LogRealSense, Error, Format, ## __VA_ARGS__)

#if UE_BUILD_SHIPPING
	#define REALSENSE_TRACE(Format, ...)
#else
	#define REALSENSE_TRACE(Format, ...) UE_LOG(LogRealSense, Display, Format, ## __VA_ARGS__)
#endif

#if 0
	// DEV MODE
	#define PROF_ENABLED
	#define PROF_CHAR TCHAR
	#define PROF_TEXT _T
	#define PROF_LOG(Format, ...) UE_LOG(LogRealSense, Display, Format, __VA_ARGS__)
	#include "Profiler.h"
#else
	#define INIT_PROFILER
	#define SHUT_PROFILER
	#define NAMED_PROFILER(name)
	#define SCOPED_PROFILER
#endif

inline FString uestr(const char* str) { return FString(ANSI_TO_TCHAR(str)); }
inline FString uestr(const std::string& str) { return FString(ANSI_TO_TCHAR(str.c_str())); }

template<typename T>
inline T* NewNamedObject(UObject* Owner, const TCHAR* Name)
{
	int Id = IRealSensePlugin::Get().GenId();
	auto UniqName = FString::Printf(TEXT("%s_UID%d"), Name, Id);
	//REALSENSE_TRACE(TEXT("NewObject %s"), *UniqName);
	auto Object = NewObject<T>((Owner ? Owner : GetTransientPackage()), FName(*UniqName), RF_NoFlags); // RF_Standalone
	return Object;
}

class FScopedTryLock
{
public:
	FScopedTryLock() {}
	~FScopedTryLock() { Unlock(); }
	bool TryLock(FCriticalSection* Mutex) { Unlock(); if (Mutex->TryLock()) { _Mutex = Mutex; return true; } return false; }
	void Unlock() { if (_Mutex) { _Mutex->Unlock(); _Mutex = nullptr; } }
	bool IsLocked() const { return _Mutex ? true : false; }
private:
	FScopedTryLock(const FScopedTryLock&);
	void operator=(const FScopedTryLock&);
	FCriticalSection* _Mutex = nullptr;
};
