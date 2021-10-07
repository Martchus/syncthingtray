cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthingmodel)
set(META_PROJECT_TYPE library)
set(META_APP_NAME "Data models of Syncthing Tray")
set(META_APP_DESCRIPTION "Data models of Syncthing Tray")
set(META_PROJECT_VARNAME_UPPER LIB_SYNCTHING_MODEL)
set(META_PUBLIC_QT_MODULES Gui Widgets)

# add project files
set(HEADER_FILES
    syncthingmodel.h
    syncthingdirectorymodel.h
    syncthingdevicemodel.h
    syncthingdownloadmodel.h
    syncthingrecentchangesmodel.h
    syncthingsortfiltermodel.h
    syncthingstatuscomputionmodel.h
    syncthingstatusselectionmodel.h
    syncthingicons.h
    colors.h)
set(SRC_FILES
    syncthingmodel.cpp
    syncthingdirectorymodel.cpp
    syncthingdevicemodel.cpp
    syncthingdownloadmodel.cpp
    syncthingrecentchangesmodel.cpp
    syncthingsortfiltermodel.cpp
    syncthingstatuscomputionmodel.cpp
    syncthingstatusselectionmodel.cpp
    syncthingicons.cpp)
set(RES_FILES resources/${META_PROJECT_NAME}icons.qrc)

set(TS_FILES translations/${META_PROJECT_NAME}_zh_CN.ts translations/${META_PROJECT_NAME}_cs_CZ.ts
             translations/${META_PROJECT_NAME}_de_DE.ts translations/${META_PROJECT_NAME}_en_US.ts)

# find c++utilities
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.0.0 REQUIRED)
use_cpp_utilities()

# find qtutilities (only CMake modules used)
find_package(${PACKAGE_NAMESPACE_PREFIX}qtutilities${CONFIGURATION_PACKAGE_SUFFIX_QTUTILITIES} 6.3.0 REQUIRED)
use_qt_utilities()

# find backend libraries
find_package(syncthingconnector ${META_APP_VERSION} REQUIRED)
use_syncthingconnector(VISIBILITY PUBLIC)

# find qtforkawesome
find_package(${PACKAGE_NAMESPACE_PREFIX}qtforkawesome${CONFIGURATION_PACKAGE_SUFFIX_QTFORKAWESOME} 0.0.1 REQUIRED)
use_qt_fork_awesome(VISIBILITY PUBLIC)

# link also explicitly against the following Qt modules
list(APPEND ADDITIONAL_QT_MODULES Network Gui Widgets Svg)

# include modules to apply configuration
include(BasicConfig)
include(QtConfig)
include(WindowsResources)
include(LibraryTarget)
include(Doxygen)
include(ConfigHeader)
