cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthingconnector)
set(META_PROJECT_TYPE library)
set(META_APP_NAME "Connection backend of Syncthing Tray")
set(META_APP_DESCRIPTION "Connection backend of Syncthing Tray")
set(META_PROJECT_VARNAME_UPPER LIB_SYNCTHING_CONNECTOR)

# add project files
set(HEADER_FILES
    syncthingdir.h
    syncthingdev.h
    syncthingconnection.h
    syncthingconnectionsettings.h
    syncthingconfig.h
    syncthingprocess.h
    utils.h
)
set(SRC_FILES
    syncthingdir.cpp
    syncthingdev.cpp
    syncthingconnection.cpp
    syncthingconnectionsettings.cpp
    syncthingconfig.cpp
    syncthingprocess.cpp
    utils.cpp
)

set(TS_FILES
    translations/${META_PROJECT_NAME}_de_DE.ts
    translations/${META_PROJECT_NAME}_en_US.ts
)

# find c++utilities
find_package(c++utilities 4.0.0 REQUIRED)
use_cpp_utilities()

# find qtutilities (only headers and CMake modules used)
find_package(qtutilities 5.0.0 REQUIRED)
include_directories(BEFORE SYSTEM ${QT_UTILITIES_INCLUDE_DIRS})
list(APPEND CMAKE_MODULE_PATH ${QT_UTILITIES_MODULE_DIRS})

# link also explicitely against the following Qt 5 modules
list(APPEND ADDITIONAL_QT_MODULES Network)

# include modules to apply configuration
include(BasicConfig)
include(QtConfig)
include(WindowsResources)
include(LibraryTarget)
include(Doxygen)
include(ConfigHeader)
