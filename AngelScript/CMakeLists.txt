cmake_minimum_required(VERSION 3.20)

project(angelscript_benchmark CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(ANGELSCRIPT_ROOT "${CMAKE_CURRENT_LIST_DIR}/angelscript-2.38.0/sdk")
set(ANGELSCRIPT_SRC_ROOT "${ANGELSCRIPT_ROOT}/angelscript")
set(ASAOT_ROOT "${CMAKE_CURRENT_LIST_DIR}/asaot")

add_subdirectory("${ANGELSCRIPT_SRC_ROOT}/projects/cmake" "${CMAKE_CURRENT_BINARY_DIR}/angelscript")

set(ASAOT_GENERATED_CPP "${CMAKE_CURRENT_BINARY_DIR}/asaot_generated_dispatch.cpp")
add_custom_command(
    OUTPUT "${ASAOT_GENERATED_CPP}"
    COMMAND "${Python3_EXECUTABLE}" "${ASAOT_ROOT}/getbytecodes.py" "${ANGELSCRIPT_SRC_ROOT}" "${ASAOT_GENERATED_CPP}"
    DEPENDS
        "${ASAOT_ROOT}/getbytecodes.py"
        "${ASAOT_ROOT}/include/AOTCompiler.h"
        "${ANGELSCRIPT_SRC_ROOT}/source/as_context.cpp"
        "${ANGELSCRIPT_SRC_ROOT}/include/angelscript.h"
    VERBATIM
)

add_library(asaot_runtime STATIC
    "${ASAOT_ROOT}/source/AOTCompiler.cpp"
    "${ASAOT_ROOT}/source/AOTFunction.cpp"
    "${ASAOT_GENERATED_CPP}"
)

target_include_directories(asaot_runtime
    PUBLIC
        "${ASAOT_ROOT}/include"
        "${ANGELSCRIPT_SRC_ROOT}/include"
        "${ANGELSCRIPT_SRC_ROOT}/source"
        "${ANGELSCRIPT_ROOT}/add_on"
)

target_link_libraries(asaot_runtime PUBLIC angelscript)

set(ANGELSCRIPT_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/scripts/sample.as")
set(ANGELSCRIPT_SOURCE "${CMAKE_CURRENT_LIST_DIR}/src/main.cpp")
set(ANGELSCRIPT_AOT_STUB "${CMAKE_CURRENT_BINARY_DIR}/angelscript_aot_stub.cpp")
set(ANGELSCRIPT_AOT_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/angelscript_aot_generated.cpp")

file(WRITE "${ANGELSCRIPT_AOT_STUB}" "// Generated at runtime by angelscript_generate\n")
file(WRITE "${ANGELSCRIPT_AOT_GENERATED}" "// Replaced by angelscript_generate\n")

add_executable(angelscript_generate
    "${ANGELSCRIPT_SOURCE}"
    "${ANGELSCRIPT_AOT_STUB}"
)

target_compile_definitions(angelscript_generate PRIVATE
    SANDBOX_GENERATE_AOT=1
    SANDBOX_USE_AOT=0
)
target_include_directories(angelscript_generate PRIVATE
    "${ASAOT_ROOT}/include"
    "${ANGELSCRIPT_SRC_ROOT}/include"
    "${ANGELSCRIPT_SRC_ROOT}/source"
    "${ANGELSCRIPT_ROOT}/add_on"
    "${CMAKE_CURRENT_LIST_DIR}/.."
)
target_link_libraries(angelscript_generate PRIVATE asaot_runtime)

add_custom_command(
    OUTPUT "${ANGELSCRIPT_AOT_GENERATED}"
    COMMAND "$<TARGET_FILE:angelscript_generate>" "${ANGELSCRIPT_SCRIPT}" "${ANGELSCRIPT_AOT_GENERATED}"
    DEPENDS angelscript_generate "${ANGELSCRIPT_SCRIPT}"
    VERBATIM
)

add_executable(angelscript_benchmark_aot
    "${ANGELSCRIPT_SOURCE}"
    "${ANGELSCRIPT_AOT_GENERATED}"
)

target_compile_definitions(angelscript_benchmark_aot PRIVATE
    SANDBOX_GENERATE_AOT=0
    SANDBOX_USE_AOT=1
)
target_include_directories(angelscript_benchmark_aot PRIVATE
    "${ASAOT_ROOT}/include"
    "${ANGELSCRIPT_SRC_ROOT}/include"
    "${ANGELSCRIPT_SRC_ROOT}/source"
    "${ANGELSCRIPT_ROOT}/add_on"
    "${CMAKE_CURRENT_LIST_DIR}/.."
)
target_link_libraries(angelscript_benchmark_aot PRIVATE asaot_runtime)
add_dependencies(angelscript_benchmark_aot angelscript_generate)

add_executable(angelscript_benchmark_noaot
    "${ANGELSCRIPT_SOURCE}"
    "${ANGELSCRIPT_AOT_STUB}"
)

target_compile_definitions(angelscript_benchmark_noaot PRIVATE
    SANDBOX_GENERATE_AOT=0
    SANDBOX_USE_AOT=0
)
target_include_directories(angelscript_benchmark_noaot PRIVATE
    "${ASAOT_ROOT}/include"
    "${ANGELSCRIPT_SRC_ROOT}/include"
    "${ANGELSCRIPT_SRC_ROOT}/source"
    "${ANGELSCRIPT_ROOT}/add_on"
    "${CMAKE_CURRENT_LIST_DIR}/.."
)
target_link_libraries(angelscript_benchmark_noaot PRIVATE asaot_runtime)

add_custom_target(angelscript_benchmark
    DEPENDS angelscript_benchmark_noaot angelscript_benchmark_aot
)
