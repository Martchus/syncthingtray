# metadata
set(META_PROJECT_NAME syncthingfileitemaction)
set(META_APP_NAME "Syncthing KIO-Plugin")
set(META_APP_DESCRIPTION "KIO file item action for ${META_SYNCTHING_NAME}")
set(META_PROJECT_TYPE qtplugin)
set(META_QT5_VERSION 5.8)

# add project files
set(HEADER_FILES syncthingmenuaction.h syncthinginfoaction.h syncthingdiractions.h syncthingfileitemactionstaticdata.h
                 syncthingfileitemaction.h)
set(SRC_FILES syncthingmenuaction.cpp syncthinginfoaction.cpp syncthingdiractions.cpp syncthingfileitemactionstaticdata.cpp
              syncthingfileitemaction.cpp)

set(TS_FILES translations/${META_PROJECT_NAME}_zh_CN.ts translations/${META_PROJECT_NAME}_cs_CZ.ts
             translations/${META_PROJECT_NAME}_de_DE.ts translations/${META_PROJECT_NAME}_en_US.ts)

# find c++utilities
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.20.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${CPP_UTILITIES_MODULE_DIRS})

# find qtutilities
find_package(${PACKAGE_NAMESPACE_PREFIX}qtutilities${CONFIGURATION_PACKAGE_SUFFIX_QTUTILITIES} 6.7.0 REQUIRED)
use_qt_utilities()

# find backend libraries
find_package(syncthingconnector ${META_APP_VERSION} REQUIRED)
use_syncthingconnector()
find_package(syncthingmodel ${META_APP_VERSION} REQUIRED)
use_syncthingmodel()

# link also explicitly against the following Qt and KF modules
list(APPEND ADDITIONAL_QT_MODULES Network)
list(APPEND ADDITIONAL_KF_MODULES KIO)
set(KF_PACKAGE_PREFIX
    "KF5"
    CACHE STRING "specifies the prefix for KDE Frameworks packages")
set(KIO_MODULE_TARGETS "${KF_PACKAGE_PREFIX}::KIOCore" "${KF_PACKAGE_PREFIX}::KIOFileWidgets"
                       "${KF_PACKAGE_PREFIX}::KIOWidgets")
if (KF_MAJOR_VERSION GREATER_EQUAL 6)
    list(APPEND KIO_MODULE_TARGETS)
    set(META_PLUGIN_CATEGORY kf${KF_MAJOR_VERSION}/kfileitemaction)
else ()
    list(APPEND KIO_MODULE_TARGETS "${KF_PACKAGE_PREFIX}::KIONTLM")
endif ()

# include modules to apply configuration
include(BasicConfig)
include(QtGuiConfig)
include(QtConfig)
include(WindowsResources)
include(LibraryTarget)
include(ConfigHeader)
include(AppUtilities)

# configure and install desktop file
include(TemplateFinder)
if (KF_MAJOR_VERSION LESS 6)
    find_template_file("${META_PROJECT_NAME}.desktop" "${META_PROJECT_NAME}" DESKTOP_TEMPLATE_FILE)
    set(DESKTOP_TARGET_FILE "${CMAKE_CURRENT_BINARY_DIR}/${META_TARGET_NAME}.desktop")
    configure_file("${DESKTOP_TEMPLATE_FILE}" "${DESKTOP_TARGET_FILE}")
    install(
        FILES "${DESKTOP_TARGET_FILE}"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/kservices${KF_MAJOR_VERSION}"
        COMPONENT desktop)
else ()
    find_template_file("${META_PROJECT_NAME}.json" "${META_PROJECT_NAME}" JSON_TEMPLATE_FILE)
    set(JSON_TARGET_FILE "${CMAKE_CURRENT_BINARY_DIR}/metadata.json")
    configure_file("${JSON_TEMPLATE_FILE}" "${JSON_TARGET_FILE}")
endif ()

# install appstream file
add_appstream_file()
