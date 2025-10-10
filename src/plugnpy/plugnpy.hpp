#pragma once

#include <string>
#include <memory>
#include <iostream>
#include <map>

#include <Python.h>
#define BOOST_NO_AUTO_PTR
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#include <boost/python.hpp>
#pragma GCC diagnostic pop

#include "pymod/bpmod.hpp"


struct GilGuard {
    PyGILState_STATE _state;
    GilGuard() : _state(PyGILState_Ensure()) {}
    ~GilGuard() {PyGILState_Release(this->_state); }
};


class PyVmMan {
    PyThreadState* _main_tstate{nullptr};
public:
    void init() {
        if (Py_IsInitialized()) return;
        init_bpmod();
        Py_Initialize();
        // TODO: setup common libs?
        PyEval_InitThreads();
        _main_tstate = PyEval_SaveThread();
    }

    void finalize() {
        if (!Py_IsInitialized()) return;
        PyEval_RestoreThread(_main_tstate);
        _main_tstate = nullptr;

        PyGC_Collect();
        PyGC_Collect();
        Py_FinalizeEx();
    }
    PyVmMan(const bool start_python = false) { 
        if (start_python) { init(); }
    }
    ~PyVmMan() { finalize(); }
};


inline std::string GetPyErrorString() {
    // basically PyErr_Print()
    GilGuard gil;
    PyObject* ptype = nullptr;
    PyObject* pvalue = nullptr;
    PyObject* ptraceback = nullptr;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
    if (ptraceback != nullptr) {
        PyException_SetTraceback(pvalue, ptraceback);
    }
    boost::python::handle<> htype(ptype);
    boost::python::handle<> hvalue(boost::python::allow_null(pvalue));
    boost::python::handle<> htraceback(boost::python::allow_null(ptraceback));
    boost::python::object traceback = boost::python::import("traceback");
    boost::python::object format_exception = traceback.attr("format_exception");
    boost::python::object formatted_list = format_exception(htype, hvalue, htraceback);
    boost::python::object formatted = boost::python::str("\n").join(formatted_list);
    std::string s = boost::python::extract<std::string>(formatted);
    return s;
}

class PyPlugin : public std::enable_shared_from_this<PyPlugin> {
    protected:
        std::string name;
        std::string script;
        boost::python::object main;
        boost::python::dict globals;
    protected:
        enum class Status {UNINIT, RUNNING, ERROR};
        Status status { Status::UNINIT };
    public:
        std::string getName() const {
            return this->name;
        }
        bool isError() const {
            return this->status == Status::ERROR;
        }
    public:
        PyPlugin(const std::string &name, const std::string &script)
            : name(name), script(script)
        {}

        ~PyPlugin() {
            if (Py_IsInitialized()) {
                GilGuard gil;
                this->globals.clear();
                this->main = boost::python::object();
            }
        }

        virtual void load() {
            this->open_script(this->script);
        }

        void open_script(const std::string &script) {
            if (!Py_IsInitialized() || this->status == Status::ERROR) return;
            GilGuard gil;
            this->script = script;
            try {
                // create a unique module for this plugin
                boost::python::object importlib = boost::python::import("importlib");
                boost::python::object types = boost::python::import("types");
                auto modname = boost::python::str("plugin_" + name);

                // new blank module
                boost::python::object module = types.attr("ModuleType")(modname);

                // provide builtins so exec has a proper environment
                module.attr("__dict__")["__builtins__"] = boost::python::import("builtins");

                this->globals = boost::python::extract<boost::python::dict>(module.attr("__dict__"));
                // make it importable by its name (optional but handy)
                boost::python::import("sys").attr("modules")[modname] = module;

                // ensure script dir is on sys.path so relative imports work
                boost::python::object sys = boost::python::import("sys");
                sys.attr("path").attr("insert")(0, boost::python::str("."));

                boost::python::exec_file(this->script.c_str(), this->globals, this->globals);
                
                this->main = module;
                this->status = Status::RUNNING;
            }
            catch(const boost::python::error_already_set &) {
                this->status = Status::ERROR;
                std::cout << GetPyErrorString() << "\n";
            }
        }

        inline boost::python::object get_callable(const char *name) {
            auto obj = this->globals.get(name);
            if (obj.is_none())
                throw std::runtime_error(std::string("Python function '") + name + "' not found");
            // PyCallable_Check
            if (!PyCallable_Check(obj.ptr()))
                throw std::runtime_error(std::string("'") + name + "' is not callable");
            return obj;
        }

        template<typename ... Args>
        void call_void(const char *name, Args&& ... args) {
            if (!Py_IsInitialized() || this->status != Status::RUNNING) return;
            GilGuard gil;
            try {
                auto func = get_callable(name);
                func(std::forward<Args>(args)...);
            } catch (const boost::python::error_already_set&) {
                this->status = Status::ERROR;
                std::cout << GetPyErrorString() << "\n";
            } catch (const std::exception& ex) {
                this->status = Status::ERROR;
                std::cout << "C++ exception: " << ex.what() << "\n";
            }
        }

        template<typename T, typename ... Args>
        T call(const char *name, Args&& ... args) {
            GilGuard gil;
            try {
                auto func = get_callable(name);
                boost::python::object r = func(std::forward<Args>(args)...);
                boost::python::extract<T> conv(r);
                if (!conv.check())
                    throw std::runtime_error(std::string("Type conversion failed calling '") + name + "'");
                return conv();
            } catch (const boost::python::error_already_set&) {
                this->status = Status::ERROR;
                std::cout << GetPyErrorString() << "\n";
                throw std::runtime_error("failed to call method");
            }
        }

        template<typename T>
        void set_global(const char* key, const T& v) {
            GilGuard gil;
            this->globals[key] = v;
        }

};

class PyPlugMan {
    private:
        PyVmMan _vm;
        std::map<std::string, std::unique_ptr<PyPlugin>> plugins;
    public:
        PyPlugMan() {};
        ~PyPlugMan() {
            this->plugins.clear();
            this->_vm.finalize();
        };
        void addPlugin(const std::string &name, const std::string &script) {
            this->_vm.init();
            auto ins = this->plugins.emplace(name, std::make_unique<PyPlugin>(
                name, script
            ));
            auto &p = *ins.first->second;
            p.load();
        }

        std::unique_ptr<PyPlugin> &getPlugin(const std::string &name) {
            return this->plugins.at(name);
        }
        
        std::map<std::string, std::unique_ptr<PyPlugin>> &getPlugins() {
            return this->plugins;
        }

        template<class T, class... Args>
        void addPluginAs(const std::string& name, Args&&... args) {
            _vm.init();
            auto up = std::make_unique<T>(name, std::forward<Args>(args)...);
            up->load();
            plugins.emplace(name, std::move(up));
        }

        template<class T>
        T* getAs(const std::string& name) {
            if (auto it = plugins.find(name); it != plugins.end())
                return dynamic_cast<T*>(it->second.get());
            return nullptr;
        }
};