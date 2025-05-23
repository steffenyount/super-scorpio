cmake_minimum_required(VERSION 3.12)
set(PICO_BOARD "adafruit_feather_rp2040")

# Pull in SDK before project
include(pico_sdk_import.cmake)

project(super-scorpio C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_COPY_TO_RAM 1)

set(SUPER_SCORPIO_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

include(auto_set_url.cmake)

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -fms-extensions
        -O3
)
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()

# Add main Folder
add_subdirectory(main)

