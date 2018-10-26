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

#define LOG_TAG "IRSA_JNI_UTILS"

#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <sys/prctl.h>

#include "jni_utils.h"

extern JavaVM * g_jvm;
extern "C" typedef void (*CALLBACK)(void *);

namespace irsa {

pthread_key_t JNIEnvManager::current_jni_env;
bool JNIEnvManager::sInitialized = false;

void JNIEnvManager::init()
{
	if(!sInitialized)
	{
		int r = pthread_key_create (&current_jni_env, (CALLBACK)detach_current_thread);
		sInitialized = true;
	}
}

void JNIEnvManager::detach_current_thread (void *env)
{
#ifdef __IRSA_DEBUG_JNI__
	char name[256] = {0};
	prctl(PR_GET_NAME, name, 0, 0, 0); 
	LOGD("detach_current_thread from thread: %d, name \"%s\"", gettid(), name);
#endif
	g_jvm->DetachCurrentThread ();
}  

JNIEnv* JNIEnvManager::getJNIEnv() {
#ifdef __IRSA_DEBUG_JNI__
    auto a = std::chrono::steady_clock::now();
#endif

	 CHECK(sInitialized == true);

     JNIEnv *env = NULL;
     if ((env = (JNIEnv *)pthread_getspecific (current_jni_env)) == NULL)
     {
         if (JNI_OK == g_jvm->AttachCurrentThread(&env, NULL))
         {
             int r = pthread_setspecific (current_jni_env, env);
#ifdef __IRSA_DEBUG_JNI__
			 char name[256] = {0};
			 prctl(PR_GET_NAME, name, 0, 0, 0); 
             LOGD("pthread_setspecific() to thread: %d, name: \"%s\", result: %d", gettid(), name, r);
#endif
         }
         else
         {
             LOGE("FAILED to attach current thread!");
             TRESPASS();
         }
     }

#ifdef __IRSA_DEBUG_JNI__
    auto b = std::chrono::steady_clock::now();
    std::chrono::microseconds delta = std::chrono::duration_cast<std::chrono::microseconds>(b - a);
    LOGD("duration of GetEnv is %d microseconds\n", delta.count());
#endif

	 CHECK(env != NULL);
     return env;

}


AutoJNIEnv::AutoJNIEnv()
		: mEnv(NULL)
		, mAttached(false) {
	initEnv();
}

AutoJNIEnv::AutoJNIEnv(JNIEnv *env)
		: mEnv(env)
		, mAttached(false) {
	initEnv();
}

AutoJNIEnv::~AutoJNIEnv() {
	if (mAttached) {
#ifdef __IRSA_DEBUG_JNI__
    auto a = std::chrono::steady_clock::now();
#endif

	g_jvm->DetachCurrentThread();

#ifdef __IRSA_DEBUG_JNI__
    auto b = std::chrono::steady_clock::now();
    std::chrono::microseconds delta = std::chrono::duration_cast<std::chrono::microseconds>(b - a);
    LOGD("duration of GetEnv is %d microseconds\n", delta.count());
#endif
	} else {
        //LOGV("not attached");
    }
}

void AutoJNIEnv::initEnv() {
	if (mEnv)
        return;

	CHECK(g_jvm != NULL);
#ifdef __IRSA_DEBUG_JNI__
    auto a = std::chrono::steady_clock::now();
#endif

	jint ret = g_jvm->GetEnv((void **) &mEnv, JNI_VERSION_1_6);
#ifdef __IRSA_DEBUG_JNI__
    auto b = std::chrono::steady_clock::now();
    std::chrono::microseconds delta = std::chrono::duration_cast<std::chrono::microseconds>(b - a);
    LOGD("duration of GetEnv is %d microseconds\n", delta.count());
#endif

	switch (ret) {
		case JNI_OK:
			break;

		case JNI_EDETACHED:
		{
			if (JNI_OK == g_jvm->AttachCurrentThread(&mEnv, NULL))
				mAttached = true;
			else
				LOGE("FAILED to attach current thread!");

			break;
		}

		case JNI_EVERSION:
		default:
			LOGE("FAILED to get JNIEnv!");
			break;
	}

	CHECK(mEnv != NULL);
}


JNIClassBase::JNIClassBase()
		: mThis(NULL) {
}

JNIClassBase::JNIClassBase(jobject thiz)
		: mThis(NULL) {
	setJObject(thiz);
}

JNIClassBase::~JNIClassBase() {
	JNIEnv *env = GET_JNIENV();

	if (mThis)
		env->DeleteGlobalRef(mThis);
}

void JNIClassBase::setJObject( jobject thiz) {
	if (thiz == mThis)
		return;

	JNIEnv *env = GET_JNIENV();

	if (mThis) {
		env->DeleteGlobalRef(mThis);
		mThis = NULL;
	}

	if (thiz)
		mThis = env->NewGlobalRef(thiz);

	notifyJObjectChanged();
}

void JNIClassBase::notifyJObjectChanged() {
	onJObjectChanged();
}


static bool getPublicMembers(const char* classDescriptor, std::vector<JNIMemberInfo>& members);
static bool findMember(const std::vector<JNIMemberInfo>& allMembers, const JNIMemberInfo& member, bool& outIsNative);

bool JNIClassBase::loadJNIMembers(const char* className, const char* classDescriptor,
		JNIMemberInfo* members, uint32_t memberCount) {
	if (!className || !classDescriptor || !members || !memberCount)
		return false;

	JNIEnv *env = GET_JNIENV();

    std::vector<JNIMemberInfo> allMembers;
	if (!getPublicMembers(classDescriptor, allMembers)) {
		LOGE("FAILED to get public members for Java class: %s", classDescriptor);
		return false;
	}

	jclass cls = env->FindClass(classDescriptor);
	if (!cls) {
		checkException(true);
		LOGE("CANNOT find Java class: %s", classDescriptor);
		return false;
	}

	LOGD("\n------------- Member infos: %s -----------------", classDescriptor);
	LOGD("methodElseField isStatic isNative isOptional name \tdescriptor");

	bool ret = true;
	for (uint32_t i = 0; i < memberCount; i++) {
		JNIMemberInfo& member = members[i];

		if (member.methodElseField) { // method
			jmethodID* id = (jmethodID *) member.id;
			CHECK(id != NULL);

			if (member.isStatic)
				*id = env->GetStaticMethodID(cls, member.name.c_str(), member.descriptor.c_str());
			else
				*id = env->GetMethodID(cls, member.name.c_str(), member.descriptor.c_str());

			if (!*id) {
				LOGW("CANNOT find Java method: %s %s", classDescriptor, member.name.c_str());
                JNIClassBase::checkException(true);

				if (!member.isOptional) {
					ret = false;
					break;
				}
			}
		} else { // field
			jfieldID* id = (jfieldID *) member.id;
			CHECK(id != NULL);

			if (member.isStatic)
				*id = env->GetStaticFieldID(cls, member.name.c_str(), member.descriptor.c_str());
			else
				*id = env->GetFieldID(cls, member.name.c_str(), member.descriptor.c_str());

			if (!*id) {
				LOGW("CANNOT find Java field: %s %s", classDescriptor, member.name.c_str());
                JNIClassBase::checkException(true);

				if (!member.isOptional) {
					ret = false;
					break;
				}
			}
		}

		bool isNative = false;
		if (findMember(allMembers, member, isNative)) {
			member.isNative = isNative;
		} else {
			LOGW("CANNOT find Java member: %s %s", classDescriptor, member.name.c_str());
			if (!member.isOptional) {
				ret = false;
				break;
			}
		}

		LOGD("%-2d %-2d %-2d %-2d %-s %s",
				member.methodElseField, member.isStatic, member.isNative, member.isOptional,
				member.name.c_str(), member.descriptor.c_str());
	}

	LOGD("------------- End of Member infos: %s -----------------\n", classDescriptor);

	env->DeleteLocalRef(cls);

	return ret;
}

int32_t JNIClassBase::getSystemAPILevel() {
	// http://developer.android.com/reference/android/os/Build.VERSION.html
	static const char* classDesc = "android/os/Build$VERSION";
	static const char* fieldName = "SDK_INT";
	static const char* fieldDesc = "I";

	int32_t level = -1;

	JNIEnv *env = GET_JNIENV();
	jclass cls = env->FindClass(classDesc);
	if (!cls) {
		checkException(true);
		LOGE("CANNOT find Java class: %s", classDesc);
		return level;
	}

	jfieldID fid = env->GetStaticFieldID(cls, fieldName, fieldDesc);
	if (fid)
		level = env->GetStaticIntField(cls, fid);
	else
		LOGE("CANNOT find Java field: %s %s", classDesc, fieldName);

	env->DeleteLocalRef(cls);

	return level;
}

bool JNIClassBase::isJavaClassAvailable( const char* classDescriptor) {
	if (!classDescriptor)
		return false;

	JNIEnv *env = GET_JNIENV();
	jclass cls = env->FindClass(classDescriptor);
	bool ret = (cls != NULL);

	checkException(true);
	env->DeleteLocalRef(cls);

	return ret;
}

bool JNIClassBase::checkException( bool clearException) {
	JNIEnv *env = GET_JNIENV();
	jthrowable exc = env->ExceptionOccurred();
	if (exc) {
		if (clearException) {
#ifdef __IRSA_DEBUG_JNI__
			env->ExceptionDescribe();
#endif
			env->ExceptionClear();
		}

		env->DeleteLocalRef(exc);
		return true;
    }

	return false;
}

bool JNIClassBase::isThisException(const char* classDescriptor) {
    if (classDescriptor == NULL) {
        return false;
    }

    bool ret = false;
	JNIEnv *env = GET_JNIENV();
	jthrowable exc = env->ExceptionOccurred();
	if (exc) {
#ifdef __IRSA_DEBUG_JNI__
        env->ExceptionDescribe();
#else
	    env->ExceptionClear();
#endif        
        jclass cls = env->FindClass(classDescriptor);
        if (cls != NULL) {
            ret = env->IsInstanceOf(exc, cls);
            env->DeleteLocalRef(cls);
        } else {
            LOGE("CANNOT find Java class: %s", classDescriptor);
        }
    }

	return ret;
}

bool JNIClassBase::getStaticFieldValue( const char* classDescriptor, jfieldID field, std::string& outValue) {
	outValue.clear();
	if (!classDescriptor || !field)
		return false;

	JNIEnv *env = GET_JNIENV();
	jclass cls = env->FindClass(classDescriptor);
	if (!cls) {
		LOGE("CANNOT find Java class: %s", classDescriptor);
		return false;
	}

	bool ret = false;
	jstring jstr = (jstring) env->GetStaticObjectField(cls, field);
	if (jstr) {
		const char* str = env->GetStringUTFChars(jstr, NULL);
		outValue = str;
		env->ReleaseStringUTFChars(jstr, str);

		ret = true;
	} else {
		LOGE("CANNOT find Java field for class:  %s", classDescriptor);
	}

	env->DeleteLocalRef(jstr);
	env->DeleteLocalRef(cls);

	return ret;
}

bool JNIClassBase::getStaticFieldValue( const char* classDescriptor, jfieldID field, int32_t& outValue) {
	outValue = -1;
	if (!classDescriptor || !field)
		return false;

	JNIEnv *env = GET_JNIENV();
	jclass cls = env->FindClass(classDescriptor);
	if (!cls) {
		LOGE("CANNOT find Java class: %s", classDescriptor);
		return false;
	}

	outValue = env->GetStaticIntField(cls, field);

	env->DeleteLocalRef(cls);

	return true;
}

enum MemberType {
	CONSTRUCTOR = 1,
	METHOD,
	FIELD,
};

static bool getMemberInfo( jobject member, MemberType type, JNIMemberInfo& memberInfo) {
	if (!member)
		return false;

	const char* classDescriptor = NULL;
	switch (type) {
		case CONSTRUCTOR:
			classDescriptor = "java/lang/reflect/Constructor";
			memberInfo.methodElseField = true;
			break;
		case METHOD:
			classDescriptor = "java/lang/reflect/Method";
			memberInfo.methodElseField = true;
			break;
		case FIELD:
			classDescriptor = "java/lang/reflect/Field";
			memberInfo.methodElseField = false;
			break;
		default:
			TRESPASS();
			return false;
	}

	bool ret = false;
	JNIEnv *env = GET_JNIENV();

	jclass clsMember = NULL;
	jmethodID mid_getName = NULL;
	jmethodID mid_getModifiers = NULL;

	jclass clsModifier = NULL;
	jmethodID mid_isStatic = NULL;
	jmethodID mid_isNative = NULL;

	jstring jstrName = NULL;
	const char* strName = NULL;
	jint modifier = 0;

	clsMember = env->FindClass(classDescriptor);
	if (!clsMember) {
		LOGE("CANNOT find Java class: %s", classDescriptor);
		goto cleanup;
	}
		
	mid_getName = env->GetMethodID(clsMember, "getName", "()Ljava/lang/String;");
	if (!mid_getName) {
		LOGE("CANNOT find Java methodID: %s %s", classDescriptor, "getName");
		goto cleanup;
	}

	mid_getModifiers = env->GetMethodID(clsMember, "getModifiers", "()I");
	if (!mid_getModifiers) {
		LOGE("CANNOT find Java methodID: %s %s", classDescriptor, "getModifiers");
		goto cleanup;
	}

	clsModifier = env->FindClass("java/lang/reflect/Modifier");
	if (!clsModifier) {
		LOGE("CANNOT find Java class: %s", "java/lang/reflect/Modifier");
		goto cleanup;
	}

	mid_isStatic = env->GetStaticMethodID(clsModifier, "isStatic", "(I)Z");
	if (!mid_isStatic) {
        LOGE("CANNOT find Java methodID: %s %s", "java/lang/reflect/Modifier", "isStatic");
        goto cleanup;
    }

	mid_isNative = env->GetStaticMethodID(clsModifier, "isNative", "(I)Z");
	if (!mid_isNative) {
        LOGE("CANNOT find Java methodID: %s %s", "java/lang/reflect/Modifier", "isNative");
        goto cleanup;
    }

	if (type == CONSTRUCTOR) {
		memberInfo.name = "<init>";
	} else {
		jstrName = (jstring) env->CallObjectMethod(member, mid_getName);
		CHECK(jstrName != NULL);

		strName = env->GetStringUTFChars(jstrName, NULL);
		memberInfo.name = strName;
		env->ReleaseStringUTFChars(jstrName, strName);

		env->DeleteLocalRef(jstrName);
	}

	modifier = env->CallIntMethod(member, mid_getModifiers);
	memberInfo.isStatic = (bool) env->CallStaticBooleanMethod(clsModifier, mid_isStatic, modifier);
	memberInfo.isNative = (bool) env->CallStaticBooleanMethod(clsModifier, mid_isNative, modifier);

    //FIXME: get member descriptor
	memberInfo.descriptor.clear();
	memberInfo.id = NULL;

	ret = true;

cleanup:
	env->DeleteLocalRef(clsMember);
	env->DeleteLocalRef(clsModifier);

	return ret;
}

static bool getPublicMembers(const char* classDescriptor, std::vector<JNIMemberInfo>& members) {
	members.clear();
	if (!classDescriptor)
		return false;

	bool ret = false;
	JNIEnv *env = GET_JNIENV();

	jclass clsClass = NULL;
	jmethodID mid_getDeclaredConstructors = NULL;	
	jmethodID mid_getMethods = NULL;	
	jmethodID mid_getFields = NULL;

	jobject objClass = NULL;	
	jobjectArray objArray = NULL;	
	jsize count = 0, i = 0;	
	jobject obj = NULL;
#if 0
	JNIMemberInfo* member = NULL;
#else
    //replace Android's Vector with C++ 11's vector
	JNIMemberInfo member;
#endif

	clsClass = env->FindClass("java/lang/Class");
	if (!clsClass) {
		LOGE("CANNOT find Java class: %s", "java/lang/Class");
		goto cleanup;
	}

	mid_getDeclaredConstructors = env->GetMethodID(clsClass, "getDeclaredConstructors", "()[Ljava/lang/reflect/Constructor;");
	if (!mid_getDeclaredConstructors) {
		LOGE("CANNOT find Java methodID: %s %s", "java/lang/Class", "getDeclaredConstructors");
		goto cleanup;
	}

	mid_getMethods = env->GetMethodID(clsClass, "getMethods", "()[Ljava/lang/reflect/Method;");
	if (!mid_getMethods) {
		LOGE("CANNOT find Java methodID: %s %s", "java/lang/Class", "getMethods");
		goto cleanup;
	}

	mid_getFields = env->GetMethodID(clsClass, "getFields", "()[Ljava/lang/reflect/Field;");
	if (!mid_getFields) {
		LOGE("CANNOT find Java methodID: %s %s", "java/lang/Class", "getFields");
		goto cleanup;
	}

	objClass = env->FindClass(classDescriptor);
	if (!objClass) {
		JNIClassBase::checkException(true);
		LOGE("CANNOT find Java class: %s", classDescriptor);
		goto cleanup;
	}

	// Constructors
	objArray = (jobjectArray) env->CallObjectMethod(objClass, mid_getDeclaredConstructors);
	if (objArray) {
		count = env->GetArrayLength(objArray);
		for (i = 0; i < count; i++) {
			obj = env->GetObjectArrayElement(objArray, i);
			CHECK(obj != NULL);

#if 0
			members.push();
			member = &members.editItemAt(members.size() - 1);
			CHECK(getMemberInfo(obj, CONSTRUCTOR, *member));
#else
			CHECK(getMemberInfo(obj, CONSTRUCTOR, member));
			members.push_back(member);
#endif

			env->DeleteLocalRef(obj);
		}

		env->DeleteLocalRef(objArray);
	}

	// Methods
	objArray = (jobjectArray) env->CallObjectMethod(objClass, mid_getMethods);
	if (objArray) {
		count = env->GetArrayLength(objArray);
		for (i = 0; i < count; i++) {
			obj = env->GetObjectArrayElement(objArray, i);
			CHECK(obj != NULL);

#if 0
			members.push();
			member = &members.editItemAt(members.size() - 1);
			CHECK(getMemberInfo(obj, METHOD, *member));
#else

			CHECK(getMemberInfo(obj, METHOD, member));
			members.push_back(member);
#endif

			env->DeleteLocalRef(obj);
		}

		env->DeleteLocalRef(objArray);
	}

	// Fields
	objArray = (jobjectArray) env->CallObjectMethod(objClass, mid_getFields);
	if (objArray) {
		count = env->GetArrayLength(objArray);
		for (i = 0; i < count; i++) {
			obj = env->GetObjectArrayElement(objArray, i);
			CHECK(obj != NULL);

#if 0
			members.push();
			member = &members.editItemAt(members.size() - 1);
			CHECK(getMemberInfo(obj, FIELD, *member));
#else

			CHECK(getMemberInfo(obj, FIELD, member));
			members.push_back(member);
#endif

			env->DeleteLocalRef(obj);
		}

		env->DeleteLocalRef(objArray);
	}

	ret = true;

cleanup:
	env->DeleteLocalRef(clsClass);
	env->DeleteLocalRef(objClass);

	return ret;
}


static bool findMember(const std::vector<JNIMemberInfo>& allMembers, const JNIMemberInfo& member, bool& outIsNative) {
	for (size_t i = 0; i < allMembers.size(); i++) {
		const JNIMemberInfo& curMember = allMembers[i];

		if(member.methodElseField != curMember.methodElseField)
			continue;

		if(member.isStatic != curMember.isStatic)
			continue;

		if(!(member.name == curMember.name))
			continue;

		outIsNative = curMember.isNative;
		return true;
	}

	return false;
}

}
