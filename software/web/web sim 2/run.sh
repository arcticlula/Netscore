#!/bin/bash
set -e

# Run the build script
./build.sh

# Start the server
echo "Starting local server at http://localhost:8000"
python3 -m http.server
