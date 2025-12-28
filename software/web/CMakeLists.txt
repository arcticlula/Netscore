cmake_minimum_required(VERSION 3.10)
project(netscore_wasm)

set(CMAKE_CXX_STANDARD 17)


# Include directories
include_directories(src)

# Add source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

add_executable(netscore ${SOURCES})

# Emscripten-specific settings
# -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']  : Allow calling compiled C functions
# -s EXPORTED_FUNCTIONS=['_get_led_value', '_init_system']    : Export our specific functions
# -s WASM=1                                      : Compile to WebAssembly
# -s NO_EXIT_RUNTIME=1                           : Keep the runtime alive (needed for event loops if we had one)
set_target_properties(netscore PROPERTIES 
    SUFFIX ".js"
    LINK_FLAGS "-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] -s EXPORTED_FUNCTIONS=['_get_led_value','_init_system','_set_mux','_run_display','_set_window','_trigger_button_event','_advance_mux_and_update','_reset_simulation','_set_muted'] -s WASM=1 -s NO_EXIT_RUNTIME=1"
)
