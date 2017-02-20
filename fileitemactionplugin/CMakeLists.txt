# metadata
set(META_PROJECT_NAME syncthingfileitemaction)
set(META_APP_NAME "Syncthing KIO-Plugin")
set(META_APP_DESCRIPTION "KIO file item action for Syncthing")
set(META_PROJECT_TYPE qtplugin)

# add project files
set(HEADER_FILES
    syncthingfileitemaction.h
)
set(SRC_FILES
    syncthingfileitemaction.cpp
)

# find c++utilities
find_package(c++utilities 4.6.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${CPP_UTILITIES_MODULE_DIRS})

# find qtutilities
find_package(qtutilities 5.0.0 REQUIRED)
use_qt_utilities()
#list(APPEND CMAKE_MODULE_PATH ${QT_UTILITIES_MODULE_DIRS})

# find backend libraries
find_package(syncthingconnector ${META_APP_VERSION} REQUIRED)
use_syncthingconnector()
find_package(syncthingmodel ${META_APP_VERSION} REQUIRED)
use_syncthingmodel()

# link also explicitely against the following Qt 5 modules
list(APPEND ADDITIONAL_QT_MODULES Network)
list(APPEND ADDITIONAL_KF_MODULES KIO)

# include modules to apply configuration
include(BasicConfig)
include(QtGuiConfig)
include(QtConfig)
include(WindowsResources)
# KIO libs can not be determined automatically
list(REMOVE_ITEM PRIVATE_LIBRARIES KF5::KIO)
list(APPEND PRIVATE_LIBRARIES KF5::KIOCore KF5::KIOFileWidgets KF5::KIOWidgets KF5::KIONTLM)
include(LibraryTarget)
include(ConfigHeader)
