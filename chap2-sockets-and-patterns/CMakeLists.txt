cmake_minimum_required(VERSION 2.6)
project(chap2-sockets-and-patterns)

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
    ..
    ../zmq_x
    ../czmq_x
    ${ZEROMQ_INCLUDE_DIRS}
    ${CZMQ_INCLUDE_DIRS}
    ${PTHREAD_INCLUDE_DIRS}
)

set(LIBS
    ${ZEROMQ_LIBRARIES}
    ${CZMQ_LIBRARIES}
    ${PTHREAD_LIBRARIES}
)

add_executable(hwserver hello/server.cpp)
target_link_libraries(hwserver ${LIBS})

add_executable(hwclient hello/client.cpp)
target_link_libraries(hwclient ${LIBS})

add_executable(hello hello.cpp)
target_link_libraries(hello ${LIBS})

if(${CMAKE_SOURCE_DIR} EQUAL ${CMAKE_CURRENT_LIST_DIR})
    add_subdirectory(../zmq_x)
    add_subdirectory(../czmq_x)
endif()

add_executable(pub pub.cpp)
target_link_libraries(pub ${LIBS} zmq_x czmq_x)

add_executable(sub sub.cpp)
target_link_libraries(sub ${LIBS} zmq_x czmq_x)
