#!/bin/bash

# Check if destination directory is provided, otherwise default to electrolula
if [ -z "$1" ]; then
    TARGET_DIR="../../../electrolula"
    echo "No destination provided. Defaulting to: $TARGET_DIR"
else
    TARGET_DIR="$1"
fi

if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: Destination directory '$TARGET_DIR' does not exist."
    if [ -z "$1" ]; then
        echo "Usage: $0 <destination_directory>"
        echo "Example: $0 ../my-vue-project"
    fi
    exit 1
fi

DEST_DIR="$TARGET_DIR"

# Smart detection for Vue/Web projects
if [ -d "$TARGET_DIR/public" ]; then
    echo "Detected Vue/Web project structure (found 'public' folder)."
    DEST_DIR="$TARGET_DIR/public/netscore/component"
else
    echo "Using provided directory as destination."
fi

# Ensure the build script exists and run it
if [ ! -f "./build.sh" ]; then
    echo "Error: build.sh not found in current directory."
    exit 1
fi

echo "Building project..."
./build.sh

if [ $? -ne 0 ]; then
    echo "Build failed! Aborting export."
    exit 1
fi

# Create destination directory structure
echo "Exporting to: $DEST_DIR"
mkdir -p "$DEST_DIR/build"

# Clean old files if they exist to avoid stale artifacts
rm -f "$DEST_DIR/index.html"
rm -f "$DEST_DIR/build/netscore.js"
rm -f "$DEST_DIR/build/netscore.wasm"

# Copy files
echo "Copying files..."
cp index.html "$DEST_DIR/"
cp ab_shutter_2btn.png "$DEST_DIR/"
cp ab_shutter_1btn.png "$DEST_DIR/"
cp build/netscore.js "$DEST_DIR/build/"
cp build/netscore.wasm "$DEST_DIR/build/"

echo "Export complete!"
echo "To use this in your Vue project:"
echo "1. Use an iframe pointing to: /netscore/index.html"
echo "   <iframe src=\"/netscore/component/index.html\" ...></iframe>"
