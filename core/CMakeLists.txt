#####################################################
# Core project, contains platform independent logic #
#####################################################
set(SMALLDOKU_CORE_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(SMALLDOKU_CORE_SOURCE
        src/smalldoku.c)

add_library(smalldoku-core STATIC ${SMALLDOKU_CORE_SOURCE})
target_include_directories(smalldoku-core PUBLIC ${SMALLDOKU_CORE_INCLUDE_DIR})
target_compile_options(smalldoku-core PRIVATE -ffreestanding -nostdlib -nostdinc -Werror -Wall -pedantic -fno-stack-protector -mno-red-zone)