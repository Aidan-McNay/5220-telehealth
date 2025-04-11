# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# ========================================================================
# CMakeLists.txt
# ========================================================================
# A build system for our project

cmake_minimum_required(VERSION 3.13)

# ------------------------------------------------------------------------
# Compiler Setup
# ------------------------------------------------------------------------

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------------------
# Initialise pico_sdk from installed location
# ------------------------------------------------------------------------
# DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code
# Extension to work

if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

set(PICO_BOARD pico_w CACHE STRING "Board type")

# ------------------------------------------------------------------------
# Include the Pico SDK
# ------------------------------------------------------------------------

include(cmake/pico_sdk_import.cmake)

# ------------------------------------------------------------------------
# Project Setup
# ------------------------------------------------------------------------

project(
  blood_pressure
  VERSION 1.0
  DESCRIPTION "Our remote blood pressure monitor for MAE 5220"
  LANGUAGES C CXX ASM
)
pico_sdk_init()

# ------------------------------------------------------------------------
# Add main executable

add_executable(blood_pressure
  blink.c
)

# pull in common dependencies
target_link_libraries(blood_pressure pico_stdlib)

if (PICO_CYW43_SUPPORTED)
    target_link_libraries(blood_pressure pico_cyw43_arch_none)
endif()

# create map/bin/hex file etc.
pico_add_extra_outputs(blood_pressure)
