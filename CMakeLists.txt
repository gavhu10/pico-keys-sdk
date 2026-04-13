# ~/dev/cpp/pico-fido/pico-keys-sdk/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# Include the standard ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Name your project
project(pico-keys-sdk)

idf_component_get_property(tinyusb_lib espressif__tinyusb COMPONENT_LIB)
