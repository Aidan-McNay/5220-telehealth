# ========================================================================
# CMakeLists.txt
# ========================================================================
# Files for the BLE functions for the Pico W

set(SRC_FILES
  ble/ble.cpp
PARENT_SCOPE)

# ------------------------------------------------------------------------
# Configure target for using btstack
# ------------------------------------------------------------------------

include_directories(
  ${CMAKE_CURRENT_LIST_DIR} # For btstack config
)
add_compile_definitions(
  RUNNING_AS_CLIENT=1
)