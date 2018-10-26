// reference to some AOSP source code
//
// replace all AOSP's c++ classes with c++ 11 STL
//
// this file used as JNI base class to access AOSP internal class in native
// layer.


/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#pragma once

#include <string>
#include <jni.h>
#include <pthread.h>

#include "cde_log.h"

namespace irsa {

struct JNIEnvManager {
	static void init();
	static JNIEnv* getJNIEnv();

private:
    static void detach_current_thread (void *env);

	static bool sInitialized;
	static pthread_key_t current_jni_env;
};

#define GET_JNIENV()	JNIEnvManager::getJNIEnv()

/**
 * The helper class to ensure the calling thread attached in JVM to get JNIEnv properly.
 */
struct AutoJNIEnv {
public:
	AutoJNIEnv();
	AutoJNIEnv(JNIEnv* env);
	~AutoJNIEnv();

    inline JNIEnv* operator->() const { return mEnv; }
    inline JNIEnv* get() const { return mEnv; }

private:
    AutoJNIEnv(const AutoJNIEnv&); 
    AutoJNIEnv &operator=(const AutoJNIEnv&);
	inline void initEnv();

private:
	JNIEnv* mEnv;
	bool mAttached;
};


typedef struct _JNIMemberInfo {
	void** id; //jmethodID* or jfieldID*
    std::string name;
    std::string descriptor;
	bool methodElseField;
	bool isStatic;
	bool isNative;
	bool isOptional;
} JNIMemberInfo;

#define MAKE_JNI_STATIC_METHOD(name, descriptor)	{ (void **) &gMethods.name,   #name,     descriptor,  true/*methodElseField*/,   true/*isStatic*/,   false/*isNative*/,  false/*isOptional*/ }
#define MAKE_JNI_INSTANCE_METHOD(name, descriptor)	{ (void **) &gMethods.name,   #name,     descriptor,  true/*methodElseField*/,   false/*isStatic*/,  false/*isNative*/,  false/*isOptional*/ }
#define MAKE_JNI_CONSTRUCTOR(id, descriptor)		{ (void **) &gMethods.id,     "<init>",  descriptor,  true/*methodElseField*/,   false/*isStatic*/,  false/*isNative*/,  false/*isOptional*/ }

#define MAKE_JNI_STATIC_METHOD_OP(name, descriptor)	{ (void **) &gMethods.name,   #name,     descriptor,  true/*methodElseField*/,   true/*isStatic*/,   false/*isNative*/,  true/*isOptional*/  }
#define MAKE_JNI_INSTANCE_METHOD_OP(name, descriptor) { (void **) &gMethods.name, #name,     descriptor,  true/*methodElseField*/,   false/*isStatic*/,  false/*isNative*/,  true/*isOptional*/  }
#define MAKE_JNI_CONSTRUCTOR_OP(id, descriptor)		{ (void **) &gMethods.id,     "<init>",  descriptor,  true/*methodElseField*/,   false/*isStatic*/,  false/*isNative*/,  true/*isOptional*/  }

#define MAKE_JNI_STATIC_FIELD(name, descriptor)		{ (void **) &gFields.name,    #name,     descriptor,  false/*methodElseField*/,  true/*isStatic*/,   false/*isNative*/,  false/*isOptional*/ }
#define MAKE_JNI_INSTANCE_FIELD(name, descriptor)	{ (void **) &gFields.name,    #name,     descriptor,  false/*methodElseField*/,  false/*isStatic*/,  false/*isNative*/,  false/*isOptional*/ }

#define MAKE_JNI_STATIC_FIELD_OP(name, descriptor)	{ (void **) &gFields.name,    #name,     descriptor,  false/*methodElseField*/,  true/*isStatic*/,   false/*isNative*/,  true/*isOptional*/  }
#define MAKE_JNI_INSTANCE_FIELD_OP(name, descriptor) { (void **) &gFields.name,   #name,     descriptor,  false/*methodElseField*/,  false/*isStatic*/,  false/*isNative*/,  true/*isOptional*/  }

#define ENSURE_JNIENV(env)							\
			AutoJNIEnv autoEnv(env ? env : mEnv);	\
			env = autoEnv.get();


struct JNIClassBase {
public:
	JNIClassBase();
	JNIClassBase(jobject thiz);

	jobject getJObject() const { return mThis; }
	void setJObject(jobject thiz);

	void notifyJObjectChanged();

public:
	static bool loadJNIMembers(const char* className, const char* classDescriptor,
			JNIMemberInfo* members, uint32_t memberCount);

	static int32_t getSystemAPILevel();

	static bool isJavaClassAvailable(const char* classDescriptor);

	static bool checkException(bool clearException = false);
	static bool isThisException(const char* classDescriptor);

	static bool getStaticFieldValue(const char* classDescriptor, jfieldID field, std::string& outValue);
	static bool getStaticFieldValue(const char* classDescriptor, jfieldID field, int32_t& outValue);

protected:
    virtual ~JNIClassBase();

	virtual void onJObjectChanged() {}

protected:
	jobject mThis;

private:
    JNIClassBase(const JNIClassBase &); 
    JNIClassBase &operator=(const JNIClassBase &);
};

} 
