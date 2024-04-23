/*
This Code is a copy paste of part of the contents of pybind11/embed.h
It allows for slightly changing the python embedding without changing the pybind11 sourcecode.
*/

#ifndef OPM_EMBED_MODULE
#define OPM_EMBED_MODULE

#ifdef EMBEDDED_PYTHON
#include <pybind11/embed.h>

#define OPM_EMBEDDED_MODULE(name, variable)                                                       \
    static ::pybind11::module_::module_def PYBIND11_CONCAT(pybind11_module_def_, name);           \
    static void PYBIND11_CONCAT(pybind11_init_, name)(::pybind11::module_ &);                     \
    static PyObject PYBIND11_CONCAT(*pybind11_init_wrapper_, name)() {                            \
        auto m = ::pybind11::module_::create_extension_module(                                    \
            PYBIND11_TOSTRING(name), nullptr, &PYBIND11_CONCAT(pybind11_module_def_, name));      \
        try {                                                                                     \
            PYBIND11_CONCAT(pybind11_init_, name)(m);                                             \
            return m.ptr();                                                                       \
        }                                                                                         \
        PYBIND11_CATCH_INIT_EXCEPTIONS                                                            \
    }                                                                                             \
    PYBIND11_EMBEDDED_MODULE_IMPL(name)                                                           \
    Opm::embed::python_module PYBIND11_CONCAT(pybind11_module_, name)(                            \
        PYBIND11_TOSTRING(name), PYBIND11_CONCAT(pybind11_init_impl_, name));                     \
    void PYBIND11_CONCAT(pybind11_init_, name)(::pybind11::module_                                \
                                               & variable) // NOLINT(bugprone-macro-parentheses)

namespace Opm {
namespace embed {

/// Python 2.7/3.x compatible version of `PyImport_AppendInittab` and error checks.
struct python_module {
    using init_t = PyObject *(*)();
    python_module(const char *name, init_t init) {
        if (Py_IsInitialized() != 0) {
            //pybind11_fail("Can't add new modules after the interpreter has been initialized");
        }

        auto result = PyImport_AppendInittab(name, init);
        if (result == -1)
            pybind11::pybind11_fail("Insufficient memory to add a new module");
    }
};

}
}




#endif

#endif
