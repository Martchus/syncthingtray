cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthingctl)
set(META_APP_NAME "Syncthing control")
set(META_APP_DESCRIPTION "Command line app to control Syncthing")
set(META_PROJECT_TYPE application)
set(META_GUI_OPTIONAL false)

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

# find c++utilities
find_package(c++utilities 4.7.0 REQUIRED)
use_cpp_utilities()

# find qtutilities
find_package(qtutilities 5.0.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${QT_UTILITIES_MODULE_DIRS})

# find backend libraries
find_package(syncthingconnector ${META_APP_VERSION} REQUIRED)
use_syncthingconnector()

# include modules to apply configuration
include(BasicConfig)
include(QtConfig)
include(WindowsResources)
include(AppTarget)
include(ShellCompletion)
include(Doxygen)
include(ConfigHeader)
