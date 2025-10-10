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

#include <vector>
#include <memory>
#include <iostream>
#include "plugnpy/plugnpy.hpp"
#include "pymod/pymodsrc/foobar.hpp"

class FooCaller : public PyPlugin {
public:
    FooCaller(const std::string n, const std::string s) : PyPlugin(n, s) {}

    void load() override {
        this->open_script(this->script);
    }
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
    void callTest4() {
        GilGuard gil;
        std::cout << "callTest4\n";
        FooBar foobar;
        try {
            boost::python::import("my_cpp_module");
            auto func = this->globals.get("test4_foobar");
            boost::python::object py_foobar(boost::ref(foobar));
            
            func(foobar);
            std::cout << "in cpp" << foobar.toStr() << "\n";
    
            func(foobar);
            std::cout << "in cpp" << foobar.toStr() << "\n";
        } catch(const boost::python::error_already_set &) {
            this->status = Status::ERROR;
            std::cout << GetPyErrorString() << "\n";
            throw std::runtime_error("failed to call method");
        }
    }

    void callTest4List(const int amount) {
        GilGuard gil;
        std::cout << "callTest4List\n";
        try {
            boost::python::import("my_cpp_module");

            // get list 
            auto func = this->globals.get("test4_foobar_list");
            boost::python::object result = func(boost::python::object(amount));
            auto r = boost::python::list(result);
            std::vector<std::shared_ptr<FooBar>> fbs;
            for (boost::python::ssize_t i = 0; i < boost::python::len(r); ++i) {
                auto fb = boost::python::extract<std::shared_ptr<FooBar>>(r[i]);
                fbs.push_back(std::move(fb));
            }
            float c = 0.0f;
            for (const auto &it : fbs) {
                c += 0.123f;
                it->a_float = c;
                c = it->a_float;
                std::cout << it->toStr() << "\n";
            }

            // return list
            func = this->globals.get("test4_to_foobar_list");
            func(r);

        } catch(const boost::python::error_already_set &) {
            this->status = Status::ERROR;
            std::cout << GetPyErrorString() << "\n";
            throw std::runtime_error("failed to call method");
        }
    }
};

int main() {
    PyPlugMan pman;

    pman.addPlugin("foo", "foo.py");
    pman.getPlugin("foo")->call_void("test");
    pman.addPluginAs<FooCaller>("foo2", "foo.py");

    auto fc = pman.getAs<FooCaller>("foo2");
    fc->callTest();
    fc->callTest2();
    fc->callTest3();
    fc->callTest4();
    fc->callTest4List(3);

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