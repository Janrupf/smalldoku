#############################################
# UEFI project, contains UEFI specific code #
#############################################

# Options
option(ENABLE_UEFI_QEMU_RUN YES)

# Find the EFI library, we link against it
find_package(EFI REQUIRED)

# Source listing
set(SMALLDOKU_UEFI_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(SMALLDOKU_UEFI_SOURCE
        src/main.c
        src/uefi-graphics.c)

# Additional resource files
set(SMALLDOKU_UEFI_FONT_FILE "${CMAKE_CURRENT_LIST_DIR}/src/font.psfu")
set(SMALLDOKU_UEFI_CURSOR_FILE "${CMAKE_CURRENT_LIST_DIR}/src/cursor.raw")

# Create the library target including all its options
add_library(smalldoku-uefi SHARED ${SMALLDOKU_UEFI_SOURCE})
target_include_directories(smalldoku-uefi PUBLIC ${SMALLDOKU_UEFI_INCLUDE_DIR})
target_link_libraries(smalldoku-uefi PUBLIC smalldoku-core smalldoku-core-ui EFI)
target_compile_options(smalldoku-uefi PUBLIC
        ${SMALLDOKU_UEFI_CFLAGS}
        -mrdrnd) # We use the rdrand instruction
target_compile_definitions(smalldoku-uefi PUBLIC
        GNU_EFI_USE_MS_ABI=1 # Allow calling UEFI functions directly using the MS-ABI feature of GCC/clang
        SMALLDOKU_UEFI_FONT_FILE=${SMALLDOKU_UEFI_FONT_FILE} # font.psfu resource path
        SMALLDOKU_UEFI_CURSOR_FILE=${SMALLDOKU_UEFI_CURSOR_FILE}) # cursor.raw resource path

create_efi_image(smalldoku-uefi smalldoku-uefi) # Create an UEFI executable out of the target

if(ENABLE_UEFI_RUN)
    # We need the OVMF BIOS for running with qemu
    find_file(OVMF_BIOS_CODE "OVMF_CODE.fd"
            HINTS "/usr/share/ovmf/x64"
            REQUIRED)

    # Find QEMU
    find_program(QEMU_EXECUTABLE
            NAMES qemu-system-x86_64 qemu)

    message(STATUS "Using OVMF BIOS from ${OVMF_BIOS_CODE}")
    message(STATUS "Using QEMU from ${QEMU_EXECUTABLE}")

    set(UEFI_RUN_DIR "${CMAKE_CURRENT_BINARY_DIR}/uefi-run") # Temporary directory to copy files to for running
    file(MAKE_DIRECTORY "${UEFI_RUN_DIR}")

    configure_file(${OVMF_BIOS_CODE} "${UEFI_RUN_DIR}/bios.bin" COPYONLY) # Copy the BIOS file

    set(UEFI_DISK_DIR "${UEFI_RUN_DIR}/hda-contents") # Directory which contains the virtual FAT drive

    # Configure a UEFI shell start script for auto starting the produced binary
    file(WRITE "${UEFI_DISK_DIR}/startup.nsh"
            "echo -on\n"
            "fs0:\n"
            ".\\EFI\\BOOT\\BOOTx64.EFI")

    # Retrieve the generated UEFI executable path
    get_target_property(UEFI_BOOT_FILE_SOURCE smalldoku-uefi EFI_IMAGE_FILE)
    set(UEFI_BOOT_DIR "${UEFI_DISK_DIR}/EFI/BOOT")
    set(UEFI_BOOT_FILE_TARGET "${UEFI_BOOT_DIR}/BOOTx64.EFI")

    # Add a command to copy the UEFI disk image
    add_custom_command(
            COMMAND "${CMAKE_COMMAND}" -E copy "${UEFI_BOOT_FILE_SOURCE}" "${UEFI_BOOT_FILE_TARGET}"
            OUTPUT "${UEFI_BOOT_FILE_TARGET}"
            DEPENDS smalldoku-uefi
    )

    # Build a command to run QEMU with the OVMF BIOS
    set(COMMON_QEMU_COMMAND
            ${QEMU_EXECUTABLE}
            --bios bios.bin
            -drive "file=fat:rw:${UEFI_DISK_DIR},format=raw"
            -enable-kvm)

    # Add a target to run QEMU
    add_custom_target(smalldoku-uefi-run-qemu
            COMMAND ${COMMON_QEMU_COMMAND}
            WORKING_DIRECTORY "${UEFI_RUN_DIR}"
            DEPENDS "${UEFI_BOOT_FILE_TARGET}")
endif()
