# 5220-telehealth

A repository of our design for MAE 5220: Introduction to IoT, which
obtains blood pressure data from an Omron Evolv BP7000, and transmits
it over LoRaWAN to The Things Network (TTN).

Unlike the code for the rest of the class, our target platform is a
Raspberry Pi Pico W, so the build process is different than labs.

## Repository Structure

 - **CMakeLists.txt**: Instructions for our CMake-based build system
 - **app**: Target applications for the Pico W (each with a `main`)
 - **ble**: Source code for interfacing over BLE
 - **cmake**: Extra CMake utilities
 - **lorawan**: Source code for interfacing over LoRaWAN
 - **lorawan-library-for-pico**: A submodule library for using LoRaWAN on the Pico W
 - **pcb**: Our PCB design for the system
 - **pico_sdk_import.cmake**: CMake support for the Raspberry Pi SDK
 - **ui**: Source code for interfacing with physical UI devices (LEDs, buttons, etc.)
 - **utils**: Utility code used throughout the project