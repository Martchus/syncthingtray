cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthingtesthelper)
set(META_PROJECT_TYPE library)
set(META_APP_NAME "Syncthing Tray Test Helper")
set(META_APP_DESCRIPTION "Helper for testing compontents of Syncthing Tray")
set(META_PUBLIC_QT_MODULES Core)
set(META_NO_INSTALL_TARGETS ON)
set(META_NO_CPP_UNIT ON)
set(META_TEST_TARGET_IS_MANUAL ON)
set(META_ADD_DEFAULT_CPP_UNIT_TEST_APPLICATION OFF)

# add project files
set(HEADER_FILES helper.h syncthingtestinstance.h)
set(SRC_FILES helper.cpp syncthingtestinstance.cpp)

set(TEST_HEADER_FILES)
set(TEST_SRC_FILES tests/manualtesting.cpp)

set(TS_FILES)

# find c++utilities
find_package(c++utilities 4.16.0 REQUIRED)
use_cpp_utilities()
set(META_PUBLIC_SHARED_LIB_DEPENDS c++utilities)
set(META_PUBLIC_STATIC_LIB_DEPENDS c++utilities_static)

# find qtutilities (only CMake modules used)
find_package(qtutilities 5.0.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${QT_UTILITIES_MODULE_DIRS})

# find syncthingconnector
find_package(syncthingconnector ${META_APP_VERSION} REQUIRED)
use_syncthingconnector()

# link also explicitely against the following Qt 5 modules
list(APPEND ADDITIONAL_QT_MODULES Network)

# include modules to apply configuration
include(BasicConfig)
include(QtConfig)
include(WindowsResources)
include(LibraryTarget)
include(TestTarget)
include(Doxygen)
include(ConfigHeader)

# exclude the targets from 'all' target so it is only built when tests are built
foreach (TESTHELPER_TARGET "${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}"
         "${TARGET_PREFIX}${META_PROJECT_NAME}${TARGET_SUFFIX}_static")
    if (TARGET "${TESTHELPER_TARGET}")
        set_target_properties("${TESTHELPER_TARGET}" PROPERTIES EXCLUDE_FROM_ALL ON)
    endif ()
endforeach ()
