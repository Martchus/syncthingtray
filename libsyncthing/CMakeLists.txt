cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthing)
set(META_PROJECT_TYPE library)
set(META_APP_NAME "Syncthing library")
set(META_APP_DESCRIPTION "Syncthing itself, built as a shared or static library")
set(META_PROJECT_VARNAME_UPPER LIB_SYNCTHING)

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
set(GO_TARGET_ARCH_OVERRIDE "" CACHE STRING "overrides the 'GOARCH' variable")
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
set(GO_TARGET_OS_OVERRIDE "" CACHE STRING "overrides the 'GOOS' variable")
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

# locate the Syncthing checkout
set(GO_DEVELOPMENT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/go" CACHE PATH "the 'GOPATH'")
set(SYNCTHING_PATH "${CMAKE_CURRENT_SOURCE_DIR}/go/src/github.com/syncthing/syncthing"
    CACHE PATH "path of Syncthing checkout")

# find Syncthing's assets caveat: newly added assets will not cause CMake to automatically regenerate
file(GLOB_RECURSE SRC_FILES_SYNCTHING_ASSETS LIST_DIRECTORIES false
     RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${SYNCTHING_PATH}/gui/*")
if (NOT SRC_FILES_SYNCTHING_ASSETS)
    message(
        FATAL_ERROR
            "No asset files found in Syncthing checkout \"${CMAKE_CURRENT_SOURCE_DIR}/go/src/github.com/syncthing/syncthing\"."
        )
endif ()

# find Syncthing's source code caveat: newly added files will not cause CMake to automatically regenerate
file(GLOB_RECURSE SRC_FILES_SYNCTHING LIST_DIRECTORIES false
     RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
              "${SYNCTHING_PATH}/cmd/*.go"
              "${SYNCTHING_PATH}/cmd/*.h"
              "${SYNCTHING_PATH}/cmd/*.c")
if (NOT SRC_FILES_SYNCTHING)
    message(
        FATAL_ERROR
            "No *.go files found in Syncthing checkout \"${CMAKE_CURRENT_SOURCE_DIR}/go/src/github.com/syncthing/syncthing\"."
        )
endif ()
list(APPEND SRC_FILES_SYNCTHING "${SYNCTHING_PATH}/lib/auto/gui.files.go")
message(STATUS "Syncthing's go files: ${SRC_FILES_SYNCTHING}")

# generate Syncthing's assets: don't set GOARCH/GOOS here, this is supposed to run on the host
add_custom_command(OUTPUT "${SYNCTHING_PATH}/lib/auto/gui.files.go"
                   COMMAND "GOPATH=${GO_DEVELOPMENT_PATH}" "${GO_BIN}" run ./script/genassets.go gui > lib/auto/gui.files.go
                   DEPENDS ${SRC_FILES_SYNCTHING_ASSETS}
                   WORKING_DIRECTORY "${SYNCTHING_PATH}"
                   COMMENT "Building Syncthing's assets")

# compile Syncthing as static library
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/libsyncthinginternal.a"
                          "${CMAKE_CURRENT_BINARY_DIR}/libsyncthinginternal.h"
                   COMMAND "CC=${CMAKE_C_COMPILER}" "CXX=${CMAKE_CXX_COMPILER}" "AR=${CMAKE_C_COMPILER_AR}"
                           "GOOS=${GO_TARGET_OS}" "GOARCH=${GO_TARGET_ARCH}" "CGO_ENABLED=1" "GOPATH=${GO_DEVELOPMENT_PATH}"
                           "${GO_BIN}" build -v -buildmode c-archive -o "${CMAKE_CURRENT_BINARY_DIR}/libsyncthinginternal.a"
                           ./cmd/syncthing && "${CMAKE_RANLIB}" "${CMAKE_CURRENT_BINARY_DIR}/libsyncthinginternal.a"
                   DEPENDS ${SRC_FILES_SYNCTHING}
                   WORKING_DIRECTORY "${SYNCTHING_PATH}"
                   COMMENT "Building Syncthing itself")

# do not use this library directly but depend on it
list(APPEND PRIVATE_LIBRARIES "${CMAKE_CURRENT_BINARY_DIR}/libsyncthinginternal.a")
# add additional libraries (not sure if go build could provide this list somehow to make this more generic)
if (WIN32)
    list(APPEND PRIVATE_LIBRARIES -lws2_32 -lwinmm)
elseif (UNIX)
    list(APPEND PRIVATE_LIBRARIES -lpthread)
endif ()

# ensure we can find libsyncthing.h from Syncthing's source tree when building the interface
list(APPEND PRIVATE_INCLUDE_DIRS "${SYNCTHING_PATH}/cmd/syncthing")

# find c++utilities
find_package(c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.0.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${CPP_UTILITIES_MODULE_DIRS})
list(APPEND PRIVATE_SHARED_INCLUDE_DIRS ${CPP_UTILITIES_INCLUDE_DIRS})
list(APPEND PRIVATE_STATIC_INCLUDE_DIRS ${CPP_UTILITIES_INCLUDE_DIRS})

# include modules to apply configuration
include(BasicConfig)
include(WindowsResources)
include(LibraryTarget)
include(TestTarget)
include(Doxygen)
include(ConfigHeader)

# create install target for static libsyncthinginternal.a if we're also creating a static libsyncthing.a
if (BUILD_STATIC_LIBS AND NOT META_NO_INSTALL_TARGETS AND ENABLE_INSTALL_TARGETS)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libsyncthinginternal.a"
            DESTINATION "lib${SELECTED_LIB_SUFFIX}"
            COMPONENT binary)
endif ()
