cmake_minimum_required(VERSION 3.20)
project(lua_benchmark LANGUAGES C CXX)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(LUA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/lua-5.5.0")

file(GLOB LUA_SOURCES "${LUA_ROOT}/*.c")
list(REMOVE_ITEM LUA_SOURCES
    "${LUA_ROOT}/lua.c"
    "${LUA_ROOT}/onelua.c"
    "${LUA_ROOT}/ltests.c"
)

add_library(lua_runtime STATIC ${LUA_SOURCES})
target_include_directories(lua_runtime PUBLIC "${LUA_ROOT}")
target_compile_definitions(lua_runtime PRIVATE LUA_USE_C89)

add_executable(lua_benchmark src/main.cpp)
target_include_directories(lua_benchmark PRIVATE
    "${LUA_ROOT}"
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
target_compile_definitions(lua_benchmark PRIVATE
    SLC_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(lua_benchmark PRIVATE lua_runtime)

if(WIN32)
    target_link_libraries(lua_benchmark PRIVATE ws2_32)
else()
    target_link_libraries(lua_benchmark PRIVATE m)
endif()
