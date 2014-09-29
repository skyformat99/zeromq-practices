cmake_minimum_required(VERSION 2.6)
project(named-pipe-win)

# Setup build-related defaults
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/runtime)
endif()

get_filename_component(DEPENDS_DIR ${CMAKE_SOURCE_DIR}/depends ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH ${DEPENDS_DIR}/cmake)

add_definitions(
    -DUNICODE
    -D_UNICODE
)

add_executable(named_pipe_server server.c)

add_executable(named_pipe_client client.c)
