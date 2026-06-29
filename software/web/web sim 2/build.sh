#!/bin/bash

# Source local emsdk environment
source ./emsdk/emsdk_env.sh

# Create build directory
mkdir -p build
cd build

# Configure with emcmake
emcmake cmake ..


# Build with cmake --build
emmake cmake --build .

echo "Build complete. Open index.html to see the result."
