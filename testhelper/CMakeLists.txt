cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthingtesthelper)
set(META_PROJECT_TYPE library)
set(META_APP_NAME "Syncthing Tray Test Helper")
set(META_APP_DESCRIPTION "Helper for testing components of Syncthing Tray")
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

# don't allow adding a target suffix to the test library
set(SYNCTHINGTESTHELPER_CONFIGURATION_TARGET_SUFFIX none)

# find c++utilities
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.0.0 REQUIRED)
use_cpp_utilities()

# find qtutilities (only CMake modules used)
find_package(${PACKAGE_NAMESPACE_PREFIX}qtutilities${CONFIGURATION_PACKAGE_SUFFIX_QTUTILITIES} 6.0.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${QT_UTILITIES_MODULE_DIRS})

# find syncthingconnector
find_package(syncthingconnector ${META_APP_VERSION} REQUIRED)
use_syncthingconnector()

# link also explicitly against the following Qt modules
list(APPEND ADDITIONAL_QT_MODULES Network)

# include modules to apply configuration
include(BasicConfig)
include(QtConfig)
include(WindowsResources)
include(LibraryTarget)
include(TestTarget)
include(Doxygen)
include(ConfigHeader)

# exclude the target from 'all' target so it is only built when tests are built
set_target_properties("${META_TARGET_NAME}" PROPERTIES EXCLUDE_FROM_ALL ON)

# avoid running this test in parallel with other tests spawning a Syncthing test instance
if (META_MAIN_TEST_NAME)
    set_tests_properties("${META_MAIN_TEST_NAME}" PROPERTIES RESOURCE_LOCK "syncthingtestinstance")
endif ()
