cmake_minimum_required(VERSION 3.22)
project(girassol_core LANGUAGES CXX)

option(GIRASSOL_BUILD_TESTS "Build native deterministic tests" ON)
option(GIRASSOL_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(girassol_core STATIC
    native/src/world.cpp
    native/src/omega.cpp
)
target_include_directories(girassol_core PUBLIC native/include)
target_compile_features(girassol_core PUBLIC cxx_std_20)
target_compile_options(girassol_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

if(GIRASSOL_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(girassol_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(girassol_core PUBLIC -fsanitize=address,undefined)
endif()

if(ANDROID)
    add_library(girassol_jni SHARED native/src/jni_bridge.cpp)
    target_link_libraries(girassol_jni PRIVATE girassol_core android log)
    target_compile_features(girassol_jni PRIVATE cxx_std_20)
    target_compile_options(girassol_jni PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
elseif(GIRASSOL_BUILD_TESTS)
    enable_testing()
    add_executable(girassol_core_tests native/tests/world_tests.cpp)
    target_link_libraries(girassol_core_tests PRIVATE girassol_core)
    add_test(NAME girassol_core_tests COMMAND girassol_core_tests)
endif()
