// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

#include <librealsense2/rs.h>
#include <librealsense2/h/rs_internal.h>
#include <librealsense2/h/rs_pipeline.h>
#include <librealsense2/hpp/rs_types.hpp>
#include <nan.h>

#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

class DictBase {
 public:
  explicit DictBase(v8::Local<v8::Object> source) : v8obj_(source) {}

  DictBase() {
    v8obj_ = v8::Object::New(v8::Isolate::GetCurrent());
  }

  ~DictBase() {}

  v8::Local<v8::Value> GetMember(const char* name) const {
    return v8obj_->Get(Nan::New(name).ToLocalChecked());
  }

  v8::Local<v8::Value> GetMember(const std::string& name) const {
    return GetMember(name.c_str());
  }

  void SetMemberUndefined(const char* name) {
    v8obj_->Set(Nan::New(name).ToLocalChecked(), Nan::Undefined());
  }

  void DeleteMember(const char* name) {
    v8obj_->Delete(Nan::New(name).ToLocalChecked());
  }

  void SetMember(const char* name, const char* value) {
    v8obj_->Set(Nan::New(name).ToLocalChecked(),
        Nan::New(value).ToLocalChecked());
  }

  void SetMember(const char* name, v8::Local<v8::Value> value) {
    v8obj_->Set(Nan::New(name).ToLocalChecked(), value);
  }

  void SetMember(const char* name, const std::string& value) {
    v8obj_->Set(Nan::New(name).ToLocalChecked(),
        Nan::New(value.c_str()).ToLocalChecked());
  }

  template <typename T, typename V, uint32_t len>
  void SetMemberArray(const char* name, V value[len]) {
    v8::Local<v8::Array> array = Nan::New<v8::Array>(len);
    for (uint32_t i = 0; i < len; i++) {
      array->Set(Nan::GetCurrentContext(), i, Nan::New<T>(value[i]));
    }
    SetMember(name, array);
  }

  template <typename T>
  void SetMemberT(const char* name, const T& value) {
    v8obj_->Set(Nan::New(name).ToLocalChecked(), Nan::New(value));
  }

  bool IsMemberPresent(const char* name) const {
    v8::Local<v8::Value> v = v8obj_->Get(Nan::New(name).ToLocalChecked());
    return !(v->IsUndefined() || v->IsNull());
  }

  bool IsMemberPresent(const std::string& name) const {
    return IsMemberPresent(name.c_str());
  }

  v8::Local<v8::Object> GetObject() const {
    return v8obj_;
  }

 protected:
  mutable v8::Local<v8::Object> v8obj_;
};

class ErrorUtil {
 public:
  class ErrorInfo {
   public:
    ErrorInfo() : is_error_(false), recoverable_(false) {}

    ~ErrorInfo() {}

    void Update(bool is_error, bool recoverable, std::string description,
        std::string function) {
      is_error_ = is_error;
      recoverable_ = recoverable;
      description_ = description;
      native_function_ = function;
    }

    void Reset() { Update(false, false, "", ""); }

    // set value to js attributes only when this method is called
    v8::Local<v8::Object> GetJSObject() {
      DictBase obj;
      obj.SetMemberT("recoverable", recoverable_);
      obj.SetMember("description", description_);
      obj.SetMember("nativeFunction", native_function_);
      return obj.GetObject();
    }

   private:
    bool is_error_;
    bool recoverable_;
    std::string description_;
    std::string native_function_;

    friend class ErrorUtil;
  };

  ~ErrorUtil() {}

  static void Init() {
    if (!singleton_) singleton_ = new ErrorUtil();
  }

  static void UpdateJSErrorCallback(
      const Nan::FunctionCallbackInfo<v8::Value>& info) {
    singleton_->js_error_container_.Reset(info[0]->ToObject());
    v8::String::Utf8Value value(info[1]->ToString());
    singleton_->js_error_callback_name_ = std::string(*value);
  }

  static void AnalyzeError(rs2_error* err) {
    if (!err) return;

    auto function = std::string(rs2_get_failed_function(err));
    auto type = rs2_get_librealsense_exception_type(err);
    auto msg = std::string(rs2_get_error_message(err));
    bool recoverable = false;

    if (type == RS2_EXCEPTION_TYPE_INVALID_VALUE ||
        type == RS2_EXCEPTION_TYPE_WRONG_API_CALL_SEQUENCE ||
        type == RS2_EXCEPTION_TYPE_NOT_IMPLEMENTED) {
      recoverable = true;
    }

    singleton_->MarkError(recoverable, msg, function);
  }

  static void ResetError() {
    singleton_->error_info_.Reset();
  }

  static v8::Local<v8::Value> GetJSErrorObject() {
    if (singleton_->error_info_.is_error_)
      return singleton_->error_info_.GetJSObject();

    return Nan::Undefined();
  }

 private:
  // Save detailed error info to the js object
  void MarkError(bool recoverable, std::string description,
      std::string native_function) {
    error_info_.Update(true, recoverable, description, native_function);
    v8::Local<v8::Value> args[1] = { GetJSErrorObject() };
    auto container = Nan::New<v8::Object>(js_error_container_);
    Nan::MakeCallback(container, js_error_callback_name_.c_str(),
        1, args);
  }

  static ErrorUtil* singleton_;
  ErrorInfo error_info_;
  Nan::Persistent<v8::Object> js_error_container_;
  std::string js_error_callback_name_;
};

ErrorUtil* ErrorUtil::singleton_ = nullptr;

template<typename R, typename F, typename... arguments>
R GetNativeResult(F func, rs2_error** error, arguments... params) {
  // reset the error pointer for each call.
  *error = nullptr;
  ErrorUtil::ResetError();
  R val = func(params...);
  ErrorUtil::AnalyzeError(*error);
  return val;
}

template<typename F, typename... arguments>
void CallNativeFunc(F func, rs2_error** error, arguments... params) {
  // reset the error pointer for each call.
  *error = nullptr;
  ErrorUtil::ResetError();
  func(params...);
  ErrorUtil::AnalyzeError(*error);
}

class MainThreadCallbackInfo {
 public:
  MainThreadCallbackInfo() : consumed_(false) {
    pending_infos_.push_back(this);
  }
  virtual ~MainThreadCallbackInfo() {
    pending_infos_.erase(
        std::find(pending_infos_.begin(), pending_infos_.end(), this));
  }
  virtual void Run() {}
  virtual void Release() {}
  void SetConsumed() { consumed_ = true; }
  static bool InfoExist(MainThreadCallbackInfo* info) {
    auto result = std::find(pending_infos_.begin(), pending_infos_.end(), info);
    return (result != pending_infos_.end());
  }
  static void ReleasePendingInfos() {
    while (pending_infos_.size()) { delete *(pending_infos_.begin()); }
  }

 protected:
  static std::list<MainThreadCallbackInfo*> pending_infos_;
  bool consumed_;
};

std::list<MainThreadCallbackInfo*> MainThreadCallbackInfo::pending_infos_;

class MainThreadCallback {
 public:
  class LockGuard {
   public:
    LockGuard() {
      MainThreadCallback::Lock();
    }
    ~LockGuard() {
      MainThreadCallback::Unlock();
    }
  };
  static void Init() {
    if (!singleton_)
      singleton_ = new MainThreadCallback();
  }
  static void Destroy() {
    if (singleton_) {
      delete singleton_;
      singleton_ = nullptr;
      MainThreadCallbackInfo::ReleasePendingInfos();
    }
  }
  ~MainThreadCallback() {
    uv_close(reinterpret_cast<uv_handle_t*>(async_),
      [](uv_handle_t* ptr) -> void {
      free(ptr);
    });
    uv_mutex_destroy(&mutex_);
  }
  static void NotifyMainThread(MainThreadCallbackInfo* info) {
    if (singleton_) {
      LockGuard guard;
      if (singleton_->async_->data) {
        MainThreadCallbackInfo* info =
          reinterpret_cast<MainThreadCallbackInfo*>(singleton_->async_->data);
        info->Release();
        delete info;
      }
      singleton_->async_->data = static_cast<void*>(info);
      uv_async_send(singleton_->async_);
    }
  }

 private:
  MainThreadCallback() {
    async_ = static_cast<uv_async_t*>(malloc(sizeof(uv_async_t)));
    async_->data = nullptr;
    uv_async_init(uv_default_loop(), async_, AsyncProc);
    uv_mutex_init(&mutex_);
  }
  static void Lock() {
    if (singleton_) uv_mutex_lock(&(singleton_->mutex_));
  }
  static void Unlock() {
    if (singleton_) uv_mutex_unlock(&(singleton_->mutex_));
  }
  static void AsyncProc(uv_async_t* async) {
    MainThreadCallbackInfo* info = nullptr;
    {
      LockGuard guard;
      if (!(async->data))
        return;

      info = reinterpret_cast<MainThreadCallbackInfo*>(async->data);
      async->data = nullptr;
    }
    info->Run();
    // As the above info->Run() enters js world and during that, any code
    // such as cleanup() could be called to release everything. So this info
    // may has been released, we need to check before releasing it.
    if (MainThreadCallbackInfo::InfoExist(info)) delete info;
  }
  static MainThreadCallback* singleton_;
  uv_async_t* async_;
  uv_mutex_t mutex_;
};

MainThreadCallback* MainThreadCallback::singleton_ = nullptr;
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class RSExtrinsics : public DictBase {
 public:
  explicit RSExtrinsics(rs2_extrinsics extrinsics) {
    SetMemberArray<v8::Number, float, 9>("rotation", extrinsics.rotation);
    SetMemberArray<v8::Number, float, 3>("translation", extrinsics.translation);
  }
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class RSIntrinsics : public DictBase {
 public:
  explicit RSIntrinsics(rs2_intrinsics intrinsics) {
    SetMemberT("width", intrinsics.width);
    SetMemberT("height", intrinsics.height);
    SetMemberT("ppx", intrinsics.ppx);
    SetMemberT("ppy", intrinsics.ppy);
    SetMemberT("fx", intrinsics.fx);
    SetMemberT("fy", intrinsics.fy);
    SetMemberT("model", intrinsics.model);
    SetMemberArray<v8::Number, float, 5>("coeffs", intrinsics.coeffs);
  }
};

class RSMotionIntrinsics : public DictBase {
 public:
  explicit RSMotionIntrinsics(rs2_motion_device_intrinsic* intri) {
    v8::Local<v8::Array> data_array = Nan::New<v8::Array>(3);
    int32_t index = 0;
    for (int32_t i = 0; i < 3; i++) {
      for (int32_t j = 0; j < 4; j++) {
        data_array->Set(Nan::GetCurrentContext(),
            index++, Nan::New(intri->data[i][j]));
      }
    }
    SetMember("data", data_array);
    SetMemberArray<v8::Number, float, 3>("noiseVariances",
        intri->noise_variances);
    SetMemberArray<v8::Number, float, 3>("biasVariances",
        intri->bias_variances);
  }
};

class RSOptionRange : public DictBase {
 public:
  RSOptionRange(float min, float max, float step, float def) {
    SetMemberT("minValue", min);
    SetMemberT("maxValue", max);
    SetMemberT("step", step);
    SetMemberT("defaultValue", def);
  }
};

class RSNotification : public DictBase {
 public:
  RSNotification(const std::string& des, rs2_time_t time,
      rs2_log_severity severity, rs2_notification_category category,
      const std::string& serialized_data) {
    SetMember("descr", des);
    SetMemberT("timestamp", time);
    SetMemberT("severity", (int32_t)severity);
    SetMemberT("category", (int32_t)category);
    SetMember("serializedData", serialized_data);
  }
};

class RSRegionOfInterest : public DictBase {
 public:
  RSRegionOfInterest(int32_t minx, int32_t miny, int32_t maxx, int32_t maxy) {
    SetMemberT("minX", minx);
    SetMemberT("minY", miny);
    SetMemberT("maxX", maxx);
    SetMemberT("maxY", maxy);
  }
};

class RSStreamProfile : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSStreamProfile").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "stream", Stream);
    Nan::SetPrototypeMethod(tpl, "format", Format);
    Nan::SetPrototypeMethod(tpl, "fps", Fps);
    Nan::SetPrototypeMethod(tpl, "index", Index);
    Nan::SetPrototypeMethod(tpl, "uniqueID", UniqueID);
    Nan::SetPrototypeMethod(tpl, "isDefault", IsDefault);
    Nan::SetPrototypeMethod(tpl, "isVideoProfile", IsVideoProfile);
    Nan::SetPrototypeMethod(tpl, "isMotionProfile", IsMotionProfile);
    Nan::SetPrototypeMethod(tpl, "width", Width);
    Nan::SetPrototypeMethod(tpl, "height", Height);
    Nan::SetPrototypeMethod(tpl, "getExtrinsicsTo", GetExtrinsicsTo);
    Nan::SetPrototypeMethod(tpl, "getVideoStreamIntrinsics",
        GetVideoStreamIntrinsics);
    Nan::SetPrototypeMethod(tpl, "getMotionIntrinsics", GetMotionIntrinsics);

    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSStreamProfile").ToLocalChecked(),
                 tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(
      rs2_stream_profile* p, bool own = false) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor_);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(instance);
    me->profile_ = p;
    me->own_profile_ = own;
    CallNativeFunc(rs2_get_stream_profile_data, &me->error_, p, &me->stream_,
        &me->format_, &me->index_, &me->unique_id_, &me->fps_, &me->error_);
    me->is_default_ = rs2_is_stream_profile_default(p, &me->error_);
    if (GetNativeResult<bool>(rs2_stream_profile_is, &me->error_, p,
        RS2_EXTENSION_VIDEO_PROFILE, &me->error_)) {
      me->is_video_ = true;
      CallNativeFunc(rs2_get_video_stream_resolution, &me->error_, p,
          &me->width_, &me->height_, &me->error_);
    } else if (GetNativeResult<bool>(rs2_stream_profile_is, &me->error_, p,
        RS2_EXTENSION_MOTION_PROFILE, &me->error_)) {
      me->is_motion_ = true;
    }

    return scope.Escape(instance);
  }

 private:
  RSStreamProfile() : error_(nullptr), profile_(nullptr), index_(0),
      unique_id_(0), fps_(0), format_(static_cast<rs2_format>(0)),
      stream_(static_cast<rs2_stream>(0)), is_video_(false), width_(0),
      height_(0), is_default_(false), own_profile_(false), is_motion_(false) {}

  ~RSStreamProfile() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;
    if (profile_ && own_profile_) rs2_delete_stream_profile(profile_);
    profile_ = nullptr;
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      me->DestroyMe();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSStreamProfile* obj = new RSStreamProfile();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(Stream) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->stream_));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }
  static NAN_METHOD(Format) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->format_));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }
  static NAN_METHOD(Fps) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->fps_));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }
  static NAN_METHOD(Index) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->index_));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }
  static NAN_METHOD(UniqueID) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->unique_id_));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }
  static NAN_METHOD(IsDefault) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->is_default_));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }
  static NAN_METHOD(IsVideoProfile) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->is_video_));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(IsMotionProfile) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    info.GetReturnValue().Set(Nan::New(me->is_motion_));
  }

  static NAN_METHOD(Width) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->width_));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(Height) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->height_));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetExtrinsicsTo) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    auto to = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info[0]->ToObject());
    if (!me || !to) return;

    rs2_extrinsics res;
    CallNativeFunc(rs2_get_extrinsics, &me->error_, me->profile_, to->profile_,
        &res, &me->error_);
    if (me->error_) return;

    RSExtrinsics rsres(res);
    info.GetReturnValue().Set(rsres.GetObject());
  }

  static NAN_METHOD(GetVideoStreamIntrinsics) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (!me) return;

    rs2_intrinsics intr;
    CallNativeFunc(rs2_get_video_stream_intrinsics, &me->error_, me->profile_,
        &intr, &me->error_);
    if (me->error_) return;

    RSIntrinsics res(intr);
    info.GetReturnValue().Set(res.GetObject());
  }

  static NAN_METHOD(GetMotionIntrinsics) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (!me) return;

    rs2_motion_device_intrinsic output;
    CallNativeFunc(rs2_get_motion_intrinsics, &me->error_, me->profile_,
        &output, &me->error_);
    if (me->error_) return;

    RSMotionIntrinsics intrinsics(&output);
    info.GetReturnValue().Set(intrinsics.GetObject());
  }

 private:
  static Nan::Persistent<v8::Function> constructor_;

  rs2_error* error_;
  rs2_stream_profile* profile_;
  int32_t index_;
  int32_t unique_id_;
  int32_t fps_;
  rs2_format format_;
  rs2_stream stream_;
  bool is_video_;
  int32_t width_;
  int32_t height_;
  bool is_default_;
  bool own_profile_;
  bool is_motion_;

  friend class RSSensor;
};

Nan::Persistent<v8::Function> RSStreamProfile::constructor_;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// 8888888b.   .d8888b.  8888888888                                          //
// 888   Y88b d88P  Y88b 888                                                 //
// 888    888 Y88b.      888                                                 //
// 888   d88P  "Y888b.   8888888 888d888 8888b.  88888b.d88b.   .d88b.       //
// 8888888P"      "Y88b. 888     888P"      "88b 888 "888 "88b d8P  Y8b      //
// 888 T88b         "888 888     888    .d888888 888  888  888 88888888      //
// 888  T88b  Y88b  d88P 888     888    888  888 888  888  888 Y8b.          //
// 888   T88b  "Y8888P"  888     888    "Y888888 888  888  888  "Y8888       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class RSFrame : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSFrame").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "getStreamProfile", GetStreamProfile);
    Nan::SetPrototypeMethod(tpl, "getData", GetData);
    Nan::SetPrototypeMethod(tpl, "writeData", WriteData);
    Nan::SetPrototypeMethod(tpl, "getWidth", GetWidth);
    Nan::SetPrototypeMethod(tpl, "getHeight", GetHeight);
    Nan::SetPrototypeMethod(tpl, "getStrideInBytes", GetStrideInBytes);
    Nan::SetPrototypeMethod(tpl, "getBitsPerPixel", GetBitsPerPixel);
    Nan::SetPrototypeMethod(tpl, "getTimestamp", GetTimestamp);
    Nan::SetPrototypeMethod(tpl, "getTimestampDomain", GetTimestampDomain);
    Nan::SetPrototypeMethod(tpl, "getFrameNumber", GetFrameNumber);
    Nan::SetPrototypeMethod(tpl, "getFrameMetadata", GetFrameMetadata);
    Nan::SetPrototypeMethod(tpl, "supportsFrameMetadata",
        SupportsFrameMetadata);
    Nan::SetPrototypeMethod(tpl, "isVideoFrame", IsVideoFrame);
    Nan::SetPrototypeMethod(tpl, "isDepthFrame", IsDepthFrame);
    Nan::SetPrototypeMethod(tpl, "isDisparityFrame", IsDisparityFrame);
    Nan::SetPrototypeMethod(tpl, "isMotionFrame", IsMotionFrame);
    Nan::SetPrototypeMethod(tpl, "isPoseFrame", IsPoseFrame);

    // Points related APIs
    Nan::SetPrototypeMethod(tpl, "canGetPoints", CanGetPoints);
    Nan::SetPrototypeMethod(tpl, "getVertices", GetVertices);
    Nan::SetPrototypeMethod(tpl, "getVerticesBufferLen", GetVerticesBufferLen);
    Nan::SetPrototypeMethod(tpl, "getTexCoordBufferLen", GetTexCoordBufferLen);
    Nan::SetPrototypeMethod(tpl, "writeVertices", WriteVertices);
    Nan::SetPrototypeMethod(tpl, "getTextureCoordinates",
                            GetTextureCoordinates);
    Nan::SetPrototypeMethod(tpl, "writeTextureCoordinates",
                            WriteTextureCoordinates);
    Nan::SetPrototypeMethod(tpl, "getPointsCount", GetPointsCount);
    Nan::SetPrototypeMethod(tpl, "exportToPly", ExportToPly);
    Nan::SetPrototypeMethod(tpl, "isValid", IsValid);
    Nan::SetPrototypeMethod(tpl, "getDistance", GetDistance);
    Nan::SetPrototypeMethod(tpl, "getBaseLine", GetBaseLine);
    Nan::SetPrototypeMethod(tpl, "keep", Keep);
    Nan::SetPrototypeMethod(tpl, "getMotionData", GetMotionData);
    Nan::SetPrototypeMethod(tpl, "getPoseData", GetPoseData);

    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSFrame").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(rs2_frame* frame) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor_);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(instance);
    me->frame_ = frame;

    return scope.Escape(instance);
  }

  void Replace(rs2_frame* value) {
    DestroyMe();
    frame_ = value;
    // As the underlying frame changed, we must clean the js side's buffer
    Nan::MakeCallback(handle(), "_internalResetBuffer", 0, nullptr);
  }

 private:
  RSFrame() : frame_(nullptr), error_(nullptr) {}

  ~RSFrame() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    if (frame_) rs2_release_frame(frame_);
    error_ = nullptr;
    frame_ = nullptr;
  }

  static void SetAFloatInVectorObject(v8::Local<v8::Object> obj, uint32_t index,
      float value) {
    const char* names[4] = {"x", "y", "z", "w"};
    v8::Local<v8::String> name = Nan::New(names[index]).ToLocalChecked();
    Nan::Set(obj, name, Nan::New(value));
  }

  static void FillAFloatVector(v8::Local<v8::Object> obj,
      const rs2_vector& vec) {
    SetAFloatInVectorObject(obj, 0, vec.x);
    SetAFloatInVectorObject(obj, 1, vec.y);
    SetAFloatInVectorObject(obj, 2, vec.z);
  }

  static void FillAFloatQuaternion(v8::Local<v8::Object> obj,
      const rs2_quaternion& quaternion) {
    SetAFloatInVectorObject(obj, 0, quaternion.x);
    SetAFloatInVectorObject(obj, 1, quaternion.y);
    SetAFloatInVectorObject(obj, 2, quaternion.z);
    SetAFloatInVectorObject(obj, 3, quaternion.w);
  }

  static void AssemblePoseData(v8::Local<v8::Object> obj,
      const rs2_pose& pose) {
    auto translation_name = Nan::New("translation").ToLocalChecked();
    auto velocity_name = Nan::New("velocity").ToLocalChecked();
    auto acceleration_name = Nan::New("acceleration").ToLocalChecked();
    auto rotation_name = Nan::New("rotation").ToLocalChecked();
    auto angular_velocity_name = Nan::New("angularVelocity").ToLocalChecked();
    auto angular_acceleration_name =
        Nan::New("angularAcceleration").ToLocalChecked();
    auto tracker_confidence_name =
        Nan::New("trackerConfidence").ToLocalChecked();
    auto mapper_confidence_name = Nan::New("mapperConfidence").ToLocalChecked();

    auto translation_obj = Nan::GetRealNamedProperty(
        obj, translation_name).ToLocalChecked()->ToObject();
    auto velocity_obj = Nan::GetRealNamedProperty(
        obj, velocity_name).ToLocalChecked()->ToObject();
    auto acceleration_obj = Nan::GetRealNamedProperty(
        obj, acceleration_name).ToLocalChecked()->ToObject();
    auto rotation_obj = Nan::GetRealNamedProperty(
        obj, rotation_name).ToLocalChecked()->ToObject();
    auto angular_velocity_obj = Nan::GetRealNamedProperty(
        obj, angular_velocity_name).ToLocalChecked()->ToObject();
    auto angular_acceleration_obj = Nan::GetRealNamedProperty(
        obj, angular_acceleration_name).ToLocalChecked()->ToObject();

    FillAFloatVector(translation_obj, pose.translation);
    FillAFloatVector(velocity_obj, pose.velocity);
    FillAFloatVector(acceleration_obj, pose.acceleration);
    FillAFloatQuaternion(rotation_obj, pose.rotation);
    FillAFloatVector(angular_velocity_obj, pose.angular_velocity);
    FillAFloatVector(angular_acceleration_obj, pose.angular_acceleration);

    Nan::Set(obj, tracker_confidence_name, Nan::New(pose.tracker_confidence));
    Nan::Set(obj, mapper_confidence_name, Nan::New(pose.mapper_confidence));
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSFrame* obj = new RSFrame();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(GetStreamProfile) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me || !me->frame_) return;

    rs2_stream stream;
    rs2_format format;
    int32_t index = 0;
    int32_t unique_id = 0;
    int32_t fps = 0;
    const rs2_stream_profile* profile_org =
        GetNativeResult<const rs2_stream_profile*>(rs2_get_frame_stream_profile,
        &me->error_, me->frame_, &me->error_);
    CallNativeFunc(rs2_get_stream_profile_data, &me->error_, profile_org,
        &stream, &format, &index, &unique_id, &fps, &me->error_);
    if (me->error_) return;

    rs2_stream_profile* profile = GetNativeResult<rs2_stream_profile*>(
        rs2_clone_stream_profile, &me->error_, profile_org, stream, index,
        format, &me->error_);
    if (!profile) return;

    info.GetReturnValue().Set(RSStreamProfile::NewInstance(profile, true));
  }

  static NAN_METHOD(GetData) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    auto buffer = GetNativeResult<const void*>(rs2_get_frame_data, &me->error_,
        me->frame_, &me->error_);
    if (!buffer) return;

    const auto stride = GetNativeResult<int>(rs2_get_frame_stride_in_bytes,
        &me->error_, me->frame_, &me->error_);
    const auto height = GetNativeResult<int>(rs2_get_frame_height, &me->error_,
        me->frame_, &me->error_);
    const auto length = stride * height;
    auto array_buffer = v8::ArrayBuffer::New(
        v8::Isolate::GetCurrent(),
        static_cast<uint8_t*>(const_cast<void*>(buffer)), length,
        v8::ArrayBufferCreationMode::kExternalized);
    info.GetReturnValue().Set(array_buffer);
  }

  static NAN_METHOD(WriteData) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto array_buffer = v8::Local<v8::ArrayBuffer>::Cast(info[0]);
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    const auto buffer = GetNativeResult<const void*>(rs2_get_frame_data,
        &me->error_, me->frame_, &me->error_);
    const auto stride = GetNativeResult<int>(rs2_get_frame_stride_in_bytes,
        &me->error_, me->frame_, &me->error_);
    const auto height = GetNativeResult<int>(rs2_get_frame_height, &me->error_,
        me->frame_, &me->error_);
    const size_t length = stride * height;
    if (buffer && array_buffer->ByteLength() >= length) {
      auto contents = array_buffer->GetContents();
      memcpy(contents.Data(), buffer, length);
    }
  }

  static NAN_METHOD(GetWidth) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    auto value = GetNativeResult<int>(rs2_get_frame_width, &me->error_,
        me->frame_, &me->error_);
    info.GetReturnValue().Set(Nan::New(value));
  }

  static NAN_METHOD(GetHeight) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    auto value = GetNativeResult<int>(rs2_get_frame_height, &me->error_,
        me->frame_, &me->error_);
    info.GetReturnValue().Set(Nan::New(value));
  }

  static NAN_METHOD(GetStrideInBytes) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    auto value = GetNativeResult<int>(rs2_get_frame_stride_in_bytes,
        &me->error_, me->frame_, &me->error_);
    info.GetReturnValue().Set(Nan::New(value));
  }

  static NAN_METHOD(GetBitsPerPixel) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    auto value = GetNativeResult<int>(rs2_get_frame_bits_per_pixel, &me->error_,
        me->frame_, &me->error_);
    info.GetReturnValue().Set(Nan::New(value));
  }

  static NAN_METHOD(GetTimestamp) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    auto value = GetNativeResult<double>(rs2_get_frame_timestamp, &me->error_,
        me->frame_, &me->error_);
    info.GetReturnValue().Set(Nan::New(value));
  }

  static NAN_METHOD(GetTimestampDomain) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    auto value = GetNativeResult<rs2_timestamp_domain>(
        rs2_get_frame_timestamp_domain, &me->error_, me->frame_, &me->error_);
    info.GetReturnValue().Set(Nan::New(value));
  }

  static NAN_METHOD(GetFrameNumber) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    uint32_t value = GetNativeResult<uint32_t>(rs2_get_frame_number,
        &me->error_, me->frame_, &me->error_);
    info.GetReturnValue().Set(Nan::New(value));
  }

  static NAN_METHOD(IsVideoFrame) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    bool isVideo = false;
    if (GetNativeResult<bool>(rs2_is_frame_extendable_to, &me->error_,
        me->frame_, RS2_EXTENSION_VIDEO_FRAME, &me->error_))
      isVideo = true;
    info.GetReturnValue().Set(Nan::New(isVideo));
  }

  static NAN_METHOD(IsDepthFrame) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    bool isDepth = false;
    if (GetNativeResult<bool>(rs2_is_frame_extendable_to, &me->error_,
        me->frame_, RS2_EXTENSION_DEPTH_FRAME, &me->error_))
      isDepth = true;
    info.GetReturnValue().Set(Nan::New(isDepth));
  }

  static NAN_METHOD(IsDisparityFrame) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto is_disparity = GetNativeResult<int>(rs2_is_frame_extendable_to,
        &me->error_, me->frame_, RS2_EXTENSION_DISPARITY_FRAME, &me->error_);
    info.GetReturnValue().Set(Nan::New(is_disparity ? true : false));
  }

  static NAN_METHOD(IsMotionFrame) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto val = GetNativeResult<int>(rs2_is_frame_extendable_to, &me->error_,
        me->frame_, RS2_EXTENSION_MOTION_FRAME, &me->error_);
    info.GetReturnValue().Set(Nan::New(val ? true : false));
  }

  static NAN_METHOD(IsPoseFrame) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto val = GetNativeResult<int>(rs2_is_frame_extendable_to, &me->error_,
        me->frame_, RS2_EXTENSION_POSE_FRAME, &me->error_);
    info.GetReturnValue().Set(Nan::New(val ? true : false));
  }

  static NAN_METHOD(GetFrameMetadata) {
    info.GetReturnValue().Set(Nan::New(false));
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    rs2_frame_metadata_value metadata =
        (rs2_frame_metadata_value)(info[0]->IntegerValue());
    Nan::TypedArrayContents<unsigned char> content(info[1]);
    unsigned char* internal_data = *content;
    if (!me || !internal_data) return;

    rs2_metadata_type output = GetNativeResult<rs2_metadata_type>(
        rs2_get_frame_metadata, &me->error_, me->frame_, metadata, &me->error_);
    unsigned char* out_ptr = reinterpret_cast<unsigned char*>(&output);
    uint32_t val = 1;
    unsigned char* val_ptr = reinterpret_cast<unsigned char*>(&val);

    if (*val_ptr == 0) {
      // big endian
      memcpy(internal_data, out_ptr, 8);
    } else {
      // little endian
      for (int32_t i=0; i < 8; i++) {
        internal_data[i] = out_ptr[7-i];
      }
    }
    info.GetReturnValue().Set(Nan::New(true));
  }

  static NAN_METHOD(SupportsFrameMetadata) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    rs2_frame_metadata_value metadata =
        (rs2_frame_metadata_value)(info[0]->IntegerValue());
    if (!me) return;

    int result = GetNativeResult<int>(rs2_supports_frame_metadata, &me->error_,
        me->frame_, metadata, &me->error_);
    info.GetReturnValue().Set(Nan::New(result ? true : false));
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      me->DestroyMe();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(CanGetPoints) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    bool result = false;
    if (GetNativeResult<int>(rs2_is_frame_extendable_to, &me->error_,
        me->frame_, RS2_EXTENSION_POINTS, &me->error_))
      result = true;
    info.GetReturnValue().Set(Nan::New(result));
  }

  static NAN_METHOD(GetVertices) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    rs2_vertex* vertices = GetNativeResult<rs2_vertex*>(rs2_get_frame_vertices,
        &me->error_, me->frame_, &me->error_);
    size_t count = GetNativeResult<size_t>(rs2_get_frame_points_count,
        &me->error_, me->frame_, &me->error_);
    if (!vertices || !count) return;

    uint32_t step = 3 * sizeof(float);
    uint32_t len = count * step;
    auto vertex_buf = static_cast<uint8_t*>(malloc(len));

    for (size_t i = 0; i < count; i++) {
      memcpy(vertex_buf+i*step, vertices[i].xyz, step);
    }
    auto array_buffer = v8::ArrayBuffer::New(
        v8::Isolate::GetCurrent(), vertex_buf, len,
        v8::ArrayBufferCreationMode::kInternalized);

    info.GetReturnValue().Set(v8::Float32Array::New(array_buffer, 0, 3*count));
  }

  static NAN_METHOD(GetVerticesBufferLen) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    const size_t count = GetNativeResult<size_t>(rs2_get_frame_points_count,
        &me->error_, me->frame_, &me->error_);
    const uint32_t step = 3 * sizeof(float);
    const uint32_t length = count * step;
    info.GetReturnValue().Set(Nan::New(length));
  }

  static NAN_METHOD(GetTexCoordBufferLen) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    const size_t count = GetNativeResult<size_t>(rs2_get_frame_points_count,
        &me->error_, me->frame_, &me->error_);
    const uint32_t step = 2 * sizeof(int);
    const uint32_t length = count * step;
    info.GetReturnValue().Set(Nan::New(length));
  }

  static NAN_METHOD(WriteVertices) {
    info.GetReturnValue().Set(Nan::False());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    auto array_buffer = v8::Local<v8::ArrayBuffer>::Cast(info[0]);
    if (!me) return;

    const rs2_vertex* vertBuf = GetNativeResult<rs2_vertex*>(
        rs2_get_frame_vertices, &me->error_, me->frame_, &me->error_);
    const size_t count = GetNativeResult<size_t>(rs2_get_frame_points_count,
        &me->error_, me->frame_, &me->error_);
    if (!vertBuf || !count) return;

    const uint32_t step = 3 * sizeof(float);
    const uint32_t length = count * step;
    if (array_buffer->ByteLength() < length) return;

    auto contents = array_buffer->GetContents();
    uint8_t* vertex_buf = static_cast<uint8_t*>(contents.Data());
    for (size_t i = 0; i < count; i++) {
      memcpy(vertex_buf+i*step, vertBuf[i].xyz, step);
    }
    info.GetReturnValue().Set(Nan::True());
  }

  static NAN_METHOD(GetTextureCoordinates) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    rs2_pixel* coords = GetNativeResult<rs2_pixel*>(
        rs2_get_frame_texture_coordinates, &me->error_, me->frame_,
        &me->error_);
    size_t count = GetNativeResult<size_t>(rs2_get_frame_points_count,
        &me->error_, me->frame_, &me->error_);
    if (!coords || !count) return;

    uint32_t step = 2 * sizeof(int);
    uint32_t len = count * step;
    auto texcoord_buf = static_cast<uint8_t*>(malloc(len));

    for (size_t i = 0; i < count; ++i) {
      memcpy(texcoord_buf + i * step, coords[i].ij, step);
    }
    auto array_buffer = v8::ArrayBuffer::New(
        v8::Isolate::GetCurrent(), texcoord_buf, len,
        v8::ArrayBufferCreationMode::kInternalized);

    info.GetReturnValue().Set(v8::Int32Array::New(array_buffer, 0, 2*count));
  }

  static NAN_METHOD(WriteTextureCoordinates) {
    info.GetReturnValue().Set(Nan::False());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    auto array_buffer = v8::Local<v8::ArrayBuffer>::Cast(info[0]);
    if (!me) return;

    const rs2_pixel* coords =
        rs2_get_frame_texture_coordinates(me->frame_, &me->error_);
    const size_t count = GetNativeResult<size_t>(rs2_get_frame_points_count,
        &me->error_, me->frame_, &me->error_);
    if (!coords || !count) return;

    const uint32_t step = 2 * sizeof(int);
    const uint32_t length = count * step;
    if (array_buffer->ByteLength() < length) return;

    auto contents = array_buffer->GetContents();
    uint8_t* texcoord_buf = static_cast<uint8_t*>(contents.Data());
    for (size_t i = 0; i < count; ++i) {
      memcpy(texcoord_buf + i * step, coords[i].ij, step);
    }
    info.GetReturnValue().Set(Nan::True());
  }

  static NAN_METHOD(GetPointsCount) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (!me) return;

    int32_t count = GetNativeResult<int>(rs2_get_frame_points_count,
        &me->error_, me->frame_, &me->error_);
    info.GetReturnValue().Set(Nan::New(count));
  }

  static NAN_METHOD(ExportToPly) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    v8::String::Utf8Value str(info[0]);
    std::string file = std::string(*str);
    auto texture = Nan::ObjectWrap::Unwrap<RSFrame>(info[1]->ToObject());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me || !texture) return;

    rs2_frame* ptr = nullptr;
    std::swap(texture->frame_, ptr);
    CallNativeFunc(rs2_export_to_ply, &me->error_, me->frame_, file.c_str(),
        ptr, &me->error_);
  }

  static NAN_METHOD(IsValid) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      if (me->frame_) {
        info.GetReturnValue().Set(Nan::True());
        return;
      }
    }
    info.GetReturnValue().Set(Nan::False());
  }

  static NAN_METHOD(GetDistance) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    auto x = info[0]->IntegerValue();
    auto y = info[1]->IntegerValue();
    if (!me) return;

    auto val = GetNativeResult<float>(rs2_depth_frame_get_distance, &me->error_,
        me->frame_, x, y, &me->error_);
    info.GetReturnValue().Set(Nan::New(val));
  }

  static NAN_METHOD(GetBaseLine) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto val = GetNativeResult<float>(rs2_depth_stereo_frame_get_baseline,
        &me->error_, me->frame_, &me->error_);
    info.GetReturnValue().Set(Nan::New(val));
  }

  static NAN_METHOD(Keep) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me || !me->frame_) return;

    rs2_keep_frame(me->frame_);
  }

  static NAN_METHOD(GetMotionData) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto obj = info[0]->ToObject();
    auto frame_data = static_cast<const float*>(GetNativeResult<const void*>(
        rs2_get_frame_data, &me->error_, me->frame_, &me->error_));
    for (uint32_t i = 0; i < 3; i++) {
      SetAFloatInVectorObject(obj, i, frame_data[i]);
    }
  }

  static NAN_METHOD(GetPoseData) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    info.GetReturnValue().Set(Nan::False());
    if (!me) return;

    rs2_pose pose_data;
    CallNativeFunc(rs2_pose_frame_get_pose_data, &me->error_, me->frame_,
        &pose_data, &me->error_);
    if (me->error_) return;

    AssemblePoseData(info[0]->ToObject(), pose_data);
    info.GetReturnValue().Set(Nan::True());
  }

 private:
  static Nan::Persistent<v8::Function> constructor_;
  rs2_frame* frame_;
  rs2_error* error_;
  friend class RSColorizer;
  friend class RSFilter;
  friend class RSFrameQueue;
  friend class RSPointCloud;
  friend class RSSyncer;
};

Nan::Persistent<v8::Function> RSFrame::constructor_;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// 8888888888                                      .d88888b.                 //
// 888                                            d88P" "Y88b                //
// 888                                            888     888                //
// 8888888 888d888 8888b.  88888b.d88b.   .d88b.  888     888                //
// 888     888P"      "88b 888 "888 "88b d8P  Y8b 888     888                //
// 888     888    .d888888 888  888  888 88888888 888 Y8b 888                //
// 888     888    888  888 888  888  888 Y8b.     Y88b.Y8b88P                //
// 888     888    "Y888888 888  888  888  "Y8888   "Y888888"                 //
//                                                                           //
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// TODO(shaoting) remove this class if not used in future.
class RSFrameQueue : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSFrameQueue").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "create", Create);
    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "waitForFrame", WaitForFrame);
    Nan::SetPrototypeMethod(tpl, "pollForFrame", PollForFrame);
    Nan::SetPrototypeMethod(tpl, "enqueueFrame", EnqueueFrame);

    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSFrameQueue").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance() {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor_);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    return scope.Escape(instance);
  }

 private:
  RSFrameQueue() : frame_queue_(nullptr), error_(nullptr) {}

  ~RSFrameQueue() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;
    if (frame_queue_) rs2_delete_frame_queue(frame_queue_);
    frame_queue_ = nullptr;
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSFrameQueue* obj = new RSFrameQueue();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(WaitForFrame) {
    info.GetReturnValue().Set(Nan::Undefined());
    int32_t timeout = info[0]->IntegerValue();  // in ms
    auto me = Nan::ObjectWrap::Unwrap<RSFrameQueue>(info.Holder());
    if (!me) return;

    auto frame = GetNativeResult<rs2_frame*>(rs2_wait_for_frame, &me->error_,
        me->frame_queue_, timeout, &me->error_);
    if (!frame) return;

    info.GetReturnValue().Set(RSFrame::NewInstance(frame));
  }

  static NAN_METHOD(Create) {
    info.GetReturnValue().Set(Nan::Undefined());
    int32_t capacity = info[0]->IntegerValue();
    auto me = Nan::ObjectWrap::Unwrap<RSFrameQueue>(info.Holder());
    if (!me) return;

    me->frame_queue_ = GetNativeResult<rs2_frame_queue*>(rs2_create_frame_queue,
        &me->error_, capacity, &me->error_);
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrameQueue>(info.Holder());
    if (me) {
      me->DestroyMe();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(PollForFrame) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrameQueue>(info.Holder());
    if (!me) return;

    rs2_frame* frame = nullptr;
    auto res = GetNativeResult<int>(rs2_poll_for_frame, &me->error_,
        me->frame_queue_, &frame, &me->error_);
    if (!res) return;

    info.GetReturnValue().Set(RSFrame::NewInstance(frame));
  }

  static NAN_METHOD(EnqueueFrame) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrameQueue>(info.Holder());
    auto frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[0]->ToObject());

    if (me && frame) {
      rs2_enqueue_frame(frame->frame_, me->frame_queue_);
      frame->frame_ = nullptr;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

 private:
  static Nan::Persistent<v8::Function> constructor_;
  rs2_frame_queue* frame_queue_;
  rs2_error* error_;
  friend class RSDevice;
};

Nan::Persistent<v8::Function> RSFrameQueue::constructor_;

class RSDevice;
class RSSensor;
class FrameCallbackInfo : public MainThreadCallbackInfo {
 public:
  FrameCallbackInfo(rs2_frame* frame,  void* data) :
      frame_(frame), sensor_(static_cast<RSSensor*>(data)) {}
  virtual ~FrameCallbackInfo() { if (!consumed_) Release(); }
  virtual void Run();
  virtual void Release() {
    if (frame_) {
      rs2_release_frame(frame_);
      frame_ = nullptr;
    }
  }

 private:
  rs2_frame* frame_;
  RSSensor* sensor_;
};

class NotificationCallbackInfo : public MainThreadCallbackInfo {
 public:
  NotificationCallbackInfo(const char* desc,
                           rs2_time_t time,
                           rs2_log_severity severity,
                           rs2_notification_category category,
                           std::string serialized_data,
                           RSSensor* s) :
      desc_(desc), time_(time), severity_(severity),
      category_(category), serialized_data_(serialized_data),
      sensor_(s) {}
  virtual ~NotificationCallbackInfo() {}
  virtual void Run();

 private:
  std::string desc_;
  rs2_time_t time_;
  rs2_log_severity severity_;
  rs2_notification_category category_;
  std::string serialized_data_;
  RSSensor* sensor_;
  rs2_error* error_;
};

class NotificationCallback : public rs2_notifications_callback {
 public:
  explicit NotificationCallback(RSSensor* s) : error_(nullptr), sensor_(s) {}
  void on_notification(rs2_notification* notification) override {
    if (notification) {
      const char* desc = GetNativeResult<const char*>(
          rs2_get_notification_description, &error_, notification, &error_);
      rs2_time_t time = GetNativeResult<rs2_time_t>(
          rs2_get_notification_timestamp, &error_, notification, &error_);
      rs2_log_severity severity = GetNativeResult<rs2_log_severity>(
          rs2_get_notification_severity, &error_, notification, &error_);
      rs2_notification_category category =
          GetNativeResult<rs2_notification_category>(
          rs2_get_notification_category, &error_, notification, &error_);
      std::string serialized_data = GetNativeResult<std::string>(
          rs2_get_notification_serialized_data, &error_, notification, &error_);
      MainThreadCallback::NotifyMainThread(new NotificationCallbackInfo(desc,
          time, severity, category, serialized_data, sensor_));
    }
  }
  void release() override { delete this; }
  rs2_error* error_;
  RSSensor* sensor_;
};

class FrameCallbackForFrameQueue : public rs2_frame_callback {
 public:
  explicit FrameCallbackForFrameQueue(rs2_frame_queue* queue)
      : frame_queue_(queue) {}
  void on_frame(rs2_frame* frame) override {
    if (frame && frame_queue_)
      rs2_enqueue_frame(frame, frame_queue_);
  }
  void release() override { delete this; }
  rs2_frame_queue* frame_queue_;
};

class FrameCallbackForProc : public rs2_frame_callback {
 public:
  explicit FrameCallbackForProc(void* data) : callback_data_(data) {}
  void on_frame(rs2_frame* frame) override {
    MainThreadCallback::NotifyMainThread(
      new FrameCallbackInfo(frame, callback_data_));
  }
  void release() override { delete this; }
  void* callback_data_;
};

class FrameCallbackForProcessingBlock : public rs2_frame_callback {
 public:
  explicit FrameCallbackForProcessingBlock(rs2_processing_block* block_ptr) :
      block_(block_ptr), error_(nullptr) {}
  virtual ~FrameCallbackForProcessingBlock() {
    if (error_) rs2_free_error(error_);
  }
  void on_frame(rs2_frame* frame) override {
    rs2_process_frame(block_, frame, &error_);
  }
  void release() override { delete this; }
  rs2_processing_block* block_;
  rs2_error* error_;
};

class PlaybackStatusCallbackInfo : public MainThreadCallbackInfo {
 public:
  PlaybackStatusCallbackInfo(rs2_playback_status status, RSDevice* dev) :
      status_(status), dev_(dev), error_(nullptr) {}
  virtual ~PlaybackStatusCallbackInfo() {}
  virtual void Run();

 private:
  rs2_playback_status status_;
  RSDevice* dev_;
  rs2_error* error_;
};

class PlaybackStatusCallback : public rs2_playback_status_changed_callback {
 public:
  explicit PlaybackStatusCallback(RSDevice* dev) : error_(nullptr), dev_(dev) {}
  void on_playback_status_changed(rs2_playback_status status) override {
    MainThreadCallback::NotifyMainThread(new PlaybackStatusCallbackInfo(status,
        dev_));
  }
  void release() override { delete this; }

 private:
  rs2_error* error_;
  RSDevice* dev_;
};

class StreamProfileExtrator {
 public:
  explicit StreamProfileExtrator(const rs2_stream_profile* profile) {
    rs2_get_stream_profile_data(profile, &stream_, &format_, &index_,
        &unique_id_, &fps_, &error_);
  }
  ~StreamProfileExtrator() {}
  rs2_stream stream_;
  rs2_format format_;
  int32_t fps_;
  int32_t index_;
  int32_t unique_id_;
  rs2_error* error_;
};

class RSFrameSet : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSFrameSet").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "getSize", GetSize);
    Nan::SetPrototypeMethod(tpl, "getFrame", GetFrame);
    Nan::SetPrototypeMethod(tpl, "replaceFrame", ReplaceFrame);
    Nan::SetPrototypeMethod(tpl, "indexToStream", IndexToStream);
    Nan::SetPrototypeMethod(tpl, "indexToStreamIndex", IndexToStreamIndex);

    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSFrameSet").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(rs2_frame* frame) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor_);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();
    auto me = Nan::ObjectWrap::Unwrap<RSFrameSet>(instance);
    me->SetFrame(frame);

    return scope.Escape(instance);
  }

  rs2_frame* GetFrames() {
    return frames_;
  }

  void Replace(rs2_frame* frame) {
    DestroyMe();
    SetFrame(frame);
  }

 private:
  RSFrameSet() {
    error_ = nullptr;
    frames_ = nullptr;
  }

  ~RSFrameSet() {
    DestroyMe();
  }

  void SetFrame(rs2_frame* frame) {
    if (!frame || (!GetNativeResult<int>(rs2_is_frame_extendable_to, &error_,
        frame, RS2_EXTENSION_COMPOSITE_FRAME, &error_))) return;

    frames_ = frame;
    frame_count_ = GetNativeResult<int>(rs2_embedded_frames_count, &error_,
        frame, &error_);
  }

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;
    if (frames_) rs2_release_frame(frames_);
    frames_ = nullptr;
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrameSet>(info.Holder());
    if (me) {
      me->DestroyMe();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSFrameSet* obj = new RSFrameSet();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(GetSize) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrameSet>(info.Holder());
    if (me && me->frames_) {
      info.GetReturnValue().Set(Nan::New(me->frame_count_));
      return;
    }
    info.GetReturnValue().Set(Nan::New(0));
  }

  static NAN_METHOD(GetFrame) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrameSet>(info.Holder());
    if (!me || !me->frames_) return;

    rs2_stream stream = static_cast<rs2_stream>(info[0]->IntegerValue());
    auto stream_index = info[1]->IntegerValue();
    // if RS2_STREAM_ANY is used, we return the first frame.
    if (stream == RS2_STREAM_ANY && me->frame_count_) {
      rs2_frame* frame = GetNativeResult<rs2_frame*>(rs2_extract_frame,
          &me->error_, me->frames_, 0, &me->error_);
      if (!frame) return;

      info.GetReturnValue().Set(RSFrame::NewInstance(frame));
      return;
    }

    for (uint32_t i=0; i < me->frame_count_; i++) {
      rs2_frame* frame = GetNativeResult<rs2_frame*>(rs2_extract_frame,
          &me->error_, me->frames_, i, &me->error_);
      if (!frame) continue;

      const rs2_stream_profile* profile =
          GetNativeResult<const rs2_stream_profile*>(
          rs2_get_frame_stream_profile, &me->error_, frame, &me->error_);
      if (profile) {
        StreamProfileExtrator extrator(profile);
        if (extrator.stream_ == stream && (!stream_index ||
            (stream_index && stream_index == extrator.index_))) {
          info.GetReturnValue().Set(RSFrame::NewInstance(frame));
          return;
        }
      }
      rs2_release_frame(frame);
    }
  }

  static NAN_METHOD(ReplaceFrame) {
    info.GetReturnValue().Set(Nan::False());
    auto me = Nan::ObjectWrap::Unwrap<RSFrameSet>(info.Holder());
    rs2_stream stream = static_cast<rs2_stream>(info[0]->IntegerValue());
    auto stream_index = info[1]->IntegerValue();
    auto target_frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[2]->ToObject());

    if (!me || !me->frames_) return;

    for (uint32_t i = 0; i < me->frame_count_; i++) {
      rs2_frame* frame = GetNativeResult<rs2_frame*>(rs2_extract_frame,
          &me->error_, me->frames_, i, &me->error_);
      if (!frame) continue;

      const rs2_stream_profile* profile =
          GetNativeResult<const rs2_stream_profile*>(
          rs2_get_frame_stream_profile, &me->error_, frame, &me->error_);
      if (profile) {
      StreamProfileExtrator extrator(profile);
        if (extrator.stream_ == stream && (!stream_index ||
            (stream_index && stream_index == extrator.index_))) {
          target_frame->Replace(frame);
          info.GetReturnValue().Set(Nan::True());
          return;
        }
      }
      rs2_release_frame(frame);
    }
  }

  static NAN_METHOD(IndexToStream) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSFrameSet>(info.Holder());
    if (!me || !me->frames_) return;

    int32_t index = info[0]->IntegerValue();
    rs2_frame* frame = GetNativeResult<rs2_frame*>(rs2_extract_frame,
        &me->error_, me->frames_, index, &me->error_);
    if (!frame) return;

    const rs2_stream_profile* profile =
        GetNativeResult<const rs2_stream_profile*>(rs2_get_frame_stream_profile,
        &me->error_, frame, &me->error_);
    if (!profile) {
      rs2_release_frame(frame);
      return;
    }

    rs2_stream stream = RS2_STREAM_ANY;
    rs2_format format = RS2_FORMAT_ANY;
    int32_t fps = 0;
    int32_t idx = 0;
    int32_t unique_id = 0;
    CallNativeFunc(rs2_get_stream_profile_data, &me->error_, profile, &stream,
        &format, &idx, &unique_id, &fps, &me->error_);
    rs2_release_frame(frame);
    if (me->error_) return;

    info.GetReturnValue().Set(Nan::New(stream));
  }

  static NAN_METHOD(IndexToStreamIndex) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrameSet>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me || !me->frames_) return;

    int32_t index = info[0]->IntegerValue();
    rs2_frame* frame = GetNativeResult<rs2_frame*>(rs2_extract_frame,
        &me->error_, me->frames_, index, &me->error_);
    if (!frame) return;

    const rs2_stream_profile* profile =
        GetNativeResult<const rs2_stream_profile*>(rs2_get_frame_stream_profile,
        &me->error_, frame, &me->error_);
    if (!profile) {
      rs2_release_frame(frame);
      return;
    }
    rs2_stream stream = RS2_STREAM_ANY;
    rs2_format format = RS2_FORMAT_ANY;
    int32_t fps = 0;
    int32_t idx = 0;
    int32_t unique_id = 0;
    CallNativeFunc(rs2_get_stream_profile_data, &me->error_, profile, &stream,
        &format, &idx, &unique_id, &fps, &me->error_);
    rs2_release_frame(frame);
    if (me->error_) return;

    info.GetReturnValue().Set(Nan::New(idx));
  }

 private:
  static Nan::Persistent<v8::Function> constructor_;
  rs2_frame* frames_;
  uint32_t frame_count_;
  rs2_error* error_;
};

Nan::Persistent<v8::Function> RSFrameSet::constructor_;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//  .d8888b.                                                                 //
// d88P  Y88b                                                                //
// Y88b.                                                                     //
//  "Y888b.   888  888 88888b.   .d8888b .d88b.  888d888                     //
//     "Y88b. 888  888 888 "88b d88P"   d8P  Y8b 888P"                       //
//       "888 888  888 888  888 888     88888888 888                         //
// Y88b  d88P Y88b 888 888  888 Y88b.   Y8b.     888                         //
//  "Y8888P"   "Y88888 888  888  "Y8888P "Y8888  888                         //
//                 888                                                       //
//            Y8b d88P                                                       //
//             "Y88P"                                                        //
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class RSSyncer : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSSyncer").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "waitForFrames", WaitForFrames);
    Nan::SetPrototypeMethod(tpl, "pollForFrames", PollForFrames);

    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSSyncer").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance() {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor_);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    return scope.Escape(instance);
  }

 private:
  RSSyncer() : syncer_(nullptr), frame_queue_(nullptr), error_(nullptr) {}

  ~RSSyncer() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;
    if (syncer_) rs2_delete_processing_block(syncer_);
    syncer_ = nullptr;
    if (frame_queue_) rs2_delete_frame_queue(frame_queue_);
    frame_queue_ = nullptr;
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSSyncer* obj = new RSSyncer();
      obj->syncer_ = GetNativeResult<rs2_processing_block*>(
          rs2_create_sync_processing_block, &obj->error_, &obj->error_);
      obj->frame_queue_ = GetNativeResult<rs2_frame_queue*>(
          rs2_create_frame_queue, &obj->error_, 1, &obj->error_);
      auto callback = new FrameCallbackForFrameQueue(obj->frame_queue_);
      CallNativeFunc(rs2_start_processing, &obj->error_, obj->syncer_, callback,
          &obj->error_);
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(WaitForFrames) {
    info.GetReturnValue().Set(Nan::False());
    auto me = Nan::ObjectWrap::Unwrap<RSSyncer>(info.Holder());
    auto frameset = Nan::ObjectWrap::Unwrap<RSFrameSet>(info[0]->ToObject());
    auto timeout = info[1]->IntegerValue();
    if (!me || !frameset) return;

    rs2_frame* frames = GetNativeResult<rs2_frame*>(rs2_wait_for_frame,
        &me->error_, me->frame_queue_, timeout, &me->error_);
    if (!frames) return;

    frameset->Replace(frames);
    info.GetReturnValue().Set(Nan::True());
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSSyncer>(info.Holder());
    if (me) {
      me->DestroyMe();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(PollForFrames) {
    info.GetReturnValue().Set(Nan::False());
    auto me = Nan::ObjectWrap::Unwrap<RSSyncer>(info.Holder());
    auto frameset = Nan::ObjectWrap::Unwrap<RSFrameSet>(info[0]->ToObject());
    if (!me || !frameset) return;

    rs2_frame* frame_ref = nullptr;
    auto res = GetNativeResult<int>(rs2_poll_for_frame, &me->error_,
        me->frame_queue_, &frame_ref, &me->error_);
    if (!res) return;

    frameset->Replace(frame_ref);
    info.GetReturnValue().Set(Nan::True());
  }

 private:
  static Nan::Persistent<v8::Function> constructor_;
  rs2_processing_block* syncer_;
  rs2_frame_queue* frame_queue_;
  rs2_error* error_;
  friend class RSSensor;
};

Nan::Persistent<v8::Function> RSSyncer::constructor_;

class Options {
 public:
  Options() : error_(nullptr) {}

  virtual ~Options() {
    if (error_) rs2_free_error(error_);
  }

  virtual rs2_options* GetOptionsPointer() = 0;

  void SupportsOptionInternal(
      const Nan::FunctionCallbackInfo<v8::Value>& info) {
    int32_t option = info[0]->IntegerValue();
    auto on = GetNativeResult<int>(rs2_supports_option, &error_,
        GetOptionsPointer(), static_cast<rs2_option>(option), &error_);
    info.GetReturnValue().Set(Nan::New(on ? true : false));
    return;
  }

  void GetOptionInternal(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    int32_t option = info[0]->IntegerValue();
    auto value = GetNativeResult<float>(rs2_get_option, &error_,
        GetOptionsPointer(), static_cast<rs2_option>(option), &error_);
    info.GetReturnValue().Set(Nan::New(value));
  }

  void GetOptionDescriptionInternal(
      const Nan::FunctionCallbackInfo<v8::Value>& info) {
    int32_t option = info[0]->IntegerValue();
    auto desc = GetNativeResult<const char*>(rs2_get_option_description,
        &error_, GetOptionsPointer(), static_cast<rs2_option>(option), &error_);
    if (desc)
      info.GetReturnValue().Set(Nan::New(desc).ToLocalChecked());
    else
      info.GetReturnValue().Set(Nan::Undefined());
  }

  void GetOptionValueDescriptionInternal(
      const Nan::FunctionCallbackInfo<v8::Value>& info) {
    int32_t option = info[0]->IntegerValue();
    auto val = info[1]->NumberValue();
    auto desc = GetNativeResult<const char*>(rs2_get_option_value_description,
        &error_, GetOptionsPointer(), static_cast<rs2_option>(option),
        val, &error_);
    if (desc)
      info.GetReturnValue().Set(Nan::New(desc).ToLocalChecked());
    else
      info.GetReturnValue().Set(Nan::Undefined());
  }

  void SetOptionInternal(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    int32_t option = info[0]->IntegerValue();
    auto val = info[1]->NumberValue();
    CallNativeFunc(rs2_set_option, &error_, GetOptionsPointer(),
        static_cast<rs2_option>(option), val, &error_);
    info.GetReturnValue().Set(Nan::Undefined());
  }

  void GetOptionRangeInternal(
      const Nan::FunctionCallbackInfo<v8::Value>& info) {
    int32_t option = info[0]->IntegerValue();
    float min = 0;
    float max = 0;
    float step = 0;
    float def = 0;
    CallNativeFunc(rs2_get_option_range, &error_, GetOptionsPointer(),
        static_cast<rs2_option>(option), &min, &max, &step, &def, &error_);
    info.GetReturnValue().Set(RSOptionRange(min, max, step, def).GetObject());
  }

  void IsOptionReadonlyInternal(
      const Nan::FunctionCallbackInfo<v8::Value>& info) {
    int32_t option = info[0]->IntegerValue();
    auto val = GetNativeResult<int>(rs2_is_option_read_only, &error_,
        GetOptionsPointer(), static_cast<rs2_option>(option), &error_);
    info.GetReturnValue().Set(Nan::New((val) ? true : false));
  }

 private:
  rs2_error* error_;
};

class RSSensor : public Nan::ObjectWrap, Options {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSSensor").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "openStream", OpenStream);
    Nan::SetPrototypeMethod(tpl, "openMultipleStream", OpenMultipleStream);
    Nan::SetPrototypeMethod(tpl, "getCameraInfo", GetCameraInfo);
    Nan::SetPrototypeMethod(tpl, "startWithSyncer", StartWithSyncer);
    Nan::SetPrototypeMethod(tpl, "startWithCallback", StartWithCallback);
    Nan::SetPrototypeMethod(tpl, "supportsOption", SupportsOption);
    Nan::SetPrototypeMethod(tpl, "getOption", GetOption);
    Nan::SetPrototypeMethod(tpl, "setOption", SetOption);
    Nan::SetPrototypeMethod(tpl, "getOptionRange", GetOptionRange);
    Nan::SetPrototypeMethod(tpl, "isOptionReadonly", IsOptionReadonly);
    Nan::SetPrototypeMethod(tpl, "getOptionDescription", GetOptionDescription);
    Nan::SetPrototypeMethod(tpl, "getOptionValueDescription",
        GetOptionValueDescription);
    Nan::SetPrototypeMethod(tpl, "stop", Stop);
    Nan::SetPrototypeMethod(tpl, "supportsCameraInfo", SupportsCameraInfo);
    Nan::SetPrototypeMethod(tpl, "getStreamProfiles", GetStreamProfiles);
    Nan::SetPrototypeMethod(tpl, "close", Close);
    Nan::SetPrototypeMethod(tpl, "setNotificationCallback",
        SetNotificationCallback);
    Nan::SetPrototypeMethod(tpl, "setRegionOfInterest", SetRegionOfInterest);
    Nan::SetPrototypeMethod(tpl, "getRegionOfInterest", GetRegionOfInterest);
    Nan::SetPrototypeMethod(tpl, "getDepthScale", GetDepthScale);
    Nan::SetPrototypeMethod(tpl, "isROISensor", IsROISensor);
    Nan::SetPrototypeMethod(tpl, "is", Is);
    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSSensor").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(rs2_sensor* sensor) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor_);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(instance);
    me->sensor_ = sensor;
    return scope.Escape(instance);
  }

  rs2_options* GetOptionsPointer() override {
    // TODO(shaoting) find better way to avoid the reinterpret_cast which was
    // caused the inheritance relation was hidden
    return reinterpret_cast<rs2_options*>(sensor_);
  }

  void ReplaceFrame(rs2_frame* raw_frame) {
    // clear old frame first.
    frame_->Replace(nullptr);
    video_frame_->Replace(nullptr);
    depth_frame_->Replace(nullptr);
    disparity_frame_->Replace(nullptr);
    motion_frame_->Replace(nullptr);
    pose_frame_->Replace(nullptr);

    if (GetNativeResult<int>(rs2_is_frame_extendable_to, &error_,
        raw_frame, RS2_EXTENSION_DISPARITY_FRAME, &error_)) {
      disparity_frame_->Replace(raw_frame);
    } else if (GetNativeResult<int>(rs2_is_frame_extendable_to, &error_,
        raw_frame, RS2_EXTENSION_DEPTH_FRAME, &error_)) {
      depth_frame_->Replace(raw_frame);
    } else if (GetNativeResult<int>(rs2_is_frame_extendable_to, &error_,
        raw_frame, RS2_EXTENSION_VIDEO_FRAME, &error_)) {
      video_frame_->Replace(raw_frame);
    } else if (GetNativeResult<int>(rs2_is_frame_extendable_to, &error_,
        raw_frame, RS2_EXTENSION_MOTION_FRAME, &error_)) {
      motion_frame_->Replace(raw_frame);
    } else if (GetNativeResult<int>(rs2_is_frame_extendable_to, &error_,
        raw_frame, RS2_EXTENSION_POSE_FRAME, &error_)) {
      pose_frame_->Replace(raw_frame);
    } else {
      frame_->Replace(raw_frame);
    }
  }

 private:
  RSSensor() : sensor_(nullptr), error_(nullptr), profile_list_(nullptr),
      frame_(nullptr), video_frame_(nullptr), depth_frame_(nullptr),
      disparity_frame_(nullptr), motion_frame_(nullptr), pose_frame_(nullptr) {}

  ~RSSensor() {
    DestroyMe();
  }

  void RegisterNotificationCallbackMethod();

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;
    if (sensor_) rs2_delete_sensor(sensor_);
    sensor_ = nullptr;
    if (profile_list_) rs2_delete_stream_profiles_list(profile_list_);
    profile_list_ = nullptr;
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSSensor* obj = new RSSensor();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(SupportsOption) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) return me->SupportsOptionInternal(info);

    info.GetReturnValue().Set(Nan::False());
  }

  static NAN_METHOD(GetOption) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) return me->GetOptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetOptionDescription) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) return me->GetOptionDescriptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetOptionValueDescription) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) return me->GetOptionValueDescriptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(SetOption) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) return me->SetOptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetOptionRange) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) return me->GetOptionRangeInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(IsOptionReadonly) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) return me->IsOptionReadonlyInternal(info);

    info.GetReturnValue().Set(Nan::False());
  }

  static NAN_METHOD(GetCameraInfo) {
    info.GetReturnValue().Set(Nan::Undefined());
    int32_t camera_info = info[0]->IntegerValue();;
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (!me) return;

    auto value = GetNativeResult<const char*>(rs2_get_sensor_info,
        &me->error_, me->sensor_, static_cast<rs2_camera_info>(camera_info),
        &me->error_);
    if (me->error_) return;

    info.GetReturnValue().Set(Nan::New(value).ToLocalChecked());
  }

  static NAN_METHOD(StartWithSyncer) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto syncer = Nan::ObjectWrap::Unwrap<RSSyncer>(info[0]->ToObject());
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (!me || !syncer) return;

    CallNativeFunc(rs2_start_cpp, &me->error_, me->sensor_,
        new FrameCallbackForProcessingBlock(syncer->syncer_), &me->error_);
  }

  static NAN_METHOD(StartWithCallback) {
    auto frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[1]->ToObject());
    auto depth_frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[2]->ToObject());
    auto video_frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[3]->ToObject());
    auto disparity_frame = Nan::ObjectWrap::Unwrap<RSFrame>(
        info[4]->ToObject());
    auto motion_frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[5]->ToObject());
    auto pose_frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[6]->ToObject());
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me && frame && depth_frame && video_frame && disparity_frame &&
        motion_frame && pose_frame) {
      me->frame_ = frame;
      me->depth_frame_ = depth_frame;
      me->video_frame_ = video_frame;
      me->disparity_frame_ = disparity_frame;
      me->motion_frame_ = motion_frame;
      me->pose_frame_ = pose_frame;
      v8::String::Utf8Value str(info[0]);
      me->frame_callback_name_ = std::string(*str);
      CallNativeFunc(rs2_start_cpp, &me->error_, me->sensor_,
          new FrameCallbackForProc(me), &me->error_);
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      me->DestroyMe();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(OpenStream) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    auto profile =
        Nan::ObjectWrap::Unwrap<RSStreamProfile>(info[0]->ToObject());
    if (!me || !profile) return;

    CallNativeFunc(rs2_open, &me->error_, me->sensor_, profile->profile_,
        &me->error_);
  }

  static NAN_METHOD(OpenMultipleStream) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (!me) return;

    auto array = v8::Local<v8::Array>::Cast(info[0]);
    uint32_t len = array->Length();
    std::vector<const rs2_stream_profile*> profs;
    for (uint32_t i=0; i < len; i++) {
      auto profile =
          Nan::ObjectWrap::Unwrap<RSStreamProfile>(array->Get(i)->ToObject());
      profs.push_back(profile->profile_);
    }
    CallNativeFunc(rs2_open_multiple, &me->error_, me->sensor_, profs.data(),
        len, &me->error_);
  }

  static NAN_METHOD(Stop) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (!me) return;

    CallNativeFunc(rs2_stop, &me->error_, me->sensor_, &me->error_);
  }

  static NAN_METHOD(GetStreamProfiles) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (!me) return;

    rs2_stream_profile_list* list = me->profile_list_;
    if (!list) {
      list = GetNativeResult<rs2_stream_profile_list*>(rs2_get_stream_profiles,
          &me->error_, me->sensor_, &me->error_);
      me->profile_list_ = list;
    }
    if (!list) return;

    int32_t size = GetNativeResult<int>(rs2_get_stream_profiles_count,
        &me->error_, list, &me->error_);
    v8::Local<v8::Array> array = Nan::New<v8::Array>(size);
    for (int32_t i = 0; i < size; i++) {
      rs2_stream_profile* profile = const_cast<rs2_stream_profile*>(
          rs2_get_stream_profile(list, i, &me->error_));
      array->Set(i, RSStreamProfile::NewInstance(profile));
    }
    info.GetReturnValue().Set(array);
  }

  static NAN_METHOD(SupportsCameraInfo) {
    info.GetReturnValue().Set(Nan::False());
    int32_t camera_info = info[0]->IntegerValue();
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (!me) return;

    int32_t on = GetNativeResult<int>(rs2_supports_sensor_info, &me->error_,
        me->sensor_, (rs2_camera_info)camera_info, &me->error_);
    info.GetReturnValue().Set(Nan::New(on ? true : false));
  }

  static NAN_METHOD(Close) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (!me) return;

    CallNativeFunc(rs2_close, &me->error_, me->sensor_, &me->error_);
  }

  static NAN_METHOD(SetNotificationCallback) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (!me) return;

    v8::String::Utf8Value value(info[0]->ToString());
    me->notification_callback_name_ = std::string(*value);
    me->RegisterNotificationCallbackMethod();
  }

  static NAN_METHOD(SetRegionOfInterest) {
    info.GetReturnValue().Set(Nan::Undefined());
    int32_t minx = info[0]->IntegerValue();
    int32_t miny = info[1]->IntegerValue();
    int32_t maxx = info[2]->IntegerValue();
    int32_t maxy = info[3]->IntegerValue();
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (!me) return;

    CallNativeFunc(rs2_set_region_of_interest, &me->error_, me->sensor_, minx,
        miny, maxx, maxy, &me->error_);
  }

  static NAN_METHOD(GetRegionOfInterest) {
    info.GetReturnValue().Set(Nan::Undefined());
    int32_t minx = 0;
    int32_t miny = 0;
    int32_t maxx = 0;
    int32_t maxy = 0;
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (!me) return;

    CallNativeFunc(rs2_get_region_of_interest, &me->error_, me->sensor_, &minx,
        &miny, &maxx, &maxy, &me->error_);
    if (me->error_) return;
    info.GetReturnValue().Set(
        RSRegionOfInterest(minx, miny, maxx, maxy).GetObject());
  }

  static NAN_METHOD(GetDepthScale) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (!me) return;

    auto scale = GetNativeResult<float>(rs2_get_depth_scale, &me->error_,
        me->sensor_, &me->error_);
    if (me->error_) return;

    info.GetReturnValue().Set(Nan::New(scale));
  }

  static NAN_METHOD(Is) {
    info.GetReturnValue().Set(Nan::Undefined());
    int32_t stype = info[0]->IntegerValue();

    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (!me) return;

    bool is_ok = GetNativeResult<int>(rs2_is_sensor_extendable_to,
        &me->error_, me->sensor_, static_cast<rs2_extension>(stype), &me->error_);
    if (me->error_) return;

    info.GetReturnValue().Set(Nan::New(is_ok));
  }

  static NAN_METHOD(IsROISensor) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (!me) return;

    bool is_roi = GetNativeResult<int>(rs2_is_sensor_extendable_to, &me->error_,
        me->sensor_, RS2_EXTENSION_ROI, &me->error_);
    if (me->error_) return;

    info.GetReturnValue().Set(Nan::New(is_roi));
  }

 private:
  static Nan::Persistent<v8::Function> constructor_;
  rs2_sensor* sensor_;
  rs2_error* error_;
  rs2_stream_profile_list* profile_list_;
  std::string frame_callback_name_;
  std::string notification_callback_name_;
  RSFrame* frame_;
  RSFrame* video_frame_;
  RSFrame* depth_frame_;
  RSFrame* disparity_frame_;
  RSFrame* motion_frame_;
  RSFrame* pose_frame_;
  friend class RSContext;
  friend class DevicesChangedCallbackInfo;
  friend class FrameCallbackInfo;
  friend class NotificationCallbackInfo;
};

Nan::Persistent<v8::Function> RSSensor::constructor_;

void RSSensor::RegisterNotificationCallbackMethod() {
  CallNativeFunc(rs2_set_notifications_callback_cpp, &error_, sensor_,
      new NotificationCallback(this), &error_);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// 8888888b.   .d8888b.  8888888b.                    d8b                    //
// 888   Y88b d88P  Y88b 888  "Y88b                   Y8P                    //
// 888    888 Y88b.      888    888                                          //
// 888   d88P  "Y888b.   888    888  .d88b.  888  888 888  .d8888b .d88b.    //
// 8888888P"      "Y88b. 888    888 d8P  Y8b 888  888 888 d88P"   d8P  Y8b   //
// 888 T88b         "888 888    888 88888888 Y88  88P 888 888     88888888   //
// 888  T88b  Y88b  d88P 888  .d88P Y8b.      Y8bd8P  888 Y88b.   Y8b.       //
// 888   T88b  "Y8888P"  8888888P"   "Y8888    Y88P   888  "Y8888P "Y8888    //
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class RSDevice : public Nan::ObjectWrap {
 public:
  enum DeviceType {
    kNormalDevice = 0,
    kRecorderDevice,
    kPlaybackDevice,
  };
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSDevice").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "getCameraInfo", GetCameraInfo);
    Nan::SetPrototypeMethod(tpl, "supportsCameraInfo", SupportsCameraInfo);
    Nan::SetPrototypeMethod(tpl, "reset", Reset);
    Nan::SetPrototypeMethod(tpl, "querySensors", QuerySensors);
    Nan::SetPrototypeMethod(tpl, "triggerErrorForTest", TriggerErrorForTest);
    Nan::SetPrototypeMethod(tpl, "spawnRecorderDevice", SpawnRecorderDevice);

    // Methods for record or playback
    Nan::SetPrototypeMethod(tpl, "pauseRecord", PauseRecord);
    Nan::SetPrototypeMethod(tpl, "resumeRecord", ResumeRecord);
    Nan::SetPrototypeMethod(tpl, "getFileName", GetFileName);
    Nan::SetPrototypeMethod(tpl, "pausePlayback", PausePlayback);
    Nan::SetPrototypeMethod(tpl, "resumePlayback", ResumePlayback);
    Nan::SetPrototypeMethod(tpl, "stopPlayback", StopPlayback);
    Nan::SetPrototypeMethod(tpl, "getPosition", GetPosition);
    Nan::SetPrototypeMethod(tpl, "getDuration", GetDuration);
    Nan::SetPrototypeMethod(tpl, "seek", Seek);
    Nan::SetPrototypeMethod(tpl, "isRealTime", IsRealTime);
    Nan::SetPrototypeMethod(tpl, "setIsRealTime", SetIsRealTime);
    Nan::SetPrototypeMethod(tpl, "setPlaybackSpeed", SetPlaybackSpeed);
    Nan::SetPrototypeMethod(tpl, "getCurrentStatus", GetCurrentStatus);
    Nan::SetPrototypeMethod(tpl, "setStatusChangedCallbackMethodName",
        SetStatusChangedCallbackMethodName);
    Nan::SetPrototypeMethod(tpl, "isTm2", IsTm2);
    Nan::SetPrototypeMethod(tpl, "isPlayback", IsPlayback);
    Nan::SetPrototypeMethod(tpl, "isRecorder", IsRecorder);

    // methods of tm2 device
    Nan::SetPrototypeMethod(tpl, "enableLoopback", EnableLoopback);
    Nan::SetPrototypeMethod(tpl, "disableLoopback", DisableLoopback);
    Nan::SetPrototypeMethod(tpl, "isLoopbackEnabled", IsLoopbackEnabled);

    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSDevice").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(rs2_device* dev,
      DeviceType type = kNormalDevice) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor_);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(instance);
    me->dev_ = dev;
    me->type_ = type;

    return scope.Escape(instance);
  }

 private:
  explicit RSDevice(DeviceType type = kNormalDevice) : dev_(nullptr),
      error_(nullptr), type_(type) {}

  ~RSDevice() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;
    if (dev_) rs2_delete_device(dev_);
    dev_ = nullptr;
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSDevice* obj = new RSDevice();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(GetCameraInfo) {
    info.GetReturnValue().Set(Nan::Undefined());
    int32_t camera_info = info[0]->IntegerValue();;
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (!me) return;

    auto value = GetNativeResult<const char*>(rs2_get_device_info,
        &me->error_, me->dev_, static_cast<rs2_camera_info>(camera_info),
        &me->error_);
    if (me->error_) return;

    info.GetReturnValue().Set(Nan::New(value).ToLocalChecked());
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (me) {
      me->DestroyMe();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }


  static NAN_METHOD(SupportsCameraInfo) {
    info.GetReturnValue().Set(Nan::False());
    int32_t camera_info = info[0]->IntegerValue();
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (!me) return;

    int32_t on = GetNativeResult<int>(rs2_supports_device_info, &me->error_,
        me->dev_, (rs2_camera_info)camera_info, &me->error_);
    if (me->error_) return;
    info.GetReturnValue().Set(Nan::New(on ? true : false));
  }

  static NAN_METHOD(Reset) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (!me) return;

    CallNativeFunc(rs2_hardware_reset, &me->error_, me->dev_, &me->error_);
  }

  static NAN_METHOD(QuerySensors) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (!me) return;

    std::shared_ptr<rs2_sensor_list> list(
        GetNativeResult<rs2_sensor_list*>(rs2_query_sensors, &me->error_,
        me->dev_, &me->error_), rs2_delete_sensor_list);
    if (!list) return;

    auto size = GetNativeResult<int>(rs2_get_sensors_count, &me->error_,
        list.get(), &me->error_);
    if (!size) return;

    v8::Local<v8::Array> array = Nan::New<v8::Array>();
    for (int32_t i = 0; i < size; i++) {
      rs2_sensor* sensor = GetNativeResult<rs2_sensor*>(rs2_create_sensor,
          &me->error_, list.get(), i, &me->error_);
      array->Set(i, RSSensor::NewInstance(sensor));
    }
    info.GetReturnValue().Set(array);
  }

  static NAN_METHOD(TriggerErrorForTest) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (!me) return;

    uint8_t raw_data[24] = {0};
    raw_data[0] = 0x14;
    raw_data[2] = 0xab;
    raw_data[3] = 0xcd;
    raw_data[4] = 0x4d;
    raw_data[8] = 4;
    CallNativeFunc(rs2_send_and_receive_raw_data, &me->error_, me->dev_,
        static_cast<void*>(raw_data), 24, &me->error_);
  }

  static NAN_METHOD(SpawnRecorderDevice) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    v8::String::Utf8Value file(info[0]->ToString());
    auto dev = GetNativeResult<rs2_device*>(rs2_create_record_device,
        &me->error_, me->dev_, *file, &me->error_);
    if (me->error_) return;

    auto obj = RSDevice::NewInstance(dev, kRecorderDevice);
    info.GetReturnValue().Set(obj);
  }

  static NAN_METHOD(PauseRecord) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (!me) return;

    CallNativeFunc(rs2_record_device_pause, &me->error_, me->dev_,
        &me->error_);
  }

  static NAN_METHOD(ResumeRecord) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (!me) return;

    CallNativeFunc(rs2_record_device_resume, &me->error_, me->dev_,
        &me->error_);
  }

  static NAN_METHOD(GetFileName) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    const char* file = nullptr;
    if (me->IsPlaybackInternal()) {
      file = GetNativeResult<const char*>(rs2_playback_device_get_file_path,
          &me->error_, me->dev_, &me->error_);
    } else if (me->IsRecorderInternal()) {
      file = GetNativeResult<const char*>(rs2_record_device_filename,
          &me->error_, me->dev_, &me->error_);
    } else {
      return;
    }
    if (me->error_) return;

    info.GetReturnValue().Set(Nan::New(file).ToLocalChecked());
  }

  static NAN_METHOD(PausePlayback) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (!me) return;

    CallNativeFunc(rs2_playback_device_pause, &me->error_, me->dev_,
        &me->error_);
  }

  static NAN_METHOD(ResumePlayback) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (!me) return;

    CallNativeFunc(rs2_playback_device_resume, &me->error_, me->dev_,
        &me->error_);
  }

  static NAN_METHOD(StopPlayback) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (!me) return;

    CallNativeFunc(rs2_playback_device_stop, &me->error_, me->dev_,
        &me->error_);
  }

  static NAN_METHOD(GetPosition) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto pos = static_cast<uint32_t>(GetNativeResult<uint32_t>(
        rs2_playback_get_position, &me->error_, me->dev_, &me->error_)/1000000);
    info.GetReturnValue().Set(Nan::New(pos));
  }

  static NAN_METHOD(GetDuration) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto duration = static_cast<uint32_t>(
        GetNativeResult<uint32_t>(rs2_playback_get_duration, &me->error_,
        me->dev_, &me->error_)/1000000);
    info.GetReturnValue().Set(Nan::New(duration));
  }

  static NAN_METHOD(Seek) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    uint64_t time = info[0]->IntegerValue();
    CallNativeFunc(rs2_playback_seek, &me->error_, me->dev_, time*1000000,
        &me->error_);
  }

  static NAN_METHOD(IsRealTime) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto val = GetNativeResult<int>(rs2_playback_device_is_real_time,
        &me->error_, me->dev_, &me->error_);
    if (me->error_) return;

    info.GetReturnValue().Set(val ? Nan::True() : Nan::False());
  }

  static NAN_METHOD(SetIsRealTime) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto val = info[0]->BooleanValue();
    CallNativeFunc(rs2_playback_device_set_real_time, &me->error_,
        me->dev_, val, &me->error_);
  }

  static NAN_METHOD(SetPlaybackSpeed) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto speed = info[0]->NumberValue();
    CallNativeFunc(rs2_playback_device_set_playback_speed, &me->error_,
        me->dev_, speed, &me->error_);
  }

  static NAN_METHOD(IsPlayback) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto val = me->IsPlaybackInternal();
    info.GetReturnValue().Set(val ? Nan::True() : Nan::False());
  }

  static NAN_METHOD(IsRecorder) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto val = me->IsRecorderInternal();
    info.GetReturnValue().Set(val ? Nan::True() : Nan::False());
  }

  static NAN_METHOD(GetCurrentStatus) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto status = GetNativeResult<rs2_playback_status>(
        rs2_playback_device_get_current_status, &me->error_,
        me->dev_, &me->error_);
    if (me->error_) return;

    info.GetReturnValue().Set(Nan::New(status));
  }

  static NAN_METHOD(SetStatusChangedCallbackMethodName) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    v8::String::Utf8Value method(info[0]->ToString());
    me->status_changed_callback_method_name_ = std::string(*method);
    CallNativeFunc(rs2_playback_device_set_status_changed_callback, &me->error_,
        me->dev_, new PlaybackStatusCallback(me), &me->error_);
  }

  static NAN_METHOD(IsTm2) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto val = GetNativeResult<int>(rs2_is_device_extendable_to, &me->error_,
        me->dev_, RS2_EXTENSION_TM2, &me->error_);
    info.GetReturnValue().Set(val ? Nan::True() : Nan::False());
  }

  static NAN_METHOD(EnableLoopback) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    v8::String::Utf8Value file(info[0]->ToString());
    CallNativeFunc(rs2_loopback_enable, &me->error_, me->dev_, *file,
        &me->error_);
  }

  static NAN_METHOD(DisableLoopback) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    CallNativeFunc(rs2_loopback_disable, &me->error_, me->dev_,
        &me->error_);
  }

  static NAN_METHOD(IsLoopbackEnabled) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto val = GetNativeResult<int>(rs2_loopback_is_enabled, &me->error_,
        me->dev_, &me->error_);
    info.GetReturnValue().Set(val ? Nan::True() : Nan::False());
  }

 private:
  bool IsPlaybackInternal() {
    auto val = GetNativeResult<int>(rs2_is_device_extendable_to, &error_, dev_,
        RS2_EXTENSION_PLAYBACK, &error_);

    return (error_ || !val) ? false : true;
  }

  bool IsRecorderInternal() {
    auto val = GetNativeResult<int>(rs2_is_device_extendable_to, &error_, dev_,
        RS2_EXTENSION_RECORD, &error_);

    return (error_ || !val) ? false : true;
  }
  static Nan::Persistent<v8::Function> constructor_;
  rs2_device* dev_;
  rs2_error* error_;
  DeviceType type_;
  std::string status_changed_callback_method_name_;
  friend class RSContext;
  friend class DevicesChangedCallbackInfo;
  friend class FrameCallbackInfo;
  friend class RSPipeline;
  friend class RSDeviceList;
  friend class RSDeviceHub;
  friend class PlaybackStatusCallbackInfo;
};

Nan::Persistent<v8::Function> RSDevice::constructor_;

void FrameCallbackInfo::Run() {
  SetConsumed();
  Nan::HandleScope scope;
  // save the rs2_frame to the sensor
  sensor_->ReplaceFrame(frame_);
  Nan::MakeCallback(sensor_->handle(), sensor_->frame_callback_name_.c_str(), 0,
      nullptr);
}

void NotificationCallbackInfo::Run() {
  SetConsumed();
  Nan::HandleScope scope;
  v8::Local<v8::Value> args[1] = {
    RSNotification(
        desc_, time_, severity_, category_, serialized_data_).GetObject()
  };
  Nan::MakeCallback(sensor_->handle(),
      sensor_->notification_callback_name_.c_str(), 1, args);
}

void PlaybackStatusCallbackInfo::Run() {
  SetConsumed();
  Nan::HandleScope scope;
  v8::Local<v8::Value> args[1] = { Nan::New(status_) };
  Nan::MakeCallback(dev_->handle(),
      dev_->status_changed_callback_method_name_.c_str(), 1, args);
}

class RSPointCloud : public Nan::ObjectWrap, Options {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSPointCloud").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "calculate", Calculate);
    Nan::SetPrototypeMethod(tpl, "mapTo", MapTo);

    // options API
    Nan::SetPrototypeMethod(tpl, "supportsOption", SupportsOption);
    Nan::SetPrototypeMethod(tpl, "getOption", GetOption);
    Nan::SetPrototypeMethod(tpl, "setOption", SetOption);
    Nan::SetPrototypeMethod(tpl, "getOptionRange", GetOptionRange);
    Nan::SetPrototypeMethod(tpl, "isOptionReadonly", IsOptionReadonly);
    Nan::SetPrototypeMethod(tpl, "getOptionDescription", GetOptionDescription);
    Nan::SetPrototypeMethod(tpl, "getOptionValueDescription",
        GetOptionValueDescription);

    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSPointCloud").ToLocalChecked(), tpl->GetFunction());
  }

  rs2_options* GetOptionsPointer() override {
    // we have to reinterpret_cast as they are unrelated types to compiler
    return reinterpret_cast<rs2_options*>(processing_block_);
  }

 private:
  RSPointCloud() : processing_block_(nullptr), error_(nullptr) {
    frame_queue_ = rs2_create_frame_queue(1, &error_);
  }

  ~RSPointCloud() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;
    if (processing_block_) rs2_delete_processing_block(processing_block_);
    processing_block_ = nullptr;
    if (frame_queue_) rs2_delete_frame_queue(frame_queue_);
    frame_queue_ = nullptr;
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSPointCloud>(info.Holder());
    if (me) {
      me->DestroyMe();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSPointCloud* obj = new RSPointCloud();
      obj->processing_block_ = GetNativeResult<rs2_processing_block*>(
          rs2_create_pointcloud, &obj->error_, &obj->error_);
      auto callback = new FrameCallbackForFrameQueue(obj->frame_queue_);
      CallNativeFunc(rs2_start_processing, &obj->error_, obj->processing_block_,
          callback, &obj->error_);

      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(Calculate) {
    info.GetReturnValue().Set(Nan::False());
    auto me = Nan::ObjectWrap::Unwrap<RSPointCloud>(info.Holder());
    auto frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[0]->ToObject());
    auto target_frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[1]->ToObject());
    if (!me || !frame || !frame->frame_ || !target_frame) return;

    // rs2_process_frame will release the input frame, so we need to addref
    CallNativeFunc(rs2_frame_add_ref, &me->error_, frame->frame_, &me->error_);
    if (me->error_) return;

    CallNativeFunc(rs2_process_frame, &me->error_, me->processing_block_,
        frame->frame_, &me->error_);
    if (me->error_) return;

    auto new_frame = GetNativeResult<rs2_frame*>(rs2_wait_for_frame,
        &me->error_, me->frame_queue_, 5000, &me->error_);
    if (!new_frame) return;

    target_frame->Replace(new_frame);
    info.GetReturnValue().Set(Nan::True());
  }

  static NAN_METHOD(MapTo) {
    auto me = Nan::ObjectWrap::Unwrap<RSPointCloud>(info.Holder());
    auto frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[0]->ToObject());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me || !frame) return;

    const rs2_stream_profile* profile =
        GetNativeResult<const rs2_stream_profile*>(rs2_get_frame_stream_profile,
        &me->error_, frame->frame_, &me->error_);
    if (!profile) return;

    StreamProfileExtrator extrator(profile);
    CallNativeFunc(rs2_set_option, &me->error_,
        reinterpret_cast<rs2_options*>(me->processing_block_),
        RS2_OPTION_TEXTURE_SOURCE,
        static_cast<float>(extrator.unique_id_),
        &me->error_);
    if (extrator.stream_ == RS2_STREAM_DEPTH) return;

    // rs2_process_frame will release the input frame, so we need to addref
    CallNativeFunc(rs2_frame_add_ref, &me->error_, frame->frame_, &me->error_);
    if (me->error_) return;

    CallNativeFunc(rs2_process_frame, &me->error_, me->processing_block_,
        frame->frame_, &me->error_);
  }

  static NAN_METHOD(SupportsOption) {
    auto me = Nan::ObjectWrap::Unwrap<RSPointCloud>(info.Holder());
    if (me) return me->SupportsOptionInternal(info);

    info.GetReturnValue().Set(Nan::False());
  }

  static NAN_METHOD(GetOption) {
    auto me = Nan::ObjectWrap::Unwrap<RSPointCloud>(info.Holder());
    if (me) return me->GetOptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetOptionDescription) {
    auto me = Nan::ObjectWrap::Unwrap<RSPointCloud>(info.Holder());
    if (me) return me->GetOptionDescriptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetOptionValueDescription) {
    auto me = Nan::ObjectWrap::Unwrap<RSPointCloud>(info.Holder());
    if (me) return me->GetOptionValueDescriptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(SetOption) {
    auto me = Nan::ObjectWrap::Unwrap<RSPointCloud>(info.Holder());
    if (me) return me->SetOptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetOptionRange) {
    auto me = Nan::ObjectWrap::Unwrap<RSPointCloud>(info.Holder());
    if (me) return me->GetOptionRangeInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(IsOptionReadonly) {
    auto me = Nan::ObjectWrap::Unwrap<RSPointCloud>(info.Holder());
    if (me) return me->IsOptionReadonlyInternal(info);

    info.GetReturnValue().Set(Nan::False());
  }

 private:
  static Nan::Persistent<v8::Function> constructor_;

  rs2_processing_block* processing_block_;
  rs2_frame_queue* frame_queue_;
  rs2_error* error_;
};

Nan::Persistent<v8::Function> RSPointCloud::constructor_;

class RSDeviceList : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSDeviceList").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "contains", Contains);
    Nan::SetPrototypeMethod(tpl, "size", Size);
    Nan::SetPrototypeMethod(tpl, "getDevice", GetDevice);

    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSDeviceList").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(rs2_device_list* list) {
    Nan::EscapableHandleScope scope;
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor_);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();
    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();
    auto me = Nan::ObjectWrap::Unwrap<RSDeviceList>(instance);
    me->list_ = list;
    return scope.Escape(instance);
  }

 private:
  RSDeviceList() : error_(nullptr), list_(nullptr) {}

  ~RSDeviceList() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;
    if (list_) rs2_delete_device_list(list_);
    list_ = nullptr;
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSDeviceList* obj = new RSDeviceList();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSDeviceList>(info.Holder());
    if (me) me->DestroyMe();

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(Contains) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSDeviceList>(info.Holder());
    auto dev = Nan::ObjectWrap::Unwrap<RSDevice>(info[0]->ToObject());
    if (!me && dev) return;

    bool contains = GetNativeResult<int>(rs2_device_list_contains, &me->error_,
        me->list_, dev->dev_, &me->error_);
    if (me->error_) return;

    info.GetReturnValue().Set(Nan::New(contains));
  }

  static NAN_METHOD(Size) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSDeviceList>(info.Holder());
    if (!me) return;

    auto cnt = GetNativeResult<int>(rs2_get_device_count, &me->error_,
        me->list_, &me->error_);
    if (me->error_) return;

    info.GetReturnValue().Set(Nan::New(cnt));
  }

  static NAN_METHOD(GetDevice) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSDeviceList>(info.Holder());
    auto index = info[0]->IntegerValue();
    if (!me) return;

    auto dev = GetNativeResult<rs2_device*>(rs2_create_device, &me->error_,
        me->list_, index, &me->error_);
    if (!dev) return;

    info.GetReturnValue().Set(RSDevice::NewInstance(dev));
  }

 private:
  static Nan::Persistent<v8::Function> constructor_;
  rs2_error* error_;
  rs2_device_list* list_;
};

Nan::Persistent<v8::Function> RSDeviceList::constructor_;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//   .d8888b.                    888                     888                 //
//  d88P  Y88b                   888                     888                 //
//  888    888                   888                     888                 //
//  888         .d88b.  88888b.  888888 .d88b.  888  888 888888              //
//  888        d88""88b 888 "88b 888   d8P  Y8b `Y8bd8P' 888                 //
//  888    888 888  888 888  888 888   88888888   X88K   888                 //
//  Y88b  d88P Y88..88P 888  888 Y88b. Y8b.     .d8""8b. Y88b.               //
//   "Y8888P"   "Y88P"  888  888  "Y888 "Y8888  888  888  "Y888              //
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class RSContext : public Nan::ObjectWrap {
 public:
  enum ContextType {
    kNormal = 0,
    kRecording,
    kPlayback,
  };
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSContext").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "create", Create);
    Nan::SetPrototypeMethod(tpl, "queryDevices", QueryDevices);
    Nan::SetPrototypeMethod(tpl, "setDevicesChangedCallback",
        SetDevicesChangedCallback);
    Nan::SetPrototypeMethod(tpl, "loadDeviceFile", LoadDeviceFile);
    Nan::SetPrototypeMethod(tpl, "unloadDeviceFile", UnloadDeviceFile);
    Nan::SetPrototypeMethod(tpl, "createDeviceFromSensor",
        CreateDeviceFromSensor);

    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSContext").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(rs2_context* ctx_ptr = nullptr) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor_);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    // If ctx_ptr is provided, no need to call create.
    if (ctx_ptr) {
      auto me = Nan::ObjectWrap::Unwrap<RSContext>(instance);
      me->ctx_ = ctx_ptr;
    }
    return scope.Escape(instance);
  }

 private:
  explicit RSContext(ContextType type = kNormal) : ctx_(nullptr),
      error_(nullptr), type_(type), mode_(RS2_RECORDING_MODE_BLANK_FRAMES) {}

  ~RSContext() {
    DestroyMe();
  }

  void RegisterDevicesChangedCallbackMethod();

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;
    if (ctx_) rs2_delete_context(ctx_);
    ctx_ = nullptr;
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (!info.IsConstructCall()) return;

    ContextType type = kNormal;
    if (info.Length()) {
      v8::String::Utf8Value type_str(info[0]->ToString());
      std::string std_type_str(*type_str);
      if (!std_type_str.compare("recording"))
        type = kRecording;
      else if (!std_type_str.compare("playback"))
        type = kPlayback;
    }
    RSContext* obj = new RSContext(type);
    if (type == kRecording || type == kPlayback) {
      v8::String::Utf8Value file(info[1]->ToString());
      v8::String::Utf8Value section(info[2]->ToString());
      obj->file_name_ = std::string(*file);
      obj->section_ = std::string(*section);
    }
    if (type == kRecording)
      obj->mode_ = static_cast<rs2_recording_mode>(info[3]->IntegerValue());
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  }

  static NAN_METHOD(Create) {
    MainThreadCallback::Init();
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSContext>(info.Holder());
    if (!me) return;

    switch (me->type_) {
      case kRecording:
        me->ctx_ = GetNativeResult<rs2_context*>(rs2_create_recording_context,
            &me->error_, RS2_API_VERSION, me->file_name_.c_str(),
            me->section_.c_str(), me->mode_, &me->error_);
        break;
      case kPlayback:
        me->ctx_ = GetNativeResult<rs2_context*>(rs2_create_mock_context,
            &me->error_, RS2_API_VERSION, me->file_name_.c_str(),
            me->section_.c_str(), &me->error_);
        break;
      default:
        me->ctx_ = GetNativeResult<rs2_context*>(rs2_create_context,
            &me->error_, RS2_API_VERSION, &me->error_);
        break;
    }
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSContext>(info.Holder());
    if (me) {
      me->DestroyMe();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(SetDevicesChangedCallback) {
    auto me = Nan::ObjectWrap::Unwrap<RSContext>(info.Holder());
    if (me) {
      v8::String::Utf8Value value(info[0]->ToString());
      me->device_changed_callback_name_ = std::string(*value);
      me->RegisterDevicesChangedCallbackMethod();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(LoadDeviceFile) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSContext>(info.Holder());
    if (!me) return;

    auto device_file = info[0]->ToString();
    v8::String::Utf8Value value(device_file);
    auto dev = GetNativeResult<rs2_device*>(rs2_context_add_device, &me->error_,
        me->ctx_, *value, &me->error_);
    if (!dev) return;

    auto jsobj = RSDevice::NewInstance(dev, RSDevice::kPlaybackDevice);
    info.GetReturnValue().Set(jsobj);
  }

  static NAN_METHOD(UnloadDeviceFile) {
    auto me = Nan::ObjectWrap::Unwrap<RSContext>(info.Holder());
    info.GetReturnValue().Set(Nan::Undefined());
    if (!me) return;

    auto device_file = info[0]->ToString();
    v8::String::Utf8Value value(device_file);
    CallNativeFunc(rs2_context_remove_device, &me->error_, me->ctx_, *value,
        &me->error_);
  }

  static NAN_METHOD(CreateDeviceFromSensor) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto sensor = Nan::ObjectWrap::Unwrap<RSSensor>(info[0]->ToObject());
    if (!sensor) return;

    rs2_error* error = nullptr;
    auto dev = GetNativeResult<rs2_device*>(rs2_create_device_from_sensor,
        &error, sensor->sensor_, &error);
    if (!dev) return;

    auto jsobj = RSDevice::NewInstance(dev);
    info.GetReturnValue().Set(jsobj);
  }

  static NAN_METHOD(QueryDevices) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSContext>(info.Holder());
    if (!me) return;

    auto dev_list = GetNativeResult<rs2_device_list*>(rs2_query_devices,
        &me->error_, me->ctx_, &me->error_);
    if (!dev_list) return;

    auto jsobj = RSDeviceList::NewInstance(dev_list);
    info.GetReturnValue().Set(jsobj);
  }

 private:
  static Nan::Persistent<v8::Function> constructor_;

  rs2_context* ctx_;
  rs2_error* error_;
  std::string device_changed_callback_name_;
  ContextType type_;
  std::string file_name_;
  std::string section_;
  rs2_recording_mode mode_;
  friend class DevicesChangedCallbackInfo;
  friend class RSPipeline;
  friend class RSDeviceHub;
};

Nan::Persistent<v8::Function> RSContext::constructor_;

class DevicesChangedCallbackInfo : public MainThreadCallbackInfo {
 public:
  DevicesChangedCallbackInfo(rs2_device_list* r,
      rs2_device_list* a, RSContext* ctx) :
      removed_(r), added_(a), ctx_(ctx) {}
  virtual ~DevicesChangedCallbackInfo() { if (!consumed_) Release(); }
  virtual void Run() {
    SetConsumed();
    Nan::HandleScope scope;
    v8::Local<v8::Value> rmlist;
    v8::Local<v8::Value> addlist;
    if (removed_)
      rmlist = RSDeviceList::NewInstance(removed_);
    else
      rmlist = Nan::Undefined();

    if (added_)
      addlist = RSDeviceList::NewInstance(added_);
    else
      addlist = Nan::Undefined();

    v8::Local<v8::Value> args[2] = {rmlist, addlist};
    Nan::MakeCallback(ctx_->handle(),
        ctx_->device_changed_callback_name_.c_str(), 2, args);
  }
  virtual void Release() {
    if (removed_) {
      rs2_delete_device_list(removed_);
      removed_ = nullptr;
    }

    if (added_) {
      rs2_delete_device_list(added_);
      added_ = nullptr;
    }
  }

 private:
  rs2_device_list* removed_;
  rs2_device_list* added_;
  RSContext* ctx_;
};

class DevicesChangedCallback : public rs2_devices_changed_callback {
 public:
  explicit DevicesChangedCallback(RSContext* context) : ctx_(context) {}
  virtual void on_devices_changed(
      rs2_device_list* removed, rs2_device_list* added) {
    MainThreadCallback::NotifyMainThread(
        new DevicesChangedCallbackInfo(removed, added, ctx_));
  }

  virtual void release() {
    delete this;
  }

  virtual ~DevicesChangedCallback() {}
  RSContext* ctx_;
};

void RSContext::RegisterDevicesChangedCallbackMethod() {
  CallNativeFunc(rs2_set_devices_changed_callback_cpp, &error_, ctx_,
      new DevicesChangedCallback(this), &error_);
}

class RSDeviceHub : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSDeviceHub").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "waitForDevice", WaitForDevice);
    Nan::SetPrototypeMethod(tpl, "isConnected", IsConnected);
    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSDeviceHub").ToLocalChecked(),
      tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance() {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor_);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    return scope.Escape(instance);
  }

 private:
  RSDeviceHub() : hub_(nullptr), ctx_(nullptr), error_(nullptr) {}

  ~RSDeviceHub() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;

    if (hub_) rs2_delete_device_hub(hub_);
    hub_ = nullptr;
    ctx_ = nullptr;
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSDeviceHub>(info.Holder());
    if (me) me->DestroyMe();
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSDeviceHub* obj = new RSDeviceHub();
      RSContext* ctx = Nan::ObjectWrap::Unwrap<RSContext>(info[0]->ToObject());
      obj->ctx_ = ctx->ctx_;
      obj->hub_ = GetNativeResult<rs2_device_hub*>(rs2_create_device_hub,
          &obj->error_, obj->ctx_, &obj->error_);
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(WaitForDevice) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSDeviceHub>(info.Holder());
    if (!me) return;

    auto dev = GetNativeResult<rs2_device*>(rs2_device_hub_wait_for_device,
        &me->error_, me->hub_, &me->error_);
    if (!dev) return;

    info.GetReturnValue().Set(RSDevice::NewInstance(dev));
  }

  static NAN_METHOD(IsConnected) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSDeviceHub>(info.Holder());
    auto dev = Nan::ObjectWrap::Unwrap<RSDevice>(info[0]->ToObject());
    if (!me || !dev) return;

    auto res = GetNativeResult<int>(rs2_device_hub_is_device_connected,
       &me->error_, me->hub_, dev->dev_, &me->error_);
    if (me->error_) return;

    info.GetReturnValue().Set(Nan::New(res ? true : false));
  }

 private:
  static Nan::Persistent<v8::Function> constructor_;

  rs2_device_hub* hub_;
  rs2_context* ctx_;
  rs2_error* error_;
};

Nan::Persistent<v8::Function> RSDeviceHub::constructor_;

class RSPipelineProfile : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSPipelineProfile").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "getStreams", GetStreams);
    Nan::SetPrototypeMethod(tpl, "getDevice", GetDevice);
    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSPipelineProfile").ToLocalChecked(),
      tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(rs2_pipeline_profile* profile) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor_);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    auto me = Nan::ObjectWrap::Unwrap<RSPipelineProfile>(instance);
    me->pipeline_profile_ = profile;
    return scope.Escape(instance);
  }

 private:
  RSPipelineProfile() : pipeline_profile_(nullptr), error_(nullptr) {}

  ~RSPipelineProfile() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;

    if (pipeline_profile_) rs2_delete_pipeline_profile(pipeline_profile_);
    pipeline_profile_ = nullptr;
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSPipelineProfile>(info.Holder());
    if (me) me->DestroyMe();
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSPipelineProfile* obj = new RSPipelineProfile();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(GetStreams) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSPipelineProfile>(info.Holder());
    if (!me) return;

    rs2_stream_profile_list* list = GetNativeResult<rs2_stream_profile_list*>(
        rs2_pipeline_profile_get_streams, &me->error_, me->pipeline_profile_,
        &me->error_);
    if (!list) return;

    int32_t size = GetNativeResult<int32_t>(rs2_get_stream_profiles_count,
        &me->error_, list, &me->error_);
    if (me->error_) return;

    v8::Local<v8::Array> array = Nan::New<v8::Array>(size);
    for (int32_t i = 0; i < size; i++) {
      rs2_stream_profile* profile = const_cast<rs2_stream_profile*>(
          GetNativeResult<const rs2_stream_profile*>(rs2_get_stream_profile,
          &me->error_, list, i, &me->error_));
      array->Set(i, RSStreamProfile::NewInstance(profile));
    }
    info.GetReturnValue().Set(array);
  }

  static NAN_METHOD(GetDevice) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSPipelineProfile>(info.Holder());
    if (!me) return;

    rs2_device* dev = GetNativeResult<rs2_device*>(
        rs2_pipeline_profile_get_device, &me->error_, me->pipeline_profile_,
        &me->error_);
    if (!dev) return;

    info.GetReturnValue().Set(RSDevice::NewInstance(dev));
  }

 private:
  static Nan::Persistent<v8::Function> constructor_;

  rs2_pipeline_profile* pipeline_profile_;
  rs2_error* error_;
};

Nan::Persistent<v8::Function> RSPipelineProfile::constructor_;

class RSPipeline;
class RSConfig : public Nan::ObjectWrap  {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSConfig").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "enableStream", EnableStream);
    Nan::SetPrototypeMethod(tpl, "enableAllStreams", EnableAllStreams);
    Nan::SetPrototypeMethod(tpl, "enableDevice", EnableDevice);
    Nan::SetPrototypeMethod(tpl, "enableDeviceFromFile", EnableDeviceFromFile);
    Nan::SetPrototypeMethod(tpl, "enableRecordToFile", EnableRecordToFile);
    Nan::SetPrototypeMethod(tpl, "disableStream", DisableStream);
    Nan::SetPrototypeMethod(tpl, "disableAllStreams", DisableAllStreams);
    Nan::SetPrototypeMethod(tpl, "resolve", Resolve);
    Nan::SetPrototypeMethod(tpl, "canResolve", CanResolve);
    Nan::SetPrototypeMethod(tpl, "enableDeviceFromFileRepeatOption",
        EnableDeviceFromFileRepeatOption);

    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSConfig").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance() {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor_);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    return scope.Escape(instance);
  }

 private:
  RSConfig() : config_(nullptr), error_(nullptr) {}

  ~RSConfig() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;
    if (config_) rs2_delete_config(config_);
    config_ = nullptr;
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSConfig>(info.Holder());
    if (me) me->DestroyMe();
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSConfig* obj = new RSConfig();
      obj->config_ = rs2_create_config(&obj->error_);
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  // TODO(halton): added all the overloads
  static NAN_METHOD(EnableStream) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSConfig>(info.Holder());
    auto stream = info[0]->IntegerValue();
    auto index = info[1]->IntegerValue();
    auto width = info[2]->IntegerValue();
    auto height = info[3]->IntegerValue();
    auto format = info[4]->IntegerValue();
    auto framerate = info[5]->IntegerValue();
    if (!me || !me->config_) return;

    CallNativeFunc(rs2_config_enable_stream, &me->error_, me->config_,
      (rs2_stream)stream,
      index,
      width,
      height,
      (rs2_format)format,
      framerate,
      &me->error_);
  }

  static NAN_METHOD(EnableAllStreams) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSConfig>(info.Holder());
    if (!me) return;

    CallNativeFunc(rs2_config_enable_all_stream, &me->error_, me->config_,
        &me->error_);
  }

  static NAN_METHOD(EnableDevice) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSConfig>(info.Holder());
    if (!me) return;

    auto device = info[0]->ToString();
    v8::String::Utf8Value value(device);
    CallNativeFunc(rs2_config_enable_device, &me->error_, me->config_, *value,
        &me->error_);
  }

  static NAN_METHOD(EnableDeviceFromFile) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSConfig>(info.Holder());
    if (!me) return;

    auto device_file = info[0]->ToString();
    v8::String::Utf8Value value(device_file);
    CallNativeFunc(rs2_config_enable_device_from_file, &me->error_, me->config_,
        *value, &me->error_);
  }

  static NAN_METHOD(EnableDeviceFromFileRepeatOption) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSConfig>(info.Holder());
    if (!me) return;

    auto device_file = info[0]->ToString();
    auto repeat = info[1]->BooleanValue();
    v8::String::Utf8Value value(device_file);
    CallNativeFunc(rs2_config_enable_device_from_file_repeat_option,
        &me->error_, me->config_, *value, repeat, &me->error_);
  }

  static NAN_METHOD(EnableRecordToFile) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSConfig>(info.Holder());
    if (!me) return;

    auto device_file = info[0]->ToString();
    v8::String::Utf8Value value(device_file);
    CallNativeFunc(rs2_config_enable_record_to_file, &me->error_, me->config_,
        *value, &me->error_);
  }

  static NAN_METHOD(DisableStream) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSConfig>(info.Holder());
    if (!me) return;

    auto stream = info[0]->IntegerValue();
    CallNativeFunc(rs2_config_disable_stream, &me->error_, me->config_,
        (rs2_stream)stream, &me->error_);
  }

  static NAN_METHOD(DisableAllStreams) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSConfig>(info.Holder());
    if (!me) return;

    CallNativeFunc(rs2_config_disable_all_streams, &me->error_, me->config_,
        &me->error_);
  }
  static NAN_METHOD(Resolve);
  static NAN_METHOD(CanResolve);

 private:
  static Nan::Persistent<v8::Function> constructor_;
  friend class RSPipeline;

  rs2_config* config_;
  rs2_error* error_;
};

Nan::Persistent<v8::Function> RSConfig::constructor_;

class RSPipeline : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSPipeline").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "start", Start);
    Nan::SetPrototypeMethod(tpl, "startWithConfig", StartWithConfig);
    Nan::SetPrototypeMethod(tpl, "stop", Stop);
    Nan::SetPrototypeMethod(tpl, "waitForFrames", WaitForFrames);
    Nan::SetPrototypeMethod(tpl, "pollForFrames", PollForFrames);
    Nan::SetPrototypeMethod(tpl, "getActiveProfile", GetActiveProfile);
    Nan::SetPrototypeMethod(tpl, "create", Create);
    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);

    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSPipeline").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance() {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor_);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();
    return scope.Escape(instance);
  }

 private:
  friend class RSConfig;

  RSPipeline() : pipeline_(nullptr), error_(nullptr) {}

  ~RSPipeline() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;
    if (pipeline_) rs2_delete_pipeline(pipeline_);
    pipeline_ = nullptr;
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSPipeline>(info.Holder());
    if (me) me->DestroyMe();
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSPipeline* obj = new RSPipeline();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(Create) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSPipeline>(info.Holder());
    auto rsctx = Nan::ObjectWrap::Unwrap<RSContext>(info[0]->ToObject());
    if (!me || !rsctx) return;

    me->pipeline_ = GetNativeResult<rs2_pipeline*>(rs2_create_pipeline,
        &me->error_, rsctx->ctx_, &me->error_);
  }

  static NAN_METHOD(StartWithConfig) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSPipeline>(info.Holder());
    if (!me || !me->pipeline_) return;

    RSConfig* config = Nan::ObjectWrap::Unwrap<RSConfig>(info[0]->ToObject());
    rs2_pipeline_profile* prof = GetNativeResult<rs2_pipeline_profile*>(
        rs2_pipeline_start_with_config, &me->error_, me->pipeline_,
        config->config_, &me->error_);
    if (!prof) return;

    info.GetReturnValue().Set(RSPipelineProfile::NewInstance(prof));
  }

  static NAN_METHOD(Start) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSPipeline>(info.Holder());
    if (!me || !me->pipeline_) return;

    rs2_pipeline_profile* prof = GetNativeResult<rs2_pipeline_profile*>(
        rs2_pipeline_start, &me->error_, me->pipeline_, &me->error_);
    if (!prof) return;

    info.GetReturnValue().Set(RSPipelineProfile::NewInstance(prof));
  }

  static NAN_METHOD(Stop) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSPipeline>(info.Holder());
    if (!me || !me->pipeline_) return;

    CallNativeFunc(rs2_pipeline_stop, &me->error_, me->pipeline_, &me->error_);
  }

  static NAN_METHOD(WaitForFrames) {
    info.GetReturnValue().Set(Nan::False());
    auto me = Nan::ObjectWrap::Unwrap<RSPipeline>(info.Holder());
    auto frameset = Nan::ObjectWrap::Unwrap<RSFrameSet>(info[0]->ToObject());
    if (!me || !frameset) return;

    auto timeout = info[1]->IntegerValue();
    rs2_frame* frames = GetNativeResult<rs2_frame*>(
        rs2_pipeline_wait_for_frames, &me->error_, me->pipeline_, timeout,
        &me->error_);
    if (!frames) return;

    frameset->Replace(frames);
    info.GetReturnValue().Set(Nan::True());
  }

  static NAN_METHOD(PollForFrames) {
    info.GetReturnValue().Set(Nan::False());
    auto me = Nan::ObjectWrap::Unwrap<RSPipeline>(info.Holder());
    auto frameset = Nan::ObjectWrap::Unwrap<RSFrameSet>(info[0]->ToObject());
    if (!me || !frameset) return;

    rs2_frame* frames = nullptr;
    auto res = GetNativeResult<int>(rs2_pipeline_poll_for_frames, &me->error_,
        me->pipeline_, &frames, &me->error_);
    if (!res) return;

    frameset->Replace(frames);
    info.GetReturnValue().Set(Nan::True());
  }

  static NAN_METHOD(GetActiveProfile) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSPipeline>(info.Holder());
    if (!me) return;

    rs2_pipeline_profile* prof = GetNativeResult<rs2_pipeline_profile*>(
        rs2_pipeline_get_active_profile, &me->error_, me->pipeline_,
        &me->error_);
    if (!prof) return;

    info.GetReturnValue().Set(RSPipelineProfile::NewInstance(prof));
  }

 private:
  static Nan::Persistent<v8::Function> constructor_;

  rs2_pipeline* pipeline_;
  rs2_error* error_;
};

Nan::Persistent<v8::Function> RSPipeline::constructor_;

NAN_METHOD(RSConfig::Resolve) {
  info.GetReturnValue().Set(Nan::Undefined());
  auto me = Nan::ObjectWrap::Unwrap<RSConfig>(info.Holder());
  if (!me) return;

  RSPipeline* pipe = Nan::ObjectWrap::Unwrap<RSPipeline>(info[0]->ToObject());
  auto pipeline_profile = GetNativeResult<rs2_pipeline_profile*>(
      rs2_config_resolve, &me->error_, me->config_, pipe->pipeline_,
      &me->error_);
  if (!pipeline_profile) return;

  info.GetReturnValue().Set(RSPipelineProfile::NewInstance(pipeline_profile));
}

NAN_METHOD(RSConfig::CanResolve) {
  info.GetReturnValue().Set(Nan::Undefined());
  auto me = Nan::ObjectWrap::Unwrap<RSConfig>(info.Holder());
  if (!me) return;

  RSPipeline* pipe = Nan::ObjectWrap::Unwrap<RSPipeline>(info[0]->ToObject());
  auto can_resolve = GetNativeResult<int>(rs2_config_can_resolve, &me->error_,
      me->config_, pipe->pipeline_, &me->error_);
  if (me->error_) return;

  info.GetReturnValue().Set(Nan::New(can_resolve ? true : false));
}

class RSColorizer : public Nan::ObjectWrap, Options {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSColorizer").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "create", Create);
    Nan::SetPrototypeMethod(tpl, "colorize", Colorize);
    Nan::SetPrototypeMethod(tpl, "supportsOption", SupportsOption);
    Nan::SetPrototypeMethod(tpl, "getOption", GetOption);
    Nan::SetPrototypeMethod(tpl, "setOption", SetOption);
    Nan::SetPrototypeMethod(tpl, "getOptionRange", GetOptionRange);
    Nan::SetPrototypeMethod(tpl, "isOptionReadonly", IsOptionReadonly);
    Nan::SetPrototypeMethod(tpl, "getOptionDescription", GetOptionDescription);
    Nan::SetPrototypeMethod(tpl, "getOptionValueDescription",
        GetOptionValueDescription);

    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSColorizer").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance() {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor_);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    return scope.Escape(instance);
  }

  rs2_options* GetOptionsPointer() override {
    // TODO(shaoting) find better way to avoid the reinterpret_cast which was
    // caused the inheritance relation was hidden
    return reinterpret_cast<rs2_options*>(colorizer_);
  }

 private:
  RSColorizer() : colorizer_(nullptr), frame_queue_(nullptr), error_(nullptr) {}

  ~RSColorizer() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;
    if (colorizer_) rs2_delete_processing_block(colorizer_);
    colorizer_ = nullptr;
    if (frame_queue_) rs2_delete_frame_queue(frame_queue_);
    frame_queue_ = nullptr;
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSColorizer>(info.Holder());
    if (me) {
      me->DestroyMe();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSColorizer* obj = new RSColorizer();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(Create) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSColorizer>(info.Holder());
    if (!me) return;

    me->colorizer_ = GetNativeResult<rs2_processing_block*>(
        rs2_create_colorizer, &me->error_, &me->error_);
    if (!me->colorizer_) return;

    me->frame_queue_ = GetNativeResult<rs2_frame_queue*>(
        rs2_create_frame_queue, &me->error_, 1, &me->error_);
    if (!me->frame_queue_) return;

    auto callback = new FrameCallbackForFrameQueue(me->frame_queue_);
    CallNativeFunc(rs2_start_processing, &me->error_, me->colorizer_, callback,
        &me->error_);
  }

  static NAN_METHOD(Colorize) {
    info.GetReturnValue().Set(Nan::False());
    auto me = Nan::ObjectWrap::Unwrap<RSColorizer>(info.Holder());
    RSFrame* depth = Nan::ObjectWrap::Unwrap<RSFrame>(info[0]->ToObject());
    RSFrame* target = Nan::ObjectWrap::Unwrap<RSFrame>(info[1]->ToObject());
    if (!me || !depth || !depth->frame_ || !target) return;

    // rs2_process_frame will release the input frame, so we need to addref
    CallNativeFunc(rs2_frame_add_ref, &me->error_, depth->frame_, &me->error_);
    if (me->error_) return;

    CallNativeFunc(rs2_process_frame, &me->error_, me->colorizer_,
        depth->frame_, &me->error_);
    if (me->error_) return;

    rs2_frame* result = GetNativeResult<rs2_frame*>(rs2_wait_for_frame,
        &me->error_, me->frame_queue_, 5000, &me->error_);
    target->DestroyMe();
    if (!result) return;

    target->Replace(result);
    info.GetReturnValue().Set(Nan::True());
  }

  static NAN_METHOD(SupportsOption) {
    auto me = Nan::ObjectWrap::Unwrap<RSColorizer>(info.Holder());
    if (me) return me->SupportsOptionInternal(info);

    info.GetReturnValue().Set(Nan::False());
  }

  static NAN_METHOD(GetOption) {
    auto me = Nan::ObjectWrap::Unwrap<RSColorizer>(info.Holder());
    if (me) return me->GetOptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetOptionDescription) {
    auto me = Nan::ObjectWrap::Unwrap<RSColorizer>(info.Holder());
    if (me) return me->GetOptionDescriptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetOptionValueDescription) {
    auto me = Nan::ObjectWrap::Unwrap<RSColorizer>(info.Holder());
    if (me) return me->GetOptionValueDescriptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(SetOption) {
    auto me = Nan::ObjectWrap::Unwrap<RSColorizer>(info.Holder());
    if (me) return me->SetOptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetOptionRange) {
    auto me = Nan::ObjectWrap::Unwrap<RSColorizer>(info.Holder());
    if (me) return me->GetOptionRangeInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(IsOptionReadonly) {
    auto me = Nan::ObjectWrap::Unwrap<RSColorizer>(info.Holder());
    if (me) return me->IsOptionReadonlyInternal(info);

    info.GetReturnValue().Set(Nan::False());
  }

 private:
  static Nan::Persistent<v8::Function> constructor_;

  rs2_processing_block* colorizer_;
  rs2_frame_queue* frame_queue_;
  rs2_error* error_;
};

Nan::Persistent<v8::Function> RSColorizer::constructor_;

class RSAlign : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSAlign").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "waitForFrames", WaitForFrames);
    Nan::SetPrototypeMethod(tpl, "process", Process);

    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSAlign").ToLocalChecked(), tpl->GetFunction());
  }

 private:
  RSAlign() : align_(nullptr), frame_queue_(nullptr), error_(nullptr) {}

  ~RSAlign() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;
    if (align_) rs2_delete_processing_block(align_);
    align_ = nullptr;
    if (frame_queue_) rs2_delete_frame_queue(frame_queue_);
    frame_queue_ = nullptr;
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSAlign>(info.Holder());
    if (me) me->DestroyMe();

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (!info.IsConstructCall()) return;

    RSAlign* obj = new RSAlign();
    auto stream = static_cast<rs2_stream>(info[0]->IntegerValue());
    obj->align_ = GetNativeResult<rs2_processing_block*>(rs2_create_align,
        &obj->error_, stream, &obj->error_);
    if (!obj->align_) return;

    obj->frame_queue_ = GetNativeResult<rs2_frame_queue*>(
        rs2_create_frame_queue, &obj->error_, 1, &obj->error_);
    if (!obj->frame_queue_) return;

    auto callback = new FrameCallbackForFrameQueue(obj->frame_queue_);
    CallNativeFunc(rs2_start_processing, &obj->error_, obj->align_, callback,
        &obj->error_);

    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  }

  static NAN_METHOD(WaitForFrames) {
    info.GetReturnValue().Set(Nan::Undefined());
    auto me = Nan::ObjectWrap::Unwrap<RSAlign>(info.Holder());
    if (!me) return;

    rs2_frame* result = GetNativeResult<rs2_frame*>(rs2_wait_for_frame,
        &me->error_, me->frame_queue_, 5000, &me->error_);
    if (!result) return;

    info.GetReturnValue().Set(RSFrameSet::NewInstance(result));
  }

  static NAN_METHOD(Process) {
    info.GetReturnValue().Set(Nan::False());
    auto me = Nan::ObjectWrap::Unwrap<RSAlign>(info.Holder());
    auto frameset = Nan::ObjectWrap::Unwrap<RSFrameSet>(info[0]->ToObject());
    auto target_fs = Nan::ObjectWrap::Unwrap<RSFrameSet>(info[1]->ToObject());
    if (!me || !frameset || !target_fs) return;

    // rs2_process_frame will release the input frame, so we need to addref
    CallNativeFunc(rs2_frame_add_ref, &me->error_, frameset->GetFrames(),
        &me->error_);
    if (me->error_) return;

    CallNativeFunc(rs2_process_frame, &me->error_, me->align_,
        frameset->GetFrames(), &me->error_);
    if (me->error_) return;

    rs2_frame* frame = nullptr;
    auto ret_code = GetNativeResult<int>(rs2_poll_for_frame, &me->error_,
        me->frame_queue_, &frame, &me->error_);
    if (!ret_code) return;

    target_fs->Replace(frame);
    info.GetReturnValue().Set(Nan::True());
  }

 private:
  static Nan::Persistent<v8::Function> constructor_;

  rs2_processing_block* align_;
  rs2_frame_queue* frame_queue_;
  rs2_error* error_;
  friend class RSPipeline;
};

Nan::Persistent<v8::Function> RSAlign::constructor_;

class RSFilter : public Nan::ObjectWrap, Options {
 public:
  enum FilterType {
    kFilterDecimation = 0,
    kFilterTemporal,
    kFilterSpatial,
    kFilterHoleFilling,
    kFilterDisparity2Depth,
    kFilterDepth2Disparity
  };
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSFilter").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "process", Process);
    Nan::SetPrototypeMethod(tpl, "supportsOption", SupportsOption);
    Nan::SetPrototypeMethod(tpl, "getOption", GetOption);
    Nan::SetPrototypeMethod(tpl, "setOption", SetOption);
    Nan::SetPrototypeMethod(tpl, "getOptionRange", GetOptionRange);
    Nan::SetPrototypeMethod(tpl, "isOptionReadonly", IsOptionReadonly);
    Nan::SetPrototypeMethod(tpl, "getOptionDescription", GetOptionDescription);
    Nan::SetPrototypeMethod(tpl, "getOptionValueDescription",
        GetOptionValueDescription);

    constructor_.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSFilter").ToLocalChecked(), tpl->GetFunction());
  }

  rs2_options* GetOptionsPointer() override {
    return reinterpret_cast<rs2_options*>(block_);
  }

 private:
  RSFilter() : block_(nullptr), frame_queue_(nullptr), error_(nullptr),
      type_(kFilterDecimation) {}

  ~RSFilter() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error_) rs2_free_error(error_);
    error_ = nullptr;
    if (block_) rs2_delete_processing_block(block_);
    block_ = nullptr;
    if (frame_queue_) rs2_delete_frame_queue(frame_queue_);
    frame_queue_ = nullptr;
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSFilter>(info.Holder());
    if (me) me->DestroyMe();

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (!info.IsConstructCall()) return;

    v8::String::Utf8Value type_str(info[0]);
    std::string type = std::string(*type_str);
    RSFilter* obj = new RSFilter();
    if (!(type.compare("decimation"))) {
      obj->type_ = kFilterDecimation;
      obj->block_ = GetNativeResult<rs2_processing_block*>(
          rs2_create_decimation_filter_block, &obj->error_, &obj->error_);
    } else if (!(type.compare("temporal"))) {
      obj->type_ = kFilterTemporal;
      obj->block_ = GetNativeResult<rs2_processing_block*>(
          rs2_create_temporal_filter_block, &obj->error_, &obj->error_);
    } else if (!(type.compare("spatial"))) {
      obj->type_ = kFilterSpatial;
      obj->block_ = GetNativeResult<rs2_processing_block*>(
          rs2_create_spatial_filter_block, &obj->error_, &obj->error_);
    } else if (!(type.compare("hole-filling"))) {
      obj->type_ = kFilterHoleFilling;
      obj->block_ = GetNativeResult<rs2_processing_block*>(
          rs2_create_hole_filling_filter_block, &obj->error_, &obj->error_);
    } else if (!(type.compare("disparity-to-depth"))) {
      obj->type_ = kFilterDisparity2Depth;
      obj->block_ = GetNativeResult<rs2_processing_block*>(
          rs2_create_disparity_transform_block, &obj->error_, 0, &obj->error_);
    } else if (!(type.compare("depth-to-disparity"))) {
      obj->type_ = kFilterDepth2Disparity;
      obj->block_ = GetNativeResult<rs2_processing_block*>(
          rs2_create_disparity_transform_block, &obj->error_, 1, &obj->error_);
    }
    if (!obj->block_) return;

    obj->frame_queue_ = GetNativeResult<rs2_frame_queue*>(
        rs2_create_frame_queue, &obj->error_, 1, &obj->error_);
    if (!obj->frame_queue_) return;

    auto callback = new FrameCallbackForFrameQueue(obj->frame_queue_);
    CallNativeFunc(rs2_start_processing, &obj->error_, obj->block_, callback,
        &obj->error_);
    if (obj->error_) return;

    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  }

  static NAN_METHOD(Process) {
    auto me = Nan::ObjectWrap::Unwrap<RSFilter>(info.Holder());
    auto input_frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[0]->ToObject());
    auto out_frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[1]->ToObject());
    info.GetReturnValue().Set(Nan::False());
    if (!me || !input_frame || !out_frame) return;

    // rs2_process_frame will release the input frame, so we need to addref
    CallNativeFunc(rs2_frame_add_ref, &me->error_, input_frame->frame_,
        &me->error_);
    if (me->error_) return;

    CallNativeFunc(rs2_process_frame, &me->error_, me->block_,
        input_frame->frame_, &me->error_);
    if (me->error_) return;

    rs2_frame* frame = nullptr;
    auto ret_code = GetNativeResult<int>(rs2_poll_for_frame, &me->error_,
        me->frame_queue_, &frame, &me->error_);
    if (!ret_code) return;

    out_frame->Replace(frame);
    info.GetReturnValue().Set(Nan::True());
  }

  static NAN_METHOD(SupportsOption) {
    auto me = Nan::ObjectWrap::Unwrap<RSFilter>(info.Holder());
    if (me) return me->SupportsOptionInternal(info);

    info.GetReturnValue().Set(Nan::False());
  }

  static NAN_METHOD(GetOption) {
    auto me = Nan::ObjectWrap::Unwrap<RSFilter>(info.Holder());
    if (me) return me->GetOptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetOptionDescription) {
    auto me = Nan::ObjectWrap::Unwrap<RSFilter>(info.Holder());
    if (me) return me->GetOptionDescriptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetOptionValueDescription) {
    auto me = Nan::ObjectWrap::Unwrap<RSFilter>(info.Holder());
    if (me) return me->GetOptionValueDescriptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(SetOption) {
    auto me = Nan::ObjectWrap::Unwrap<RSFilter>(info.Holder());
    if (me) return me->SetOptionInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetOptionRange) {
    auto me = Nan::ObjectWrap::Unwrap<RSFilter>(info.Holder());
    if (me) return me->GetOptionRangeInternal(info);

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(IsOptionReadonly) {
    auto me = Nan::ObjectWrap::Unwrap<RSFilter>(info.Holder());
    if (me) return me->IsOptionReadonlyInternal(info);

    info.GetReturnValue().Set(Nan::False());
  }

 private:
  static Nan::Persistent<v8::Function> constructor_;

  rs2_processing_block* block_;
  rs2_frame_queue* frame_queue_;
  rs2_error* error_;
  FilterType type_;
};

Nan::Persistent<v8::Function> RSFilter::constructor_;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// 888b     d888               888          888                              //
// 8888b   d8888               888          888                              //
// 88888b.d88888               888          888                              //
// 888Y88888P888  .d88b.   .d88888 888  888 888  .d88b.                      //
// 888 Y888P 888 d88""88b d88" 888 888  888 888 d8P  Y8b                     //
// 888  Y8P  888 888  888 888  888 888  888 888 88888888                     //
// 888   "   888 Y88..88P Y88b 888 Y88b 888 888 Y8b.                         //
// 888       888  "Y88P"   "Y88888  "Y88888 888  "Y8888                      //
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

NAN_METHOD(GlobalCleanup) {
  MainThreadCallback::Destroy();
  ErrorUtil::ResetError();
  info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(GetTime) {
  rs2_error* e = nullptr;
  auto time = rs2_get_time(&e);
  info.GetReturnValue().Set(Nan::New(time));
}

NAN_METHOD(RegisterErrorCallback) {
  ErrorUtil::Init();
  ErrorUtil::UpdateJSErrorCallback(info);
}

NAN_METHOD(GetError) {
  info.GetReturnValue().Set(ErrorUtil::GetJSErrorObject());
}

#define _FORCE_SET_ENUM(name) \
  Nan::DefineOwnProperty(exports, \
      Nan::New(#name).ToLocalChecked(), \
      Nan::New(static_cast<int>((name))), \
      static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete));

void InitModule(v8::Local<v8::Object> exports) {
  exports->Set(Nan::New("globalCleanup").ToLocalChecked(),
      Nan::New<v8::FunctionTemplate>(GlobalCleanup)->GetFunction());
  exports->Set(Nan::New("getTime").ToLocalChecked(),
      Nan::New<v8::FunctionTemplate>(GetTime)->GetFunction());
  exports->Set(Nan::New("registerErrorCallback").ToLocalChecked(),
      Nan::New<v8::FunctionTemplate>(RegisterErrorCallback)->GetFunction());
  exports->Set(Nan::New("getError").ToLocalChecked(),
      Nan::New<v8::FunctionTemplate>(GetError)->GetFunction());
  // rs2_error* error = nullptr;
  // rs2_log_to_console(RS2_LOG_SEVERITY_DEBUG, &error);

  RSContext::Init(exports);
  RSPointCloud::Init(exports);
  RSPipelineProfile::Init(exports);
  RSConfig::Init(exports);
  RSPipeline::Init(exports);
  RSFrameSet::Init(exports);
  RSSensor::Init(exports);
  RSDevice::Init(exports);
  RSDeviceList::Init(exports);
  RSDeviceHub::Init(exports);
  RSStreamProfile::Init(exports);
  RSColorizer::Init(exports);
  RSFrameQueue::Init(exports);
  RSFrame::Init(exports);
  RSSyncer::Init(exports);
  RSAlign::Init(exports);
  RSFilter::Init(exports);

  // rs2_exception_type
  _FORCE_SET_ENUM(RS2_EXCEPTION_TYPE_UNKNOWN);
  _FORCE_SET_ENUM(RS2_EXCEPTION_TYPE_CAMERA_DISCONNECTED);
  _FORCE_SET_ENUM(RS2_EXCEPTION_TYPE_BACKEND);
  _FORCE_SET_ENUM(RS2_EXCEPTION_TYPE_INVALID_VALUE);
  _FORCE_SET_ENUM(RS2_EXCEPTION_TYPE_WRONG_API_CALL_SEQUENCE);
  _FORCE_SET_ENUM(RS2_EXCEPTION_TYPE_NOT_IMPLEMENTED);
  _FORCE_SET_ENUM(RS2_EXCEPTION_TYPE_DEVICE_IN_RECOVERY_MODE);
  _FORCE_SET_ENUM(RS2_EXCEPTION_TYPE_COUNT);

  // rs2_stream
  _FORCE_SET_ENUM(RS2_STREAM_ANY);
  _FORCE_SET_ENUM(RS2_STREAM_DEPTH);
  _FORCE_SET_ENUM(RS2_STREAM_COLOR);
  _FORCE_SET_ENUM(RS2_STREAM_INFRARED);
  _FORCE_SET_ENUM(RS2_STREAM_FISHEYE);
  _FORCE_SET_ENUM(RS2_STREAM_GYRO);
  _FORCE_SET_ENUM(RS2_STREAM_ACCEL);
  _FORCE_SET_ENUM(RS2_STREAM_GPIO);
  _FORCE_SET_ENUM(RS2_STREAM_POSE);
  _FORCE_SET_ENUM(RS2_STREAM_CONFIDENCE);
  _FORCE_SET_ENUM(RS2_STREAM_COUNT);

  // rs2_format
  _FORCE_SET_ENUM(RS2_FORMAT_ANY);
  _FORCE_SET_ENUM(RS2_FORMAT_Z16);
  _FORCE_SET_ENUM(RS2_FORMAT_DISPARITY16);
  _FORCE_SET_ENUM(RS2_FORMAT_XYZ32F);
  _FORCE_SET_ENUM(RS2_FORMAT_YUYV);
  _FORCE_SET_ENUM(RS2_FORMAT_RGB8);
  _FORCE_SET_ENUM(RS2_FORMAT_BGR8);
  _FORCE_SET_ENUM(RS2_FORMAT_RGBA8);
  _FORCE_SET_ENUM(RS2_FORMAT_BGRA8);
  _FORCE_SET_ENUM(RS2_FORMAT_Y8);
  _FORCE_SET_ENUM(RS2_FORMAT_Y16);
  _FORCE_SET_ENUM(RS2_FORMAT_RAW10);
  _FORCE_SET_ENUM(RS2_FORMAT_RAW16);
  _FORCE_SET_ENUM(RS2_FORMAT_RAW8);
  _FORCE_SET_ENUM(RS2_FORMAT_UYVY);
  _FORCE_SET_ENUM(RS2_FORMAT_MOTION_RAW);
  _FORCE_SET_ENUM(RS2_FORMAT_MOTION_XYZ32F);
  _FORCE_SET_ENUM(RS2_FORMAT_GPIO_RAW);
  _FORCE_SET_ENUM(RS2_FORMAT_6DOF);
  _FORCE_SET_ENUM(RS2_FORMAT_DISPARITY32);
  _FORCE_SET_ENUM(RS2_FORMAT_Y10BPACK);
  _FORCE_SET_ENUM(RS2_FORMAT_DISTANCE);
  _FORCE_SET_ENUM(RS2_FORMAT_MJPEG);
  _FORCE_SET_ENUM(RS2_FORMAT_Y8I);
  _FORCE_SET_ENUM(RS2_FORMAT_Y12I);
  _FORCE_SET_ENUM(RS2_FORMAT_INZI);
  _FORCE_SET_ENUM(RS2_FORMAT_INVI);
  _FORCE_SET_ENUM(RS2_FORMAT_W10);
  _FORCE_SET_ENUM(RS2_FORMAT_COUNT);

  // rs2_frame_type_value
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_FRAME_COUNTER);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_FRAME_TIMESTAMP);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_SENSOR_TIMESTAMP);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_GAIN_LEVEL);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_AUTO_EXPOSURE);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_WHITE_BALANCE);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_TIME_OF_ARRIVAL);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_TEMPERATURE);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_BACKEND_TIMESTAMP);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_ACTUAL_FPS);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_FRAME_LASER_POWER);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_EXPOSURE_PRIORITY);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_EXPOSURE_ROI_LEFT);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_EXPOSURE_ROI_RIGHT);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_EXPOSURE_ROI_TOP);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_EXPOSURE_ROI_BOTTOM);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_BRIGHTNESS);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_CONTRAST);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_SATURATION);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_SHARPNESS);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_AUTO_WHITE_BALANCE_TEMPERATURE);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_BACKLIGHT_COMPENSATION);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_HUE);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_GAMMA);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_MANUAL_WHITE_BALANCE);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_POWER_LINE_FREQUENCY);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_LOW_LIGHT_COMPENSATION);
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_COUNT);

  // rs2_distortion
  _FORCE_SET_ENUM(RS2_DISTORTION_NONE);
  _FORCE_SET_ENUM(RS2_DISTORTION_MODIFIED_BROWN_CONRADY);
  _FORCE_SET_ENUM(RS2_DISTORTION_INVERSE_BROWN_CONRADY);
  _FORCE_SET_ENUM(RS2_DISTORTION_FTHETA);
  _FORCE_SET_ENUM(RS2_DISTORTION_BROWN_CONRADY);
  _FORCE_SET_ENUM(RS2_DISTORTION_COUNT);

  // rs2_option
  _FORCE_SET_ENUM(RS2_OPTION_BACKLIGHT_COMPENSATION);
  _FORCE_SET_ENUM(RS2_OPTION_BRIGHTNESS);
  _FORCE_SET_ENUM(RS2_OPTION_CONTRAST);
  _FORCE_SET_ENUM(RS2_OPTION_EXPOSURE);
  _FORCE_SET_ENUM(RS2_OPTION_GAIN);
  _FORCE_SET_ENUM(RS2_OPTION_GAMMA);
  _FORCE_SET_ENUM(RS2_OPTION_HUE);
  _FORCE_SET_ENUM(RS2_OPTION_SATURATION);
  _FORCE_SET_ENUM(RS2_OPTION_SHARPNESS);
  _FORCE_SET_ENUM(RS2_OPTION_WHITE_BALANCE);
  _FORCE_SET_ENUM(RS2_OPTION_ENABLE_AUTO_EXPOSURE);
  _FORCE_SET_ENUM(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);
  _FORCE_SET_ENUM(RS2_OPTION_VISUAL_PRESET);
  _FORCE_SET_ENUM(RS2_OPTION_LASER_POWER);
  _FORCE_SET_ENUM(RS2_OPTION_ACCURACY);
  _FORCE_SET_ENUM(RS2_OPTION_MOTION_RANGE);
  _FORCE_SET_ENUM(RS2_OPTION_FILTER_OPTION);
  _FORCE_SET_ENUM(RS2_OPTION_CONFIDENCE_THRESHOLD);
  _FORCE_SET_ENUM(RS2_OPTION_EMITTER_ENABLED);
  _FORCE_SET_ENUM(RS2_OPTION_FRAMES_QUEUE_SIZE);
  _FORCE_SET_ENUM(RS2_OPTION_TOTAL_FRAME_DROPS);
  _FORCE_SET_ENUM(RS2_OPTION_AUTO_EXPOSURE_MODE);
  _FORCE_SET_ENUM(RS2_OPTION_POWER_LINE_FREQUENCY);
  _FORCE_SET_ENUM(RS2_OPTION_ASIC_TEMPERATURE);
  _FORCE_SET_ENUM(RS2_OPTION_ERROR_POLLING_ENABLED);
  _FORCE_SET_ENUM(RS2_OPTION_PROJECTOR_TEMPERATURE);
  _FORCE_SET_ENUM(RS2_OPTION_OUTPUT_TRIGGER_ENABLED);
  _FORCE_SET_ENUM(RS2_OPTION_MOTION_MODULE_TEMPERATURE);
  _FORCE_SET_ENUM(RS2_OPTION_DEPTH_UNITS);
  _FORCE_SET_ENUM(RS2_OPTION_ENABLE_MOTION_CORRECTION);
  _FORCE_SET_ENUM(RS2_OPTION_AUTO_EXPOSURE_PRIORITY);
  _FORCE_SET_ENUM(RS2_OPTION_COLOR_SCHEME);
  _FORCE_SET_ENUM(RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED);
  _FORCE_SET_ENUM(RS2_OPTION_MIN_DISTANCE);
  _FORCE_SET_ENUM(RS2_OPTION_MAX_DISTANCE);
  _FORCE_SET_ENUM(RS2_OPTION_TEXTURE_SOURCE);
  _FORCE_SET_ENUM(RS2_OPTION_FILTER_MAGNITUDE);
  _FORCE_SET_ENUM(RS2_OPTION_FILTER_SMOOTH_ALPHA);
  _FORCE_SET_ENUM(RS2_OPTION_FILTER_SMOOTH_DELTA);
  _FORCE_SET_ENUM(RS2_OPTION_HOLES_FILL);
  _FORCE_SET_ENUM(RS2_OPTION_STEREO_BASELINE);
  _FORCE_SET_ENUM(RS2_OPTION_AUTO_EXPOSURE_CONVERGE_STEP);
  _FORCE_SET_ENUM(RS2_OPTION_INTER_CAM_SYNC_MODE);
  _FORCE_SET_ENUM(RS2_OPTION_STREAM_FILTER);
  _FORCE_SET_ENUM(RS2_OPTION_STREAM_FORMAT_FILTER);
  _FORCE_SET_ENUM(RS2_OPTION_STREAM_INDEX_FILTER);
  _FORCE_SET_ENUM(RS2_OPTION_EMITTER_ON_OFF);
  _FORCE_SET_ENUM(RS2_OPTION_ZERO_ORDER_POINT_X);
  _FORCE_SET_ENUM(RS2_OPTION_ZERO_ORDER_POINT_Y);
  _FORCE_SET_ENUM(RS2_OPTION_LLD_TEMPERATURE);
  _FORCE_SET_ENUM(RS2_OPTION_MC_TEMPERATURE);
  _FORCE_SET_ENUM(RS2_OPTION_MA_TEMPERATURE);
  _FORCE_SET_ENUM(RS2_OPTION_HARDWARE_PRESET);
  _FORCE_SET_ENUM(RS2_OPTION_GLOBAL_TIME_ENABLED);
  _FORCE_SET_ENUM(RS2_OPTION_APD_TEMPERATURE);
  _FORCE_SET_ENUM(RS2_OPTION_ENABLE_MAPPING);
  _FORCE_SET_ENUM(RS2_OPTION_ENABLE_RELOCALIZATION);
  _FORCE_SET_ENUM(RS2_OPTION_ENABLE_POSE_JUMPING);
  _FORCE_SET_ENUM(RS2_OPTION_ENABLE_DYNAMIC_CALIBRATION);
  _FORCE_SET_ENUM(RS2_OPTION_DEPTH_OFFSET);
  _FORCE_SET_ENUM(RS2_OPTION_LED_POWER);
  _FORCE_SET_ENUM(RS2_OPTION_ZERO_ORDER_ENABLED);
  _FORCE_SET_ENUM(RS2_OPTION_ENABLE_MAP_PRESERVATION);
  _FORCE_SET_ENUM(RS2_OPTION_FREEFALL_DETECTION_ENABLED);
  _FORCE_SET_ENUM(RS2_OPTION_AVALANCHE_PHOTO_DIODE);
  _FORCE_SET_ENUM(RS2_OPTION_POST_PROCESSING_SHARPENING);
  _FORCE_SET_ENUM(RS2_OPTION_PRE_PROCESSING_SHARPENING);
  _FORCE_SET_ENUM(RS2_OPTION_NOISE_FILTERING);
  _FORCE_SET_ENUM(RS2_OPTION_INVALIDATION_BYPASS);
  _FORCE_SET_ENUM(RS2_OPTION_AMBIENT_LIGHT);
  _FORCE_SET_ENUM(RS2_OPTION_SENSOR_MODE);
  _FORCE_SET_ENUM(RS2_OPTION_EMITTER_ALWAYS_ON);
  _FORCE_SET_ENUM(RS2_OPTION_COUNT);

  // rs2_camera_info
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_NAME);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_SERIAL_NUMBER);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_FIRMWARE_VERSION);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_RECOMMENDED_FIRMWARE_VERSION);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_PHYSICAL_PORT);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_DEBUG_OP_CODE);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_ADVANCED_MODE);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_PRODUCT_ID);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_CAMERA_LOCKED);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_COUNT);

  // rs2_log_severity
  _FORCE_SET_ENUM(RS2_LOG_SEVERITY_DEBUG);
  _FORCE_SET_ENUM(RS2_LOG_SEVERITY_INFO);
  _FORCE_SET_ENUM(RS2_LOG_SEVERITY_WARN);
  _FORCE_SET_ENUM(RS2_LOG_SEVERITY_ERROR);
  _FORCE_SET_ENUM(RS2_LOG_SEVERITY_FATAL);
  _FORCE_SET_ENUM(RS2_LOG_SEVERITY_NONE);
  _FORCE_SET_ENUM(RS2_LOG_SEVERITY_COUNT);

  // rs2_notification_category
  _FORCE_SET_ENUM(RS2_NOTIFICATION_CATEGORY_FRAMES_TIMEOUT);
  _FORCE_SET_ENUM(RS2_NOTIFICATION_CATEGORY_FRAME_CORRUPTED);
  _FORCE_SET_ENUM(RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR);
  _FORCE_SET_ENUM(RS2_NOTIFICATION_CATEGORY_HARDWARE_EVENT);
  _FORCE_SET_ENUM(RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR);
  _FORCE_SET_ENUM(RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED);
  _FORCE_SET_ENUM(RS2_NOTIFICATION_CATEGORY_COUNT);

  // rs2_timestamp_domain
  _FORCE_SET_ENUM(RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK);
  _FORCE_SET_ENUM(RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME);
  _FORCE_SET_ENUM(RS2_TIMESTAMP_DOMAIN_COUNT);

  // rs2_recording_mode
  _FORCE_SET_ENUM(RS2_RECORDING_MODE_BLANK_FRAMES);
  _FORCE_SET_ENUM(RS2_RECORDING_MODE_COMPRESSED);
  _FORCE_SET_ENUM(RS2_RECORDING_MODE_BEST_QUALITY);
  _FORCE_SET_ENUM(RS2_RECORDING_MODE_COUNT);

  // rs2_sr300_visual_preset
  _FORCE_SET_ENUM(RS2_SR300_VISUAL_PRESET_SHORT_RANGE);
  _FORCE_SET_ENUM(RS2_SR300_VISUAL_PRESET_LONG_RANGE);
  _FORCE_SET_ENUM(RS2_SR300_VISUAL_PRESET_BACKGROUND_SEGMENTATION);
  _FORCE_SET_ENUM(RS2_SR300_VISUAL_PRESET_GESTURE_RECOGNITION);
  _FORCE_SET_ENUM(RS2_SR300_VISUAL_PRESET_OBJECT_SCANNING);
  _FORCE_SET_ENUM(RS2_SR300_VISUAL_PRESET_FACE_ANALYTICS);
  _FORCE_SET_ENUM(RS2_SR300_VISUAL_PRESET_FACE_LOGIN);
  _FORCE_SET_ENUM(RS2_SR300_VISUAL_PRESET_GR_CURSOR);
  _FORCE_SET_ENUM(RS2_SR300_VISUAL_PRESET_DEFAULT);
  _FORCE_SET_ENUM(RS2_SR300_VISUAL_PRESET_MID_RANGE);
  _FORCE_SET_ENUM(RS2_SR300_VISUAL_PRESET_IR_ONLY);
  _FORCE_SET_ENUM(RS2_SR300_VISUAL_PRESET_COUNT);

  // rs2_rs400_visual_preset
  _FORCE_SET_ENUM(RS2_RS400_VISUAL_PRESET_CUSTOM);
  _FORCE_SET_ENUM(RS2_RS400_VISUAL_PRESET_DEFAULT);
  _FORCE_SET_ENUM(RS2_RS400_VISUAL_PRESET_HAND);
  _FORCE_SET_ENUM(RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY);
  _FORCE_SET_ENUM(RS2_RS400_VISUAL_PRESET_HIGH_DENSITY);
  _FORCE_SET_ENUM(RS2_RS400_VISUAL_PRESET_MEDIUM_DENSITY);
  _FORCE_SET_ENUM(RS2_RS400_VISUAL_PRESET_REMOVE_IR_PATTERN);
  _FORCE_SET_ENUM(RS2_RS400_VISUAL_PRESET_COUNT);

  // rs2_playback_status
  _FORCE_SET_ENUM(RS2_PLAYBACK_STATUS_UNKNOWN);
  _FORCE_SET_ENUM(RS2_PLAYBACK_STATUS_PLAYING);
  _FORCE_SET_ENUM(RS2_PLAYBACK_STATUS_PAUSED);
  _FORCE_SET_ENUM(RS2_PLAYBACK_STATUS_STOPPED);
  _FORCE_SET_ENUM(RS2_PLAYBACK_STATUS_COUNT);


  // rs2_extension
  _FORCE_SET_ENUM(RS2_EXTENSION_UNKNOWN);
  _FORCE_SET_ENUM(RS2_EXTENSION_DEBUG);
  _FORCE_SET_ENUM(RS2_EXTENSION_INFO);
  _FORCE_SET_ENUM(RS2_EXTENSION_MOTION);
  _FORCE_SET_ENUM(RS2_EXTENSION_OPTIONS);
  _FORCE_SET_ENUM(RS2_EXTENSION_VIDEO);
  _FORCE_SET_ENUM(RS2_EXTENSION_ROI);
  _FORCE_SET_ENUM(RS2_EXTENSION_DEPTH_SENSOR);
  _FORCE_SET_ENUM(RS2_EXTENSION_VIDEO_FRAME);
  _FORCE_SET_ENUM(RS2_EXTENSION_MOTION_FRAME);
  _FORCE_SET_ENUM(RS2_EXTENSION_COMPOSITE_FRAME);
  _FORCE_SET_ENUM(RS2_EXTENSION_POINTS);
  _FORCE_SET_ENUM(RS2_EXTENSION_DEPTH_FRAME);
  _FORCE_SET_ENUM(RS2_EXTENSION_ADVANCED_MODE);
  _FORCE_SET_ENUM(RS2_EXTENSION_RECORD);
  _FORCE_SET_ENUM(RS2_EXTENSION_VIDEO_PROFILE);
  _FORCE_SET_ENUM(RS2_EXTENSION_PLAYBACK);
  _FORCE_SET_ENUM(RS2_EXTENSION_DEPTH_STEREO_SENSOR);
  _FORCE_SET_ENUM(RS2_EXTENSION_DISPARITY_FRAME);
  _FORCE_SET_ENUM(RS2_EXTENSION_MOTION_PROFILE);
  _FORCE_SET_ENUM(RS2_EXTENSION_POSE_FRAME);
  _FORCE_SET_ENUM(RS2_EXTENSION_POSE_PROFILE);
  _FORCE_SET_ENUM(RS2_EXTENSION_TM2);
  _FORCE_SET_ENUM(RS2_EXTENSION_SOFTWARE_DEVICE);
  _FORCE_SET_ENUM(RS2_EXTENSION_SOFTWARE_SENSOR);
  _FORCE_SET_ENUM(RS2_EXTENSION_DECIMATION_FILTER);
  _FORCE_SET_ENUM(RS2_EXTENSION_THRESHOLD_FILTER);
  _FORCE_SET_ENUM(RS2_EXTENSION_DISPARITY_FILTER);
  _FORCE_SET_ENUM(RS2_EXTENSION_SPATIAL_FILTER);
  _FORCE_SET_ENUM(RS2_EXTENSION_TEMPORAL_FILTER);
  _FORCE_SET_ENUM(RS2_EXTENSION_HOLE_FILLING_FILTER);
  _FORCE_SET_ENUM(RS2_EXTENSION_ZERO_ORDER_FILTER);
  _FORCE_SET_ENUM(RS2_EXTENSION_RECOMMENDED_FILTERS);
  _FORCE_SET_ENUM(RS2_EXTENSION_POSE);
  _FORCE_SET_ENUM(RS2_EXTENSION_POSE_SENSOR);
  _FORCE_SET_ENUM(RS2_EXTENSION_WHEEL_ODOMETER);
  _FORCE_SET_ENUM(RS2_EXTENSION_GLOBAL_TIMER);
  _FORCE_SET_ENUM(RS2_EXTENSION_UPDATABLE);
  _FORCE_SET_ENUM(RS2_EXTENSION_UPDATE_DEVICE);
  _FORCE_SET_ENUM(RS2_EXTENSION_L500_DEPTH_SENSOR);
  _FORCE_SET_ENUM(RS2_EXTENSION_TM2_SENSOR);
  _FORCE_SET_ENUM(RS2_EXTENSION_AUTO_CALIBRATED_DEVICE);
  _FORCE_SET_ENUM(RS2_EXTENSION_COLOR_SENSOR);
  _FORCE_SET_ENUM(RS2_EXTENSION_MOTION_SENSOR);
  _FORCE_SET_ENUM(RS2_EXTENSION_FISHEYE_SENSOR);
  _FORCE_SET_ENUM(RS2_EXTENSION_COUNT);

}

NODE_MODULE(node_librealsense, InitModule);
