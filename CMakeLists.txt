cmake_minimum_required(VERSION 3.10)
project(sparseset C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS_DEBUG "-g -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall -Wextra")

# Output executable directly to bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(ss
    ss.c
    sparseset.c
    benchmark.c
)

target_include_directories(ss PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
