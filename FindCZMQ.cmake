if(UNIX)
    include(FindPkgConfig)
    PKG_CHECK_MODULES(PC_CZMQ "libczmq")
endif()

find_path(
    CZMQ_INCLUDE_DIRS
    NAMES czmq.h
    HINTS ${PC_CZMQ_INCLUDE_DIRS}
          $ENV{CZMQ_ROOT}/include
)

set(_CZMQ_LIB_NAMES czmq)
find_library(
    CZMQ_LIBRARIES
    NAMES ${_CZMQ_LIB_NAMES}
    HINTS ${PC_CZMQ_LIBRARY_DIRS}
          $ENV{CZMQ_ROOT}/lib
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CZMQ DEFAULT_MSG CZMQ_LIBRARIES CZMQ_INCLUDE_DIRS)
mark_as_advanced(CZMQ_LIBRARIES CZMQ_INCLUDE_DIRS)
