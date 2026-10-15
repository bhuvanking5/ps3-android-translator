#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "arm64_runtime.h"

#include <stdint.h>
#include <stddef.h>

static PyObject *py_execute_u64(PyObject *self, PyObject *args) {
    (void)self;

    Py_buffer view;
    if (!PyArg_ParseTuple(args, "y*:execute_u64", &view)) {
        return NULL;
    }

    uint64_t result = 0;
    int status = arm64_execute_u64(
        (const uint8_t *)view.buf,
        (size_t)view.len,
        &result
    );
    PyBuffer_Release(&view);

    if (status == ARM64_RUNTIME_UNSUPPORTED_ARCH) {
        PyErr_SetString(PyExc_RuntimeError,
                        "native ARM64 execution requires an AArch64 process");
        return NULL;
    }
    if (status == ARM64_RUNTIME_INVALID_ARGUMENT) {
        PyErr_SetString(PyExc_ValueError,
                        "code must be 1..4096 bytes and 4-byte aligned");
        return NULL;
    }
    if (status != ARM64_RUNTIME_OK) {
        PyErr_SetString(PyExc_RuntimeError,
                        "native runtime failed to allocate or release executable memory");
        return NULL;
    }

    return PyLong_FromUnsignedLongLong((unsigned long long)result);
}

static PyMethodDef methods[] = {
    {"execute_u64", py_execute_u64, METH_VARARGS,
     "Execute a trusted AArch64 function with no arguments; return x0."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    "ps3_native",
    "Experimental native ARM64 runtime binding.",
    -1,
    methods
};

PyMODINIT_FUNC PyInit_ps3_native(void) {
    return PyModule_Create(&module);
}
