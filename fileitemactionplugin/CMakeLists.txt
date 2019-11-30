# metadata
set(META_PROJECT_NAME syncthingfileitemaction)
set(META_APP_NAME "Syncthing KIO-Plugin")
set(META_APP_DESCRIPTION "KIO file item action for Syncthing")
set(META_PROJECT_TYPE qtplugin)
set(META_QT5_VERSION 5.8)

# add project files
set(HEADER_FILES syncthingmenuaction.h syncthinginfoaction.h syncthingdiractions.h syncthingfileitemactionstaticdata.h
                 syncthingfileitemaction.h)
set(SRC_FILES syncthingmenuaction.cpp syncthinginfoaction.cpp syncthingdiractions.cpp syncthingfileitemactionstaticdata.cpp
              syncthingfileitemaction.cpp)

set(TS_FILES translations/${META_PROJECT_NAME}_cs_CZ.ts translations/${META_PROJECT_NAME}_de_DE.ts
             translations/${META_PROJECT_NAME}_en_US.ts)

# find c++utilities
find_package(c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.0.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${CPP_UTILITIES_MODULE_DIRS})

# find qtutilities
find_package(qtutilities${CONFIGURATION_PACKAGE_SUFFIX} 6.0.0 REQUIRED)
use_qt_utilities()

# find backend libraries
find_package(syncthingconnector ${META_APP_VERSION} REQUIRED)
use_syncthingconnector()
find_package(syncthingmodel ${META_APP_VERSION} REQUIRED)
use_syncthingmodel()

# link also explicitely against the following Qt and KF modules
list(APPEND ADDITIONAL_QT_MODULES Network)
list(APPEND ADDITIONAL_KF_MODULES KIO)
set(KIO_MODULE_TARGETS KF5::KIOCore KF5::KIOFileWidgets KF5::KIOWidgets KF5::KIONTLM)

# include modules to apply configuration
include(BasicConfig)
include(QtGuiConfig)
include(QtConfig)
include(WindowsResources)
include(LibraryTarget)
include(ConfigHeader)

install(
    FILES ${META_PROJECT_NAME}.desktop
    DESTINATION "share/kservices5"
    COMPONENT desktop)
