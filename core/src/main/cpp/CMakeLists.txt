cmake_minimum_required(VERSION 3.22.1)

add_library(core SHARED
    core.cpp
    injector.cpp
    features.cpp
    ipc.cpp
    inject.cpp
    anticheat.cpp
    jni_bridge.cpp
)

target_include_directories(core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(core PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "core"
)

find_library(log-lib log)
target_link_libraries(core ${log-lib} android)