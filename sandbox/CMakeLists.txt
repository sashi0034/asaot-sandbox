cmake_minimum_required(VERSION 3.20)

project(asaot_sandbox CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(ANGELSCRIPT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../angelscript-2.38.0/sdk")
set(ANGELSCRIPT_SRC_ROOT "${ANGELSCRIPT_ROOT}/angelscript")
set(ASAOT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../asaot")

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

set(SANDBOX_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/sample.as")
set(SANDBOX_SOURCE "${CMAKE_CURRENT_LIST_DIR}/sample.cpp")
set(SANDBOX_AOT_STUB "${CMAKE_CURRENT_BINARY_DIR}/sandbox_aot_stub.cpp")
set(SANDBOX_AOT_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/sandbox_aot_generated.cpp")

file(WRITE "${SANDBOX_AOT_STUB}" "// Generated at runtime by sandbox_generate\n")
file(WRITE "${SANDBOX_AOT_GENERATED}" "// Replaced by sandbox_generate\n")

add_executable(sandbox_generate
    "${SANDBOX_SOURCE}"
    "${SANDBOX_AOT_STUB}"
    "${ANGELSCRIPT_ROOT}/add_on/scriptstdstring/scriptstdstring.cpp"
)

target_compile_definitions(sandbox_generate PRIVATE SANDBOX_GENERATE_AOT=1)
target_include_directories(sandbox_generate PRIVATE
    "${ASAOT_ROOT}/include"
    "${ANGELSCRIPT_SRC_ROOT}/include"
    "${ANGELSCRIPT_SRC_ROOT}/source"
    "${ANGELSCRIPT_ROOT}/add_on"
)
target_link_libraries(sandbox_generate PRIVATE asaot_runtime)

add_custom_command(
    OUTPUT "${SANDBOX_AOT_GENERATED}"
    COMMAND "$<TARGET_FILE:sandbox_generate>" "${SANDBOX_SCRIPT}" "${SANDBOX_AOT_GENERATED}"
    DEPENDS sandbox_generate "${SANDBOX_SCRIPT}"
    VERBATIM
)

add_executable(sandbox_run
    "${SANDBOX_SOURCE}"
    "${SANDBOX_AOT_GENERATED}"
    "${ANGELSCRIPT_ROOT}/add_on/scriptstdstring/scriptstdstring.cpp"
)

target_compile_definitions(sandbox_run PRIVATE SANDBOX_GENERATE_AOT=0)
target_include_directories(sandbox_run PRIVATE
    "${ASAOT_ROOT}/include"
    "${ANGELSCRIPT_SRC_ROOT}/include"
    "${ANGELSCRIPT_SRC_ROOT}/source"
    "${ANGELSCRIPT_ROOT}/add_on"
)
target_link_libraries(sandbox_run PRIVATE asaot_runtime)
add_dependencies(sandbox_run sandbox_generate)

