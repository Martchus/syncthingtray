cmake_minimum_required(VERSION 3.17.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthingctl)
set(META_APP_NAME "Syncthing control")
set(META_APP_DESCRIPTION "Command line app to control ${META_SYNCTHING_NAME}")
set(META_PROJECT_TYPE application)
set(META_JS_SRC_DIR .)
set(META_TEST_REQUIRE_MAIN_TARGET ON)

# add project files
set(HEADER_FILES helper.h args.h application.h)
set(SRC_FILES main.cpp args.cpp application.cpp)
set(TEST_HEADER_FILES)
set(TEST_SRC_FILES tests/application.cpp)

# use testfiles directory from syncthingconnector
set(META_SRCDIR_REFS "${CMAKE_CURRENT_SOURCE_DIR}\n${CMAKE_CURRENT_SOURCE_DIR}/../syncthingconnector")

# find c++utilities
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.30.0 REQUIRED)
use_cpp_utilities()

# find qtutilities
find_package(${PACKAGE_NAMESPACE_PREFIX}qtutilities${CONFIGURATION_PACKAGE_SUFFIX_QTUTILITIES} 6.0.0 REQUIRED)
use_qt_utilities(ONLY_HEADERS VISIBILITY PUBLIC)

# find backend libraries
find_package(syncthingconnector ${META_APP_VERSION} REQUIRED)
use_syncthingconnector()

# find libraries used by tests
find_package(syncthingtesthelper ${META_APP_VERSION} REQUIRED)
list(APPEND TEST_LIBRARIES ${SYNCTHINGTESTHELPER_LIB})
list(APPEND PRIVATE_INCLUDE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/../testhelper/include")
use_standard_filesystem(LIBRARIES_VARIABLE "TEST_LIBRARIES" PACKAGES_VARIABLE "TEST_PACKAGES" OPTIONAL)

# include modules to apply configuration
include(BasicConfig)
include(QtJsProviderConfig)
if (JS_PROVIDER)
    list(APPEND HEADER_FILES jsconsole.h)
    list(APPEND SRC_FILES jsconsole.cpp)
    list(APPEND RES_FILES resources/js/js.qrc)
endif ()
include(QtConfig)
include(WindowsResources)
include(AppTarget)
include(TestTarget)
include(ShellCompletion)
include(Doxygen)
include(ConfigHeader)

# export target name to use syncthingctl for shortcuts in syncthingtray's desktop file
set(${META_PROJECT_VARNAME_UPPER}_TARGET_NAME
    "${META_TARGET_NAME}"
    PARENT_SCOPE)

# avoid running this test in parallel with other tests spawning a Syncthing test instance
if (META_MAIN_TEST_NAME)
    set_tests_properties("${META_MAIN_TEST_NAME}" PROPERTIES RESOURCE_LOCK "syncthingtestinstance")
endif ()
