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

# Suppress warnings for LoRaWAN library
add_compile_options(
  -Wno-stringop-overflow
)
# Define LED pin as dummpy pin to avoid errors
add_compile_definitions(
  PICO_DEFAULT_LED_PIN=0
)
add_subdirectory(lorawan-library-for-pico)

# Hacky way to make our own LoRaWAN patch
add_library(custom_pico_lorawan INTERFACE)

target_sources(custom_pico_lorawan INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/lorawan/lorawan-library-for-pico.c
  ${CMAKE_CURRENT_LIST_DIR}/lorawan/confirm.c
)
target_include_directories(custom_pico_lorawan INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/lorawan-library-for-pico/src/include
)
target_link_libraries(custom_pico_lorawan INTERFACE pico_loramac_node)

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

# Uncomment to compile in debug
add_compile_definitions(DEBUG)

set(PICO_LIBS
  hardware_sync
  pico_stdlib
  pico_btstack_ble
  pico_btstack_cyw43
  pico_cyw43_arch_none
  custom_pico_lorawan
)

set(SUBDIRS
  ble
  lorawan
  ui
)

set(ALL_SRC_FILES "")

foreach(SUBDIR ${SUBDIRS})
  unset(SRC_FILES)
  add_subdirectory(${SUBDIR})
  foreach(SRC_FILE ${SRC_FILES})
    set_source_files_properties(
      ${SRC_FILE}
      PROPERTIES
      COMPILE_OPTIONS -O3 -Wall -Wextra -Wpedantic -Werror
    )
  endforeach()
  set(ALL_SRC_FILES ${ALL_SRC_FILES} ${SRC_FILES})
endforeach(SUBDIR)

#-------------------------------------------------------------------------
# get_leaf
#-------------------------------------------------------------------------
# Gets the leaf file of a path.

function(get_leaf VARNAME FILE_NAME)
  string(REGEX REPLACE "[^/]*/" "" NAME_WITHOUT_EXT ${FILE_NAME})
  set(${VARNAME} "${NAME_WITHOUT_EXT}" PARENT_SCOPE)
endfunction()

#------------------------------------------------------------------------
# remove_extension
#------------------------------------------------------------------------
# A function to remove a file's extension

function(remove_extension VARNAME FILE_NAME)
  string(REGEX REPLACE "\\.[^.]*$" "" NAME_WITHOUT_EXT ${FILE_NAME})
  set(${VARNAME} "${NAME_WITHOUT_EXT}" PARENT_SCOPE)
endfunction()

# ------------------------------------------------------------------------
# Add main executable
# ------------------------------------------------------------------------

add_subdirectory(app)
foreach(APP_FILE ${APP_FILES})
  get_leaf(APP_FILE_NAME ${APP_FILE})
  remove_extension(APP_FILE_BIN ${APP_FILE_NAME})
  add_executable(${APP_FILE_BIN} ${APP_FILE} ${ALL_SRC_FILES})
  set_property(TARGET ${APP_FILE_BIN} PROPERTY COMPILE_DEFINITIONS "")

  # pull in common dependencies
  target_link_libraries(${APP_FILE_BIN} ${PICO_LIBS})

  # create map/bin/hex file etc.
  pico_add_extra_outputs(${APP_FILE_BIN})

  # Allow for printf over USB
  pico_enable_stdio_usb(${APP_FILE_BIN} 1)
  pico_enable_stdio_uart(${APP_FILE_BIN} 0)
endforeach(APP_FILE)
