if(UNIX)
    include(FindPkgConfig)
    PKG_CHECK_MODULES(PC_ZEROMQ "libzmq")
endif()

find_path(
    ZEROMQ_INCLUDE_DIRS
    NAMES zmq.h
    HINTS ${PC_ZEROMQ_INCLUDE_DIRS}
          $ENV{ZMQ_ROOT}/include
)

set(_ZEROMQ_LIB_NAMES zmq)
if(MSVC)
    set(_ZEROMQ_LIB_NAMES libzmq-v100-mt-3_2_4.lib)
endif()
find_library(
    ZEROMQ_LIBRARIES
    NAMES ${_ZEROMQ_LIB_NAMES}
    HINTS ${PC_ZEROMQ_LIBRARY_DIRS}
          $ENV{ZMQ_ROOT}/lib
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ZEROMQ DEFAULT_MSG ZEROMQ_LIBRARIES ZEROMQ_INCLUDE_DIRS)
mark_as_advanced(ZEROMQ_LIBRARIES ZEROMQ_INCLUDE_DIRS)
