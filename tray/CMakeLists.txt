cmake_minimum_required(VERSION 3.17.0 FATAL_ERROR)

# metadata
set(META_PROJECT_TYPE application)
set(META_APP_NAME "Syncthing Tray")
set(META_APP_NAME_QUICK_GUI "Syncthing App")
set(META_GUI_OPTIONAL ON)
if (QUICK_GUI)
    set(META_QT_VERSION 6.8.0)
endif ()

# use testfiles directory from syncthingconnector
set(META_SRCDIR_REFS "${CMAKE_CURRENT_SOURCE_DIR}\n${CMAKE_CURRENT_SOURCE_DIR}/../syncthingconnector")

# add project files
set(HEADER_FILES)
set(SRC_FILES application/main.cpp)
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
set(QML_HEADER_FILES gui/quick/appbase.h gui/quick/appservice.h gui/quick/app.h gui/quick/scenegraph/managedtexturenode.h)
set(QML_SRC_FILES gui/quick/appbase.cpp gui/quick/appservice.cpp gui/quick/app.cpp
                  gui/quick/scenegraph/managedtexturenode.cpp)
if (ANDROID)
    list(APPEND QML_HEADER_FILES gui/quick/android.h)
    list(APPEND QML_SRC_FILES gui/quick/android.cpp)
else ()
    list(APPEND EXCLUDED_FILES gui/quick/android.h gui/quick/android.cpp)
endif ()

set(TS_FILES
    translations/${META_PROJECT_NAME}_zh_CN.ts translations/${META_PROJECT_NAME}_cs_CZ.ts
    translations/${META_PROJECT_NAME}_de_DE.ts translations/${META_PROJECT_NAME}_ru_RU.ts
    translations/${META_PROJECT_NAME}_en_US.ts)

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
    emblem-warning
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

# configure use of C++ 20 in Android-specific code
if (ANDROID)
    set(META_CXX_STANDARD 20)
endif ()

# find c++utilities
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.28.0 REQUIRED)
use_cpp_utilities()

# find qtutilities
set(QT_UTILITIES_REQUIRED_VERSION 6.15.0)
if (ANDROID)
    set(QT_UTILITIES_REQUIRED_VERSION 6.16.0)
endif ()
find_package(${PACKAGE_NAMESPACE_PREFIX}qtutilities${CONFIGURATION_PACKAGE_SUFFIX_QTUTILITIES}
             ${QT_UTILITIES_REQUIRED_VERSION} REQUIRED)
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

# enable private Qt headers for QAndroidService, QAndroidBinder and QAndroidServiceConnection
if (ANDROID)
    list(APPEND ADDITIONAL_QT_MODULES CorePrivate)
endif ()

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

# link against library to use Android's Bitmap API (see https://developer.android.com/ndk/reference/group/bitmap)
if (ANDROID)
    find_library(JNI_GRAPHICS_LIB jnigraphics REQUIRED)
    list(APPEND PRIVATE_LIBRARIES ${JNI_GRAPHICS_LIB})
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

    unset(ADDITIONAL_QML_FILES)
    list(APPEND ADDITIONAL_QT_MODULES Concurrent Svg QuickControls2Material)
    list(APPEND ADDITIONAL_QT_REPOS svg)
    list(APPEND PRIVATE_INCLUDE_DIRS gui/quick)

    # allow showing the web interface as page in the app (not used right now; hence disabled by default)
    option(QUICK_GUI_WEBVIEW_PAGE "disables web view support in the Qt Quick GUI" OFF)
    if (QUICK_GUI_WEBVIEW_PAGE)
        list(APPEND ADDITIONAL_QML_FILES gui/qml/WebViewPage.qml)
        include(QtLinkage)
        unset(WEBVIEWITEM_QML_FILES)
        # check whether Qt WebView is present
        option(QUICK_GUI_NO_WEBVIEW "disables web view support in the Qt Quick GUI" ON)
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
        set_property(
            SOURCE application/main.cpp
            APPEND
            PROPERTY COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_HAS_WEBVIEW_PAGE)

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
    qt_policy(SET QTP0004 NEW)
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
        gui/qml/AppControl.qml
        gui/qml/AppWindow.qml
        gui/qml/ChangesPage.qml
        gui/qml/CopyPasteButtons.qml
        gui/qml/CloseDialog.qml
        gui/qml/CustomDialog.qml
        gui/qml/CustomFlickable.qml
        gui/qml/CustomListView.qml
        gui/qml/CustomToolButton.qml
        gui/qml/DetailsListView.qml
        gui/qml/DevConfigPage.qml
        gui/qml/DevDelegate.qml
        gui/qml/DevListView.qml
        gui/qml/DevsPage.qml
        gui/qml/DiscardChangesDialog.qml
        gui/qml/DirConfigPage.qml
        gui/qml/DirDelegate.qml
        gui/qml/DirListView.qml
        gui/qml/DirErrorsPage.qml
        gui/qml/DirsPage.qml
        gui/qml/ErrorsDelegate.qml
        gui/qml/ErrorsPage.qml
        gui/qml/ExpandableDelegate.qml
        gui/qml/ExpandableItemDelegate.qml
        gui/qml/ExpandableListView.qml
        gui/qml/FilesPage.qml
        gui/qml/ForkAwesomeIcon.qml
        gui/qml/HelpButton.qml
        gui/qml/HomeDirPage.qml
        gui/qml/IconOnlyButton.qml
        gui/qml/IgnorePatternPage.qml
        gui/qml/ImportPage.qml
        gui/qml/InternalErrorsPage.qml
        gui/qml/LeftDrawer.qml
        gui/qml/LoadingPane.qml
        gui/qml/LogPage.qml
        gui/qml/MainTabBar.qml
        gui/qml/MainTabButton.qml
        gui/qml/MainToolBar.qml
        gui/qml/MenuItemInstantiator.qml
        gui/qml/Meta.qml
        gui/qml/NeededPage.qml
        gui/qml/Notifications.qml
        gui/qml/ObjectConfigDelegate.qml
        gui/qml/ObjectConfigPage.qml
        gui/qml/OutOfSyncDirs.qml
        gui/qml/PageStack.qml
        gui/qml/PendingDevices.qml
        gui/qml/PendingDirs.qml
        gui/qml/SelectiveImportDelegate.qml
        gui/qml/SettingsPage.qml
        gui/qml/StartPage.qml
        gui/qml/Statistics.qml
        gui/qml/StatisticsPage.qml
        gui/qml/Theming.qml
        ${ADDITIONAL_QML_FILES}
        SOURCES
        gui/quick/quickicon.h
        gui/quick/quickicon.cpp
        RESOURCE_PREFIX
        "/qt/qml/")

    # workaround build errors due to implicit conversions in generated code
    if (TREAT_WARNINGS_AS_ERRORS)
        foreach (QML_FILE AdvancedDevConfigPage AdvancedDirConfigPage)
            set_source_files_properties(
                "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/${META_TARGET_NAME}_gui/qml/${QML_FILE}_qml.cpp"
                PROPERTIES COMPILE_FLAGS "-Wno-error=conversion")
        endforeach ()
    endif ()
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
    set_target_properties(
        ${META_TARGET_NAME}
        PROPERTIES QT_ANDROID_PACKAGE_NAME "${META_PROJECT_RDNS}"
                   QT_ANDROID_VERSION_NAME "${META_APP_VERSION}"
                   QT_ANDROID_VERSION_CODE "1"
                   QT_ANDROID_PACKAGE_SOURCE_DIR "${ANDROID_PACKAGE_SOURCE_DIR}"
                   QT_ANDROID_TARGET_SDK_VERSION "34")
    option(FORCE_WIDGETS_GUI_ON_ANDOROID "forces using the Qt Widgets GUI on Android" OFF)
    if (QUICK_GUI AND NOT FORCE_WIDGETS_GUI_ON_ANDOROID)
        set(ANDROID_APPLICATION_ARGUMENTS "qt-quick-gui")
    else ()
        set(ANDROID_APPLICATION_ARGUMENTS "qt-widgets-gui --windowed --wip")
    endif ()

    # allow keeping the app running after the activity has been destroyed (requires Qt patches from my qtbase fork)
    option(BACKGROUND_RUNNING_AFTER_ANDROID_ACTIVITY_DESTRUCTION
           "keep the app running after activity has been destroyed (requires Qt patches)" OFF)
    if (BACKGROUND_RUNNING_AFTER_ANDROID_ACTIVITY_DESTRUCTION)
        set(META_BACKGROUND_RUNNING_AFTER_DESTRUCTION true)
    else ()
        set(META_BACKGROUND_RUNNING_AFTER_DESTRUCTION false)
    endif ()

    # allow use of newer native Android APIs, see https://developer.android.com/ndk/guides/using-newer-apis
    set_source_files_properties(
        "gui/quick/app.cpp" PROPERTIES COMPILE_FLAGS
                                       "-D__ANDROID_UNAVAILABLE_SYMBOLS_ARE_WEAK__ -Werror=unguarded-availability")

    set(ANDROID_MANIFEST_PATH "${ANDROID_PACKAGE_SOURCE_DIR}/AndroidManifest.xml")
    configure_file("resources/AndroidManifest.xml.in" "${ANDROID_MANIFEST_PATH}" @ONLY)
    set(ANDROID_SHORTCUTS_PATH "${ANDROID_PACKAGE_SOURCE_DIR}/res/xml/shortcuts.xml")
    configure_file("resources/shortcuts.xml.in" "${ANDROID_SHORTCUTS_PATH}" @ONLY)

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

    # specify required plugins so androiddeployqt knows what we need and don't need
    # cmake-format: off
    qt_import_plugins(${META_TARGET_NAME}
        INCLUDE_BY_TYPE imageformats ${QT_PACKAGE_PREFIX}::QSvgPlugin
        INCLUDE_BY_TYPE iconengines ${QT_PACKAGE_PREFIX}::QSvgIconPlugin
        INCLUDE_BY_TYPE tls ${QT_PACKAGE_PREFIX}::QTlsBackendOpenSSLPlugin
        INCLUDE_BY_TYPE networkinformation ${QT_PACKAGE_PREFIX}::QAndroidNetworkInformationPlugin
        INCLUDE_BY_TYPE platforms ${QT_PACKAGE_PREFIX}::QAndroidIntegrationPlugin ${QT_PACKAGE_PREFIX}::QMinimalIntegrationPlugin
        EXCLUDE_BY_TYPE platforminputcontexts webview
    )
    # cmake-format: on
    if (NOT CMAKE_BUILD_TYPE STREQUAL Debug)
        qt_import_plugins(${META_TARGET_NAME} EXCLUDE_BY_TYPE qmltooling)
    endif ()

    # assert existance of required platform plugins as those MUST be present (or the app will crash on startup)
    if (NOT TARGET ${QT_PACKAGE_PREFIX}::QAndroidIntegrationPlugin)
        message(FATAL_ERROR "The required Android platform plugin is not present in your Qt build.")
    endif ()
    if (NOT TARGET ${QT_PACKAGE_PREFIX}::QMinimalIntegrationPlugin)
        message(FATAL_ERROR "The required \"minimal\" platform plugin is not present in your Qt build.")
    endif ()

    if (USE_LIBSYNCTHING)
        # bundle internal Syncthing library
        set_property(
            TARGET ${META_TARGET_NAME}
            APPEND
            PROPERTY QT_ANDROID_EXTRA_LIBS "${SYNCTHINGINTERNAL_LIBRARY_PATH}")

        # workaround https://github.com/golang/go/issues/70508
        set(ANDROID_SIGNAL_HANDLER_LIBRARY "androidsignalhandler")
        add_library("${ANDROID_SIGNAL_HANDLER_LIBRARY}" MODULE android/signalhandler.cpp)
        find_library(ANDROID_LOG_LIB log)
        target_link_libraries("${ANDROID_SIGNAL_HANDLER_LIBRARY}" "${ANDROID_LOG_LIB}")
        set_property(
            TARGET ${META_TARGET_NAME}
            APPEND
            PROPERTY QT_ANDROID_EXTRA_LIBS "${CMAKE_CURRENT_BINARY_DIR}/lib${ANDROID_SIGNAL_HANDLER_LIBRARY}.so")
    endif ()

    set(QT_ANDROID_SIGN_APK ON)
    if (COMMAND _qt_internal_configure_android_multiabi_target)
        _qt_internal_configure_android_multiabi_target(${META_TARGET_NAME}) # required as of Qt 6.9.0 (probably Qt bug)
    endif ()
    qt_android_generate_deployment_settings(${META_TARGET_NAME})
    qt_android_add_apk_target(${META_TARGET_NAME})
endif ()
