cmake_minimum_required(VERSION 2.6)
project(libczmq_x)

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

find_package(ZeroMQ REQUIRED)
find_package(CZMQ REQUIRED)
find_package(Pthread REQUIRED)

include_directories(
    .
    ../zmq_x
    ${ZEROMQ_INCLUDE_DIRS}
    ${CZMQ_INCLUDE_DIRS}
    ${PTHREAD_INCLUDE_DIRS}
)

set(LIBS
    ${ZEROMQ_LIBRARIES}
    ${CZMQ_LIBRARIES}
    ${PTHREAD_LIBRARIES}
)

if(${CMAKE_SOURCE_DIR} EQUAL ${CMAKE_CURRENT_LIST_DIR})
    add_subdirectory(../zmq_x)
endif()

add_library(czmq_x czmq_x.c)
target_compile_definitions(czmq_x PRIVATE CZMQ_X_COMPILE)
target_link_libraries(czmq_x ${CZMQ_LIBRARIES} zmq_x)

add_executable(czmq_x_test czmq_x_test.c)
target_link_libraries(czmq_x_test ${LIBS} zmq_x czmq_x)
