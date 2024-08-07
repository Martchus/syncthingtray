cmake_minimum_required(VERSION 3.17.0 FATAL_ERROR)

# metadata
set(META_PROJECT_TYPE application)
set(META_APP_NAME "Syncthing Tray")

# use testfiles directory from syncthingconnector
set(META_SRCDIR_REFS "${CMAKE_CURRENT_SOURCE_DIR}\n${CMAKE_CURRENT_SOURCE_DIR}/../syncthingconnector")

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
    document-edit
    document-open
    document-open-remote
    download
    edit-copy
    edit-clear
    edit-cut
    edit-delete
    edit-paste
    edit-redo
    edit-select
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
    question
    qtcreator
    system-run
    system-search
    system-file-manager
    text-x-generic
    quickwizard
    view-refresh
    window-close
    window-pin)

# find c++utilities
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.21.0 REQUIRED)
use_cpp_utilities()

# find qtutilities
find_package(${PACKAGE_NAMESPACE_PREFIX}qtutilities${CONFIGURATION_PACKAGE_SUFFIX_QTUTILITIES} 6.13.0 REQUIRED)
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
find_package(${PACKAGE_NAMESPACE_PREFIX}qtforkawesomeiconengine${CONFIGURATION_PACKAGE_SUFFIX_QTFORKAWESOME} 0.1.0 REQUIRED)
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

# add shortcuts/actions to desktop file
set(ACTIONS
    "Actions=ShowTrayMenu;ShowSyncthing;RescanAll;PauseAllDevs;ResumeAllDevs;Restart\n\n\
[Desktop Action ShowTrayMenu]\n\
Name=Show tray menu\n\
Exec=${META_TARGET_NAME} qt-widgets-gui --trigger\n\
[Desktop Action ShowSyncthing]\n\
Name=Show Syncthing (currently selected instance)\n\
Exec=${META_TARGET_NAME} qt-widgets-gui --webui\n\
[Desktop Action RescanAll]\n\
Name=Rescan all folders (of local Syncthing instance)\n\
Exec=${SYNCTHINGCTL_TARGET_NAME} rescan-all\n\
[Desktop Action PauseAllDevs]\n\
Name=Pause all devices (of local Syncthing instance)\n\
Exec=${SYNCTHINGCTL_TARGET_NAME} pause --all-devs\n\
[Desktop Action ResumeAllDevs]\n\
Name=Resume all devices (of local Syncthing instance)\n\
Exec=${SYNCTHINGCTL_TARGET_NAME} resume --all-devs\n\
[Desktop Action Restart]\n\
Name=Restart Syncthing (local instance)\n\
Exec=${SYNCTHINGCTL_TARGET_NAME} restart")
set(DESKTOP_FILE_ADDITIONAL_ENTRIES "${DESKTOP_FILE_ADDITIONAL_ENTRIES}${ACTIONS}\n")

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

# configure macOS bundle
if (APPLE)
    set_target_properties(${META_TARGET_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST
                                                         "${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist.in")
endif ()

# configure creating an Android package using androiddeployqt
if (ANDROID)
    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    set_target_properties(${META_TARGET_NAME} PROPERTIES QT_ANDROID_PACKAGE_SOURCE_DIR "${ANDROID_PACKAGE_SOURCE_DIR}")

    set(ANDROID_MANIFEST_PATH "${ANDROID_PACKAGE_SOURCE_DIR}/AndroidManifest.xml")
    configure_file("resources/AndroidManifest.xml.in" "${ANDROID_MANIFEST_PATH}")

    find_package(OpenSSL)
    if (OPENSSL_FOUND)
        # link the OpenSSL libraries into the build directory so they have the names that the Qt framework expects
        set(OPENSSL_CRYPTO_LIBRARY_LINK "${CMAKE_CURRENT_BINARY_DIR}/libcrypto_3.so")
        set(OPENSSL_SSL_LIBRARY_LINK "${CMAKE_CURRENT_BINARY_DIR}/libssl_3.so")
        file(CREATE_LINK "${OPENSSL_CRYPTO_LIBRARY}" "${OPENSSL_CRYPTO_LIBRARY_LINK}" COPY_ON_ERROR SYMBOLIC)
        file(CREATE_LINK "${OPENSSL_SSL_LIBRARY}" "${OPENSSL_SSL_LIBRARY_LINK}" COPY_ON_ERROR SYMBOLIC)
        set_target_properties(${META_TARGET_NAME} PROPERTIES QT_ANDROID_EXTRA_LIBS
                                                             "${OPENSSL_CRYPTO_LIBRARY_LINK};${OPENSSL_SSL_LIBRARY_LINK}")
    else ()
        message(WARNING "Unable to find OpenSSL libraries. HTTPS will not be supported.")
    endif ()

    set(QT_ANDROID_SIGN_APK ON)
    qt_android_generate_deployment_settings(${META_TARGET_NAME})
    qt_android_add_apk_target(${META_TARGET_NAME})
endif ()
