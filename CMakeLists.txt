cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthingtray)
set(META_PROJECT_TYPE application)
set(META_APP_NAME "Syncthing Tray")
set(META_APP_AUTHOR "Martchus")
set(META_APP_URL "https://github.com/${META_APP_AUTHOR}/${META_PROJECT_NAME}")
set(META_APP_DESCRIPTION "Tray application for Syncthing")
set(META_APP_CATEGORIES "Utility;Network;")
set(META_GUI_OPTIONAL false)
set(META_VERSION_MAJOR 0)
set(META_VERSION_MINOR 0)
set(META_VERSION_PATCH 2)
set(META_APP_VERSION ${META_VERSION_MAJOR}.${META_VERSION_MINOR}.${META_VERSION_PATCH})

# add project files
set(HEADER_FILES
    data/syncthingconnection.h
    data/syncthingdirectorymodel.h
    data/syncthingdevicemodel.h
    data/syncthingdownloadmodel.h
    data/syncthingconfig.h
    data/syncthingprocess.h
    data/utils.h
)
set(SRC_FILES
    data/syncthingconnection.cpp
    data/syncthingdirectorymodel.cpp
    data/syncthingdevicemodel.cpp
    data/syncthingdownloadmodel.cpp
    data/syncthingconfig.cpp
    data/syncthingprocess.cpp
    data/utils.cpp
)

set(WIDGETS_HEADER_FILES
    application/settings.h
    gui/trayicon.h
    gui/traywidget.h
    gui/traymenu.h
    gui/settingsdialog.h
    gui/webpage.h
    gui/webviewdialog.h
    gui/webviewprovider.h
    gui/dirbuttonsitemdelegate.h
    gui/devbuttonsitemdelegate.h
    gui/downloaditemdelegate.h
    gui/dirview.h
    gui/devview.h
    gui/downloadview.h
    gui/textviewdialog.h
)
set(WIDGETS_SRC_FILES
    application/main.cpp
    application/settings.cpp
    gui/trayicon.cpp
    gui/traywidget.cpp
    gui/traymenu.cpp
    gui/settingsdialog.cpp
    gui/webpage.cpp
    gui/webviewdialog.cpp
    gui/dirbuttonsitemdelegate.cpp
    gui/devbuttonsitemdelegate.cpp
    gui/downloaditemdelegate.cpp
    gui/dirview.cpp
    gui/devview.cpp
    gui/downloadview.cpp
    gui/textviewdialog.cpp
    resources/icons.qrc
)
set(WIDGETS_UI_FILES
    gui/traywidget.ui
    gui/connectionoptionpage.ui
    gui/notificationsoptionpage.ui
    gui/appearanceoptionpage.ui
    gui/autostartoptionpage.ui
    gui/launcheroptionpage.ui
    gui/webviewoptionpage.ui
)

#set(QUICK_HEADER_FILES
#)
#set(QUICK_SRC_FILES
#)

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
    folder
    folder-download
    folder-open
    folder-sync
    help-about
    internet-web-browser
    list-add
    list-remove
    media-playback-pause
    media-playback-start
    network-card
    network-connect
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

# find c++utilities
find_package(c++utilities 4.0.0 REQUIRED)
use_cpp_utilities()
include(BasicConfig)

# find qtutilities
if(WIDGETS_GUI OR QUICK_GUI)
    find_package(qtutilities 5.0.0 REQUIRED)
    use_qt_utilities()
endif()

list(APPEND ADDITIONAL_QT_MODULES Network Svg)

# include modules to apply configuration
include(BasicConfig)
include(QtGuiConfig)
include(QtConfig)
include(WindowsResources)
include(WebViewProviderConfig)
include(AppTarget)
include(ShellCompletion)
include(ConfigHeader)

# create desktop file using previously defined meta data
add_desktop_file()
