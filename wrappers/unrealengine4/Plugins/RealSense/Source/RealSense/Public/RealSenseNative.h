#pragma once

#pragma warning(push)
#pragma warning(disable: 4456)
#pragma warning(disable: 4458)
#pragma warning(disable: 4577)
#include <librealsense2/rs.hpp>
#pragma warning(pop)

namespace rs2 {

inline void release_context_ref(rs2_context* Handle) {} // do nothing

class context_ref : public context {
public:
	context_ref(rs2_context* Handle) : context(std::shared_ptr<rs2_context>(Handle, release_context_ref)) { }
};

class error_ref {
public:
	inline error_ref() : m_error(nullptr) {}
	inline ~error_ref() { release(); }
	inline void release() { if (m_error) { rs2_free_error(m_error); m_error = nullptr; } }
	inline rs2_error** operator&() { release(); return &m_error; }

	inline bool success() const { return m_error ? false : true; }
	inline const char* get_message() const { return rs2_get_error_message(m_error); }
	inline rs2_exception_type get_type() const { return rs2_get_librealsense_exception_type(m_error); }
private:
	rs2_error* m_error;
};

} // namespace
