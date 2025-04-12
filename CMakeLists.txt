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
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# ------------------------------------------------------------------------
# btstack configuration
# ------------------------------------------------------------------------

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/ble # For btstack config
)
add_compile_definitions(
  RUNNING_AS_CLIENT=1
)

# ------------------------------------------------------------------------
# Compile subdirectories as libraries
# ------------------------------------------------------------------------

set(PICO_LIBS
  hardware_sync
  pico_stdlib
  pico_btstack_ble
  pico_btstack_cyw43
  pico_cyw43_arch_none
)

set(SUBDIRS
  ble
  lorawan
  ui
)

set(USER_LIBS "")

foreach(SUBDIR ${SUBDIRS})
  add_subdirectory(${SUBDIR})
  add_library(${SUBDIR} STATIC ${SRC_FILES})
  target_link_libraries(${SUBDIR} ${PICO_LIBS})
  foreach(SRC_FILE ${SRC_FILES})
    set_source_files_properties(
      ${SRC_FILE}
      PROPERTIES
      COMPILE_OPTIONS -O3 -Wall -Wextra -Wpedantic -Werror
    )
  endforeach()
  set(USER_LIBS ${USER_LIBS} ${SUBDIR})
endforeach(SUBDIR)

# ------------------------------------------------------------------------
# Add main executable
# ------------------------------------------------------------------------

add_executable(blood_pressure
  blink.c
)

# pull in common dependencies
target_link_libraries(blood_pressure ${USER_LIBS} ${PICO_LIBS})

# create map/bin/hex file etc.
pico_add_extra_outputs(blood_pressure)
