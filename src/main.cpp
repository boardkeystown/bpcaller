#include <iostream>

#include <Python.h>
#define BOOST_NO_AUTO_PTR
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#include <boost/python.hpp>
#pragma GCC diagnostic pop

void basic_test() {
    Py_Initialize();
    {
        boost::python::object main = boost::python::import("__main__");
        boost::python::dict globals = boost::python::dict(main.attr("__dict__"));
        // boost::python::exec("print('hello from python')", globals, globals);
        boost::python::exec_file("foo.py", globals, globals);
        globals.clear();
        main = boost::python::object();
    }
    Py_FinalizeEx();
}


#include <iostream>
#include "plugnpy/plugnpy.hpp"

class FooCaller : public PyPlugin {
public:
    FooCaller(const std::string n, const std::string s) : PyPlugin(n, s) {}
    void callTest() {
        this->call_void("test");
    }
    void callTest2() {
        this->call_void("test2", "hello from cpp");
        this->call_void("test2", 124.123);
        this->call_void("test2", 123);
        this->call_void("test2", true);
    }
    void callTest3() {
        std::cout << "callTest3\n"
            << "test3_str: " << this->call<std::string>("test3_str") << '\n'
            << "test3_float: " << this->call<float>("test3_float") << '\n'
            << "test3_int: " << this->call<int>("test3_int") << '\n'
            << "test3_bool: " << this->call<bool>("test3_bool") << '\n';
    }
};

int main() {
    std::cout << "cringe\n";

    PyPlugMan pman;

    pman.addPlugin("foo", "foo.py");
    pman.getPlugin("foo")->call_void("test");
    pman.addPluginAs<FooCaller>("foo2", "foo.py");

    auto fc = pman.getAs<FooCaller>("foo2");
    fc->callTest();
    fc->callTest2();
    fc->callTest3();

    // pman.getPlugins().emplace("foo2", std::make_unique<FooCaller>("foo2", "foo.py"));
    // auto& pbase = pman.getPlugin("foo2");
    // pbase->load();
    // if (auto fc2 = dynamic_cast<FooCaller*>(pbase.get())) {
    //     fc2->callTest();
    //     fc2->callTest();
    //     fc2->callTest();
    // }

    return 0;
}