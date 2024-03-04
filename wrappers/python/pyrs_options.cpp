// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "pyrealsense2.h"
#include <librealsense2/hpp/rs_options.hpp>

using rsutils::json;


void init_options(py::module &m) {
    /** rs_options.hpp **/

    // Expose option values as a custom struct rather than 'rs2_option_value*'
    struct option_value
    {
        rs2_option id;
        rs2_option_type type;
        py::object value;

        option_value( rs2::option_value const & value_ )
            : id( value_->id )
            , type( value_->type )
        {
            if( ! value_->is_valid )
                value = py::cast< py::none >( Py_None );
            else if( RS2_OPTION_TYPE_FLOAT == value_->type )
                value = py::float_( value_->as_float );
            else if( RS2_OPTION_TYPE_STRING == value_->type )
                value = py::str( value_->as_string );
            else if( RS2_OPTION_TYPE_INTEGER == value_->type )
                value = py::int_( value_->as_integer );
            else if( RS2_OPTION_TYPE_BOOLEAN == value_->type )
                value = py::bool_( value_->as_integer );
            else
                value = py::cast< py::none >( Py_None );
        }
    };
    py::class_< option_value >( m, "option_value" )
        .def_readwrite( "id", &option_value::id )
        .def_readwrite( "value", &option_value::value )  // None if no value available
        .def_readwrite( "type", &option_value::type )
        .def( "__repr__",
              []( option_value const & self )
              {
                  std::ostringstream os;
                  os << '<' << rs2_option_to_string( self.id );
                  os << ' ' << py::str( self.value );
                  os << '>';
                  return os.str();
              } );
    // given an iterator, return an option_value
    struct option_value_from_iterator
    {
        option_value operator()( rs2::options_list::iterator const & it ) { return option_value( *it ); }
    };
    py::class_< rs2::options_list >( m, "options_list" )
        .def( "__getitem__",
              []( const rs2::options_list & self, size_t i )
              {
                  if( i >= self.size() )
                      throw py::index_error();
                  return option_value( self[i] );
              } )
        .def( "__len__", &rs2::options_list::size )
        .def( "__iter__",
              []( const rs2::options_list & self )
              {
                  return py::detail::make_iterator_impl< option_value_from_iterator,   // how to access value from iterator
                                                         py::return_value_policy::reference_internal,  // pybind default
                                                         rs2::options_list::iterator,  // iterator type
                                                         rs2::options_list::iterator,  // sentinel (end) type
                                                         option_value                  // value type
                                                         >( self.begin(), self.end() );
              },
              py::keep_alive< 0, 1 >() );

    py::class_<rs2::options> options(m, "options", "Base class for options interface. Should be used via sensor or processing_block."); // No docstring in C++
    options.def("is_option_read_only", &rs2::options::is_option_read_only, "Check if particular option "
                "is read only.", "option"_a)
        .def("get_option", &rs2::options::get_option, "Read option value from the device.", "option"_a, py::call_guard<py::gil_scoped_release>())
        .def( "get_option_value",
              []( rs2::options const & self, rs2_option option_id ) -> option_value
                  { return self.get_option_value( option_id ); } )
        .def( "set_option_value",
              []( rs2::options const & self, rs2_option option_id, json value )
              {
                  rs2::option_value rs2_value;
                  switch( value.type() )
                  {
                  case json::value_t::null:
                      rs2_value = rs2::option_value( option_id, rs2::option_value::invalid );
                      break;

                  case json::value_t::string:
                      rs2_value = rs2::option_value( option_id, value.string_ref().c_str() );
                      break;

                  case json::value_t::number_float:
                      rs2_value = rs2::option_value( option_id, value.get< float >() );
                      break;

                  case json::value_t::number_unsigned:
                  case json::value_t::number_integer:
                      rs2_value = rs2::option_value( option_id, value.get< int64_t >() );
                      break;

                  case json::value_t::boolean:
                      rs2_value = rs2::option_value( option_id, value.get< bool >() );
                      break;

                  default:
                      throw std::runtime_error( "invalid value type: " + value.dump() );
                  }
                  self.set_option_value( rs2_value );
              } )
        .def("get_option_range", &rs2::options::get_option_range, "Retrieve the available range of values "
             "of a supported option", "option"_a, py::call_guard<py::gil_scoped_release>())
        .def("set_option", &rs2::options::set_option, "Write new value to device option", "option"_a, "value"_a, py::call_guard<py::gil_scoped_release>())
        .def("supports", (bool (rs2::options::*)(rs2_option option) const) &rs2::options::supports, "Check if particular "
             "option is supported by a subdevice", "option"_a)
        .def("get_option_description", &rs2::options::get_option_description, "Get option description.", "option"_a)
        .def("get_option_value_description", &rs2::options::get_option_value_description, "Get option value description "
             "(In case a specific option value holds special meaning)", "option"_a, "value"_a)
        .def("get_supported_options", &rs2::options::get_supported_options, "Retrieve list of supported options") // No docstring in C++
        .def( "on_options_changed", &rs2::options::on_options_changed,
              "Sets a callback to notify in case options in this container change value", "callback"_a );

    /** end rs_options.hpp **/
}
