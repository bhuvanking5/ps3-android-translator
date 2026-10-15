cmake_minimum_required(VERSION 3.22.1)
project(translator_bridge C)

add_library(translator_bridge SHARED native_bridge.c ../../../../../../native/arm64_runtime.c)
target_include_directories(translator_bridge PRIVATE ../../../../../../native)
target_compile_features(translator_bridge PRIVATE c_std_11)
