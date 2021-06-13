#[=======================================================================[.rst:
Findefi
-------

This module finds the gnuefi and efi libraries on the system.
Additionally finds linker script and crt objects that corresponds to current
platform (CMAKE_SYSTEM_PROCESSOR), provides create_efi_image() function
to geneate resulting efi image.

Add path where Findefi.cmake is located to CMAKE_MODULE_PATH variable.
For example, if Findefi.cmake is located next to CMakeLists.txt file, use::

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR})

CMAKE_MODULE_PATH should be set before calling find_package().
See example project.

For more information on find package modules see:
https://cmake.org/cmake/help/latest/command/find_package.html

The following will produce hello<arch>.efi image::

  find_package(efi REQUIRED)
  add_library(hello_efi SHARED main.c)
  target_link_libraries(hello_efi efi)
  create_efi_image(hello_efi hello)

The following variables are provided to indicate gnuefi support:

.. variable:: EFI_FOUND

  Variable indicating if the gnuefi and efi support was found.

.. variable:: EFI_INCLUDE_DIRS

  The directories containing the efi headers.

.. variable:: EFI_C_FLAGS

  The gnuefi and efi compilation flags

.. variable:: EFI_LIBRARIES

  The gnuefi and efi libraries to be linked.

.. variable:: EFI_LINKER_FLAGS

  The gnuefi and efi linker flags to be used when linking with these libraries.

Additionally, the following :prop_tgt:`IMPORTED` target is being provided:

.. variable:: efi

  Interface target for using gnuefi and efi libraries. This target has all
  the required libraries, flags and defintions set.

.. variable:: efi::gnuefi

  Imported target for gnuefi library (not to be used directly).

.. variable:: EFI::EFI

  Imported target for efi library (not to be used directly).

The following cache variables may also be set:

.. variable:: EFI_INCLUDE_DIR

  The directory containing the efi headers.

.. variable:: EFI_LIBRARY

  The efi library.

.. variable:: GNUEFI_LIBRARY

  The gnuefi library.

#]=======================================================================]

find_library(EFI_LIBRARY NAMES efi DOC "efi library")

find_path(EFI_INCLUDE_DIR efi/efi.h DOC "efi include directory")

find_library(GNUEFI_LIBRARY NAMES gnuefi DOC "gnuefi library")

# handle the QUIETLY and REQUIRED arguments and set xxx_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(EFI
        DEFAULT_MSG
        EFI_LIBRARY
        EFI_INCLUDE_DIR
        GNUEFI_LIBRARY)

if (EFI_FOUND)
    set(EFI_ARCH "${CMAKE_SYSTEM_PROCESSOR}")

    if (EFI_ARCH MATCHES "amd64.*|x86_64.*|AMD64.*")
        set(EFI_ARCH "x86_64")
        set(EFI_OUT_ARCH "x64")
    elseif (EFI_ARCH MATCHES "i686.*|i386.*")
        set(EFI_ARCH "ia32")
        set(EFI_OUT_ARCH "x86")
    else()
        set(EFI_OUT_ARCH "")
    endif()

    get_filename_component(EFI_LIBRARY_DIR ${EFI_LIBRARY} DIRECTORY)
    set(EFI_LDS "${EFI_LIBRARY_DIR}/elf_${EFI_ARCH}_efi.lds")
    set(EFI_CRT_OBJS "${EFI_LIBRARY_DIR}/crt0-efi-${EFI_ARCH}.o")

    set(EFI_C_FLAG -fno-stack-protector -fshort-wchar -mno-red-zone -mno-mmx -mno-sse)
    set(EFI_LINKER_FLAGS ${EFI_CRT_OBJS} -Wl,-nostdlib -Wl,-znocombreloc -Wl,-T${EFI_LDS} -Wl,-Bsymbolic -Wl,--no-undefined)
    set(EFI_INCLUDE_DIRS "${EFI_INCLUDE_DIR}/efi"
            "${EFI_INCLUDE_DIR}/efi/protocol"
            "${EFI_INCLUDE_DIR}/efi/${EFI_ARCH}")
    set(EFI_LIBRARIES ${EFI_LIBRARY} ${GNUEFI_LIBRARY})

    if(NOT TARGET EFI::EFI)
        add_library(EFI::EFI IMPORTED STATIC)
    endif()
    set_property(TARGET EFI::EFI PROPERTY IMPORTED_LOCATION ${EFI_LIBRARY})

    if(NOT TARGET EFI::GNUEFI)
        add_library(EFI::GNUEFI IMPORTED STATIC)
    endif()
    set_property(TARGET EFI::GNUEFI PROPERTY IMPORTED_LOCATION ${GNUEFI_LIBRARY})

    if(NOT TARGET EFI)
        add_library(EFI INTERFACE)
    endif()
    set_property(TARGET EFI PROPERTY INTERFACE_LINK_LIBRARIES ${EFI_LINKER_FLAGS} EFI::EFI EFI::GNUEFI)
    set_property(TARGET EFI PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET EFI PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${EFI_INCLUDE_DIRS})
    set_property(TARGET EFI PROPERTY INTERFACE_COMPILE_OPTIONS ${EFI_C_FLAGS})

    mark_as_advanced(
            EFI_LIBRARY
            EFI_INCLUDE_DIR
            GNUEFI_LIBRARY)
endif()

function (create_efi_image target out)
    set(EFI_IMAGE_FILE ${CMAKE_CURRENT_BINARY_DIR}/${out}${EFI_OUT_ARCH}.efi)
    set(PDB_FILE ${CMAKE_CURRENT_BINARY_DIR}/${out}.pdb)

    add_custom_command(TARGET ${target} POST_BUILD
            COMMENT "Creating EFI image: ${out}${EFI_OUT_ARCH}.efi"
            COMMAND ${CMAKE_OBJCOPY}
            -j .text
            -j .sdata
            -j .data
            -j .dynamic
            -j .dynsym
            -j .rel
            -j .rela
            -j .reloc
            --target=efi-app-${EFI_ARCH}
            $<TARGET_FILE:${target}>
            ${EFI_IMAGE_FILE})

    add_custom_command(TARGET ${target} POST_BUILD
            COMMENT "Creating debug info: ${out}.pdb"
            COMMAND ${CMAKE_OBJCOPY}
            -j .text
            -j .sdata
            -j .data
            -j .dynamic
            -j .dynsym
            -j .rel
            -j .rela
            -j .reloc
            -j .debug_info
            -j .debug_abbrev
            -j .debug_loc
            -j .debug_aranges
            -j .debug_line
            -j .debug_macinfo
            -j .debug_str
            $<TARGET_FILE:${target}>
            ${PDB_FILE}
            )

    set_property(TARGET ${target} PROPERTY EFI_IMAGE_FILE ${EFI_IMAGE_FILE})
endfunction()
