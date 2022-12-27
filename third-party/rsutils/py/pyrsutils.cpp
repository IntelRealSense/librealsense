// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <rsutils/py/pybind11.h>
#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/string/split.h>
#include <rsutils/string/from.h>
#include <rsutils/version.h>
#include <rsutils/number/running-average.h>
#include <rsutils/number/stabilized-value.h>


#define NAME pyrsutils
#define SNAME "pyrsutils"


PYBIND11_MODULE(NAME, m) {
    m.doc() = R"pbdoc(
        RealSense Utilities Python Bindings
    )pbdoc";
    m.attr( "__version__" ) = "0.1";  // RS2_API_VERSION_STR;

    rsutils::configure_elpp_logger();

    m.def( "debug",
           &rsutils::configure_elpp_logger,
           py::arg( "enable" ),
           py::arg( "nested-string" ) = "",
           py::arg( "logger" ) = LIBREALSENSE_ELPP_ID );

    m.def( "split", &rsutils::string::split );

    using rsutils::version;
    py::class_< version >( m, "version" )
        .def( py::init<>() )
        .def( py::init< std::string const & >() )
        .def( py::init< version::sub_type, version::sub_type, version::sub_type, version::sub_type >(),
              py::arg( "major" ),
              py::arg( "minor" ),
              py::arg( "patch" ),
              py::arg( "build" ) = 0 )
        .def_static( "from_number", []( version::number_type n ) { return version( n ); } )
        .def( "is_valid", &version::is_valid )
        .def( "__bool__", &version::is_valid )
        .def( "major", &version::get_major )
        .def( "minor", &version::get_minor )
        .def( "patch", &version::get_patch )
        .def( "build", &version::get_build )
        .def( "without_build", &version::without_build )
        .def( "to_string", &version::to_string )
        .def( "__str__", &version::to_string )
        .def( "__repr__",
              []( version const & self ) {
                  std::ostringstream os;
                  os << "<" SNAME ".version";
                  if( self.is_valid() )
                      os << " " << self.to_string();
                  os << ">";
                  return os.str();
              } )
        .def_readwrite( "number", &version::number )
        .def( py::self < py::self )
        .def( py::self <= py::self )
        .def( py::self == py::self )
        .def( py::self != py::self )
        .def( py::self >= py::self )
        .def( py::self > py::self )
        .def( "is_between", &version::is_between );

    using int_avg = rsutils::number::running_average< int64_t >;
    py::class_< int_avg >( m, "running_average_i" )
        .def( py::init<>() )
        .def( "__nonzero__", &int_avg::size )  // Called to implement truth value testing in Python 2
        .def( "__bool__", &int_avg::size )     // Called to implement truth value testing in Python 3
        .def( "size", &int_avg::size )
        .def( "get", &int_avg::get )
        .def( "leftover", &int_avg::leftover )
        .def( "fraction", &int_avg::fraction )
        .def( "get_double", &int_avg::get_double )
        .def( "__int__", &int_avg::get )
        .def( "__float__", &int_avg::get_double )
        .def( "__str__", []( int_avg const & self ) -> std::string { return rsutils::string::from( self.get_double() ); } )
        .def( "__repr__",
              []( int_avg const & self ) -> std::string {
                  return rsutils::string::from() << "<" SNAME ".running_average<int64_t>"
                                                 << " " << self.get() << " "
                                                 << ( self.leftover() < 0 ? "" : "+" ) << self.leftover()
                                                 << "/" << self.size() << ">";
              } )
        .def( "add", &int_avg::add );

    using double_avg = rsutils::number::running_average< double >;
    py::class_< double_avg >( m, "running_average" )
        .def( py::init<>() )
        .def( "__nonzero__", &double_avg::size )  // Called to implement truth value testing in Python 2
        .def( "__bool__", &double_avg::size )     // Called to implement truth value testing in Python 3
        .def( "size", &double_avg::size )
        .def( "get", &double_avg::get )
        .def( "__float__", &double_avg::get )
        .def( "__str__", []( double_avg const & self ) -> std::string { return rsutils::string::from( self.get() ); } )
        .def( "__repr__",
              []( double_avg const & self ) -> std::string {
                  return rsutils::string::from() << "<" SNAME ".running_average<double>"
                                                 << " " << self.get() << " /" << self.size() << ">";
              } )
        .def( "add", &double_avg::add );

    using stabilized_value = rsutils::number::stabilized_value< double >;
    auto not_empty = []( stabilized_value const & self ) {
        return ! self.empty();
    };
    auto to_string = []( stabilized_value const & self ) -> std::string {
        if( self.empty() )
            return "EMPTY";
        return rsutils::string::from( self.get() );
    };
    py::class_< stabilized_value >( m, "stabilized_value" )
        .def( py::init< size_t >() )
        .def( "empty", &stabilized_value::empty )
        .def( "__bool__", not_empty )
        .def( "add", &stabilized_value::add )
        .def( "get", &stabilized_value::get, py::arg( "stabilization-percent" ) = 0.75 )
        .def( "clear", &stabilized_value::clear )
        .def( "to_string", to_string )
        .def( "__str__", to_string );
}
