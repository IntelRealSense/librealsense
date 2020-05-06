#pragma once
#include "librealsense2/rs.hpp"
#include "librealsense2/rs_advanced_mode.hpp"
#include <type_traits>

// extra checks to help specialization priority resoultion
template <typename T> using extra_checks = std::bool_constant<std::is_base_of<rs2::frame, T>::value
//                                                           || std::is_base_of<rs2::options, T>::value
                                                           || std::is_base_of<rs2::stream_profile, T>::value>;

// C API

// rs_types.hpp

// rs_frame.hpp
template<typename T> struct MatlabParamParser::type_traits<T, typename std::enable_if<std::is_base_of<rs2::stream_profile, T>::value>::type> {
    using rs2_internal_t = const rs2_stream_profile*;
    using use_cells = std::true_type;
    static T from_internal(rs2_internal_t * ptr) { return T(rs2::stream_profile(*ptr)); }
};
template<typename T> struct MatlabParamParser::type_traits<T, typename std::enable_if<std::is_base_of<rs2::frame, T>::value>::type> { using rs2_internal_t = rs2_frame * ; };

// rs_sensor.hpp
template<> struct MatlabParamParser::type_traits<rs2::options> {
    // since it is impossible to create an rs2::options object (you must cast from a deriving type)
    // The carrier's job is to help jump bridge that gap. Each type that derives directly from rs2::options
    // must be added to the carrier's logic
    struct carrier {
        void * ptr;
        enum class types { rs2_sensor, rs2_processing_block } type;
        carrier(void *ptr_, types t) : ptr(ptr_), type(t) {}
        ~carrier(); // implemented at the bottom with an explanation as to why
    };
    using rs2_internal_t = carrier;
    static rs2::options from_internal(rs2_internal_t * ptr);
};
template<typename T> struct MatlabParamParser::type_traits<T, typename std::enable_if<std::is_base_of<rs2::sensor, T>::value>::type>
    : MatlabParamParser::type_traits<rs2::options> {
    using carrier_t = std::shared_ptr<rs2_sensor>;
    using carrier_enum = std::integral_constant<rs2_internal_t::types, rs2_internal_t::types::rs2_sensor>;
    static T from_internal(rs2_internal_t * ptr) {
        if (ptr->type == carrier_enum::value) return T(rs2::sensor(*reinterpret_cast<carrier_t*>(ptr->ptr)));
        mexErrMsgTxt("Error parsing argument, object is not a sensor");
    }
    static rs2_internal_t* to_internal(T&& var) { mexLock(); return new rs2_internal_t(new carrier_t(var), carrier_enum::value); }
};

// rs_device.hpp
template<> struct MatlabParamParser::type_traits<rs2::device> { using rs2_internal_t = std::shared_ptr<rs2_device>; };
template<> struct MatlabParamParser::type_traits<rs2::device_list> {
    using rs2_internal_t = std::shared_ptr<rs2_device_list>;
    using use_cells = std::true_type;
};

// rs_record_playback.hpp
template<> struct MatlabParamParser::type_traits<rs2::playback> : MatlabParamParser::type_traits<rs2::device> {
    static rs2::playback from_internal(rs2_internal_t * ptr) { return traits_trampoline::from_internal<rs2::device>(ptr); }
};
template<> struct MatlabParamParser::type_traits<rs2::recorder> : MatlabParamParser::type_traits<rs2::device> {
    static rs2::recorder from_internal(rs2_internal_t * ptr) { return traits_trampoline::from_internal<rs2::device>(ptr); }
};

// rs_processing.hpp
template <typename T> struct over_wrapper {
    using rs2_internal_t = std::shared_ptr<T>;
    static T from_internal(rs2_internal_t * ptr) { return T(**ptr); }
    static rs2_internal_t* to_internal(T&& val) { mexLock(); return new rs2_internal_t(new T(val)); }
};
template<typename T> struct MatlabParamParser::type_traits<T, typename std::enable_if<std::is_base_of<rs2::processing_block, T>::value>::type>
    : MatlabParamParser::type_traits<rs2::options> {
    using carrier_t = std::shared_ptr<rs2::processing_block>;
    using carrier_enum = std::integral_constant<rs2_internal_t::types, rs2_internal_t::types::rs2_processing_block>;
    static T from_internal(rs2_internal_t * ptr) {
        if (ptr->type == carrier_enum::value) return *std::dynamic_pointer_cast<T>(*static_cast<carrier_t*>(ptr->ptr));
        mexErrMsgTxt("Error parsing argument, object is not a processing block");
    }
    static rs2_internal_t* to_internal(T&& var) { mexLock(); return new rs2_internal_t(new carrier_t(new T(var)), carrier_enum::value); }
};
template<> struct MatlabParamParser::type_traits<rs2::syncer> : over_wrapper<rs2::syncer> {};
template<> struct MatlabParamParser::type_traits<rs2::frame_queue> : over_wrapper<rs2::frame_queue> {};

// rs_context.hpp
// rs2::event_information                       [?]
template<> struct MatlabParamParser::type_traits<rs2::context> { using rs2_internal_t = std::shared_ptr<rs2_context>; };
template<> struct MatlabParamParser::type_traits<rs2::device_hub> { using rs2_internal_t = std::shared_ptr<rs2_device_hub>; };

// rs_pipeline.hpp
template<> struct MatlabParamParser::type_traits<rs2::pipeline> { using rs2_internal_t = std::shared_ptr<rs2_pipeline>; };
template<> struct MatlabParamParser::type_traits<rs2::config> { using rs2_internal_t = std::shared_ptr<rs2_config>; };
template<> struct MatlabParamParser::type_traits<rs2::pipeline_profile> { using rs2_internal_t = std::shared_ptr<rs2_pipeline_profile>; };

// rs_advanced_mode.hpp
template <> struct MatlabParamParser::type_traits<rs400::advanced_mode> : MatlabParamParser::type_traits<rs2::device> {
    static rs400::advanced_mode from_internal(rs2_internal_t * ptr) { return traits_trampoline::from_internal<rs2::device>(ptr); }
};

// This needs to go at the bottom so that all the relevant type_traits specializations will have already occured.
MatlabParamParser::type_traits<rs2::options>::carrier::~carrier() {
    switch (type) {
    case types::rs2_sensor: delete reinterpret_cast<type_traits<rs2::sensor>::carrier_t*>(ptr); break;
    case types::rs2_processing_block: delete reinterpret_cast<type_traits<rs2::processing_block>::carrier_t*>(ptr); break;
    }
}

rs2::options MatlabParamParser::type_traits<rs2::options>::from_internal(rs2_internal_t * ptr) {
    switch (ptr->type) {
    case carrier::types::rs2_sensor: return traits_trampoline::from_internal<rs2::sensor>(ptr).as<rs2::options>();
    // TODO: Fix
    case carrier::types::rs2_processing_block: return *std::shared_ptr<rs2::options>(*static_cast<type_traits<rs2::processing_block>::carrier_t*>(ptr->ptr));
    default: mexErrMsgTxt("Error parsing argument of type rs2::options: unrecognized carrier type");
    }

}
