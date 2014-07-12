MAKE = nmake        # Visual Studio make tool(much like the Unix `make`)
TOUCH = type NUL >  # Unix `touch` replacement in Batch

CMAKE_GENERATOR = Visual Studio 10
CMAKE_BUILD_DIR = $(MAKEDIR)\build      # Generated project files/makefiles are
                                        # here
CMAKE_INSTALL_DIR = $(MAKEDIR)\runtime  # Install dir
RUNTIME_OUTPUT_DIR = $(MAKEDIR)\runtime # Executable output dir
CMAKE_INSTALL_CONFIG = release  # Can be set to MinSizeRel to reduce file size

BUILD_PROJ = ALL_BUILD.vcxproj  # Default project CMake generates for VS that
                                # add all target projects as dependencies
INSTALL_PROJ = INSTALL.vcxproj  # Default project CMake generates that installs
                                # all targets(for libraries, it consists of
                                # headers, lib and dll)

CMAKEFILES = CMakeLists.txt

.PHONY:

all: build install

build: Debug Release

Debug Release MinSizeRel RelWithDebInfo: run-cmake
    cd $(CMAKE_BUILD_DIR) && \
    msbuild /p:Configuration=$@;WarningLevel=4 /m:2 /t:build $(BUILD_PROJ)

install: $(CMAKE_INSTALL_CONFIG)
    cd $(CMAKE_BUILD_DIR) && \
    msbuild /p:Configuration=$(CMAKE_INSTALL_CONFIG);WarningLevel=4 /t:build \
        $(INSTALL_PROJ)

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

