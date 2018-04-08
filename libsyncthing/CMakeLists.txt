cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthing)
set(META_PROJECT_TYPE library)
set(META_APP_NAME "Syncthing library")
set(META_APP_DESCRIPTION "Syncthing itself, built as a shared or static library")
set(META_PROJECT_VARNAME_UPPER LIB_SYNCTHING)

# add project files
set(HEADER_FILES
    interface.h
)
set(SRC_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libsyncthinginternal.h"
    interface.cpp
)

# find the go binary
find_program(GO_BIN go)
if(NOT GO_BIN)
    message(FATAL_ERROR "The go binary could not be located.")
endif()

# locate the Syncthing checkout
set(GO_DEVELOPMENT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/go" CACHE STRING "the 'GOPATH'")
set(SYNCTHING_PATH "${CMAKE_CURRENT_SOURCE_DIR}/go/src/github.com/syncthing/syncthing" CACHE STRING "path of Syncthing checkout")

# find Syncthing's assets
file(GLOB_RECURSE SRC_FILES_SYNCTHING_ASSETS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${SYNCTHING_PATH}/gui/*"
)
if(NOT SRC_FILES_SYNCTHING_ASSETS)
    message(FATAL_ERROR "No asset files found in Syncthing checkout \"${CMAKE_CURRENT_SOURCE_DIR}/go/src/github.com/syncthing/syncthing\".")
endif()

# find Syncthing's source code
file(GLOB_RECURSE SRC_FILES_SYNCTHING
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${SYNCTHING_PATH}/cmd/*.go"
)
if(NOT SRC_FILES_SYNCTHING)
    message(FATAL_ERROR "No *.go files found in Syncthing checkout \"${CMAKE_CURRENT_SOURCE_DIR}/go/src/github.com/syncthing/syncthing\".")
endif()
list(APPEND SRC_FILES_SYNCTHING "${SYNCTHING_PATH}/lib/auto/gui.files.go")
message(STATUS "Syncthing's go files: ${SRC_FILES_SYNCTHING}")

# generate Syncthing's assets
add_custom_command(
    OUTPUT
        "${SYNCTHING_PATH}/lib/auto/gui.files.go"
    COMMAND
        "GOPATH=${GO_DEVELOPMENT_PATH}"
        "${GO_BIN}" run ./script/genassets.go gui > lib/auto/gui.files.go
    DEPENDS ${SRC_FILES_SYNCTHING_ASSETS}
    WORKING_DIRECTORY "${SYNCTHING_PATH}"
    COMMENT "Building Syncthing's assets"
)

# compile Syncthing as static library
# FIXME: handle cross-compilation
add_custom_command(
    OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/libsyncthinginternal.a"
        "${CMAKE_CURRENT_BINARY_DIR}/libsyncthinginternal.h"
    COMMAND
        "GOPATH=${GO_DEVELOPMENT_PATH}"
        "${GO_BIN}" build -v -buildmode c-archive
        -o "${CMAKE_CURRENT_BINARY_DIR}/libsyncthinginternal.a"
        ./cmd/syncthing
    DEPENDS ${SRC_FILES_SYNCTHING}
    WORKING_DIRECTORY "${SYNCTHING_PATH}"
    COMMENT "Building Syncthing itself"
)

# do not use this library directly but depend on it
list(APPEND PRIVATE_LIBRARIES "${CMAKE_CURRENT_BINARY_DIR}/libsyncthinginternal.a")

# find c++utilities
find_package(c++utilities 4.9.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${CPP_UTILITIES_MODULE_DIRS})
list(APPEND PRIVATE_SHARED_INCLUDE_DIRS ${CPP_UTILITIES_INCLUDE_DIRS})
list(APPEND PRIVATE_STATIC_INCLUDE_DIRS ${CPP_UTILITIES_INCLUDE_DIRS})

# include modules to apply configuration
include(BasicConfig)
include(WindowsResources)
include(LibraryTarget)
include(ConfigHeader)

# create install target for static libsyncthinginternal.a if we're also creating a static libsyncthing.a
if(BUILD_STATIC_LIBS AND NOT META_NO_INSTALL_TARGETS AND ENABLE_INSTALL_TARGETS)
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/libsyncthinginternal.a"
        DESTINATION "lib${SELECTED_LIB_SUFFIX}"
        COMPONENT binary
    )
endif()
