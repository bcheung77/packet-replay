
#include <filesystem>
#include <stdexcept>
#include <string>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "python_api.h"

namespace packet_replay
{
    PythonApi* PythonApi::instance__ = nullptr;;

    PythonApi* PythonApi::getInstance() {
        if (instance__ == nullptr) {
            instance__ = new PythonApi();
        }

        return instance__;
    }

    PythonApi::PythonApi() {
        Py_Initialize();
    }

    PythonApi::~PythonApi() {
        Py_Finalize();
    }

    PythonCall::PythonCall(const std::string& script_file, const std::string& func_name) {
        std::filesystem::path path(script_file);

        std::string sys_path_cmd("sys.path.append(\"");

        std::string parent_path = path.parent_path();

        sys_path_cmd.append(parent_path.empty() ? "." : parent_path.c_str()).append("\")");

        PyRun_SimpleString("import sys");
        PyRun_SimpleString(sys_path_cmd.c_str());

        if (path.extension() != ".py") {
            throw std::runtime_error("python file must end with .py");
        }
        PyObject* py_module_name = PyUnicode_FromString(path.stem().c_str());
        PyObject* py_module = PyImport_Import(py_module_name);

        if (py_module == nullptr) {
            PyErr_Print();
            throw std::runtime_error("importing python module failed");
        }

        func_name_ = func_name;
        py_func_ = PyObject_GetAttrString(py_module, func_name.c_str());

        Py_DECREF(py_module);
        Py_DECREF(py_module_name);
    }

    PythonCall::~PythonCall() {
        if (py_func_ != nullptr) {
            Py_DECREF(py_func_);
        }
    }

    bool ValidatePythonCall::validate(const uint8_t* expected, int expected_len, const uint8_t* actual, int actual_len) {
        PyObject* args = Py_BuildValue("(y#y#)", expected, expected_len, actual, actual_len);

        if (args == nullptr) {
            PyErr_Print();
            throw std::runtime_error("building python arguments failed");
        }

        PyObject* result = PyObject_CallObject(py_func_, args);

        if (result == nullptr) {
            PyErr_Print();
            throw std::runtime_error("caling python function failed");
        }

        Py_DECREF(args);

        if (!PyBool_Check(result)) {
            std::string err = "python function ";
            err.append(func_name_).append(" must return bool");
            throw std::runtime_error(err);
        }

        bool ret = Py_True == result;
        Py_DECREF(args);
        Py_DECREF(result);

        return ret;
    }    
} // namespace packet_replay

