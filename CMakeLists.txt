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

include_directories(${ZMQ_ROOT}/include)
link_directories(${ZMQ_ROOT}/lib)

add_executable(task_ventilator task_ventilator.cpp)
target_link_libraries(task_ventilator libzmq-v100-mt-3_2_4.lib)

add_executable(task_worker task_worker.cpp)
target_link_libraries(task_worker libzmq-v100-mt-3_2_4.lib)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(task_sink task_sink.cpp)
target_link_libraries(task_sink
    libzmq-v100-mt-3_2_4.lib
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
target_link_libraries(mtrelay libzmq-v100-mt-3_2_4.lib pthreadVCE2.lib)

