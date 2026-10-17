cmake_minimum_required(VERSION 3.16)
project(ps3_android_translator_native C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

add_library(ps3_arm64_runtime SHARED native/arm64_runtime.c)
target_include_directories(ps3_arm64_runtime PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/native")
target_compile_options(ps3_arm64_runtime PRIVATE -Wall -Wextra -Werror)

add_executable(test_arm64_runtime native/test_arm64_runtime.c)
target_link_libraries(test_arm64_runtime PRIVATE ps3_arm64_runtime)
target_compile_options(test_arm64_runtime PRIVATE -Wall -Wextra -Werror)

enable_testing()
add_test(NAME native_runtime_smoke COMMAND test_arm64_runtime)

add_executable(test_ppc_to_arm64
  tests/test_ppc_to_arm64.c
  native/ppc_to_arm64.c
)
target_include_directories(test_ppc_to_arm64 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/native")
target_compile_options(test_ppc_to_arm64 PRIVATE -Wall -Wextra -Werror)
add_test(NAME ppc_to_arm64_translation COMMAND test_ppc_to_arm64)

add_executable(test_ppc_to_arm64_execution
  tests/test_ppc_to_arm64_execution.c
  native/ppc_to_arm64.c
  native/arm64_runtime.c
  android/app/src/main/cpp/ppc_cpu.c
)
target_include_directories(test_ppc_to_arm64_execution PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/native"
  "${CMAKE_CURRENT_SOURCE_DIR}/android/app/src/main/cpp"
)
target_compile_options(test_ppc_to_arm64_execution PRIVATE -Wall -Wextra -Werror)
add_test(NAME ppc_to_arm64_execution COMMAND test_ppc_to_arm64_execution)

option(PS3_TRANSLATOR_BUILD_PYTHON_BINDING "Build the optional CPython extension" OFF)
if(PS3_TRANSLATOR_BUILD_PYTHON_BINDING)
  find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
  Python3_add_library(ps3_native MODULE
    native/python_binding.c
    native/ppc_to_arm64.c
    android/app/src/main/cpp/ppc_cpu.c
    WITH_SOABI
  )
  target_link_libraries(ps3_native PRIVATE ps3_arm64_runtime)
  target_include_directories(ps3_native PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/native"
    "${CMAKE_CURRENT_SOURCE_DIR}/android/app/src/main/cpp"
  )
  target_compile_options(ps3_native PRIVATE -Wall -Wextra -Werror)
endif()

if(PS3_TRANSLATOR_BUILD_PYTHON_BINDING)
  add_test(
    NAME python_native_binding_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_binding.py
  )
  set_tests_properties(
    python_native_binding_smoke
    PROPERTIES ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:ps3_native>"
  )
endif()

add_executable(test_ppc_cpu
  tests/test_ppc_cpu.c
  android/app/src/main/cpp/ppc_cpu.c
)
target_compile_options(test_ppc_cpu PRIVATE -Wall -Wextra -Werror)
add_test(NAME ppc_cpu_subset COMMAND test_ppc_cpu)

add_executable(test_controller_input
  tests/test_controller_input.c
  android/app/src/main/cpp/controller_input.c
)
target_include_directories(test_controller_input PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/android/app/src/main/cpp"
)
target_compile_options(test_controller_input PRIVATE -Wall -Wextra -Werror)
add_test(NAME native_controller_input COMMAND test_controller_input)

add_executable(test_prototype_runtime
  tests/test_prototype_runtime.c
  android/app/src/main/cpp/prototype_runtime.c
  android/app/src/main/cpp/prototype_guest_io.c
  android/app/src/main/cpp/controller_input.c
  android/app/src/main/cpp/ppc_cpu.c
)
target_include_directories(test_prototype_runtime PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/android/app/src/main/cpp"
)
target_compile_options(test_prototype_runtime PRIVATE -Wall -Wextra -Werror)
add_test(NAME prototype_runtime_integration COMMAND test_prototype_runtime)

add_executable(test_prototype_guest_io
  tests/test_prototype_guest_io.c
  android/app/src/main/cpp/prototype_guest_io.c
  android/app/src/main/cpp/controller_input.c
)
target_include_directories(test_prototype_guest_io PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/android/app/src/main/cpp"
)
target_compile_options(test_prototype_guest_io PRIVATE -Wall -Wextra -Werror)
add_test(NAME prototype_guest_io COMMAND test_prototype_guest_io)

add_executable(test_guest_memory
  tests/test_guest_memory.c
  android/app/src/main/cpp/guest_memory.c
)
target_include_directories(test_guest_memory PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/android/app/src/main/cpp"
)
target_compile_options(test_guest_memory PRIVATE -Wall -Wextra -Werror)
add_test(NAME bounded_guest_memory COMMAND test_guest_memory)

add_executable(test_ppc_guest_memory
  tests/test_ppc_guest_memory.c
  android/app/src/main/cpp/guest_memory.c
  android/app/src/main/cpp/ppc_cpu.c
)
target_include_directories(test_ppc_guest_memory PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/android/app/src/main/cpp"
)
target_compile_options(test_ppc_guest_memory PRIVATE -Wall -Wextra -Werror)
add_test(NAME ppc_guest_memory_integration COMMAND test_ppc_guest_memory)

add_executable(test_guest_elf_loader
  tests/test_guest_elf_loader.c
  android/app/src/main/cpp/guest_elf_loader.c
  android/app/src/main/cpp/guest_memory.c
)
target_include_directories(test_guest_elf_loader PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/android/app/src/main/cpp"
)
target_compile_options(test_guest_elf_loader PRIVATE -Wall -Wextra -Werror)
add_test(NAME native_guest_elf_loader COMMAND test_guest_elf_loader)
