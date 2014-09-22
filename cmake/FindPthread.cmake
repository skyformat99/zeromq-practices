if(UNIX)
    include(FindPkgConfig)
    PKG_CHECK_MODULES(PC_PTHREAD "libpthread")
endif()

find_path(
    PTHREAD_INCLUDE_DIRS
    NAMES pthread.h
    HINTS ${PC_PTHREAD_INCLUDE_DIRS}
          $ENV{PTHREAD_ROOT}/include
)

set(_PTHREAD_LIB_NAMES pthread)
if(MSVC)
    set(_PTHREAD_LIB_NAMES pthreadVC2)
elseif(MINGW)
    set(_PTHREAD_LIB_NAMES pthreadGC2)
endif()
find_library(
    PTHREAD_LIBRARIES
    NAMES ${_PTHREAD_LIB_NAMES}
    HINTS ${PC_PTHREAD_LIBRARY_DIRS}
          $ENV{PTHREAD_ROOT}/lib
          $ENV{PTHREAD_ROOT}/lib/x86
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PTHREAD DEFAULT_MSG PTHREAD_LIBRARIES PTHREAD_INCLUDE_DIRS)
mark_as_advanced(PTHREAD_LIBRARIES PTHREAD_INCLUDE_DIRS)
