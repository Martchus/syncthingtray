cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthing)
set(META_PROJECT_TYPE library)
set(META_APP_NAME "Syncthing library")
set(META_APP_DESCRIPTION "Syncthing itself, built as a shared or static library")
set(META_PROJECT_VARNAME_UPPER LIB_SYNCTHING)
set(META_SYNCTHING_VERSION "v1.20.4")

# add project files
set(HEADER_FILES interface.h)
set(SRC_FILES "${CMAKE_CURRENT_BINARY_DIR}/libsyncthinginternal.h" interface.cpp)

set(TEST_HEADER_FILES)
set(TEST_SRC_FILES tests/interfacetests.cpp)

# find the go binary
find_program(GO_BIN go)
if (NOT GO_BIN)
    message(FATAL_ERROR "The go binary could not be located.")
endif ()

# determine GOARCH for target
set(GO_TARGET_ARCH_OVERRIDE
    ""
    CACHE STRING "overrides the 'GOARCH' variable")
if (GO_TARGET_ARCH_OVERRIDE)
    set(GO_TARGET_ARCH "${GO_TARGET_ARCH_OVERRIDE}")
elseif (NOT CMAKE_CROSSCOMPILING)
    set(GO_TARGET_ARCH "")
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(GO_TARGET_ARCH "amd64")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86")
    set(GO_TARGET_ARCH "386")
elseif (CMAKE_SYSTEM_NAME MATCHES "(ppc64|ppc64le|arm|arm64|s390x)")
    set(GO_TARGET_ARCH "${CMAKE_SYSTEM_NAME}")
else ()
    message(FATAL_ERROR "Unable to auto-determine GOARCH. Please set GO_TARGET_ARCH_OVERRIDE manually.")
endif ()

# determine GOOS for target
set(GO_TARGET_OS_OVERRIDE
    ""
    CACHE STRING "overrides the 'GOOS' variable")
if (GO_TARGET_OS_OVERRIDE)
    set(GO_TARGET_OS "${GO_TARGET_OS_OVERRIDE}")
elseif (NOT CMAKE_CROSSCOMPILING)
    set(GO_TARGET_OS "")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(GO_TARGET_OS "linux")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(GO_TARGET_OS "android")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(GO_TARGET_OS "windows")
elseif (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(GO_TARGET_OS "freebsd")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(GO_TARGET_OS "darwin")
else ()
    message(FATAL_ERROR "Unable to auto-determine GOOS. Please set GO_TARGET_OS_OVERRIDE manually.")
endif ()
message(STATUS "Using GOOS=${GO_TARGET_OS} and GOARCH=${GO_TARGET_ARCH}")

# define default GOFLAGS
string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
set(GO_FLAGS_OVERRIDE
    ""
    CACHE STRING "overrides the 'GOFLAGS' variable")
if (GO_FLAGS_OVERRIDE)
    set(GO_FLAGS "${GO_FLAGS_OVERRIDE}")
else ()
    set(GO_FLAGS "-trimpath -mod=readonly -modcacherw")
endif ()
foreach (LANGUAGE C CXX)
    set(CGO_${LANGUAGE}FLAGS_OVERRIDE
        ""
        CACHE STRING "overrides the 'CGO_${LANGUAGE}FLAGS' variable")
    if (CGO_${LANGUAGE}FLAGS_OVERRIDE)
        set(CGO_${LANGUAGE}FLAGS "${CGO_${LANGUAGE}FLAGS_OVERRIDE}")
    else ()
        set(CGO_${LANGUAGE}FLAGS "${CMAKE_${LANGUAGE}_FLAGS_INIT} ${CMAKE_${LANGUAGE}_FLAGS_${CMAKE_BUILD_TYPE_UPPER}_INIT}")
    endif ()
endforeach ()
set(CGO_LDFLAGS_OVERRIDE
    ""
    CACHE STRING "overrides the 'CGO_LDFLAGS' variable")
if (CGO_LDFLAGS_OVERRIDE)
    set(CGO_LDFLAGS "${CGO_CFLAGS_OVERRIDE}")
else ()
    set(CGO_LDFLAGS "${CMAKE_SHARED_LINKER_FLAGS_INIT} ${CMAKE_SHARED_LINKER_FLAGS_${CMAKE_BUILD_TYPE_UPPER}_INIT}")
endif ()

# locate the Syncthing checkout
set(GO_DEVELOPMENT_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/go"
    CACHE PATH "the 'GOPATH'")
set(SYNCTHING_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/go/src/github.com/syncthing/syncthing"
    CACHE PATH "path of Syncthing checkout")

# find Syncthing's assets (caveat: newly added assets will not cause CMake to automatically regenerate)
file(
    GLOB_RECURSE SRC_FILES_SYNCTHING_ASSETS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${SYNCTHING_PATH}/gui/*")
if (NOT SRC_FILES_SYNCTHING_ASSETS)
    message(
        FATAL_ERROR
            "No asset files found in Syncthing checkout \"${CMAKE_CURRENT_SOURCE_DIR}/go/src/github.com/syncthing/syncthing\"."
    )
endif ()

# find Syncthing's source code (caveat: newly added files will not cause CMake to automatically regenerate)
file(
    GLOB_RECURSE SRC_FILES_SYNCTHING
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${SYNCTHING_PATH}/c-bindings/*.go"
    "${SYNCTHING_PATH}/c-bindings/*.h"
    "${SYNCTHING_PATH}/c-bindings/*.c"
    "${SYNCTHING_PATH}/cmd/syncthing/cli/*.go"
    "${SYNCTHING_PATH}/lib/*.go"
    "${SYNCTHING_PATH}/lib/*.h"
    "${SYNCTHING_PATH}/lib/*.c")
if (NOT SRC_FILES_SYNCTHING)
    message(
        FATAL_ERROR
            "No *.go files found in Syncthing checkout \"${CMAKE_CURRENT_SOURCE_DIR}/go/src/github.com/syncthing/syncthing\"."
    )
endif ()
set(GENERATED_SYNCTHING_GUI_FILES_RELATIVE_PATH "lib/api/auto/gui.files.go")
list(APPEND SRC_FILES_SYNCTHING "${SYNCTHING_PATH}/${GENERATED_SYNCTHING_GUI_FILES_RELATIVE_PATH}")
message(STATUS "Syncthing's source files: ${SRC_FILES_SYNCTHING}")

# set Go linker flags
set(GO_LINKER_FLAGS_OVERRIDE
    ""
    CACHE STRING "overrides flags passed to Go's build system via -ldflags")
set(GO_LINKER_FLAGS "${GO_LINKER_FLAGS_OVERRIDE}")
if (CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL MinSizeRel)
    set(GO_LINKER_FLAGS "${GO_LINKER_FLAGS} -w")
endif ()
if (META_SYNCTHING_VERSION)
    set(GO_LINKER_FLAGS "${GO_LINKER_FLAGS} -X github.com/syncthing/syncthing/lib/build.Version=${META_SYNCTHING_VERSION}")
endif ()
if (DEFINED $ENV{USER})
    set(GO_LINKER_FLAGS "${GO_LINKER_FLAGS} -X github.com/syncthing/syncthing/lib/build.User=$ENV{USER}")
endif ()
if (DEFINED $ENV{BUILD_HOST})
    set(BUILD_HOST $ENV{BUILD_HOST})
else ()
    cmake_host_system_information(RESULT BUILD_HOST QUERY HOSTNAME)
endif ()
if (BUILD_HOST)
    set(GO_LINKER_FLAGS "${GO_LINKER_FLAGS} -X github.com/syncthing/syncthing/lib/build.Host=${BUILD_HOST}")
endif ()
if (DEFINED $ENV{BUILD_STAMP})
    set(GO_LINKER_FLAGS "${GO_LINKER_FLAGS} -X github.com/syncthing/syncthing/lib/build.Stamp=$ENV{BUILD_STAMP}")
endif ()

# generate Syncthing's assets (not setting GOARCH/GOOS here, this is supposed to run on the host)
add_custom_command(
    OUTPUT "${SYNCTHING_PATH}/${GENERATED_SYNCTHING_GUI_FILES_RELATIVE_PATH}"
    COMMAND "GO111MODULE=on" "GOPATH=${GO_DEVELOPMENT_PATH}" "GOFLAGS=${GO_FLAGS}" "${GO_BIN}" run ./script/genassets.go gui
            > "${GENERATED_SYNCTHING_GUI_FILES_RELATIVE_PATH}"
    DEPENDS ${SRC_FILES_SYNCTHING_ASSETS}
    WORKING_DIRECTORY "${SYNCTHING_PATH}"
    COMMENT "Building Syncthing's assets")

# compile Syncthing as static library
set(SYNCTHINGINTERNAL_LIBRARY_PATH "${CMAKE_CURRENT_BINARY_DIR}/libsyncthinginternal.a")
set(SYNCTHINGINTERNAL_HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/libsyncthinginternal.h")
add_custom_command(
    OUTPUT "${SYNCTHINGINTERNAL_LIBRARY_PATH}" "${SYNCTHINGINTERNAL_HEADER_PATH}"
    COMMAND
        "CC=${CMAKE_C_COMPILER}" "CXX=${CMAKE_CXX_COMPILER}" "AR=${CMAKE_C_COMPILER_AR}" "GOOS=${GO_TARGET_OS}"
        "CGO_CFLAGS=${CGO_CFLAGS}" "CGO_CXXFLAGS=${CGO_CXXFLAGS}" "CGO_LDFLAGS=${CGO_LDFLAGS}" "GOARCH=${GO_TARGET_ARCH}"
        "CGO_ENABLED=1" "GO111MODULE=on" "GOPATH=${GO_DEVELOPMENT_PATH}" "GOFLAGS=${GO_FLAGS}" "${GO_BIN}" build -v
        -buildmode c-archive -o "${SYNCTHINGINTERNAL_LIBRARY_PATH}" -ldflags "${GO_LINKER_FLAGS}" ./c-bindings &&
        "${CMAKE_RANLIB}" "${SYNCTHINGINTERNAL_LIBRARY_PATH}"
    DEPENDS ${SRC_FILES_SYNCTHING}
    WORKING_DIRECTORY "${SYNCTHING_PATH}"
    COMMENT "Building Syncthing itself (as library)")

# add target to compile Syncthing as standalone executable (helpful for testing)
set(SYNCTHING_EXECUTABLE_PATH "${CMAKE_CURRENT_BINARY_DIR}/syncthing")
add_custom_target(
    syncthing_executable
    COMMAND
        "CC=${CMAKE_C_COMPILER}" "CXX=${CMAKE_CXX_COMPILER}" "AR=${CMAKE_C_COMPILER_AR}" "GOOS=${GO_TARGET_OS}"
        "CGO_CFLAGS=${CGO_CFLAGS}" "CGO_CXXFLAGS=${CGO_CXXFLAGS}" "CGO_LDFLAGS=${CGO_LDFLAGS}" "GOARCH=${GO_TARGET_ARCH}"
        "CGO_ENABLED=1" "GO111MODULE=on" "GOPATH=${GO_DEVELOPMENT_PATH}" "GOFLAGS=${GO_FLAGS}" "${GO_BIN}" build -v
        -buildmode exe -o "${SYNCTHING_EXECUTABLE_PATH}" -ldflags "${GO_LINKER_FLAGS}" ./cmd/syncthing
    DEPENDS ${SRC_FILES_SYNCTHING}
    WORKING_DIRECTORY "${SYNCTHING_PATH}"
    COMMENT "Building Syncthing itself (as executable, ${SYNCTHING_EXECUTABLE_PATH})")

# find c++utilities to use CMake modules and headers from it privately
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.0.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${CPP_UTILITIES_MODULE_DIRS})
if (CPP_UTILITIES_SOURCE_DIR)
    list(APPEND PRIVATE_INCLUDE_DIRS $<BUILD_INTERFACE:${CPP_UTILITIES_SOURCE_DIR}/..>
         $<INSTALL_INTERFACE:${CPP_UTILITIES_INCLUDE_DIRS}>)
else ()
    list(APPEND PRIVATE_INCLUDE_DIRS ${CPP_UTILITIES_INCLUDE_DIRS})
endif ()

# apply basic config
include(BasicConfig)

# add imported target for library generated by the Go build system
add_library(syncthinginternal STATIC IMPORTED)
set_property(TARGET syncthinginternal PROPERTY IMPORTED_LOCATION "${SYNCTHINGINTERNAL_LIBRARY_PATH}")
set_property(TARGET syncthinginternal PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${SYNCTHING_PATH}/c-bindings")

# add additional libraries (not sure if go build could provide this list somehow to make this more generic)
if (WIN32)
    set(SYNCTHING_INTERNAL_LIBS ws2_32 winmm)
elseif (UNIX)
    set_property(
        TARGET syncthinginternal
        APPEND
        PROPERTY INTERFACE_LINK_OPTIONS "-pthread")
endif ()
foreach (LIBRARY ${SYNCTHING_INTERNAL_LIBS})
    find_library(SYNCTHING_INTERNAL_LIBRARY_PATH_${LIBRARY} ${LIBRARY})
    set_property(
        TARGET syncthinginternal
        APPEND
        PROPERTY IMPORTED_LINK_INTERFACE_LIBRARIES ${SYNCTHING_INTERNAL_LIBRARY_PATH_${LIBRARY}})
endforeach ()

# depend on that imported target
list(APPEND PRIVATE_LIBRARIES $<BUILD_INTERFACE:syncthinginternal>
     $<INSTALL_INTERFACE:${LIB_INSTALL_DESTINATION}/libsyncthinginternal.a>)

# include modules to apply configuration
include(WindowsResources)
include(LibraryTarget)
include(TestTarget)
include(Doxygen)
include(ConfigHeader)

# create install target for static libsyncthinginternal.a if we're also creating a static libsyncthing.a (note: It is not
# possible to add syncthinginternal to BUNDLED_TARGETS because it is an import library which apparently can not be exported.)
if (NOT BUILD_SHARED_LIBS
    AND NOT META_NO_INSTALL_TARGETS
    AND ENABLE_INSTALL_TARGETS)
    install(
        FILES "${SYNCTHINGINTERNAL_LIBRARY_PATH}"
        DESTINATION "lib${SELECTED_LIB_SUFFIX}"
        COMPONENT binary)
endif ()

# avoid running this test in parallel with other tests spawning a Syncthing test instance
if (META_MAIN_TEST_NAME)
    set_tests_properties("${META_MAIN_TEST_NAME}" PROPERTIES RESOURCE_LOCK "syncthingtestinstance")
endif ()
