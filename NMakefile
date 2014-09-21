MAKE = nmake        # Visual Studio make tool(much like the Unix `make`)
TOUCH = type NUL >  # Unix `touch` replacement in Batch

CMAKE_GENERATOR = Visual Studio 10
CMAKE_BUILD_DIR = $(MAKEDIR)\build      # Generated project files/makefiles are
                                        # here
CMAKE_INSTALL_DIR = $(MAKEDIR)\runtime  # Install dir
RUNTIME_OUTPUT_DIR = $(MAKEDIR)\runtime # Executable output dir
CMAKE_INSTALL_CONFIG = release  # Can be set to MinSizeRel to reduce file size

CMAKEFILES = CMakeLists.txt

.PHONY:

all: build install

build: Debug Release

Debug Release MinSizeRel RelWithDebInfo: run-cmake
    cmake --build build --config $@ --use-stderr

install: $(CMAKE_INSTALL_CONFIG)
    cmake --build build --target $@ --config $(CMAKE_INSTALL_CONFIG) --use-stderr

run-cmake: $(CMAKEFILES)
    if not exist $(CMAKE_BUILD_DIR) mkdir $(CMAKE_BUILD_DIR)
    pushd $(CMAKE_BUILD_DIR) && \
    cmake -G "$(CMAKE_GENERATOR)" \
        -DCMAKE_INSTALL_PREFIX="$(CMAKE_INSTALL_DIR)" \
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG="$(RUNTIME_OUTPUT_DIR)" \
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE="$(RUNTIME_OUTPUT_DIR)" \
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO="$(RUNTIME_OUTPUT_DIR)" \
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL="$(RUNTIME_OUTPUT_DIR)" \
        .. && \
    popd && \
    $(TOUCH) $@

FORCE:

