cmake_minimum_required(VERSION 2.6)
project(libzmq_x)

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
    ${ZEROMQ_INCLUDE_DIRS}
    ${CZMQ_INCLUDE_DIRS}
    ${PTHREAD_INCLUDE_DIRS}
)

add_definitions(-DUSE_RANDOM_IDENTITY)

set(LIBS
    ${ZEROMQ_LIBRARIES}
    ${CZMQ_LIBRARIES}
    ${PTHREAD_LIBRARIES}
)

add_library(zmq_x zmq_x.c)
target_compile_definitions(zmq_x PRIVATE ZMQ_X_COMPILE)
target_link_libraries(zmq_x ${ZEROMQ_LIBRARIES} ${PTHREAD_LIBRARIES})

add_executable(zmq_x_test zmq_x_test.c)
target_link_libraries(zmq_x_test ${LIBS} zmq_x)

add_executable(zmq_x_test1 zmq_x_test1.c)
target_link_libraries(zmq_x_test1 ${LIBS} zmq_x)

add_executable(zmq_x_rep zmq_x_rep.c)
target_link_libraries(zmq_x_rep ${LIBS} zmq_x)

add_executable(zmq_x_req zmq_x_req.c)
target_link_libraries(zmq_x_req ${LIBS} zmq_x)
