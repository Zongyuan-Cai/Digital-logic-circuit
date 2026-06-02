cmake_minimum_required(VERSION 3.16)
project(logic_sim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# Simulation core library
# ============================================================
set(SIM_CORE_HEADERS
    include/logic_sim/signal.hpp
    include/logic_sim/pin.hpp
    include/logic_sim/node.hpp
    include/logic_sim/device.hpp
    include/logic_sim/circuit.hpp
    include/logic_sim/event_queue.hpp
    include/logic_sim/wave_recorder.hpp
    include/logic_sim/simulator.hpp
    include/logic_sim/devices/gates.hpp
    include/logic_sim/devices/flip_flops.hpp
    include/logic_sim/devices/chips.hpp
)

set(SIM_CORE_SOURCES
    src/signal.cpp
    src/device.cpp
    src/node.cpp
    src/circuit.cpp
    src/event_queue.cpp
    src/wave_recorder.cpp
    src/simulator.cpp
    src/devices/gates.cpp
    src/devices/flip_flops.cpp
    src/devices/chips.cpp
)

add_library(sim_core STATIC ${SIM_CORE_SOURCES} ${SIM_CORE_HEADERS})
target_include_directories(sim_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_options(sim_core PRIVATE -Wall -Wextra -fPIC)

# ============================================================
# Tests — try gitee mirror for GTest (github blocked in some regions)
# ============================================================
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(TEST_SOURCES
    tests/test_gates.cpp
    tests/test_flip_flops.cpp
    tests/test_chips_encoders.cpp
    tests/test_chips_decoders.cpp
    tests/test_chips_mux.cpp
    tests/test_chips_arithmetic.cpp
    tests/test_chips_registers.cpp
    tests/test_chips_counters.cpp
    tests/test_simulator.cpp
)

foreach(TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE sim_core GTest::gtest GTest::gtest_main)
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# ============================================================
# Python bindings (optional)
# ============================================================
find_package(pybind11 QUIET)
if(pybind11_FOUND)
    pybind11_add_module(logic_sim
        ${CMAKE_CURRENT_SOURCE_DIR}/../bindings/python/logic_sim_pybind.cpp
    )
    target_link_libraries(logic_sim PRIVATE sim_core)
    target_include_directories(logic_sim PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
endif()
