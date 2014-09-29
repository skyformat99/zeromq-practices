cmake_minimum_required(VERSION 2.6)
project(chap3-advanced-rep-req-patterns)

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

add_executable(task_ventilator task_ventilator.cpp)
target_link_libraries(task_ventilator ${LIBS})

add_executable(task_worker task_worker.cpp)
target_link_libraries(task_worker ${LIBS})

add_executable(task_sink task_sink.cpp)
target_link_libraries(task_sink ${LIBS})

add_executable(mtrelay mtrelay.cpp)
target_link_libraries(mtrelay ${LIBS})

add_executable(identity identity.cpp)
target_link_libraries(identity ${LIBS})

add_executable(rtreq rtreq.cpp)
target_link_libraries(rtreq ${LIBS})

add_executable(rtdealer rtdealer.cpp)
target_link_libraries(rtdealer ${LIBS})

add_executable(mtrand mtrand.cpp)
target_link_libraries(mtrand ${PTHREAD_LIBRARIES})

add_executable(asyncsrv asyncsrv.cpp)
target_link_libraries(asyncsrv ${LIBS})

add_library(endpoints cluster/endpoints.cpp cluster/endpoints.h)
add_library(endpoint cluster/endpoint.cpp cluster/endpoint.h)

add_executable(state_flow cluster/state_flow.cpp)
target_link_libraries(state_flow ${LIBS} endpoints)

add_executable(task_flow cluster/task_flow.cpp)
target_link_libraries(task_flow ${LIBS} endpoint)

add_executable(cluster cluster/prototype.cpp)
target_link_libraries(cluster ${LIBS} endpoint)
