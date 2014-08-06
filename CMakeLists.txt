cmake_minimum_required(VERSION 2.6)
project(zeromq_playground)

# ZeroMQ
set(ZMQ_ROOT "$ENV{ZMQ_ROOT}")
if(NOT ZMQ_ROOT)
    set(ZMQ_ROOT "C:/Program Files (x86)/ZeroMQ/3.2.4")
endif()

# Boost
set(BOOST_ROOT "$ENV{BOOST_ROOT}")
set(BOOST_LIBRARYDIR "$ENV{BOOST_LIBRARYDIR}")
set(Boost_DEBUG OFF)
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.41.0
    COMPONENTS system chrono
    REQUIRED
)

# czmq
set(CZMQ_ROOT "$ENV{CZMQ_ROOT}")
if(NOT CZMQ_ROOT)
    message(FATAL_ERROR "czmq needed to compile")
endif()

include_directories(
    .
    ${ZMQ_ROOT}/include
    ${CZMQ_ROOT}/include
)
link_directories(
    ${ZMQ_ROOT}/lib
    ${CZMQ_ROOT}/lib
)

add_definitions(-DUSE_RANDOM_IDENTITY)

set(Common_LIBS libzmq-v100-mt-3_2_4.lib czmq.lib)

add_executable(task_ventilator task_ventilator.cpp)
target_link_libraries(task_ventilator ${Common_LIBS})

add_executable(task_worker task_worker.cpp)
target_link_libraries(task_worker ${Common_LIBS})

include_directories(${Boost_INCLUDE_DIRS})

add_executable(task_sink task_sink.cpp)
target_link_libraries(task_sink
    ${Common_LIBS}
    ${Boost_LIBRARIES}
)

# Pthread-w32
set(PTHREAD_ROOT $ENV{PTHREAD_ROOT})
if(NOT PTHREAD_ROOT)
    message(FATA_ERROR "Pthread required")
endif()
include_directories(${PTHREAD_ROOT}/include)
link_directories(${PTHREAD_ROOT}/lib/x86)

add_executable(mtrelay mtrelay.cpp)
target_link_libraries(mtrelay ${Common_LIBS} pthreadVCE2.lib)

# hello-world server/client
add_executable(hwserver hello/server.cpp)
target_link_libraries(hwserver ${Common_LIBS})

add_executable(hwclient hello/client.cpp)
target_link_libraries(hwclient ${Common_LIBS})

add_executable(identity zhelpers.h identity.cpp)
target_link_libraries(identity ${Common_LIBS})

add_executable(rtreq zhelpers.h rtreq.cpp)
target_link_libraries(rtreq ${Common_LIBS} pthreadVCE2.lib)

add_executable(rtdealer zhelpers.h rtdealer.cpp)
target_link_libraries(rtdealer ${Common_LIBS} pthreadVCE2.lib)

add_executable(mtrand mtrand.cpp)
target_link_libraries(mtrand pthreadVCE2.lib)

add_executable(asyncsrv asyncsrv.cpp)
target_link_libraries(asyncsrv ${Common_LIBS})

