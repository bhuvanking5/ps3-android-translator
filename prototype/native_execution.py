"""Python entry point for the bounded native PowerPC-to-ARM64 prototype.

The native extension validates each PowerPC instruction, translates only the
implemented subset, executes generated ARM64 on AArch64, and compares the
result against the reference interpreter. This is not a PS3 game runner.
"""


def execute_ppc_program_native(instruction_words, result_gpr):
    """Run a tiny PowerPC instruction sequence through the native C bridge.

    Args:
        instruction_words: Sequence of 32-bit PowerPC instruction words.
        result_gpr: PowerPC general-purpose register (0..17) to return.

    Returns:
        The matching unsigned 64-bit result from native ARM64 execution.

    Raises:
        RuntimeError: If the extension is not built or the process is not
            running on AArch64.
        ValueError: If an instruction, register, or program size is unsupported.
    """
    try:
        import ps3_native
    except ImportError as exc:
        raise RuntimeError(
            "native extension is not built; configure CMake with "
            "PS3_TRANSLATOR_BUILD_PYTHON_BINDING=ON"
        ) from exc

    return ps3_native.translate_and_execute_ppc(instruction_words, result_gpr)
