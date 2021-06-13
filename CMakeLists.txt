#################
# Project setup #
#################
cmake_minimum_required(VERSION 3.20)
project(smalldoku C ASM)

#####################
# Set CMake options #
#####################
set(CMAKE_C_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake")

###################
# Add subprojects #
###################
add_subdirectory(core)
add_subdirectory(linux-ui)
add_subdirectory(uefi)
