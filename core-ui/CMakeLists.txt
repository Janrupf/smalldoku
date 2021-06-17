##################################################################
# Base ui project, contains platform independent rendering logic #
##################################################################
set(SMALLDOKU_CORE_UI_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(SMALLDOKU_CORE_UI_SOURCE
        src/smalldoku-core-graphics.c
        src/smalldoku-core-ui.c)

add_library(smalldoku-core-ui STATIC ${SMALLDOKU_CORE_UI_SOURCE})
target_include_directories(smalldoku-core-ui PUBLIC ${SMALLDOKU_CORE_UI_INCLUDE_DIR})
target_compile_options(smalldoku-core-ui PRIVATE ${SMALLDOKU_STANDALONE_CFLAGS})
target_link_libraries(smalldoku-core-ui PUBLIC smalldoku-core)
