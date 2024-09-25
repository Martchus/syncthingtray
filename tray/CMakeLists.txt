cmake_minimum_required(VERSION 3.17.0 FATAL_ERROR)

# metadata
set(META_PROJECT_TYPE application)
set(META_APP_NAME "Syncthing Tray")
set(META_APP_NAME_QUICK_GUI "Syncthing App")
set(META_HAS_QUICK_GUI ON)
set(META_QUICK_GUI_OPTIONAL ON)
set(META_USE_QQC2 ON)

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
set(QML_HEADER_FILES gui/app.h gui/quick/scenegraph/managedtexturenode.h)
set(QML_SRC_FILES gui/app.cpp gui/quick/scenegraph/managedtexturenode.cpp)

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

if (QUICK_GUI)
    find_package(${PACKAGE_NAMESPACE_PREFIX}qtquickforkawesome${CONFIGURATION_PACKAGE_SUFFIX_QTFORKAWESOME} 0.1.0 REQUIRED)
    use_qt_quick_fork_awesome()
endif ()

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

# configure Qt Quick GUI
include(QtGuiConfig)
if (QUICK_GUI)
    use_standard_filesystem()

    list(APPEND ADDITIONAL_QT_MODULES Svg)
    list(APPEND ADDITIONAL_QT_REPOS svg)
    list(APPEND PRIVATE_INCLUDE_DIRS gui/quick)

    # check whether Qt WebView is present
    include(QtLinkage)
    unset(WEBVIEWITEM_QML_FILES)
    option(QUICK_GUI_NO_WEBVIEW "disables web view support in the Qt Quick GUI" OFF)
    if (NOT QUICK_GUI_NO_WEBVIEW)
        find_package("${QT_PACKAGE_PREFIX}WebView" "${META_QT_VERSION}")
        find_package("${QT_PACKAGE_PREFIX}WebViewQuick" "${META_QT_VERSION}")
        if ("${${QT_PACKAGE_PREFIX}WebView_FOUND}" AND "${${QT_PACKAGE_PREFIX}WebViewQuick_FOUND}")
            list(APPEND ADDITIONAL_QT_MODULES WebView WebViewQuick)
            list(APPEND ADDITIONAL_QT_REPOS webview)
            list(APPEND WEBVIEWITEM_QML_FILES gui/qml/webview-webview/WebViewItem.qml)
            set_property(
                SOURCE application/main.cpp
                APPEND
                PROPERTY COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_HAS_WEBVIEW)
        endif ()
    endif ()
    if (NOT WEBVIEWITEM_QML_FILES)
        list(APPEND WEBVIEWITEM_QML_FILES gui/qml/webview-none/WebViewItem.qml)
    endif ()

    # find Qt Qml and enable automoc which is required for the subsequent qt_add_qml_module calls
    find_package("${QT_PACKAGE_PREFIX}Qml" "${META_QT_VERSION}")
    set(CMAKE_AUTOMOC ON)

    # add additional module for the web view
    qt_add_qml_module(
        ${META_TARGET_NAME}-webviewitem
        URI
        "WebViewItem"
        VERSION
        1.0
        STATIC
        QML_FILES
        ${WEBVIEWITEM_QML_FILES}
        RESOURCE_PREFIX
        "/qt/qml/"
        OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/WebViewItem"
        NO_PLUGIN_OPTIONAL)
    list(APPEND PRIVATE_LIBRARIES ${META_TARGET_NAME}-webviewitemplugin)
endif ()

# include further modules to apply configuration
include(QtConfig)
include(WindowsResources)
include(AppTarget)
include(ShellCompletion)
include(Doxygen)
include(ConfigHeader)

# deduce major Qt version from package prefix
if (NOT QT_PACKAGE_PREFIX)
    set(MAJOR_QT_VERSION "5")
elseif (QT_PACKAGE_PREFIX MATCHES ".*Qt([0-9]+).*")
    set(MAJOR_QT_VERSION "${CMAKE_MATCH_1}")
endif ()

# require Qt 6 for the Qt Quick GUI
if (QUICK_GUI AND (MAJOR_QT_VERSION VERSION_LESS 6 OR MAJOR_QT_VERSION VERSION_GREATER_EQUAL 7))
    message(FATAL_ERROR "The Qt Quick GUI is only compatible with Qt 6 (but Qt ${MAJOR_QT_VERSION} was found).")
endif ()

# add Qml module
if (QUICK_GUI)
    qt_add_qml_module(
        ${META_TARGET_NAME}
        URI
        "Main"
        VERSION
        1.0
        NO_PLUGIN
        QML_FILES
        gui/qml/AboutDialog.qml
        gui/qml/AdvancedConfigPage.qml
        gui/qml/AdvancedDevConfigPage.qml
        gui/qml/AdvancedDirConfigPage.qml
        gui/qml/AdvancedPage.qml
        gui/qml/ArrayElementButtons.qml
        gui/qml/ChangesPage.qml
        gui/qml/DetailsListView.qml
        gui/qml/DevDelegate.qml
        gui/qml/DevListView.qml
        gui/qml/DevsPage.qml
        gui/qml/DirDelegate.qml
        gui/qml/DirListView.qml
        gui/qml/DirErrorsPage.qml
        gui/qml/DirsPage.qml
        gui/qml/ExpandableDelegate.qml
        gui/qml/ExpandableItemDelegate.qml
        gui/qml/ExpandableListView.qml
        gui/qml/FilesPage.qml
        gui/qml/HelpButton.qml
        gui/qml/IgnorePatternPage.qml
        gui/qml/Main.qml
        gui/qml/ObjectConfigPage.qml
        gui/qml/SettingsPage.qml
        gui/qml/WebViewPage.qml
        SOURCES
        gui/quick/quickicon.h
        gui/quick/quickicon.cpp
        RESOURCE_PREFIX
        "/qt/qml/")
endif ()

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
    option(FORCE_WIDGETS_GUI_ON_ANDOROID "forces using the Qt Widgets GUI on Android" OFF)
    if (QUICK_GUI AND NOT FORCE_WIDGETS_GUI_ON_ANDOROID)
        set(ANDROID_APPLICATION_ARGUMENTS "qt-quick-gui --insecure")
    else ()
        set(ANDROID_APPLICATION_ARGUMENTS "qt-widgets-gui --windowed --wip --insecure")
    endif ()

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

    if (USE_LIBSYNCTHING)
        set_property(
            TARGET ${META_TARGET_NAME}
            APPEND
            PROPERTY QT_ANDROID_EXTRA_LIBS "${SYNCTHINGINTERNAL_LIBRARY_PATH}")
    endif ()

    set(QT_ANDROID_SIGN_APK ON)
    qt_android_generate_deployment_settings(${META_TARGET_NAME})
    qt_android_add_apk_target(${META_TARGET_NAME})
endif ()
