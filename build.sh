#!/bin/bash
set -e

BUILD_DIR="build"

cmake -B "${BUILD_DIR}" -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build "${BUILD_DIR}"
