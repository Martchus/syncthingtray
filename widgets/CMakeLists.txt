cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthingwidgets)
set(META_PROJECT_TYPE library)
set(META_APP_NAME "Widgets of Syncthing Tray")
set(META_PUBLIC_QT_MODULES Gui Widgets)
set(META_WEBVIEW_SRC_DIR webview)
set(META_PUBLIC_SHARED_LIB_DEPENDS syncthingconnector syncthingmodel)
set(META_PUBLIC_STATIC_LIB_DEPENDS syncthingconnector_static syncthingmodel_static)

# add project files
set(WIDGETS_HEADER_FILES
    settings/settings.h
    settings/settingsdialog.h
    webview/webpage.h
    webview/webviewdialog.h
    misc/textviewdialog.h
    misc/errorviewdialog.h
    misc/statusinfo.h
    misc/dbusstatusnotifier.h
    misc/internalerror.h
    misc/otherdialogs.h
    misc/syncthingkiller.h
)
set(WIDGETS_SRC_FILES
    settings/settings.cpp
    settings/settingsdialog.cpp
    webview/webpage.cpp
    webview/webviewdialog.cpp
    webview/webviewinterceptor.h
    webview/webviewinterceptor.cpp
    misc/textviewdialog.cpp
    misc/errorviewdialog.cpp
    misc/statusinfo.cpp
    misc/dbusstatusnotifier.cpp
    misc/internalerror.cpp
    misc/otherdialogs.cpp
    misc/syncthingkiller.cpp
)
set(RES_FILES
    resources/${META_PROJECT_NAME}icons.qrc
)
set(WIDGETS_UI_FILES
    settings/connectionoptionpage.ui
    settings/notificationsoptionpage.ui
    settings/appearanceoptionpage.ui
    settings/autostartoptionpage.ui
    settings/launcheroptionpage.ui
    settings/systemdoptionpage.ui
    settings/webviewoptionpage.ui
)

set(TS_FILES
    translations/${META_PROJECT_NAME}_de_DE.ts
    translations/${META_PROJECT_NAME}_en_US.ts
)

set(REQUIRED_ICONS
    preferences-other
    process-stop
    list-add
    preferences-desktop
    internet-web-browser
    system-run
    edit-paste
    list-remove
    preferences-desktop-notification
    view-refresh
    emblem-checked
    network-connect
    emblem-remove
    go-down
    go-up
)

# find c++utilities
find_package(c++utilities 4.6.0 REQUIRED)
use_cpp_utilities()

# find qtutilities
find_package(qtutilities 5.9.0 REQUIRED)
use_qt_utilities()

# find backend libraries
find_package(syncthingconnector ${META_APP_VERSION} REQUIRED)
use_syncthingconnector()
find_package(syncthingmodel ${META_APP_VERSION} REQUIRED)
use_syncthingmodel()

# link also explicitely against the following Qt 5 modules
list(APPEND ADDITIONAL_QT_MODULES Network)

# include modules to apply configuration
include(BasicConfig)
include(QtGuiConfig)
include(QtConfig)
include(WindowsResources)
include(WebViewProviderConfig)
include(LibraryTarget)
include(Doxygen)
include(ConfigHeader)
