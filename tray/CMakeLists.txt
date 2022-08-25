cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# metadata
set(META_PROJECT_TYPE application)
set(META_APP_NAME "Syncthing Tray")
set(META_APP_ACTIONS OPEN_WEBUI)
set(META_APP_ACTION_OPEN_WEBUI "open-webui" "Open web UI" "--webui")

# add project files
set(WIDGETS_HEADER_FILES
    application/singleinstance.h
    gui/trayicon.h
    gui/traywidget.h
    gui/traymenu.h
    gui/dirbuttonsitemdelegate.h
    gui/devbuttonsitemdelegate.h
    gui/downloaditemdelegate.h
    gui/dirview.h
    gui/devview.h
    gui/downloadview.h
    gui/helper.h)
set(WIDGETS_SRC_FILES
    application/main.cpp
    application/singleinstance.cpp
    gui/trayicon.cpp
    gui/traywidget.cpp
    gui/traymenu.cpp
    gui/dirbuttonsitemdelegate.cpp
    gui/devbuttonsitemdelegate.cpp
    gui/downloaditemdelegate.cpp
    gui/dirview.cpp
    gui/devview.cpp
    gui/downloadview.cpp
    gui/helper.cpp)
set(RES_FILES resources/${META_PROJECT_NAME}icons.qrc)
set(WIDGETS_UI_FILES gui/traywidget.ui)

set(TS_FILES translations/${META_PROJECT_NAME}_zh_CN.ts translations/${META_PROJECT_NAME}_cs_CZ.ts
             translations/${META_PROJECT_NAME}_de_DE.ts translations/${META_PROJECT_NAME}_en_US.ts)

set(ICON_FILES resources/icons/hicolor/scalable/apps/${META_PROJECT_NAME}.svg)

set(DOC_FILES README.md)

# declare required icons; used when bundling icon themes (Icons required by libraries the tray application depends on need to
# be specified as well.)
set(REQUIRED_ICONS
    color-profile
    dialog-cancel
    dialog-ok
    dialog-ok-apply
    document-open
    edit-copy
    edit-clear
    edit-cut
    edit-delete
    edit-paste
    edit-redo
    edit-undo
    emblem-checked
    emblem-error
    emblem-remove
    folder
    folder-download
    folder-open
    folder-sync
    go-down
    go-up
    help-about
    help-contents
    internet-web-browser
    list-add
    list-remove
    media-playback-pause
    media-playback-start
    network-connect
    network-disconnect
    network-server
    preferences-desktop
    preferences-desktop-icons
    preferences-desktop-locale
    preferences-desktop-notification
    preferences-system-startup
    preferences-system-startup
    preferences-system-services
    preferences-other
    process-stop
    qtcreator
    system-run
    system-search
    system-file-manager
    text-x-generic
    tools-wizard
    view-refresh
    window-close)

# find c++utilities
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.19.0 REQUIRED)
use_cpp_utilities()

# find qtutilities
find_package(${PACKAGE_NAMESPACE_PREFIX}qtutilities${CONFIGURATION_PACKAGE_SUFFIX_QTUTILITIES} 6.7.0 REQUIRED)
use_qt_utilities()

# find backend libraries
find_package(syncthingconnector ${META_APP_VERSION} REQUIRED)
use_syncthingconnector()
find_package(syncthingmodel ${META_APP_VERSION} REQUIRED)
use_syncthingmodel()
find_package(syncthingwidgets ${META_APP_VERSION} REQUIRED)
use_syncthingwidgets()

# link against the qtforkawesomeiconengine plugin when it is a static library
include(3rdParty)
find_package(${PACKAGE_NAMESPACE_PREFIX}qtforkawesomeiconengine${CONFIGURATION_PACKAGE_SUFFIX_QTFORKAWESOME} 0.0.1 REQUIRED)
if (NOT QT_FORK_AWESOME_ICON_ENGINE_LIB_IS_SHARED)
    use_qt_fork_awesome_icon_engine()
endif ()

# link also explicitly against the following Qt modules
list(APPEND ADDITIONAL_QT_MODULES Network)

# configure libsyncthing
option(USE_LIBSYNCTHING "whether libsyncthing should be included for syncthingtray's CLI" OFF)
if (USE_LIBSYNCTHING)
    find_package(syncthing ${META_APP_VERSION} REQUIRED)
    use_syncthing()
    list(APPEND META_PUBLIC_COMPILE_DEFINITIONS SYNCTHINGTRAY_USE_LIBSYNCTHING)
endif ()

# apply basic configuration
include(BasicConfig)

# add an option to unify left- and right-click context menus useful on Mac OS
if (APPLE)
    set(UNIFY_TRAY_MENUS_BY_DEFAULT ON)
else ()
    set(UNIFY_TRAY_MENUS_BY_DEFAULT OFF)
endif ()
option(UNIFY_TRAY_MENUS "unifies the left- and right-click tray menus" ${UNIFY_TRAY_MENUS_BY_DEFAULT})
if (UNIFY_TRAY_MENUS)
    list(APPEND META_PUBLIC_COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_UNIFY_TRAY_MENUS)
    message(STATUS "left- and right-click context menus will be unified")
endif ()

# include modules to apply configuration
include(QtGuiConfig)
include(QtConfig)
include(WindowsResources)
include(AppTarget)
include(ShellCompletion)
include(Doxygen)
include(ConfigHeader)

# create desktop file using previously defined meta data
add_desktop_file()

if (APPLE)
    set_target_properties(${META_TARGET_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST
                                                         "${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist.in")
endif ()
