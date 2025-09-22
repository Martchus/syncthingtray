cmake_minimum_required(VERSION 3.17.0 FATAL_ERROR)

# metadata
set(META_PROJECT_TYPE application)
set(META_APP_NAME "Syncthing Tray")
set(META_APP_NAME_QUICK_GUI "Syncthing App")
set(META_GUI_OPTIONAL ON)
if (QUICK_GUI)
    set(META_QT_VERSION 6.9.0)
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
set(META_HAS_QUICK_GUI ON)

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
    system-software-update-symbolic
    text-x-generic
    quickwizard
    view-refresh
    window-close
    window-pin)

# configure use of C++ 20 in Android-specific code
if (ANDROID)
    set(META_CXX_STANDARD 20)
endif ()

# determine required version of c++utilities
option(
    SETUP_TOOLS
    "enables setup tools; needs qtutilities and syncthingwidgets built with this option as well; makes likely sense to disable when distributing via a package manager"
    OFF)
option(USE_LIBSYNCTHING "whether libsyncthing should be included for syncthingtray's CLI" OFF)
set(CPP_UTILITIES_REQUIRED_VERSION 5.30.0)

# find c++utilities
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} ${CPP_UTILITIES_REQUIRED_VERSION}
             REQUIRED)
use_cpp_utilities()

# find qtutilities
set(QT_UTILITIES_REQUIRED_VERSION 6.15.0)
if (ANDROID)
    set(QT_UTILITIES_REQUIRED_VERSION 6.16.0)
endif ()
if (SETUP_TOOLS)
    set(QT_UTILITIES_REQUIRED_VERSION 6.18.0)
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

# enable private Qt headers for QAndroidService
if (ANDROID)
    list(APPEND ADDITIONAL_QT_MODULES CorePrivate)
endif ()

# configure libsyncthing
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

# link against library to use Android's Font API (see https://developer.android.com/ndk/reference/group/font)
if (ANDROID)
    find_library(ANDROID_SYSTEM_LIB android REQUIRED)
    list(APPEND PRIVATE_LIBRARIES ${ANDROID_SYSTEM_LIB})
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

# configure whether setup tools are enabled
if (SETUP_TOOLS)
    # validate the signature using libsyncthing if enabled to avoid additional dependency on OpenSSL; otherwise use OpenSSL
    if (NOT USE_LIBSYNCTHING)
        use_crypto()
    endif ()
    list(APPEND REQUIRED_ICONS install info)
    list(APPEND META_PRIVATE_COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_SETUP_TOOLS_ENABLED)
endif ()

# configure Qt Quick GUI
include(QtGuiConfig)
if (QUICK_GUI)
    # allow showing the web interface as page in the app (not used right now; hence disabled by default)
    if (SYNCTHINGWIDGETS_HAS_WEBVIEW)
        set_property(
            SOURCE application/main.cpp
            APPEND
            PROPERTY COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_HAS_WEBVIEW)
    endif ()
    if (SYNCTHINGWIDGETS_HAS_WEBVIEW_PAGE)
        set_property(
            SOURCE application/main.cpp
            APPEND
            PROPERTY COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_HAS_WEBVIEW_PAGE)
    endif ()
endif ()

# consider Java classes when generating API documentation
set(ANDROID_JAVA_SRC_DIR "android/src/io/github/martchus/syncthingtray")
list(
    APPEND
    DOC_FILES
    "${ANDROID_JAVA_SRC_DIR}/Activity.java"
    "${ANDROID_JAVA_SRC_DIR}/Application.java"
    "${ANDROID_JAVA_SRC_DIR}/DocumentsProvider.java"
    "${ANDROID_JAVA_SRC_DIR}/SyncthingService.java"
    "${ANDROID_JAVA_SRC_DIR}/Util.java")

# include further modules to apply configuration
include(QtConfig)
include(WindowsResources)
include(AppTarget)
include(ShellCompletion)
include(Doxygen)
include(Sphinx)
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
    set_target_properties(
        ${META_TARGET_NAME}
        PROPERTIES QT_ANDROID_PACKAGE_NAME "${META_PROJECT_RDNS}"
                   QT_ANDROID_VERSION_NAME "${META_APP_VERSION}"
                   QT_ANDROID_VERSION_CODE "1"
                   QT_ANDROID_PACKAGE_SOURCE_DIR "${ANDROID_PACKAGE_SOURCE_DIR}"
                   QT_ANDROID_TARGET_SDK_VERSION "35")
    option(FORCE_WIDGETS_GUI_ON_ANDOROID "forces using the Qt Widgets GUI on Android" OFF)
    if (QUICK_GUI AND NOT FORCE_WIDGETS_GUI_ON_ANDOROID)
        set(ANDROID_APPLICATION_ARGUMENTS "qt-quick-gui")
    else ()
        set(ANDROID_APPLICATION_ARGUMENTS "qt-widgets-gui --windowed --wip")
    endif ()

    # allow running the service process with QGuiApplication instantiated (explicitly, not just for icon rendering)
    option(ANDROID_SERVICE_WITH_GUI_APP "run service with QGuiApplication instantiated" OFF)
    if (ANDROID_SERVICE_WITH_GUI_APP OR ANDROID_SERVICE_WITH_ICON_RENDERING)
        set_property(
            SOURCE application/main.cpp
            APPEND
            PROPERTY COMPILE_DEFINITIONS ${META_PROJECT_VARNAME_UPPER}_GUI_CODE_IN_SERVICE)
    endif ()

    # allow keeping the app running after the activity has been destroyed (requires Qt patches from my qtbase fork)
    option(BACKGROUND_RUNNING_AFTER_ANDROID_ACTIVITY_DESTRUCTION
           "keep the app running after activity has been destroyed (requires Qt patches)" OFF)
    if (BACKGROUND_RUNNING_AFTER_ANDROID_ACTIVITY_DESTRUCTION)
        set(META_BACKGROUND_RUNNING_AFTER_DESTRUCTION true)
    else ()
        set(META_BACKGROUND_RUNNING_AFTER_DESTRUCTION false)
    endif ()

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
    set(REQUIRED_PLATFORMS ${QT_PACKAGE_PREFIX}::QAndroidIntegrationPlugin)
    if (ANDROID_SERVICE_WITH_GUI_APP OR ANDROID_SERVICE_WITH_ICON_RENDERING)
        list(APPEND REQUIRED_PLATFORMS ${QT_PACKAGE_PREFIX}::QMinimalIntegrationPlugin)
    endif ()
    # cmake-format: off
    qt_import_plugins(${META_TARGET_NAME}
        INCLUDE_BY_TYPE imageformats ${QT_PACKAGE_PREFIX}::QSvgPlugin
        INCLUDE_BY_TYPE iconengines ${QT_PACKAGE_PREFIX}::QSvgIconPlugin
        INCLUDE_BY_TYPE tls ${QT_PACKAGE_PREFIX}::QTlsBackendOpenSSLPlugin
        INCLUDE_BY_TYPE networkinformation ${QT_PACKAGE_PREFIX}::QAndroidNetworkInformationPlugin
        INCLUDE_BY_TYPE platforms ${REQUIRED_PLATFORMS}
        EXCLUDE_BY_TYPE platforminputcontexts webview
    )
    # cmake-format: on
    if (NOT CMAKE_BUILD_TYPE STREQUAL Debug)
        qt_import_plugins(${META_TARGET_NAME} EXCLUDE_BY_TYPE qmltooling)
    endif ()

    # assert existance of required platform plugins as those MUST be present (or the app will crash on startup)
    foreach (REQUIRED_PLATFORM ${REQUIRED_PLATFORMS})
        if (NOT TARGET "${REQUIRED_PLATFORM}")
            message(FATAL_ERROR "The required platform plugin ${REQUIRED_PLATFORM} is not present in your Qt build.")
        endif ()
    endforeach ()

    if (USE_LIBSYNCTHING)
        # bundle internal Syncthing library
        set_property(
            TARGET ${META_TARGET_NAME}
            APPEND
            PROPERTY QT_ANDROID_EXTRA_LIBS "${SYNCTHINGINTERNAL_LIBRARY_PATH}")
        if (SYNCTHING_USE_SYSTEM_SQLITE)
            find_package(SQLite3 REQUIRED)
            set_property(
                TARGET ${META_TARGET_NAME}
                APPEND
                PROPERTY QT_ANDROID_EXTRA_LIBS "${SQLite3_LIBRARIES}")
        endif ()

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
