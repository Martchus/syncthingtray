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
)
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
)
set(RES_FILES
    resources/${META_PROJECT_NAME}icons.qrc
)
set(WIDGETS_UI_FILES
    gui/traywidget.ui
)

set(TS_FILES
    translations/${META_PROJECT_NAME}_de_DE.ts
    translations/${META_PROJECT_NAME}_en_US.ts
)

set(ICON_FILES
    resources/icons/hicolor/scalable/app/${META_PROJECT_NAME}.svg
)
set(PNG_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/hicolor/scalable/app/${META_PROJECT_NAME}.png")
set(PNG_ICON_NO_CROP ON)

set(DOC_FILES
    README.md
)

set(REQUIRED_ICONS
    dialog-cancel
    dialog-ok
    dialog-ok-apply
    edit-copy
    edit-paste
    emblem-error
    folder
    folder-download
    folder-open
    folder-sync
    go-down
    go-up
    help-about
    internet-web-browser
    list-add
    list-remove
    media-playback-pause
    media-playback-start
    network-card
    network-connect
    network-disconnect
    network-server
    preferences-desktop
    preferences-desktop-icons
    preferences-desktop-locale
    preferences-desktop-notification
    preferences-other
    qtcreator
    system-run
    system-search
    system-file-manager
    text-plain
    view-barcode
    view-refresh
    window-close
)
# fallback icons can not replace built-in Breeze icons (yet)
#set(FALLBACK_ICON_THEME breeze)

# find c++utilities
find_package(c++utilities 4.11.0 REQUIRED)
use_cpp_utilities()

# find qtutilities
find_package(qtutilities 5.6.0 REQUIRED)
use_qt_utilities()

# find backend libraries
find_package(syncthingconnector ${META_APP_VERSION} REQUIRED)
use_syncthingconnector()
find_package(syncthingmodel ${META_APP_VERSION} REQUIRED)
use_syncthingmodel()
find_package(syncthingwidgets ${META_APP_VERSION} REQUIRED)
use_syncthingwidgets()

# link also explicitely against the following Qt 5 modules
list(APPEND ADDITIONAL_QT_MODULES Network)

# include modules to apply configuration
include(BasicConfig)
include(QtGuiConfig)
include(QtConfig)
include(WindowsResources)
include(AppTarget)
include(ShellCompletion)
include(Doxygen)
include(ConfigHeader)

# create desktop file using previously defined meta data
add_desktop_file()
