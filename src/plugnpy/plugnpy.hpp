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

struct GilGuard {
    PyGILState_STATE _state;
    GilGuard() : _state(PyGILState_Ensure()) {}
    ~GilGuard() {PyGILState_Release(this->_state); }
};

struct PyVmMan {

    PyVmMan(const bool start = false) {
        if (start) { this->init(); }
    }
    ~PyVmMan() { this->finalize(); }
    void init() {
        if (Py_IsInitialized()) return;
        Py_Initialize();
    }

    void finalize() {
        if (!Py_IsInitialized()) return;
        {
            GilGuard gill;
            PyGC_Collect();
            PyGC_Collect();
        }
        Py_FinalizeEx();
    }
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
                this->main = boost::python::import("__main__");
                this->globals = boost::python::dict(this->main.attr("__dict__"));
                boost::python::exec_file(this->script.c_str(), this->globals, this->globals);
                this->status = Status::RUNNING;
            }
            catch(const boost::python::error_already_set &) {
                this->status = Status::ERROR;
                std::cout << GetPyErrorString() << "\n";
            }
        }

        template<typename ... Args>
        void call_void(const char *name, Args&& ... args) {
            if (!Py_IsInitialized() || this->status != Status::RUNNING) return;
            GilGuard gil;
            try {
                boost::python::object func = this->globals.get(name);
                func(std::forward<Args>(args)...);
            }
            catch(const boost::python::error_already_set &) {
                this->status = Status::ERROR;
                std::cout << GetPyErrorString() << "\n";
            }
        }

        template<typename T, typename ... Args>
        T call(const char *name, Args&& ... args) {
            // TODO: need better way to handel fail state
            // if (!Py_IsInitialized() || this->status != Status::RUNNING)
            GilGuard gil;
            try {
                boost::python::object func = this->globals.get(name);
                boost::python::object r = func(std::forward<Args>(args)...);
                return boost::python::extract<T>(r);
            }
            catch(const boost::python::error_already_set &) {
                this->status = Status::ERROR;
                std::cout << GetPyErrorString() << "\n";
                throw std::runtime_error("failed to get method");
            }
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