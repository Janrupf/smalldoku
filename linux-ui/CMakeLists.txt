###############################################
# Linux project, contains linux specific code #
###############################################
set(SMALLDOKU_LINUX_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(SMALLDOKU_LINUX_SOURCE
        src/main.c
        src/x11.c)

find_package(X11 REQUIRED)

add_executable(smalldoku-linux ${SMALLDOKU_LINUX_SOURCE})
target_include_directories(smalldoku-linux PUBLIC ${SMALLDOKU_LINUX_INCLUDE_DIR})
target_link_libraries(smalldoku-linux PUBLIC smalldoku-core smalldoku-core-ui X11::X11)
