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
