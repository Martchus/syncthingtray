cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthingctl)
set(META_APP_NAME "Syncthing control")
set(META_APP_DESCRIPTION "Command line app to control Syncthing")
set(META_PROJECT_TYPE application)
set(META_JS_SRC_DIR .)

# add project files
set(HEADER_FILES
    helper.h
    args.h
    application.h
)
set(SRC_FILES
    main.cpp
    args.cpp
    application.cpp
)
set(TEST_HEADER_FILES
)
set(TEST_SRC_FILES
    tests/application.cpp
)

# find c++utilities
find_package(c++utilities 4.17.0 REQUIRED)
use_cpp_utilities()

# find qtutilities
find_package(qtutilities 5.0.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${QT_UTILITIES_MODULE_DIRS})

# find backend libraries
find_package(syncthingconnector ${META_APP_VERSION} REQUIRED)
use_syncthingconnector()

find_package(syncthingtesthelper ${META_APP_VERSION} REQUIRED)
if(SYNCTHINGTESTHELPER_HAS_SHARED_LIB)
    list(APPEND TEST_LIBRARIES syncthingtesthelper)
elseif(SYNCTHINGTESTHELPER_HAS_STATIC_LIB)
    list(APPEND TEST_LIBRARIES syncthingtesthelper_static)
else()
    message(WARNING "Unable to build tests. Testhelper not found.")
endif()

# include modules to apply configuration
include(BasicConfig)
include(JsProviderConfig)
if(JS_PROVIDER)
    list(APPEND HEADER_FILES jsconsole.h)
    list(APPEND SRC_FILES jsconsole.cpp)
    list(APPEND RES_FILES resources/js/js.qrc)
endif()
include(QtConfig)
include(WindowsResources)
include(AppTarget)
if(SYNCTHINGTESTHELPER_HAS_SHARED_LIB OR SYNCTHINGTESTHELPER_HAS_STATIC_LIB)
    include(TestTarget)
endif()
include(ShellCompletion)
include(Doxygen)
include(ConfigHeader)
