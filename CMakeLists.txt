cmake_minimum_required (VERSION 2.6)
project (mem_mon)

set (VERSION 0.1)

set(CMAKE_EXE_LINKER_FLAGS -static)

add_definitions ("-Wall")
set(LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin")
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin")
include_directories ("${PROJECT_SOURCE_DIR}/include")
link_directories ("${PROJECT_SOURCE_DIR}/bin")
add_subdirectory (src)
add_subdirectory (tests)

