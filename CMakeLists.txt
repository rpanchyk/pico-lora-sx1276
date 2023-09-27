# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(pico_lora_sx1276 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Create a pico-sdk subdirectory in project for the libraries
pico_sdk_init()

# Create an INTERFACE library aggregating all the common parts
add_library(commons INTERFACE)

# Note: CMake policy is to use absolute paths for interface libraries
target_sources(commons INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/driver/radio.c
)

target_compile_definitions(commons INTERFACE)

# Include library dependencies
target_link_libraries(commons INTERFACE
    pico_stdlib
    hardware_uart
    hardware_spi
)

add_executable(receiver ${CMAKE_CURRENT_LIST_DIR}/receiver.c)
target_link_libraries(receiver commons)
target_compile_definitions(receiver PRIVATE)
pico_add_extra_outputs(receiver)

add_executable(transmitter ${CMAKE_CURRENT_LIST_DIR}/transmitter.c)
target_link_libraries(transmitter commons)
target_compile_definitions(transmitter PRIVATE)
pico_add_extra_outputs(transmitter)
