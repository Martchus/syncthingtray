cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthingmodel)
set(META_PROJECT_TYPE library)
set(META_APP_NAME "Data models of Syncthing Tray")
set(META_APP_DESCRIPTION "Data models of Syncthing Tray")
set(META_PROJECT_VARNAME_UPPER LIB_SYNCTHING_MODEL)

# add project files
set(HEADER_FILES
    syncthingdirectorymodel.h
    syncthingdevicemodel.h
    syncthingdownloadmodel.h
)
set(SRC_FILES
    syncthingdirectorymodel.cpp
    syncthingdevicemodel.cpp
    syncthingdownloadmodel.cpp
)

set(TS_FILES
    translations/${META_PROJECT_NAME}_de_DE.ts
    translations/${META_PROJECT_NAME}_en_US.ts
)

# find c++utilities
find_package(c++utilities 4.0.0 REQUIRED)
use_cpp_utilities()

# find qtutilities (only CMake modules used)
find_package(qtutilities 5.0.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${QT_UTILITIES_MODULE_DIRS})

# find backend libraries
find_package(syncthingconnector ${META_APP_VERSION} REQUIRED)
use_syncthingconnector()

# link also explicitely against the following Qt 5 modules
list(APPEND ADDITIONAL_QT_MODULES Network Gui Widgets)

# include modules to apply configuration
include(BasicConfig)
include(QtConfig)
include(WindowsResources)
include(LibraryTarget)
include(Doxygen)
include(ConfigHeader)
