#include "bpmod.hpp"

#include <memory>
#include <Python.h>
#define BOOST_NO_AUTO_PTR
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#include <boost/python.hpp>
#pragma GCC diagnostic pop

#include "pymodsrc/foobar.hpp"

// template <class T>
// std::vector<T> py_iterable_to_vector(const boost::python::object& iterable) {
//     std::vector<T> v;
//     for (boost::python::stl_input_iterator<boost::python::object> it(iterable), end; it != end; ++it) {
//         v.push_back(boost::python::extract<T>(*it));
//     }
//     return v;
// }

static std::shared_ptr<FooBar> make_foobar(
    int a_int = 0,
    float a_float = 0.0f,
    double a_double = 0.0,
    bool a_bool = false,
    std::string a_string = std::string()
) {
    auto p = std::make_shared<FooBar>();
    p->a_int    = a_int;
    p->a_float  = a_float;
    p->a_double = a_double;
    p->a_bool   = a_bool;
    p->a_string = std::move(a_string);
    return p;
}

BOOST_PYTHON_MODULE(my_cpp_module) {
    using namespace boost::python;
    class_<FooBar, std::shared_ptr<FooBar> >("FooBar", init<>())
        .def_readwrite("a_int", &FooBar::a_int)
        .def_readwrite("a_float", &FooBar::a_float)
        .def_readwrite("a_double", &FooBar::a_double)
        .def_readwrite("a_bool", &FooBar::a_bool)
        .def_readwrite("a_string", &FooBar::a_string)
        .def("toStr", &FooBar::toStr)
        .def("__init__", make_constructor(
            &make_foobar,
            default_call_policies(),
            ( arg("a_int")    = 0,
              arg("a_float")  = 0.0f,
              arg("a_double") = 0.0,
              arg("a_bool")   = false,
              arg("a_string") = std::string() )
        ))
    ;
}

void init_bpmod() {
    if (PyImport_AppendInittab("my_cpp_module", &PyInit_my_cpp_module) == -1) {
        throw std::runtime_error(
            "Failed to append 'my_cpp_module' to the built-in modules table"
        );
    }
}