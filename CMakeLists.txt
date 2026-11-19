cmake_minimum_required(VERSION 3.10)
project(sparseset C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS_DEBUG "-g -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall -Wextra")

# Compile sparse set benchmarking
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# ECS interface prototype
add_executable(ecs
    sparseset.c
    ecs/ecs_p.c
    ecs/ecs.c
)
target_include_directories(ecs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ECS stress test
add_executable(ecsst
    sparseset.c
    ecs/ecsst.c
    ecs/ecs.c
    benchmarking/benchmark.c
)
target_include_directories(ecsst PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})