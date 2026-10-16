#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "arm64_runtime.h"
#include "ppc_cpu.h"
#include "ppc_to_arm64.h"

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
                        "code must be 1..8192 bytes and 4-byte aligned");
        return NULL;
    }
    if (status != ARM64_RUNTIME_OK) {
        PyErr_SetString(PyExc_RuntimeError,
                        "native runtime failed to allocate or release executable memory");
        return NULL;
    }

    return PyLong_FromUnsignedLongLong((unsigned long long)result);
}

static int parse_ppc_words(PyObject *sequence_object,
                           uint32_t instructions[PPC_CPU_MAX_INSTRUCTIONS],
                           Py_ssize_t *count_out) {
    PyObject *sequence = PySequence_Fast(
        sequence_object, "instruction_words must be a sequence of integers");
    if (sequence == NULL) return 0;

    const Py_ssize_t count = PySequence_Fast_GET_SIZE(sequence);
    if (count < 1 || count > (Py_ssize_t)PPC_CPU_MAX_INSTRUCTIONS) {
        Py_DECREF(sequence);
        PyErr_SetString(PyExc_ValueError,
                        "instruction_words must contain 1..256 PowerPC words");
        return 0;
    }

    for (Py_ssize_t i = 0; i < count; ++i) {
        PyObject *item = PySequence_Fast_GET_ITEM(sequence, i);
        unsigned long value = PyLong_AsUnsignedLong(item);
        if (PyErr_Occurred()) {
            Py_DECREF(sequence);
            return 0;
        }
        if (value > UINT32_MAX) {
            Py_DECREF(sequence);
            PyErr_SetString(PyExc_ValueError,
                            "each PowerPC instruction must fit in uint32");
            return 0;
        }
        instructions[i] = (uint32_t)value;
    }

    Py_DECREF(sequence);
    *count_out = count;
    return 1;
}

static int translate_words(const uint32_t *instructions, size_t count,
                           int result_gpr,
                           uint32_t output[PPC_TO_ARM64_MAX_WORDS],
                           size_t *output_count) {
    if (result_gpr < 0 || result_gpr >= 32) {
        PyErr_SetString(PyExc_ValueError,
                        "result_gpr must be between 0 and 31");
        return 0;
    }

    ppc_to_arm64_status status = ppc_translate_to_arm64(
        instructions, count, (unsigned)result_gpr,
        output, PPC_TO_ARM64_MAX_WORDS, output_count);

    if (status == PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION) {
        PyErr_SetString(PyExc_ValueError,
                        "program contains an unsupported PowerPC instruction");
        return 0;
    }
    if (status != PPC_TO_ARM64_OK) {
        PyErr_SetString(PyExc_ValueError,
                        "PowerPC program could not be translated within configured limits");
        return 0;
    }
    return 1;
}

/*
 * Safe, deliberately narrow bridge: callers provide PowerPC instruction
 * words, never ARM64 machine-code bytes. The translator emits only its
 * explicitly supported instruction subset; execution is checked against
 * the reference interpreter before the result is returned.
 */
static PyObject *py_translate_and_execute_ppc(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *sequence_object = NULL;
    int result_gpr = -1;
    if (!PyArg_ParseTuple(args, "Oi:translate_and_execute_ppc",
                          &sequence_object, &result_gpr)) {
        return NULL;
    }

    uint32_t instructions[PPC_CPU_MAX_INSTRUCTIONS];
    Py_ssize_t py_count = 0;
    if (!parse_ppc_words(sequence_object, instructions, &py_count)) {
        return NULL;
    }

    uint32_t arm64_words[PPC_TO_ARM64_MAX_WORDS] = {0};
    size_t arm64_count = 0;
    if (!translate_words(instructions, (size_t)py_count, result_gpr,
                         arm64_words, &arm64_count)) {
        return NULL;
    }

    uint64_t registers[32] = {0};
    ppc_cpu_status reference_status = ppc_cpu_run(
        instructions, (size_t)py_count, registers);
    if (reference_status != PPC_CPU_OK) {
        PyErr_SetString(PyExc_ValueError,
                        "PowerPC reference interpreter rejected the program");
        return NULL;
    }

    uint64_t native_result = 0;
    int runtime_status = arm64_execute_u64(
        (const uint8_t *)arm64_words,
        arm64_count * sizeof(arm64_words[0]),
        &native_result);

    if (runtime_status == ARM64_RUNTIME_UNSUPPORTED_ARCH) {
        PyErr_SetString(PyExc_RuntimeError,
                        "translated execution requires an AArch64 process");
        return NULL;
    }
    if (runtime_status != ARM64_RUNTIME_OK) {
        PyErr_SetString(PyExc_RuntimeError,
                        "native translated-code execution failed");
        return NULL;
    }

    if (native_result != registers[result_gpr]) {
        PyErr_SetString(PyExc_RuntimeError,
                        "translated ARM64 result did not match the PowerPC reference");
        return NULL;
    }

    return PyLong_FromUnsignedLongLong((unsigned long long)native_result);
}

/*
 * Return little-endian ARM64 bytes emitted by the bounded C translator.
 * This is useful for comparing the Python and C encoders; it does not execute
 * the returned bytes.
 */
static PyObject *py_translate_ppc_to_arm64(PyObject *self, PyObject *args) {
    (void)self;
    PyObject *sequence_object = NULL;
    int result_gpr = -1;
    if (!PyArg_ParseTuple(args, "Oi:translate_ppc_to_arm64",
                          &sequence_object, &result_gpr)) {
        return NULL;
    }

    uint32_t instructions[PPC_CPU_MAX_INSTRUCTIONS];
    Py_ssize_t py_count = 0;
    if (!parse_ppc_words(sequence_object, instructions, &py_count)) {
        return NULL;
    }

    uint32_t arm64_words[PPC_TO_ARM64_MAX_WORDS] = {0};
    size_t arm64_count = 0;
    if (!translate_words(instructions, (size_t)py_count, result_gpr,
                         arm64_words, &arm64_count)) {
        return NULL;
    }

    uint8_t machine_code[PPC_TO_ARM64_MAX_WORDS * 4u];
    for (size_t i = 0; i < arm64_count; ++i) {
        machine_code[i * 4u] = (uint8_t)(arm64_words[i] & 0xffu);
        machine_code[i * 4u + 1u] = (uint8_t)((arm64_words[i] >> 8) & 0xffu);
        machine_code[i * 4u + 2u] = (uint8_t)((arm64_words[i] >> 16) & 0xffu);
        machine_code[i * 4u + 3u] = (uint8_t)((arm64_words[i] >> 24) & 0xffu);
    }

    return PyBytes_FromStringAndSize(
        (const char *)machine_code, (Py_ssize_t)(arm64_count * 4u));
}

static PyMethodDef methods[] = {
    {"execute_u64", py_execute_u64, METH_VARARGS,
     "Execute trusted AArch64 machine-code bytes with no arguments; return x0."},
    {"translate_and_execute_ppc", py_translate_and_execute_ppc, METH_VARARGS,
     "Translate bounded PowerPC words to ARM64, execute on AArch64, and compare with the reference interpreter."},
    {"translate_ppc_to_arm64", py_translate_ppc_to_arm64, METH_VARARGS,
     "Translate bounded PowerPC words to little-endian ARM64 bytes without executing them."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    "ps3_native",
    "Experimental native ARM64 runtime and bounded PowerPC translator.",
    -1,
    methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit_ps3_native(void) {
    return PyModule_Create(&module);
}
