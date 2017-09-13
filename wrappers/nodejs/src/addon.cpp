// Copyright (c) 2017 Intel Corporation. All rights reserved.
// Use of this source code is governed by an Apache 2.0 license
// that can be found in the LICENSE file.

#include <librealsense2/rs.h>
#include <librealsense2/h/rs_pipeline.h>
#include <librealsense2/hpp/rs_types.hpp>
#include <nan.h>

#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

class MainThreadCallbackInfo {
 public:
  MainThreadCallbackInfo() {}
  virtual ~MainThreadCallbackInfo() {}
  virtual void Run() {}
};

class MainThreadCallback {
 public:
  static void Init() {
    if (!singleton_)
      singleton_ = new MainThreadCallback();
  }
  static void Destroy() {
    if (singleton_) {
      delete singleton_;
      singleton_ = nullptr;
    }
  }
  ~MainThreadCallback() {
    uv_close(reinterpret_cast<uv_handle_t*>(async_),
      [](uv_handle_t* ptr) -> void {
      free(ptr);
    });
  }
  static void NotifyMainThread(MainThreadCallbackInfo* info) {
    if (singleton_) {
      singleton_->async_->data = static_cast<void*>(info);
      uv_async_send(singleton_->async_);
    }
  }

 private:
  MainThreadCallback() {
    async_ = static_cast<uv_async_t*>(malloc(sizeof(uv_async_t)));
    uv_async_init(uv_default_loop(), async_, AsyncProc);
  }

  static void AsyncProc(uv_async_t* async) {
    if (async->data) {
      MainThreadCallbackInfo* info =
        reinterpret_cast<MainThreadCallbackInfo*>(async->data);
      info->Run();
      delete info;
    }
  }
  static MainThreadCallback* singleton_;
  uv_async_t* async_;
};

MainThreadCallback* MainThreadCallback::singleton_ = nullptr;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// 8888888b.  d8b          888    888888b.                                   //
// 888  "Y88b Y8P          888    888  "88b                                  //
// 888    888              888    888  .88P                                  //
// 888    888 888  .d8888b 888888 8888888K.   8888b.  .d8888b   .d88b.       //
// 888    888 888 d88P"    888    888  "Y88b     "88b 88K      d8P  Y8b      //
// 888    888 888 888      888    888    888 .d888888 "Y8888b. 88888888      //
// 888  .d88P 888 Y88b.    Y88b.  888   d88P 888  888      X88 Y8b.          //
// 8888888P"  888  "Y8888P  "Y888 8888888P"  "Y888888  88888P'  "Y8888       //
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class DictBase {
 public:
  explicit DictBase(v8::Local<v8::Object> source) : v8obj_(source) {
  }

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
  RSNotification(const char* des, rs2_time_t time, rs2_log_severity severity,
      rs2_notification_category category) {
    SetMember("descr", des);
    SetMemberT("timestamp", time);
    SetMemberT("severity", (int32_t)severity);
    SetMemberT("category", (int32_t)category);
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
    Nan::SetPrototypeMethod(tpl, "size", Size);
    Nan::SetPrototypeMethod(tpl, "isVideoProfile", IsVideoProfile);
    Nan::SetPrototypeMethod(tpl, "width", Width);
    Nan::SetPrototypeMethod(tpl, "height", Height);

    constructor.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSStreamProfile").ToLocalChecked(),
                 tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(
      rs2_stream_profile* p, bool own = false) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(instance);
    me->profile = p;
    me->own_profile = own;
    rs2_get_stream_profile_data(p, &me->stream, &me->format, &me->index,
                                &me->unique_id, &me->fps, &me->error);
    me->size = rs2_get_stream_profile_size(p, &me->error);
    me->is_default = rs2_is_stream_profile_default(p, &me->error);
    if (rs2_stream_profile_is(p, RS2_EXTENSION_VIDEO_PROFILE, &me->error)) {
      me->is_video = true;
      rs2_get_video_stream_resolution(p, &me->width, &me->height, &me->error);
    }

    return scope.Escape(instance);
  }

 private:
  RSStreamProfile() {
    error = nullptr;
    profile = nullptr;
    index = 0;
    unique_id = 0;
    fps = 0;
    format = static_cast<rs2_format>(0);
    stream = static_cast<rs2_stream>(0);
    is_video = false;
    width = 0;
    height = 0;
    size = 0;
    is_default = false;
    own_profile = false;
  }

  ~RSStreamProfile() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error) rs2_free_error(error);
    error = nullptr;
    if (profile && own_profile) rs2_delete_stream_profile(profile);
    profile = nullptr;
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
      info.GetReturnValue().Set(Nan::New(me->stream));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }
  static NAN_METHOD(Format) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->format));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }
  static NAN_METHOD(Fps) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->fps));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }
  static NAN_METHOD(Index) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->index));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }
  static NAN_METHOD(UniqueID) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->unique_id));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }
  static NAN_METHOD(IsDefault) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->is_default));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }
  static NAN_METHOD(Size) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->size));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(IsVideoProfile) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->is_video));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(Width) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->width));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(Height) {
    auto me = Nan::ObjectWrap::Unwrap<RSStreamProfile>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(Nan::New(me->height));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

 private:
  static Nan::Persistent<v8::Function> constructor;

  rs2_error* error;
  rs2_stream_profile* profile;
  int32_t index;
  int32_t unique_id;
  int32_t fps;
  rs2_format format;
  rs2_stream stream;
  bool is_video;
  int32_t width;
  int32_t height;
  int32_t size;
  bool is_default;
  bool own_profile;

  friend class RSSensor;
};

Nan::Persistent<v8::Function> RSStreamProfile::constructor;

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

    // Points related APIs
    Nan::SetPrototypeMethod(tpl, "canGetPoints", CanGetPoints);
    Nan::SetPrototypeMethod(tpl, "getVertices", GetVertices);
    Nan::SetPrototypeMethod(tpl, "getTextureCoordinates",
                            GetTextureCoordinates);
    Nan::SetPrototypeMethod(tpl, "getPointsCount", GetPointsCount);

    constructor.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSFrame").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(rs2_frame* frame) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(instance);
    me->frame = frame;

    return scope.Escape(instance);
  }

 private:
  RSFrame() {
    error = nullptr;
    frame = nullptr;
  }

  ~RSFrame() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error) rs2_free_error(error);
    error = nullptr;
    rs2_release_frame(frame);
    frame = nullptr;
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSFrame* obj = new RSFrame();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(GetStreamProfile) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      rs2_stream stream;
      rs2_format format;
      int32_t index = 0;
      int32_t unique_id = 0;
      int32_t fps = 0;
      const rs2_stream_profile* profile_org =
          rs2_get_frame_stream_profile(me->frame, &me->error);
      rs2_get_stream_profile_data(profile_org, &stream, &format,
                                  &index, &unique_id, &fps, &me->error);
      rs2_stream_profile* profile = rs2_clone_stream_profile(
            profile_org, stream, index, format, &me->error);
      if (profile) {
        info.GetReturnValue().Set(RSStreamProfile::NewInstance(profile, true));
        return;
      }
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetData) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      auto buffer = rs2_get_frame_data(me->frame, &me->error);
      const auto stride = rs2_get_frame_stride_in_bytes(me->frame, &me->error);
      const auto height = rs2_get_frame_height(me->frame, &me->error);
      const auto length = stride * height;
      if (buffer) {
        auto array_buffer = v8::ArrayBuffer::New(
            v8::Isolate::GetCurrent(),
            static_cast<uint8_t*>(const_cast<void*>(buffer)), length,
            v8::ArrayBufferCreationMode::kExternalized);

        info.GetReturnValue().Set(v8::Uint8Array::New(array_buffer, 0, length));
        return;
      }
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetWidth) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      auto value = rs2_get_frame_width(me->frame, &me->error);
      info.GetReturnValue().Set(Nan::New(value));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetHeight) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      auto value = rs2_get_frame_height(me->frame, &me->error);
      info.GetReturnValue().Set(Nan::New(value));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetStrideInBytes) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      auto value = rs2_get_frame_stride_in_bytes(me->frame, &me->error);
      info.GetReturnValue().Set(Nan::New(value));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetBitsPerPixel) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      auto value = rs2_get_frame_bits_per_pixel(me->frame, &me->error);
      info.GetReturnValue().Set(Nan::New(value));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetTimestamp) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      auto value = rs2_get_frame_timestamp(me->frame, &me->error);
      info.GetReturnValue().Set(Nan::New(value));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetTimestampDomain) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      auto value = rs2_get_frame_timestamp_domain(me->frame, &me->error);
      info.GetReturnValue().Set(Nan::New(value));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetFrameNumber) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      // TODO(tingshao): higher 32 bits
      uint32_t value = rs2_get_frame_number(me->frame, &me->error);
      info.GetReturnValue().Set(Nan::New(value));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(IsVideoFrame) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      bool isVideo = false;
      if (rs2_is_frame_extendable_to(
          me->frame, RS2_EXTENSION_VIDEO_FRAME, &me->error))
        isVideo = true;
      info.GetReturnValue().Set(Nan::New(isVideo));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(IsDepthFrame) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      bool isDepth = false;
      if (rs2_is_frame_extendable_to(
          me->frame, RS2_EXTENSION_DEPTH_FRAME, &me->error))
        isDepth = true;
      info.GetReturnValue().Set(Nan::New(isDepth));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetFrameMetadata) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    rs2_frame_metadata_value metadata =
            (rs2_frame_metadata_value)(info[0]->IntegerValue());
    Nan::TypedArrayContents<unsigned char> content(info[1]);
    unsigned char* internal_data = *content;

    if (me && internal_data) {
      rs2_metadata_type output = rs2_get_frame_metadata(me->frame,
          metadata, &me->error);
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
      return;
    }
    info.GetReturnValue().Set(Nan::New(false));
  }

  static NAN_METHOD(SupportsFrameMetadata) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    rs2_frame_metadata_value metadata =
            (rs2_frame_metadata_value)(info[0]->IntegerValue());
    if (me) {
      int32_t result = rs2_supports_frame_metadata(me->frame,
          metadata, &me->error);
      info.GetReturnValue().Set(Nan::New(result?true:false));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      me->DestroyMe();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(CanGetPoints) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      bool result = false;
      if (rs2_is_frame_extendable_to(
          me->frame, RS2_EXTENSION_POINTS, &me->error))
        result = true;
      info.GetReturnValue().Set(Nan::New(result));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetVertices) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      rs2_vertex* vertices = rs2_get_frame_vertices(me->frame, &me->error);
      size_t count = rs2_get_frame_points_count(me->frame, &me->error);
      if (vertices && count) {
        uint32_t step = 3 * sizeof(float);
        uint32_t len = count * step;
        auto vertex_buf = static_cast<uint8_t*>(malloc(len));

        for (size_t i = 0; i < count; i++) {
          memcpy(vertex_buf+i*step, vertices[i].xyz, step);
        }
        auto array_buffer = v8::ArrayBuffer::New(
            v8::Isolate::GetCurrent(), vertex_buf, len,
            v8::ArrayBufferCreationMode::kInternalized);

        info.GetReturnValue().Set(
            v8::Float32Array::New(array_buffer, 0, 3*count));
        return;
      }
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }


  static NAN_METHOD(GetTextureCoordinates) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      rs2_pixel* coords =
          rs2_get_frame_texture_coordinates(me->frame, &me->error);
      size_t count = rs2_get_frame_points_count(me->frame, &me->error);
      if (coords && count) {
        uint32_t step = 2 * sizeof(int);
        uint32_t len = count * step;
        auto texcoord_buf = static_cast<uint8_t*>(malloc(len));

        for (size_t i=0; i < count; i++) {
          memcpy(texcoord_buf + i * step, coords[i].ij, step);
        }
        auto array_buffer = v8::ArrayBuffer::New(
            v8::Isolate::GetCurrent(), texcoord_buf, len,
            v8::ArrayBufferCreationMode::kInternalized);

        info.GetReturnValue().Set(
            v8::Int32Array::New(array_buffer, 0, 2*count));
        return;
      }
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetPointsCount) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrame>(info.Holder());
    if (me) {
      int32_t count = rs2_get_frame_points_count(me->frame, &me->error);
      info.GetReturnValue().Set(Nan::New(count));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

 private:
  static Nan::Persistent<v8::Function> constructor;
  rs2_frame* frame;
  rs2_error* error;
  friend class RSFrameQueue;
  friend class RSSyncer;
  friend class RSColorizer;
  friend class RSPointcloud;
};

Nan::Persistent<v8::Function> RSFrame::constructor;


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
    // Nan::SetPrototypeMethod(tpl, "flush", Flush);

    constructor.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSFrameQueue").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance() {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    // auto me = Nan::ObjectWrap::Unwrap<RSFrameQueue>(instance);

    return scope.Escape(instance);
  }

 private:
  RSFrameQueue() {
    error = nullptr;
    frame_queue = nullptr;
  }

  ~RSFrameQueue() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error) rs2_free_error(error);
    error = nullptr;
    if (frame_queue) rs2_delete_frame_queue(frame_queue);
    frame_queue = nullptr;
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSFrameQueue* obj = new RSFrameQueue();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(WaitForFrame) {
    int32_t timeout = info[0]->IntegerValue();  // in ms
    auto me = Nan::ObjectWrap::Unwrap<RSFrameQueue>(info.Holder());
    if (me) {
      auto frame = rs2_wait_for_frame(me->frame_queue, timeout, &me->error);
      info.GetReturnValue().Set(RSFrame::NewInstance(frame));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(Create) {
    int32_t capacity = info[0]->IntegerValue();
    auto me = Nan::ObjectWrap::Unwrap<RSFrameQueue>(info.Holder());
    if (me) {
      me->frame_queue = rs2_create_frame_queue(capacity, &me->error);
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrameQueue>(info.Holder());
    if (me) {
      me->DestroyMe();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(PollForFrame) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrameQueue>(info.Holder());
    if (me) {
      rs2_frame* frame = nullptr;
      auto res = rs2_poll_for_frame(me->frame_queue, &frame, &me->error);
      if (res) {
        info.GetReturnValue().Set(RSFrame::NewInstance(frame));
        return;
      }
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(EnqueueFrame) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrameQueue>(info.Holder());
    auto frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[0]->ToObject());

    if (me && frame) {
      rs2_enqueue_frame(frame->frame, me->frame_queue);
      frame->frame = nullptr;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

 private:
  static Nan::Persistent<v8::Function> constructor;
  rs2_frame_queue* frame_queue;
  rs2_error* error;
  friend class RSDevice;
};

Nan::Persistent<v8::Function> RSFrameQueue::constructor;


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

    // Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    // Nan::SetPrototypeMethod(tpl, "waitForFrames", WaitForFrames);
    // Nan::SetPrototypeMethod(tpl, "pollForFrames", PollForFrames);
    // Nan::SetPrototypeMethod(tpl, "enqueueFrame", EnqueueFrame);

    constructor.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSSyncer").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(rs2_device* device) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    // auto me = Nan::ObjectWrap::Unwrap<RSSyncer>(instance);
    // me->syncer = rs2_create_syncer(device, &me->error);

    return scope.Escape(instance);
  }

 private:
  RSSyncer() {
    error = nullptr;
    // syncer = nullptr;
  }

  ~RSSyncer() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error) rs2_free_error(error);
    error = nullptr;
    // if (syncer) rs2_delete_syncer(syncer);
    // syncer = nullptr;
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSSyncer* obj = new RSSyncer();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  // static NAN_METHOD(WaitForFrames) {
  //   int32_t timeout = info[0]->IntegerValue();  // in ms
  //   auto me = Nan::ObjectWrap::Unwrap<RSSyncer>(info.Holder());
  //   if (me) {
  //     std::vector<rs2_frame*> frames(RS2_STREAM_COUNT, nullptr);
  //     rs2_wait_for_frames(me->syncer, timeout, frames.data(), &me->error);
  //     uint32_t len = 0;
  //     for (rs2_frame* f : frames) {
  //       if (f)
  //         len++;
  //     };
  //     if (len) {
  //       auto array = Nan::New<v8::Array>(len);
  //       int32_t index = 0;
  //       for (rs2_frame* f : frames) {
  //         if (f)
  //           array->Set(index++, RSFrame::NewInstance(f));
  //       }
  //       info.GetReturnValue().Set(array);
  //       return;
  //     }
  //   }
  //   info.GetReturnValue().Set(Nan::Undefined());
  // }

  // static NAN_METHOD(Destroy) {
  //   auto me = Nan::ObjectWrap::Unwrap<RSSyncer>(info.Holder());
  //   if (me) {
  //     me->DestroyMe();
  //   }
  //   info.GetReturnValue().Set(Nan::Undefined());
  // }

  // static NAN_METHOD(PollForFrames) {
  //   auto me = Nan::ObjectWrap::Unwrap<RSSyncer>(info.Holder());
  //   if (me) {
  //     std::vector<rs2_frame*> frame_refs(RS2_STREAM_COUNT, nullptr);
  //     auto res = rs2_poll_for_frames(
  //         me->syncer, frame_refs.data(), &me->error);
  //     if (res) {
  //       uint32_t len = 0;
  //       for (rs2_frame* f : frame_refs) {
  //         if (f)
  //           len++;
  //       };
  //       if (len) {
  //         auto array = Nan::New<v8::Array>(len);
  //         int32_t index = 0;
  //         for (rs2_frame* f : frame_refs) {
  //           if (f)
  //             array->Set(index++, RSFrame::NewInstance(f));
  //         }
  //         info.GetReturnValue().Set(array);
  //         return;
  //       }
  //     }
  //   }
  //   info.GetReturnValue().Set(Nan::Undefined());
  // }

  // static NAN_METHOD(EnqueueFrame) {
  //   auto me = Nan::ObjectWrap::Unwrap<RSSyncer>(info.Holder());
  //   RSFrame* frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[0]->ToObject());
  //   if (me && frame) {
  //     rs2_sync_frame(frame->frame, me->syncer);
  //   }
  //   info.GetReturnValue().Set(Nan::Undefined());
  // }

 private:
  static Nan::Persistent<v8::Function> constructor;
  // rs2_syncer* syncer;
  rs2_error* error;
  friend class RSDevice;
};

Nan::Persistent<v8::Function> RSSyncer::constructor;

class RSDevice;
class RSSensor;
class FrameCallbackInfo : public MainThreadCallbackInfo {
 public:
  FrameCallbackInfo(rs2_frame* frame,  void* data) :
      frame_(frame), sensor_(static_cast<RSSensor*>(data)) {}
  virtual ~FrameCallbackInfo() {}
  virtual void Run();
  rs2_frame* frame_;
  RSSensor* sensor_;
};

class NotificationCallbackInfo : public MainThreadCallbackInfo {
 public:
  NotificationCallbackInfo(const char* desc,
                           rs2_time_t time,
                           rs2_log_severity severity,
                           rs2_notification_category category,
                           RSDevice* dev) :
      desc_(desc), time_(time), severity_(severity),
      category_(category), dev_(dev) {}
  virtual ~NotificationCallbackInfo() {}
  virtual void Run();
  const char* desc_;
  rs2_time_t time_;
  rs2_log_severity severity_;
  rs2_notification_category category_;
  RSDevice* dev_;
  rs2_error* error_;
};

class FrameCallbackForFrameQueue : public rs2_frame_callback {
 public:
  explicit FrameCallbackForFrameQueue(rs2_frame_queue* queue)
      : frame_queue(queue) {}
  void on_frame(rs2_frame* frame) override {
    rs2_enqueue_frame(frame, frame_queue);
  }
  void release() override { delete this; }
  rs2_frame_queue* frame_queue;
};

class FrameCallbackForProc : public rs2_frame_callback {
 public:
  explicit FrameCallbackForProc(void* data) : callback_data(data) {}
  void on_frame(rs2_frame* frame) override {
    MainThreadCallback::NotifyMainThread(
      new FrameCallbackInfo(frame, callback_data));
  }
  void release() override { delete this; }
  void* callback_data;
};

class FrameCallbackForProcessingBlock : public rs2_frame_callback {
 public:
  explicit FrameCallbackForProcessingBlock(rs2_processing_block* block_ptr) :
      block(block_ptr), error(nullptr) {}
  virtual ~FrameCallbackForProcessingBlock() {
    if (error) rs2_free_error(error);
  }
  void on_frame(rs2_frame* frame) override {
    rs2_process_frame(block, frame, &error);
  }
  void release() override { delete this; }
  rs2_processing_block* block;
  rs2_error* error;
};

class RSSensor : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSSensor").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "openStream", OpenStream);
    Nan::SetPrototypeMethod(tpl, "openMultipleStream", OpenMultipleStream);
    Nan::SetPrototypeMethod(tpl, "getCameraInfo", GetCameraInfo);
    // Nan::SetPrototypeMethod(tpl, "startWithFrameQueue", StartWithFrameQueue);
    // Nan::SetPrototypeMethod(tpl, "startWithSyncer", StartWithSyncer);
    Nan::SetPrototypeMethod(tpl, "startWithCallback", StartWithCallback);
    Nan::SetPrototypeMethod(tpl, "supportsOption", SupportsOption);
    Nan::SetPrototypeMethod(tpl, "getOption", GetOption);
    Nan::SetPrototypeMethod(tpl, "setOption", SetOption);
    Nan::SetPrototypeMethod(tpl, "getOptionRange", GetOptionRange);
    Nan::SetPrototypeMethod(tpl, "isOptionReadonly", IsOptionReadonly);
    Nan::SetPrototypeMethod(tpl, "getOptionDescription", GetOptionDescription);
    Nan::SetPrototypeMethod(tpl, "GetOptionValueDescription",
        GetOptionValueDescription);
    // Nan::SetPrototypeMethod(tpl, "createSyncer", CreateSyncer);
    Nan::SetPrototypeMethod(tpl, "getMotionIntrinsics", GetMotionIntrinsics);
    Nan::SetPrototypeMethod(tpl, "stop", Stop);
    Nan::SetPrototypeMethod(tpl, "supportsCameraInfo", SupportsCameraInfo);
    Nan::SetPrototypeMethod(tpl, "getStreamProfiles", GetStreamProfiles);
    Nan::SetPrototypeMethod(tpl, "close", Close);
    // Nan::SetPrototypeMethod(tpl, "setNotificationCallback",
        // SetNotificationCallback);
    Nan::SetPrototypeMethod(tpl, "setRegionOfInterest", SetRegionOfInterest);
    Nan::SetPrototypeMethod(tpl, "getRegionOfInterest", GetRegionOfInterest);
    Nan::SetPrototypeMethod(tpl, "equals", Equals);
    Nan::SetPrototypeMethod(tpl, "getDepthScale", GetDepthScale);
    Nan::SetPrototypeMethod(tpl, "isDepthSensor", IsDepthSensor);
    Nan::SetPrototypeMethod(tpl, "isROISensor", IsROISensor);
    constructor.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSSensor").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(rs2_sensor* sensor) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(instance);
    me->sensor = sensor;

    return scope.Escape(instance);
  }

 private:
  RSSensor() {
    error = nullptr;
    sensor = nullptr;
    profile_list = nullptr;
  }

  ~RSSensor() {
    DestroyMe();
  }

  static void NotificationCallbackProc(rs2_notification* notification,
      void* user);

  void DestroyMe() {
    if (error) rs2_free_error(error);
    error = nullptr;
    if (sensor) rs2_delete_sensor(sensor);
    sensor = nullptr;
    if (profile_list) rs2_delete_stream_profiles_list(profile_list);
    profile_list = nullptr;
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSSensor* obj = new RSSensor();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(SupportsOption) {
    int32_t option = info[0]->IntegerValue();
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      int32_t on = rs2_supports_option(
          me->sensor, (rs2_option)option, &me->error);
      info.GetReturnValue().Set(Nan::New(on != 0));
      return;
    }
    info.GetReturnValue().Set(Nan::False());
  }

  static NAN_METHOD(GetOption) {
    int32_t option = info[0]->IntegerValue();
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      auto value = rs2_get_option(me->sensor, (rs2_option)option, &me->error);
      info.GetReturnValue().Set(Nan::New(value));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetOptionDescription) {
    int32_t option = info[0]->IntegerValue();
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      auto desc = rs2_get_option_description(me->sensor,
          static_cast<rs2_option>(option), &me->error);
      info.GetReturnValue().Set(Nan::New(desc).ToLocalChecked());
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetOptionValueDescription) {
    int32_t option = info[0]->IntegerValue();
    auto val = info[1]->NumberValue();
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      auto desc = rs2_get_option_value_description(me->sensor,
          static_cast<rs2_option>(option), val, &me->error);
      info.GetReturnValue().Set(Nan::New(desc).ToLocalChecked());
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(SetOption) {
    int32_t option = info[0]->IntegerValue();
    auto value = info[1]->NumberValue();
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      rs2_set_option(me->sensor, (rs2_option)option, value, &me->error);
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetOptionRange) {
    int32_t option = info[0]->IntegerValue();
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      float min = 0;
      float max = 0;
      float step = 0;
      float def = 0;
      rs2_get_option_range(me->sensor,
          static_cast<rs2_option>(option), &min, &max, &step, &def, &me->error);
      info.GetReturnValue().Set(RSOptionRange(min, max, step, def).GetObject());
      return;
    }
    info.GetReturnValue().Set(Nan::False());
  }

  static NAN_METHOD(IsOptionReadonly) {
    int32_t option = info[0]->IntegerValue();
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      int32_t yes = rs2_is_option_read_only(me->sensor,
          static_cast<rs2_option>(option), &me->error);
      info.GetReturnValue().Set(Nan::New((yes)?true:false));
      return;
    }
    info.GetReturnValue().Set(Nan::False());
  }

  static NAN_METHOD(GetCameraInfo) {
    int32_t camera_info = info[0]->IntegerValue();;
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      std::string value(rs2_get_sensor_info(me->sensor,
          static_cast<rs2_camera_info>(camera_info), &me->error));
      info.GetReturnValue().Set(Nan::New(value).ToLocalChecked());
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  // static NAN_METHOD(StartWithFrameQueue) {
  //   auto frameQueue = Nan::ObjectWrap::Unwrap<RSFrameQueue>(
  //       info[0]->ToObject());
  //   bool has_stream = info[1]->BooleanValue();
  //   int32_t stream = info[2]->IntegerValue();
  //   auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
  //   if (me) {
  //     if (has_stream) {
  //       rs2_start_stream(me->sensor,
  //         static_cast<rs2_stream>(stream), rs2_enqueue_frame,
  //         (void*)(frameQueue->frame_queue), &me->error);
  //     } else {
  //       rs2_start(me->sensor, rs2_enqueue_frame,
  //         (void*)(frameQueue->frame_queue), &me->error);
  //     }
  //   }
  //   info.GetReturnValue().Set(Nan::Undefined());
  // }

  // static NAN_METHOD(StartWithSyncer) {
  //   auto syncer = Nan::ObjectWrap::Unwrap<RSSyncer>(info[0]->ToObject());
  //   bool has_stream = info[1]->BooleanValue();
  //   int32_t stream = info[2]->IntegerValue();
  //   auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
  //   if (me) {
  //     if (has_stream) {
  //       rs2_start_stream(me->sensor,
  //         static_cast<rs2_stream>(stream), rs2_sync_frame,
  //         syncer->syncer, &me->error);
  //     } else {
  //       rs2_start(me->sensor, rs2_sync_frame, syncer->syncer,
  //         &me->error);
  //     }
  //   }
  //   info.GetReturnValue().Set(Nan::Undefined());
  // }

  static NAN_METHOD(StartWithCallback) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
       me->frame_callback_name = info[0]->ToString();
      rs2_start_cpp(me->sensor, new FrameCallbackForProc(me), &me->error);
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
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    auto profile =
        Nan::ObjectWrap::Unwrap<RSStreamProfile>(info[0]->ToObject());
    if (me && profile) {
      rs2_open(me->sensor, profile->profile, &me->error);
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(OpenMultipleStream) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      auto array = v8::Local<v8::Array>::Cast(info[0]);
      uint32_t len = array->Length();
      std::vector<const rs2_stream_profile*> profs;
      for (uint32_t i=0; i < len; i++) {
        auto profile =
            Nan::ObjectWrap::Unwrap<RSStreamProfile>(array->Get(i)->ToObject());
        profs.push_back(profile->profile);
      }
      rs2_open_multiple(me->sensor,
        profs.data(),
        len,
        &me->error);
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  // static NAN_METHOD(CreateSyncer) {
  //   auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
  //   if (me) {
  //     info.GetReturnValue().Set(RSSyncer::NewInstance(me->sensor));
  //   }
  // }

  static NAN_METHOD(GetMotionIntrinsics) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      rs2_stream stream = (rs2_stream)(info[0]->IntegerValue());
      rs2_motion_device_intrinsic output;
      rs2_get_motion_intrinsics(me->sensor, stream, &output, &me->error);
      RSMotionIntrinsics intrinsics(&output);
      info.GetReturnValue().Set(intrinsics.GetObject());
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(Stop) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      rs2_stop(me->sensor, &me->error);
    }
  }

  static NAN_METHOD(GetStreamProfiles) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      rs2_stream_profile_list* list = me->profile_list;
      if (!list) {
        list = rs2_get_stream_profiles(me->sensor, &me->error);
        me->profile_list = list;
      }
      if (list) {
        int32_t size = rs2_get_stream_profiles_count(list, &me->error);
        v8::Local<v8::Array> array = Nan::New<v8::Array>(size);
        for (int32_t i = 0; i < size; i++) {
          rs2_stream_profile* profile = const_cast<rs2_stream_profile*>(
              rs2_get_stream_profile(list, i, &me->error));
          array->Set(i, RSStreamProfile::NewInstance(profile));
        }
        info.GetReturnValue().Set(array);
        return;
      }
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(SupportsCameraInfo) {
    int32_t camera_info = info[0]->IntegerValue();
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      int32_t on = rs2_supports_sensor_info(me->sensor,
          (rs2_camera_info)camera_info, &me->error);
      info.GetReturnValue().Set(Nan::New(on));
      return;
    }
    info.GetReturnValue().Set(Nan::False());
  }

  static NAN_METHOD(Close) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      rs2_close(me->sensor, &me->error);
    }
  }

  // static NAN_METHOD(SetNotificationCallback) {
  //   auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
  //   if (me) {
  //     me->notification_callback_name = info[0]->ToString();
  //     rs2_set_notifications_callback(me->sensor,
  //         NotificationCallbackProc, (void*)me, &me->error);
  //   }
  // }

  static NAN_METHOD(SetRegionOfInterest) {
    int32_t minx = info[0]->IntegerValue();
    int32_t miny = info[1]->IntegerValue();
    int32_t maxx = info[2]->IntegerValue();
    int32_t maxy = info[3]->IntegerValue();
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me)
      rs2_set_region_of_interest(me->sensor, minx, miny,
                                 maxx, maxy, &me->error);
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetRegionOfInterest) {
    int32_t minx = 0;
    int32_t miny = 0;
    int32_t maxx = 0;
    int32_t maxy = 0;
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      rs2_get_region_of_interest(me->sensor,
          &minx, &miny, &maxx, &maxy, &me->error);
      info.GetReturnValue().Set(
          RSRegionOfInterest(minx, miny, maxx, maxy).GetObject());
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(Equals) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      // TODO(tingshao): impl this
    }
  }

  static NAN_METHOD(GetDepthScale) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      auto scale = rs2_get_depth_scale(me->sensor, &me->error);
      info.GetReturnValue().Set(Nan::New(scale));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(IsDepthSensor) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      bool is_depth = rs2_is_sensor_extendable_to(me->sensor,
          RS2_EXTENSION_DEPTH_SENSOR, &me->error);
      info.GetReturnValue().Set(Nan::New(is_depth));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(IsROISensor) {
    auto me = Nan::ObjectWrap::Unwrap<RSSensor>(info.Holder());
    if (me) {
      bool is_roi = rs2_is_sensor_extendable_to(me->sensor,
          RS2_EXTENSION_ROI, &me->error);
      info.GetReturnValue().Set(Nan::New(is_roi));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

 private:
  static Nan::Persistent<v8::Function> constructor;
  rs2_sensor* sensor;
  rs2_error* error;
  rs2_stream_profile_list* profile_list;
  v8::Local<v8::String> frame_callback_name;
  // v8::Local<v8::String> notification_callback_name;
  friend class RSContext;
  friend class DevicesChangedCallbackInfo;
  friend class FrameCallbackInfo;
  friend class NotificationCallbackInfo;
};

Nan::Persistent<v8::Function> RSSensor::constructor;

// void RSDevice::NotificationCallbackProc(rs2_notification* notification,
//     void* user) {
//   if (notification && user) {
//     RSDevice* dev = reinterpret_cast<RSDevice*>(user);
//     const char* desc = rs2_get_notification_description(notification,
//         &dev->error);
//     rs2_time_t time = rs2_get_notification_timestamp(notification,
//         &dev->error);
//     rs2_log_severity severity = rs2_get_notification_severity(notification,
//         &dev->error);
//     rs2_notification_category category =
//         rs2_get_notification_category(notification, &dev->error);
//     MainThreadCallback::NotifyMainThread(
//         new NotificationCallbackInfo(desc,
//             time, severity, category, (RSDevice*)user));
//   }
// }

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
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSDevice").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "getCameraInfo", GetCameraInfo);
    Nan::SetPrototypeMethod(tpl, "supportsCameraInfo", SupportsCameraInfo);
    Nan::SetPrototypeMethod(tpl, "reset", Reset);
    Nan::SetPrototypeMethod(tpl, "querySensors", QuerySensors);

    constructor.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSDevice").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(rs2_device* dev) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(instance);
    me->dev = dev;

    return scope.Escape(instance);
  }

 private:
  RSDevice() {
    error = nullptr;
    dev = nullptr;
  }

  ~RSDevice() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error) rs2_free_error(error);
    error = nullptr;
    if (dev) rs2_delete_device(dev);
    dev = nullptr;
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSDevice* obj = new RSDevice();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(GetCameraInfo) {
    int32_t camera_info = info[0]->IntegerValue();;
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (me) {
      std::string value(rs2_get_device_info(me->dev,
          static_cast<rs2_camera_info>(camera_info), &me->error));
      info.GetReturnValue().Set(Nan::New(value).ToLocalChecked());
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (me) {
      me->DestroyMe();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }


  static NAN_METHOD(SupportsCameraInfo) {
    int32_t camera_info = info[0]->IntegerValue();
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (me) {
      int32_t on = rs2_supports_device_info(me->dev,
          (rs2_camera_info)camera_info, &me->error);
      info.GetReturnValue().Set(Nan::New(on));
      return;
    }
    info.GetReturnValue().Set(Nan::False());
  }

  static NAN_METHOD(Reset) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (me) {
      rs2_hardware_reset(me->dev, &me->error);
    }
  }

  static NAN_METHOD(QuerySensors) {
    auto me = Nan::ObjectWrap::Unwrap<RSDevice>(info.Holder());
    if (me) {
      rs2_sensor_list* list = rs2_query_sensors(me->dev, &me->error);
      if (list) {
        auto size = rs2_get_sensors_count(list, &me->error);
        if (size) {
          v8::Local<v8::Array> array = Nan::New<v8::Array>();
          for (int32_t i = 0; i < size; i++) {
            rs2_sensor* sensor = rs2_create_sensor(list, i, &me->error);
            array->Set(i, RSSensor::NewInstance(sensor));
          }
          info.GetReturnValue().Set(array);
          return;
        }
      }
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

 private:
  static Nan::Persistent<v8::Function> constructor;
  rs2_device* dev;
  rs2_error* error;
  // v8::Local<v8::String> frame_callback_name;
  // v8::Local<v8::String> notification_callback_name;
  friend class RSContext;
  friend class DevicesChangedCallbackInfo;
  friend class FrameCallbackInfo;
  friend class NotificationCallbackInfo;
  friend class RSPipeline;
};

Nan::Persistent<v8::Function> RSDevice::constructor;

void FrameCallbackInfo::Run() {
  Nan::HandleScope scope;

  v8::Local<v8::Value> args[1] = {RSFrame::NewInstance(frame_)};
  Nan::MakeCallback(sensor_->handle(), sensor_->frame_callback_name, 1, args);
}

void NotificationCallbackInfo::Run() {
  // v8::Local<v8::Value> args[1] = {
  //   RSNotification(desc_, time_, severity_, category_).GetObject()
  // };
  // Nan::MakeCallback(sensor_->handle(),
  //     sensor_->notification_callback_name, 1, args);
}

class RSPointcloud : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSPointcloud").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "calculate", Calculate);
    Nan::SetPrototypeMethod(tpl, "mapTo", MapTo);

    constructor.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSPointcloud").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(rs2_processing_block* pc) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    auto me = Nan::ObjectWrap::Unwrap<RSPointcloud>(instance);
    me->pc = pc;
    auto callback = new FrameCallbackForFrameQueue(me->frame_queue);
    rs2_start_processing(me->pc, callback, &me->error);

    return scope.Escape(instance);
  }

 private:
  RSPointcloud() {
    error = nullptr;
    pc = nullptr;
    frame_queue = rs2_create_frame_queue(1, &error);
  }

  ~RSPointcloud() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error) rs2_free_error(error);
    error = nullptr;
    if (pc) rs2_delete_processing_block(pc);
    pc = nullptr;
    if (frame_queue) rs2_delete_frame_queue(frame_queue);
    frame_queue = nullptr;
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSPointcloud>(info.Holder());
    if (me) {
      me->DestroyMe();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSPointcloud* obj = new RSPointcloud();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(Calculate) {
    auto me = Nan::ObjectWrap::Unwrap<RSPointcloud>(info.Holder());
    auto frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[0]->ToObject());
    if (me && frame) {
      rs2_process_frame(me->pc, frame->frame, &me->error);
      auto frame = rs2_wait_for_frame(me->frame_queue, 5000, &me->error);
      if (frame) {
        info.GetReturnValue().Set(RSFrame::NewInstance(frame));
        return;
      }
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(MapTo) {
    auto me = Nan::ObjectWrap::Unwrap<RSPointcloud>(info.Holder());
    auto frame = Nan::ObjectWrap::Unwrap<RSFrame>(info[0]->ToObject());
    if (me && frame) {
      rs2_process_frame(me->pc, frame->frame, &me->error);
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

 private:
  static Nan::Persistent<v8::Function> constructor;

  rs2_processing_block* pc;
  rs2_frame_queue* frame_queue;
  rs2_error* error;
};

Nan::Persistent<v8::Function> RSPointcloud::constructor;

class PlaybackStatusChangedCallback :
    public rs2_playback_status_changed_callback {
  virtual void on_playback_status_changed(rs2_playback_status status) {
    // TODO(tingshao): add more logic here.
  }
  virtual void release() { delete this; }
  virtual ~PlaybackStatusChangedCallback() {}
};


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
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSContext").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "create", Create);
    Nan::SetPrototypeMethod(tpl, "getDeviceCount", GetDeviceCount);
    Nan::SetPrototypeMethod(tpl, "getDevice", GetDevice);
    Nan::SetPrototypeMethod(tpl, "setDeviceChangedCallback",
        SetDeviceChangedCallback);
    Nan::SetPrototypeMethod(tpl, "isDeviceConnected", IsDeviceConnected);
    Nan::SetPrototypeMethod(tpl, "loadDeviceFile", LoadDeviceFile);

    constructor.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSContext").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(rs2_context* ctx_ptr = nullptr) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    // If ctx_ptr is provided, no need to call create.
    if (ctx_ptr) {
      auto me = Nan::ObjectWrap::Unwrap<RSContext>(instance);
      me->ctx = ctx_ptr;
      me->device_list = rs2_query_devices(me->ctx, &me->error);
    }
    return scope.Escape(instance);
  }

 private:
  static NAN_METHOD(CreateAlign);
  RSContext() {
    error = nullptr;
    ctx = nullptr;
    device_list = nullptr;
  }

  ~RSContext() {
    DestroyMe();
  }
  void RegisterDevicesChangedCallbackMethod();
  void DestroyMe() {
    if (error) rs2_free_error(error);
    error = nullptr;
    if (ctx) rs2_delete_context(ctx);
    ctx = nullptr;
    if (device_list) rs2_delete_device_list(device_list);
    device_list = nullptr;
    for (auto it = devices.begin(); it != devices.end(); ++it) {
      (*it)->Reset();
    }
    devices.clear();
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (info.IsConstructCall()) {
      RSContext* obj = new RSContext();
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  }

  static NAN_METHOD(Create) {
    MainThreadCallback::Init();
    auto me = Nan::ObjectWrap::Unwrap<RSContext>(info.Holder());
    if (me) {
      me->ctx = rs2_create_context(RS2_API_VERSION, &me->error);
      me->device_list = rs2_query_devices(me->ctx, &me->error);
    }
    info.GetReturnValue().Set(Nan::True());
  }

  static NAN_METHOD(GetDeviceCount) {
    auto me = Nan::ObjectWrap::Unwrap<RSContext>(info.Holder());
    if (me) {
      info.GetReturnValue().Set(
          Nan::New(rs2_get_device_count(me->device_list, &me->error)));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetDevice) {
    int32_t index = info[0]->IntegerValue();
    auto me = Nan::ObjectWrap::Unwrap<RSContext>(info.Holder());
    if (me) {
      auto jsdev = me->CreateNewDevice(me->device_list, index, &me->error);
      info.GetReturnValue().Set(jsdev);
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSContext>(info.Holder());
    if (me) {
      me->DestroyMe();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(SetDeviceChangedCallback) {
    auto me = Nan::ObjectWrap::Unwrap<RSContext>(info.Holder());
    if (me) {
      me->device_changed_callback_name = info[0]->ToString();
      me->RegisterDevicesChangedCallbackMethod();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  v8::Local<v8::Object> CreateNewDevice(rs2_device_list* list,
      uint32_t index, rs2_error** error) {
    auto dev = rs2_create_device(list, index, error);
    auto jsdev = RSDevice::NewInstance(dev);
    Nan::Persistent<v8::Object> *pjsobj =
        new Nan::Persistent<v8::Object>(jsdev);
    // devices.push_back(Nan::Persistent<v8::Object>(jsdev));
    devices.push_back(pjsobj);
    return jsdev;
  }

  static NAN_METHOD(IsDeviceConnected) {
    auto me = Nan::ObjectWrap::Unwrap<RSContext>(info.Holder());
    RSDevice* dev = Nan::ObjectWrap::Unwrap<RSDevice>(info[0]->ToObject());
    if (me && dev) {
      rs2_device_list* list = rs2_query_devices(me->ctx, &me->error);
      if (list && rs2_device_list_contains(list, dev->dev, &me->error)) {
        info.GetReturnValue().Set(Nan::New(true));
        return;
      }
      info.GetReturnValue().Set(Nan::New(false));
      return;
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(LoadDeviceFile) {
    auto me = Nan::ObjectWrap::Unwrap<RSContext>(info.Holder());
    if (me) {
      auto device_file = info[0]->ToString();
      v8::String::Utf8Value value(device_file);
      auto dev = rs2_context_add_device(me->ctx, *value, &me->error);
      if (dev) {
        auto jsobj = RSDevice::NewInstance(dev);
        info.GetReturnValue().Set(jsobj);
        return;
      }
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

 private:
  static Nan::Persistent<v8::Function> constructor;

  rs2_context* ctx;
  rs2_device_list* device_list;
  rs2_error* error;
  std::list<Nan::Persistent<v8::Object>*> devices;
  v8::Local<v8::String> device_changed_callback_name;
  friend class DevicesChangedCallbackInfo;
  friend class RSPipeline;
};

Nan::Persistent<v8::Function> RSContext::constructor;

class DevicesChangedCallbackInfo : public MainThreadCallbackInfo {
 public:
  DevicesChangedCallbackInfo(rs2_device_list* r,
      rs2_device_list* a, RSContext* ctx) :
      removed_(r), added_(a), ctx_(ctx) {}
  virtual ~DevicesChangedCallbackInfo() {
    rs2_delete_device_list(removed_);
    rs2_delete_device_list(added_);
  }
  virtual void Run() {
    v8::Local<v8::Array> removed_array = Nan::New<v8::Array>();
    v8::Local<v8::Array> added_array = Nan::New<v8::Array>();
    uint32_t removed_index = 0;
    uint32_t added_index = 0;

    // prepare removed devices
    if (ctx_->devices.size()) {
      for (auto it = ctx_->devices.begin(); it != ctx_->devices.end(); ++it) {
        RSDevice* rsdevice =
            Nan::ObjectWrap::Unwrap<RSDevice>(Nan::New(*(*it)));
        if (rs2_device_list_contains(removed_, rsdevice->dev, &ctx_->error)) {
          removed_array->Set(removed_index++, Nan::New(*it));
        }
      }
    }

    // // TODO: Do we need to remove all removed deivces from the devices list?.
    // devices.remove_if([&removed](v8::Persistent<v8::Object> jsdev) {
    //   RSDevice* rsdevice =
    //       Nan::ObjectWrap::Unwrap<RSDevice>(Nan::New(jsdev));
    //   if (rs2_device_list_contains(removed, rsdevice->dev))
    //     return true;
    //   else
    //     return false;
    // });

    // prepare newly connected devices
    auto cnt = rs2_get_device_count(added_, &ctx_->error);
    for (int32_t i=0; i < cnt; i++) {
      auto jsdev = ctx_->CreateNewDevice(ctx_->device_list, i, &ctx_->error);
      added_array->Set(added_index++, jsdev);
    }
    v8::Local<v8::Value> args[2] = {removed_array, added_array};
    Nan::MakeCallback(ctx_->handle(),
        ctx_->device_changed_callback_name, 2, args);
  }
  rs2_device_list* removed_;
  rs2_device_list* added_;
  RSContext* ctx_;
};

class DevicesChangedCallback : public rs2_devices_changed_callback {
 public:
  explicit DevicesChangedCallback(RSContext* context) : ctx(context) {}
  virtual void on_devices_changed(
      rs2_device_list* removed, rs2_device_list* added) {
    // TODO(tingshao):
    //  (1) delete this new DevicesChangedCallBackInfo() object later
    //  (2) find a way not to overwrite the previous pointer
    //      in NotifyMainThread() function, when there are frequent callbacks
    MainThreadCallback::NotifyMainThread(
        new DevicesChangedCallbackInfo(removed, added, ctx));
  }
  virtual void release() {
    delete this;
  }
  virtual ~DevicesChangedCallback() {}
  RSContext* ctx;
};

void RSContext::RegisterDevicesChangedCallbackMethod() {
  rs2_set_devices_changed_callback_cpp(ctx, new DevicesChangedCallback(this),
      &error);
}

class StreamProfileExtrator {
 public:
  explicit StreamProfileExtrator(const rs2_stream_profile* profile) {
    rs2_get_stream_profile_data(profile, &stream, &format,  &index,  &unique_id,
        &fps, &error);
  }
  ~StreamProfileExtrator() {}
  rs2_stream stream;
  rs2_format format;
  int32_t fps;
  int32_t index;
  int32_t unique_id;
  rs2_error* error;
};


class RSFrameSet : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSFrameSet").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "getSize", GetSize);
    Nan::SetPrototypeMethod(tpl, "at", At);
    Nan::SetPrototypeMethod(tpl, "getFrame", GetFrame);

    constructor.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSFrameSet").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(rs2_frame* frame) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();
    auto me = Nan::ObjectWrap::Unwrap<RSFrameSet>(instance);
    me->SetFrame(frame);

    return scope.Escape(instance);
  }

 private:
  RSFrameSet() {
    error = nullptr;
    frames = nullptr;
  }

  ~RSFrameSet() {
    DestroyMe();
  }

  void SetFrame(rs2_frame* frame) {
    if (rs2_is_frame_extendable_to(
        frame, RS2_EXTENSION_COMPOSITE_FRAME, &error)) {
      frames = frame;
      frame_count = rs2_embedded_frames_count(frame, &error);
    }
  }

  void DestroyMe() {
    if (error) rs2_free_error(error);
    error = nullptr;
    if (frames) rs2_release_frame(frames);
    frames = nullptr;
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
    if (me && me->frames) {
      info.GetReturnValue().Set(Nan::New(me->frame_count));
      return;
    }
    info.GetReturnValue().Set(Nan::New(0));
  }

  static NAN_METHOD(GetFrame) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrameSet>(info.Holder());
    rs2_stream stream = static_cast<rs2_stream>(info[0]->IntegerValue());
    if (me && me->frames) {
      for (uint32_t i=0; i < me->frame_count; i++) {
        rs2_frame* frame = rs2_extract_frame(me->frames, i, &me->error);
        if (frame) {
          const rs2_stream_profile* profile = rs2_get_frame_stream_profile(
              frame, &me->error);
          StreamProfileExtrator extrator(profile);
          if (extrator.stream == stream) {
            info.GetReturnValue().Set(RSFrame::NewInstance(frame));
            return;
          }
          rs2_release_frame(frame);
        }
      }
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(At) {
    auto me = Nan::ObjectWrap::Unwrap<RSFrameSet>(info.Holder());
    int32_t index = info[0]->IntegerValue();
    if (me && me->frames) {
      rs2_frame* frame = rs2_extract_frame(me->frames, index, &me->error);
      if (frame) {
        info.GetReturnValue().Set(RSFrame::NewInstance(frame));
        return;
      }
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

 private:
  static Nan::Persistent<v8::Function> constructor;
  rs2_frame* frames;
  uint32_t frame_count;
  rs2_error* error;
};

Nan::Persistent<v8::Function> RSFrameSet::constructor;

class RSPipeline : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSPipeline").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "waitForFrames", WaitForFrames);
    Nan::SetPrototypeMethod(tpl, "startWithAlign", StartWithAlign);
    Nan::SetPrototypeMethod(tpl, "start", Start);
    Nan::SetPrototypeMethod(tpl, "stop", Stop);
    Nan::SetPrototypeMethod(tpl, "getDevice", GetDevice);

    constructor.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSPipeline").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance() {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    // auto me = Nan::ObjectWrap::Unwrap<RSPipeline>(instance);

    return scope.Escape(instance);
  }

 private:
  static NAN_METHOD(StartWithAlign);
  RSPipeline() {
    error = nullptr;
    pipeline = nullptr;
    ctx = nullptr;
  }

  ~RSPipeline() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error) rs2_free_error(error);
    error = nullptr;
    if (pipeline) rs2_delete_pipeline(pipeline);
    pipeline = nullptr;
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

  static NAN_METHOD(Start) {
    auto me = Nan::ObjectWrap::Unwrap<RSPipeline>(info.Holder());
    if (me && me->pipeline) {
      rs2_start_pipeline(me->pipeline, &me->error);
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(Stop) {
    auto me = Nan::ObjectWrap::Unwrap<RSPipeline>(info.Holder());
    if (me && me->pipeline) {
      rs2_stop_pipeline(me->pipeline, &me->error);
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(WaitForFrames) {
    auto me = Nan::ObjectWrap::Unwrap<RSPipeline>(info.Holder());
    auto timeout = info[0]->IntegerValue();
    if (me) {
      rs2_frame* frames = rs2_pipeline_wait_for_frames(
          me->pipeline, timeout, &me->error);
      if (frames) {
        info.GetReturnValue().Set(RSFrameSet::NewInstance(frames));
        return;
      }
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(GetDevice) {
    auto me = Nan::ObjectWrap::Unwrap<RSPipeline>(info.Holder());
    if (me) {
      rs2_device* dev = rs2_pipeline_get_device(me->ctx, me->pipeline,
          &me->error);
      if (dev) {
        info.GetReturnValue().Set(RSDevice::NewInstance(dev));
        return;
      }
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

 private:
  static Nan::Persistent<v8::Function> constructor;

  rs2_pipeline* pipeline;
  rs2_context* ctx;
  rs2_error* error;
};

Nan::Persistent<v8::Function> RSPipeline::constructor;

class RSColorizer : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSColorizer").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "create", Create);
    Nan::SetPrototypeMethod(tpl, "colorize", Colorize);

    constructor.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSColorizer").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance() {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();

    // auto me = Nan::ObjectWrap::Unwrap<RSColorizer>(instance);

    return scope.Escape(instance);
  }

 private:
  RSColorizer() {
    error = nullptr;
    colorizer = nullptr;
    frame_queue = nullptr;
  }

  ~RSColorizer() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error) rs2_free_error(error);
    error = nullptr;
    if (colorizer) rs2_delete_processing_block(colorizer);
    colorizer = nullptr;
    if (frame_queue) rs2_delete_frame_queue(frame_queue);
    frame_queue = nullptr;
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
    auto me = Nan::ObjectWrap::Unwrap<RSColorizer>(info.Holder());
    if (me) {
      me->colorizer = rs2_create_colorizer(&me->error);
      me->frame_queue = rs2_create_frame_queue(1, &me->error);
      auto callback = new FrameCallbackForFrameQueue(me->frame_queue);
      rs2_start_processing(me->colorizer, callback, &me->error);
      // callback->release();
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

  static NAN_METHOD(Colorize) {
    auto me = Nan::ObjectWrap::Unwrap<RSColorizer>(info.Holder());
    RSFrame* depth = Nan::ObjectWrap::Unwrap<RSFrame>(info[0]->ToObject());
    if (me && depth) {
      rs2_process_frame(me->colorizer, depth->frame, &me->error);
      rs2_frame* result = rs2_wait_for_frame(me->frame_queue, 5000, &me->error);
      if (result) {
        info.GetReturnValue().Set(RSFrame::NewInstance(result));
        return;
      }
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

 private:
  static Nan::Persistent<v8::Function> constructor;

  rs2_processing_block* colorizer;
  rs2_frame_queue* frame_queue;
  rs2_error* error;
};

Nan::Persistent<v8::Function> RSColorizer::constructor;

class RSAlign : public Nan::ObjectWrap {
 public:
  static void Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RSAlign").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "destroy", Destroy);
    Nan::SetPrototypeMethod(tpl, "waitForFrames", WaitForFrames);

    constructor.Reset(tpl->GetFunction());
    exports->Set(Nan::New("RSAlign").ToLocalChecked(), tpl->GetFunction());
  }

  static v8::Local<v8::Object> NewInstance(rs2_processing_block* align_ptr) {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    v8::Local<v8::Context> context =
        v8::Isolate::GetCurrent()->GetCurrentContext();

    v8::Local<v8::Object> instance =
        cons->NewInstance(context, 0, nullptr).ToLocalChecked();
    auto me = Nan::ObjectWrap::Unwrap<RSAlign>(instance);
    me->align = align_ptr;
    me->frame_queue = rs2_create_frame_queue(1, &me->error);
    auto callback = new FrameCallbackForFrameQueue(me->frame_queue);
    rs2_start_processing(me->align, callback, &me->error);
    return scope.Escape(instance);
  }

 private:
  RSAlign() {
    error = nullptr;
    align = nullptr;
    frame_queue = nullptr;
  }

  ~RSAlign() {
    DestroyMe();
  }

  void DestroyMe() {
    if (error) rs2_free_error(error);
    error = nullptr;
    if (align) rs2_delete_processing_block(align);
    align = nullptr;
    if (frame_queue) rs2_delete_frame_queue(frame_queue);
    frame_queue = nullptr;
  }

  static NAN_METHOD(Destroy) {
    auto me = Nan::ObjectWrap::Unwrap<RSAlign>(info.Holder());
    if (me) me->DestroyMe();

    info.GetReturnValue().Set(Nan::Undefined());
  }

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (!info.IsConstructCall()) return;

    RSAlign* obj = new RSAlign();
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  }

  static NAN_METHOD(WaitForFrames) {
    auto me = Nan::ObjectWrap::Unwrap<RSAlign>(info.Holder());
    if (me) {
      rs2_frame* result = rs2_wait_for_frame(me->frame_queue, 5000, &me->error);
      if (result) {
        info.GetReturnValue().Set(RSFrameSet::NewInstance(result));
        return;
      }
    }
    info.GetReturnValue().Set(Nan::Undefined());
  }

 private:
  static Nan::Persistent<v8::Function> constructor;

  rs2_processing_block* align;
  rs2_frame_queue* frame_queue;
  rs2_error* error;
  friend class RSPipeline;
};

Nan::Persistent<v8::Function> RSAlign::constructor;

NAN_METHOD(RSPipeline::StartWithAlign) {
  auto me = Nan::ObjectWrap::Unwrap<RSPipeline>(info.Holder());
  auto align = Nan::ObjectWrap::Unwrap<RSAlign>(info[0]->ToObject());
  if (me && align && me->pipeline) {
    rs2_start_pipeline_with_callback_cpp(me->pipeline,
        new FrameCallbackForProcessingBlock(align->align), &me->error);
  }
  info.GetReturnValue().Set(Nan::Undefined());
}


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
  info.GetReturnValue().Set(Nan::Undefined());
}

#define _FORCE_SET_ENUM(name) \
  exports->ForceSet(Nan::New(#name).ToLocalChecked(), \
  Nan::New(static_cast<int>((name))), \
  static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete));

// TODO(tingshao): try DefineOwnProperty or CreateDataProperty
// e.g.
// exports->DefineOwnProperty(env->context(),
//                            OneByteString(env->isolate(), str),
//                            True(env->isolate()), ReadOnly).FromJust();

void InitModule(v8::Local<v8::Object> exports) {
  exports->Set(Nan::New("globalCleanup").ToLocalChecked(),
    Nan::New<v8::FunctionTemplate>(GlobalCleanup)->GetFunction());

  RSContext::Init(exports);
  RSPointcloud::Init(exports);
  RSPipeline::Init(exports);
  RSFrameSet::Init(exports);
  RSSensor::Init(exports);
  RSDevice::Init(exports);
  RSStreamProfile::Init(exports);
  RSColorizer::Init(exports);
  RSFrameQueue::Init(exports);
  RSFrame::Init(exports);
  RSSyncer::Init(exports);
  RSAlign::Init(exports);

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
  _FORCE_SET_ENUM(RS2_FRAME_METADATA_COUNT);

  // rs2_distortion
  _FORCE_SET_ENUM(RS2_DISTORTION_NONE);
  _FORCE_SET_ENUM(RS2_DISTORTION_MODIFIED_BROWN_CONRADY);
  _FORCE_SET_ENUM(RS2_DISTORTION_INVERSE_BROWN_CONRADY);
  _FORCE_SET_ENUM(RS2_DISTORTION_FTHETA);
  _FORCE_SET_ENUM(RS2_DISTORTION_BROWN_CONRADY);
  _FORCE_SET_ENUM(RS2_DISTORTION_COUNT);

  // rs2_ivcam_preset
  // _FORCE_SET_ENUM(RS2_VISUAL_PRESET_SHORT_RANGE);
  // _FORCE_SET_ENUM(RS2_VISUAL_PRESET_LONG_RANGE);
  // _FORCE_SET_ENUM(RS2_VISUAL_PRESET_BACKGROUND_SEGMENTATION);
  // _FORCE_SET_ENUM(RS2_VISUAL_PRESET_GESTURE_RECOGNITION);
  // _FORCE_SET_ENUM(RS2_VISUAL_PRESET_OBJECT_SCANNING);
  // _FORCE_SET_ENUM(RS2_VISUAL_PRESET_FACE_ANALYTICS);
  // _FORCE_SET_ENUM(RS2_VISUAL_PRESET_FACE_LOGIN);
  // _FORCE_SET_ENUM(RS2_VISUAL_PRESET_GR_CURSOR);
  // _FORCE_SET_ENUM(RS2_VISUAL_PRESET_DEFAULT);
  // _FORCE_SET_ENUM(RS2_VISUAL_PRESET_MID_RANGE);
  // _FORCE_SET_ENUM(RS2_VISUAL_PRESET_IR_ONLY);
  // _FORCE_SET_ENUM(RS2_VISUAL_PRESET_COUNT);

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
  _FORCE_SET_ENUM(RS2_OPTION_COUNT);

  // rs2_camera_info
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_NAME);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_SERIAL_NUMBER);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_FIRMWARE_VERSION);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_PHYSICAL_PORT);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_DEBUG_OP_CODE);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_ADVANCED_MODE);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_PRODUCT_ID);
  _FORCE_SET_ENUM(RS2_CAMERA_INFO_CAMERA_LOCKED);
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
  _FORCE_SET_ENUM(RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR);
  _FORCE_SET_ENUM(RS2_NOTIFICATION_CATEGORY_COUNT);

  // rs2_timestamp_domain
  _FORCE_SET_ENUM(RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK);
  _FORCE_SET_ENUM(RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME);
  _FORCE_SET_ENUM(RS2_TIMESTAMP_DOMAIN_COUNT);
}

NODE_MODULE(node_librealsense, InitModule);
