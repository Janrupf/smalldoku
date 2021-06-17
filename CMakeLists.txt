#################
# Project setup #
#################
cmake_minimum_required(VERSION 3.20)
project(smalldoku C ASM)

###############################
# Configure project variables #
###############################
set(SMALLDOKU_COMMON_CFLAGS
        -Werror
        -Wall
        -pedantic)

set(SMALLDOKU_UEFI_CFLAGS
        ${SMALLDOKU_COMMON_CFLAGS}
        -nostdlib
        -ffreestanding
        -fno-stack-protector
        -mno-red-zone)

set(SMALLDOKU_STANDALONE_CFLAGS
        ${SMALLDOKU_COMMON_CFLAGS}
        -ffreestanding
        -nostdlib
        -nostdinc
        -fno-stack-protector
        -mno-red-zone)

#####################
# Set CMake options #
#####################
set(CMAKE_C_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake")

###################
# Add subprojects #
###################
add_subdirectory(core)
add_subdirectory(core-ui)
add_subdirectory(linux-ui)
add_subdirectory(uefi)
