#!/bin/bash

# Source local emsdk environment
source ./emsdk/emsdk_env.sh

# Create build directory
mkdir -p build
cd build

# Configure with emcmake
emcmake cmake ..


# Build with emmake
emmake make

echo "Build complete. Open index.html to see the result."
