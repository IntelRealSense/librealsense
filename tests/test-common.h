#include "catch/catch.hpp"
#include <librealsense/rs.h>

#include <thread> // For std::this_thread::sleep_for

// RAII wrapper to ensure that contexts are always cleaned up. If this is not done, subsequent 
// contexts may not enumerate devices correctly. Long term, we should probably try to remove 
// this requirement, or at least throw an error if the user attempts to open multiple contexts at once.
class safe_context
{
    rs_context * context;
    safe_context(int) : context() {}
public:
    safe_context() : safe_context(1) 
    {
        rs_error * error = nullptr;
        context = rs_create_context(RS_API_VERSION, &error);  
        REQUIRE(error == nullptr);
        REQUIRE(context != nullptr);
    }

    ~safe_context()
    {
        if(context)
        {
            rs_delete_context(context, nullptr);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    operator rs_context * () const { return context; }
};

#ifdef WIN32
#define NOEXCEPT_FALSE
#else
#define NOEXCEPT_FALSE noexcept(false)
#endif

class require_error
{
    std::string message;
    rs_error * err;
public:
    require_error(std::string message) : message(move(message)), err() {}
    require_error(const require_error &) = delete;
    ~require_error() NOEXCEPT_FALSE
    {
        if(std::uncaught_exception()) return;
        REQUIRE(err != nullptr);
        REQUIRE(rs_get_error_message(err) == std::string(message));
    }
    require_error &  operator = (const require_error &) = delete;
    operator rs_error ** () { return &err; }
};

class require_no_error
{
    rs_error * err;
public:
    require_no_error() : err() {}
    require_no_error(const require_error &) = delete;
    ~require_no_error() NOEXCEPT_FALSE 
    { 
        if(std::uncaught_exception()) return;
        REQUIRE(rs_get_error_message(err) == nullptr);        
        REQUIRE(err == nullptr);
    }
    require_no_error &  operator = (const require_no_error &) = delete;
    operator rs_error ** () { return &err; }
};

inline float dot_product(const float (& a)[3], const float (& b)[3])
{ 
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; 
}

inline void require_cross_product(const float (& r)[3], const float (& a)[3], const float (& b)[3])
{
    REQUIRE( r[0] == Approx(a[1]*b[2] - a[2]*b[1]) );
    REQUIRE( r[1] == Approx(a[2]*b[0] - a[0]*b[2]) );
    REQUIRE( r[2] == Approx(a[0]*b[1] - a[1]*b[0]) );
}

inline void require_rotation_matrix(const float (& matrix)[9])
{
    const float row0[] = {matrix[0], matrix[3], matrix[6]};
    const float row1[] = {matrix[1], matrix[4], matrix[7]};
    const float row2[] = {matrix[2], matrix[5], matrix[8]};
    REQUIRE( dot_product(row0, row0) == Approx(1) );
    REQUIRE( dot_product(row1, row1) == Approx(1) );
    REQUIRE( dot_product(row2, row2) == Approx(1) );
    REQUIRE( dot_product(row0, row1) == Approx(0) );
    REQUIRE( dot_product(row1, row2) == Approx(0) );
    REQUIRE( dot_product(row2, row0) == Approx(0) );
    require_cross_product(row0, row1, row2);
    require_cross_product(row0, row1, row2);
    require_cross_product(row0, row1, row2); 
}

inline void require_identity_matrix(const float (& matrix)[9])
{
    static const float identity_matrix_3x3[] = {1,0,0, 0,1,0, 0,0,1};
    for(int i=0; i<9; ++i) REQUIRE( matrix[i] == identity_matrix_3x3[i] );
}