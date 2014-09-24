cmake_minimum_required(VERSION 2.6)
project(zeromq_playground)

# Setup build-related defaults
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/runtime)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

find_package(ZeroMQ REQUIRED)
find_package(CZMQ REQUIRED)
find_package(Pthread REQUIRED)

# Boost
#set(BOOST_ROOT "$ENV{BOOST_ROOT}")
#set(BOOST_LIBRARYDIR "$ENV{BOOST_LIBRARYDIR}")
#set(Boost_DEBUG OFF)
#set(Boost_USE_STATIC_LIBS ON)
#find_package(Boost 1.41.0
#    COMPONENTS system chrono
#    REQUIRED
#)

include_directories(
    .
    ${ZEROMQ_INCLUDE_DIRS}
    ${CZMQ_INCLUDE_DIRS}
    ${PTHREAD_INCLUDE_DIRS}
    #${Boost_INCLUDE_DIRS}
)

add_definitions(-DUSE_RANDOM_IDENTITY)

set(LIBS
    ${ZEROMQ_LIBRARIES}
    ${CZMQ_LIBRARIES}
    ${PTHREAD_LIBRARIES}
    ${Boost_LIBRARIES}
)

add_executable(task_ventilator task_ventilator.cpp)
target_link_libraries(task_ventilator ${LIBS})

add_executable(task_worker task_worker.cpp)
target_link_libraries(task_worker ${LIBS})

add_executable(task_sink task_sink.cpp)
target_link_libraries(task_sink ${LIBS})

add_executable(mtrelay mtrelay.cpp)
target_link_libraries(mtrelay ${LIBS})

# hello-world server/client
add_executable(hwserver hello/server.cpp)
target_link_libraries(hwserver ${LIBS})

add_executable(hwclient hello/client.cpp)
target_link_libraries(hwclient ${LIBS})

add_executable(identity identity.cpp)
target_link_libraries(identity ${LIBS})

add_executable(rtreq rtreq.cpp)
target_link_libraries(rtreq ${LIBS})

add_executable(rtdealer rtdealer.cpp)
target_link_libraries(rtdealer ${LIBS})

add_executable(mtrand mtrand.cpp)
target_link_libraries(mtrand pthreadVCE2.lib)

add_executable(asyncsrv asyncsrv.cpp)
target_link_libraries(asyncsrv ${LIBS})

add_executable(hello hello.cpp)
target_link_libraries(hello ${LIBS})

add_executable(state_flow cluster/state_flow.cpp)
target_link_libraries(state_flow ${LIBS})

add_executable(pub pub.cpp)
target_link_libraries(pub ${LIBS})

add_executable(sub sub.cpp)
target_link_libraries(sub ${LIBS})

add_executable(task_flow cluster/task_flow.cpp)
target_link_libraries(task_flow ${LIBS})

add_executable(cluster cluster/prototype.cpp)
target_link_libraries(cluster ${LIBS})

add_library(zmq_x zmq_x.c)
target_compile_definitions(zmq_x PRIVATE ZMQ_X_COMPILE)
target_link_libraries(zmq_x ${ZEROMQ_LIBRARIES})

add_executable(zmq_x_test zmq_x_test.c)
target_link_libraries(zmq_x_test ${LIBS} zmq_x)

add_library(czmq_x czmq_x.c)
target_compile_definitions(czmq_x PRIVATE CZMQ_X_COMPILE)
target_link_libraries(czmq_x ${CZMQ_LIBRARIES} zmq_x)

add_executable(czmq_x_test czmq_x_test.c)
target_link_libraries(czmq_x_test ${LIBS} zmq_x czmq_x)

add_executable(named_pipe_server named_pipe_win/server.c)
target_compile_definitions(named_pipe_server PRIVATE UNICODE _UNICODE)

add_executable(named_pipe_client named_pipe_win/client.c)
target_compile_definitions(named_pipe_client PRIVATE UNICODE _UNICODE)

add_executable(zmq_x_test1 zmq_x_test1.c)
target_link_libraries(zmq_x_test1 ${LIBS} zmq_x)
