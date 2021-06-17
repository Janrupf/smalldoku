#############################################
# UEFI project, contains UEFI specific code #
#############################################
set(SMALLDOKU_UEFI_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(SMALLDOKU_UEFI_SOURCE
        src/main.c
        src/uefi-graphics.c)

set(SMALLDOKU_UEFI_FONT_FILE "${CMAKE_CURRENT_LIST_DIR}/src/font.psfu")
set(SMALLDOKU_UEFI_CURSOR_FILE "${CMAKE_CURRENT_LIST_DIR}/src/cursor.raw")

find_package(EFI REQUIRED)

add_custom_target(smalldoku-uefi-resources
        SOURCES ${SMALLDOKU_UEFI_FONT_FILE} ${SMALLDOKU_UEFI_CURSOR_FILE})

add_library(smalldoku-uefi SHARED ${SMALLDOKU_UEFI_SOURCE})
add_dependencies(smalldoku-uefi smalldoku-uefi-resources)
target_include_directories(smalldoku-uefi PUBLIC ${SMALLDOKU_UEFI_INCLUDE_DIR})
target_link_libraries(smalldoku-uefi PUBLIC smalldoku-core smalldoku-core-ui EFI)
target_compile_options(smalldoku-uefi PUBLIC -fno-stack-protector -ffreestanding -nostdlib -Werror -Wall -pedantic -mrdrnd)
target_compile_definitions(smalldoku-uefi PUBLIC
        GNU_EFI_USE_MS_ABI=1
        SMALLDOKU_UEFI_FONT_FILE=${SMALLDOKU_UEFI_FONT_FILE}
        SMALLDOKU_UEFI_CURSOR_FILE=${SMALLDOKU_UEFI_CURSOR_FILE})
target_link_options(smalldoku-uefi PUBLIC -nostdlib)
create_efi_image(smalldoku-uefi smalldoku-uefi)

set(UEFI_RUN_DIR "${CMAKE_CURRENT_BINARY_DIR}/uefi-run")
file(MAKE_DIRECTORY "${UEFI_RUN_DIR}")
configure_file("/usr/share/ovmf/x64/OVMF_CODE.fd" "${UEFI_RUN_DIR}/bios.bin" COPYONLY)

set(UEFI_DISK_DIR "${UEFI_RUN_DIR}/hda-contents")
file(WRITE "${UEFI_DISK_DIR}/startup.nsh"
        "echo -on\n"
        "fs0:\n"
        ".\\EFI\\BOOT\\BOOTx64.EFI")

get_target_property(UEFI_BOOT_FILE_SOURCE smalldoku-uefi EFI_IMAGE_FILE)
set(UEFI_BOOT_DIR "${UEFI_DISK_DIR}/EFI/BOOT")

add_custom_target(smalldoku-uefi-copy-boot DEPENDS smalldoku-uefi)
add_custom_command(
        TARGET smalldoku-uefi-copy-boot
        COMMAND "${CMAKE_COMMAND}" -E copy "${UEFI_BOOT_FILE_SOURCE}" "${UEFI_BOOT_DIR}/BOOTx64.EFI"
)

set(COMMON_QEMU_COMMAND
        qemu-system-x86_64
        --bios bios.bin
        -drive "file=fat:rw:${UEFI_DISK_DIR},format=raw"
        -enable-kvm
        -usb
        -device usb-host,vendorid=0x062a,productid=0x4101
        )

add_custom_target(smalldoku-uefi-run-qemu DEPENDS smalldoku-uefi-copy-boot)
add_custom_command(
        TARGET smalldoku-uefi-run-qemu
        COMMAND ${COMMON_QEMU_COMMAND}
        WORKING_DIRECTORY "${UEFI_RUN_DIR}"
)
