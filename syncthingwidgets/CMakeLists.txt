cmake_minimum_required(VERSION 3.17.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthingwidgets)
set(META_PROJECT_TYPE library)
set(META_APP_NAME "UI elements of Syncthing Tray")
set(META_PUBLIC_QT_MODULES Gui Widgets WebEngineCore WebEngineWidgets)
set(META_WEBVIEW_SRC_DIR webview)
set(META_WEBVIEW_WITH_CORE ON)
set(META_GUI_OPTIONAL ON)
set(META_CXX_STANDARD 20)
if (QUICK_GUI)
    set(META_QT_VERSION 6.9.0)
endif ()

# add project files
set(HEADER_FILES misc/diffhighlighter.h misc/internalerror.h misc/statusinfo.h misc/syncthinglauncher.h misc/utils.h)
set(SRC_FILES misc/diffhighlighter.cpp misc/internalerror.cpp misc/statusinfo.cpp misc/syncthinglauncher.cpp misc/utils.cpp)
set(WIDGETS_HEADER_FILES
    settings/settings.h
    settings/settingsdialog.h
    settings/wizard.h
    settings/wizardenums.h
    webview/webpage.h
    webview/webviewdialog.h
    misc/textviewdialog.h
    misc/internalerrorsdialog.h
    misc/direrrorsdialog.h
    misc/dbusstatusnotifier.h
    misc/otherdialogs.h
    misc/syncthingkiller.h)
set(WIDGETS_SRC_FILES
    settings/settings.cpp
    settings/settingsdialog.cpp
    settings/setupdetection.h
    settings/setupdetection.cpp
    settings/wizard.cpp
    webview/webpage.cpp
    webview/webviewdialog.cpp
    webview/webviewinterceptor.h
    webview/webviewinterceptor.cpp
    misc/textviewdialog.cpp
    misc/internalerrorsdialog.cpp
    misc/direrrorsdialog.cpp
    misc/dbusstatusnotifier.cpp
    misc/otherdialogs.cpp
    misc/syncthingkiller.cpp)
set(RES_FILES resources/${META_PROJECT_NAME}icons.qrc)
set(WIDGETS_UI_FILES
    settings/connectionoptionpage.ui
    settings/notificationsoptionpage.ui
    settings/appearanceoptionpage.ui
    settings/iconsoptionpage.ui
    settings/autostartoptionpage.ui
    settings/launcheroptionpage.ui
    settings/systemdoptionpage.ui
    settings/generalwebviewoptionpage.ui
    settings/builtinwebviewoptionpage.ui
    settings/mainconfigwizardpage.ui
    settings/autostartwizardpage.ui
    settings/applywizardpage.ui)
set(QML_HEADER_FILES quick/appbase.h quick/appservice.h quick/app.h quick/scenegraph/managedtexturenode.h)
set(QML_SRC_FILES quick/appbase.cpp quick/appservice.cpp quick/app.cpp quick/scenegraph/managedtexturenode.cpp)
set(QML_MAIN_FILES
    quick/qml/AboutDialog.qml
    quick/qml/AdvancedConfigPage.qml
    quick/qml/AdvancedDevConfigPage.qml
    quick/qml/AdvancedDirConfigPage.qml
    quick/qml/AdvancedPage.qml
    quick/qml/ArrayElementButtons.qml
    quick/qml/AppControl.qml
    quick/qml/AppWindow.qml
    quick/qml/ChangesPage.qml
    quick/qml/CopyPasteButtons.qml
    quick/qml/CloseDialog.qml
    quick/qml/CustomDialog.qml
    quick/qml/CustomFlickable.qml
    quick/qml/CustomListView.qml
    quick/qml/CustomToolButton.qml
    quick/qml/DetailsListView.qml
    quick/qml/DevConfigPage.qml
    quick/qml/DevDelegate.qml
    quick/qml/DevListView.qml
    quick/qml/DevsPage.qml
    quick/qml/DiscardChangesDialog.qml
    quick/qml/DirConfigPage.qml
    quick/qml/DirDelegate.qml
    quick/qml/DirListView.qml
    quick/qml/DirErrorsPage.qml
    quick/qml/DirsPage.qml
    quick/qml/ErrorsDelegate.qml
    quick/qml/ErrorsPage.qml
    quick/qml/ExpandableDelegate.qml
    quick/qml/ExpandableItemDelegate.qml
    quick/qml/ExpandableListView.qml
    quick/qml/FilesPage.qml
    quick/qml/ForkAwesomeIcon.qml
    quick/qml/HelpButton.qml
    quick/qml/HomeDirPage.qml
    quick/qml/IconOnlyButton.qml
    quick/qml/IgnorePatternPage.qml
    quick/qml/ImportPage.qml
    quick/qml/InternalErrorsPage.qml
    quick/qml/LeftDrawer.qml
    quick/qml/LoadingPane.qml
    quick/qml/LogPage.qml
    quick/qml/MainTabBar.qml
    quick/qml/MainTabButton.qml
    quick/qml/MainToolBar.qml
    quick/qml/MenuItemInstantiator.qml
    quick/qml/Meta.qml
    quick/qml/NeededPage.qml
    quick/qml/Notifications.qml
    quick/qml/ObjectConfigDelegate.qml
    quick/qml/ObjectConfigPage.qml
    quick/qml/OutOfSyncDirs.qml
    quick/qml/PageStack.qml
    quick/qml/PendingDevices.qml
    quick/qml/PendingDirs.qml
    quick/qml/SelectiveImportDelegate.qml
    quick/qml/SettingsPage.qml
    quick/qml/StartPage.qml
    quick/qml/Statistics.qml
    quick/qml/StatisticsPage.qml
    quick/qml/Theming.qml)
list(APPEND EXCLUDED_FILES ${QML_MAIN_FILES} quick/qml/WebViewPage.qml quick/qml/webview-webview/WebViewItem.qml
     quick/qml/webview-none/WebViewItem.qml)
if (ANDROID)
    list(APPEND QML_HEADER_FILES quick/android.h)
    list(APPEND QML_SRC_FILES quick/android.cpp)
else ()
    list(APPEND EXCLUDED_FILES quick/android.h quick/android.cpp)
endif ()

set(TS_FILES
    translations/${META_PROJECT_NAME}_zh_CN.ts translations/${META_PROJECT_NAME}_cs_CZ.ts
    translations/${META_PROJECT_NAME}_de_DE.ts translations/${META_PROJECT_NAME}_ru_RU.ts
    translations/${META_PROJECT_NAME}_en_US.ts)

set(REQUIRED_ICONS
    color-profile
    dialog-ok
    dialog-cancel
    document-open
    document-edit
    folder
    preferences-other
    process-stop
    list-add
    preferences-desktop
    internet-web-browser
    system-run
    edit-copy
    edit-paste
    edit-select
    edit-undo
    list-remove
    preferences-desktop-notification
    preferences-system-startup
    preferences-system-services
    system-software-update-symbolic
    view-refresh
    emblem-checked
    network-connect
    emblem-remove
    go-down
    go-up
    quickwizard
    help-contents
    question)

set(QT_TESTS wizard)

# find c++utilities
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.31.0 REQUIRED)
use_cpp_utilities(VISIBILITY PUBLIC)

# find qtutilities
find_package(${PACKAGE_NAMESPACE_PREFIX}qtutilities${CONFIGURATION_PACKAGE_SUFFIX_QTUTILITIES} 6.18.0 REQUIRED)
use_qt_utilities()

# find backend libraries
find_package(syncthingconnector ${META_APP_VERSION} REQUIRED)
use_syncthingconnector(VISIBILITY PUBLIC)
find_package(syncthingmodel ${META_APP_VERSION} REQUIRED)
use_syncthingmodel(VISIBILITY PUBLIC)
find_package(syncthingtesthelper ${META_APP_VERSION} REQUIRED)
list(APPEND PRIVATE_INCLUDE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/../testhelper/include")

# configure libsyncthing
if (NO_LIBSYNCTHING)
    set(USE_LIBSYNCTHING_DEFAULT OFF)
else ()
    set(USE_LIBSYNCTHING_DEFAULT ON)
endif ()
option(USE_LIBSYNCTHING "whether libsyncthing should be included for the launcher" ${USE_LIBSYNCTHING_DEFAULT})
if (USE_LIBSYNCTHING)
    find_package(syncthing ${META_APP_VERSION} REQUIRED)
    use_syncthing(VISIBILITY PUBLIC)
    list(APPEND META_PUBLIC_COMPILE_DEFINITIONS SYNCTHINGWIDGETS_USE_LIBSYNCTHING)
endif ()

# allow disabling autostart configuration
option(NO_AUTOSTART_SETTINGS
       "whether autostart settings should be disabled - useful if it would not work anyway on the target platform/packaging"
       OFF)
if (NO_AUTOSTART_SETTINGS)
    set_property(
        SOURCE settings/settingsdialog.cpp settings/wizard.cpp
        APPEND
        PROPERTY COMPILE_DEFINITIONS SYNCTHINGWIDGETS_AUTOSTART_DISABLED)
endif ()

# configure autostart .desktop file exec path
set(AUTOSTART_EXEC_PATH
    ""
    CACHE STRING "Set the Exec= path for Linux' autostart/syncthingtray.desktop file - useful for Nix & Guix")
if (AUTOSTART_EXEC_PATH)
    set_property(
        SOURCE settings/settingsdialog.cpp
        APPEND
        PROPERTY COMPILE_DEFINITIONS SYNCTHINGWIDGETS_AUTOSTART_EXEC_PATH="${AUTOSTART_EXEC_PATH}")
endif ()

# configure whether setup tools are enabled
option(
    SETUP_TOOLS
    "enables setup tools; needs qtutilities built with this option as well; makes likely sense to disable when distributing via a package manager"
    OFF)
if (SETUP_TOOLS)
    set_property(
        SOURCE settings/settingsdialog.cpp
        APPEND
        PROPERTY COMPILE_DEFINITIONS SYNCTHINGWIDGETS_SETUP_TOOLS_ENABLED)
    list(APPEND REQUIRED_ICONS install info)
endif ()

# allow enabling extended client area
option(
    EXTENDED_CLIENT_AREA
    "enables extended client area of Qt Quick GUI for Edge-to-edge support"
    OFF)
if (EXTENDED_CLIENT_AREA)
    list(APPEND META_PRIVATE_COMPILE_DEFINITIONS SYNCTHINGWIDGETS_EXTENDED_CLIENT_AREA)
endif ()

# link also explicitly against the following Qt modules
list(APPEND ADDITIONAL_QT_MODULES Network Concurrent)

# apply basic configuration
include(BasicConfig)

# configure Qt Quick GUI
include(QtGuiConfig)
include(QtLinkage)
set(${META_PROJECT_VARNAME_UPPER}_HAS_WEBVIEW
    OFF
    PARENT_SCOPE)
set(${META_PROJECT_VARNAME_UPPER}_HAS_WEBVIEW_PAGE
    OFF
    PARENT_SCOPE)
if (QUICK_GUI)
    find_package(${PACKAGE_NAMESPACE_PREFIX}qtquickforkawesome${CONFIGURATION_PACKAGE_SUFFIX_QTFORKAWESOME} 0.1.0 REQUIRED)
    use_qt_quick_fork_awesome()

    use_standard_filesystem()

    list(APPEND ADDITIONAL_QT_MODULES Concurrent Svg QuickControls2Material)
    list(APPEND ADDITIONAL_QT_REPOS svg)
    list(APPEND PRIVATE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/quick")

    # enable private Qt headers for QAndroidIntent
    if (ANDROID)
        list(APPEND ADDITIONAL_QT_MODULES CorePrivate)
    endif ()

    # find Qt Qml and enable automoc which is required for the subsequent qt_add_qml_module calls
    find_package("${QT_PACKAGE_PREFIX}Qml" "${META_QT_VERSION}")

    # ensure all auto variables are enabled before adding the library target via qt_add_qml_module
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    # allow showing the web interface as page in the app (not used right now; hence disabled by default)
    option(QUICK_GUI_WEBVIEW_PAGE "enables the web view page in the Qt Quick GUI" OFF)
    if (QUICK_GUI_WEBVIEW_PAGE)
        list(APPEND QML_MAIN_FILES quick/qml/WebViewPage.qml)
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
                list(APPEND WEBVIEWITEM_QML_FILES quick/qml/webview-webview/WebViewItem.qml)
                set(${META_PROJECT_VARNAME_UPPER}_HAS_WEBVIEW
                    ON
                    PARENT_SCOPE)
            endif ()
        endif ()
        if (NOT WEBVIEWITEM_QML_FILES)
            list(APPEND WEBVIEWITEM_QML_FILES quick/qml/webview-none/WebViewItem.qml)
        endif ()
        set(${META_PROJECT_VARNAME_UPPER}_HAS_WEBVIEW_PAGE
            ON
            PARENT_SCOPE)

        # add additional QML module for the web view
        qt_add_qml_module(
            ${META_TARGET_NAME}_webviewitem
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
        list(APPEND PUBLIC_LIBRARIES ${META_TARGET_NAME}_webviewitemplugin)
    endif ()

    # add main QML module
    if (BUILD_SHARED_LIBS)
        set(META_LIBRARY_TYPE SHARED)
        set(PLUGIN_ARG NO_PLUGIN) # creating a plugin library can be avoided
    else ()
        set(META_LIBRARY_TYPE STATIC)
        set(PLUGIN_ARG) # Icon will not be available without having a plugin (and importing it via
                        # Q_IMPORT_PLUGIN(MainPlugin))
    endif ()
    qt_policy(SET QTP0004 NEW)
    qt_add_qml_module(
        ${META_TARGET_NAME}
        URI
        "Main"
        VERSION
        1.0
        ${META_LIBRARY_TYPE}
        QML_FILES
        ${QML_MAIN_FILES}
        SOURCES
        quick/quickicon.h
        quick/quickicon.cpp
        RESOURCE_PREFIX
        "/qt/qml/"
        ${PLUGIN_ARG}
        NO_PLUGIN_OPTIONAL)
    if (NOT BUILD_SHARED_LIBS)
        list(APPEND PUBLIC_LIBRARIES ${META_TARGET_NAME}plugin)
        list(APPEND BUNDLED_TARGETS ${META_TARGET_NAME}plugin ${META_TARGET_NAME}plugin_init)
    endif ()

    # export additional interface link libraries added by qt_add_qml_module
    get_target_property(REQUIRED_TARGETS "${META_TARGET_NAME}" INTERFACE_LINK_LIBRARIES)
    set(REQUIRED_TARGET_NAMES)
    foreach (REQUIRED_TARGET ${REQUIRED_TARGETS})
        if ("${REQUIRED_TARGET}" MATCHES ".*(${META_TARGET_NAME}_resources_[0-9]+).*")
            list(APPEND BUNDLED_TARGETS "${CMAKE_MATCH_1}")
        endif ()
    endforeach ()
endif ()

# include further modules to apply configuration
include(QtWebViewProviderConfig)
include(QtConfig)
include(WindowsResources)
include(LibraryTarget)
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
    # workaround build errors due to implicit conversions in generated code
    if (TREAT_WARNINGS_AS_ERRORS)
        foreach (QML_FILE AdvancedDevConfigPage AdvancedDirConfigPage)
            set_source_files_properties(
                "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/${META_TARGET_NAME}_quick/qml/${QML_FILE}_qml.cpp"
                PROPERTIES COMPILE_FLAGS "-Wno-error=conversion")
        endforeach ()
    endif ()

    # allow use of newer native Android APIs, see https://developer.android.com/ndk/guides/using-newer-apis
    if (ANDROID)
        set_source_files_properties(
            "quick/app.cpp" PROPERTIES COMPILE_FLAGS
                                       "-D__ANDROID_UNAVAILABLE_SYMBOLS_ARE_WEAK__ -Werror=unguarded-availability")
    endif ()
endif ()

# configure test target
include(TestUtilities)
list(APPEND QT_TEST_LIBRARIES ${CPP_UTILITIES_LIB} ${META_TARGET_NAME})
use_qt_module(LIBRARIES_VARIABLE "QT_TEST_LIBRARIES" PREFIX "${QT_PACKAGE_PREFIX}" MODULE "Test")
foreach (TEST ${QT_TESTS})
    configure_test_target(
        TEST_NAME
        "${TEST}_tests"
        SRC_FILES
        "tests/${TEST}.cpp"
        LIBRARIES
        "${QT_TEST_LIBRARIES}"
        FULL_TEST_NAME_OUT_VAR
        FULL_TEST_NAME_${TEST})
    # avoid running wizard tests in parallel with other tests spawning a Syncthing test instance
    if ("${TEST}" STREQUAL "wizard")
        set_tests_properties("${FULL_TEST_NAME_${TEST}}" PROPERTIES RESOURCE_LOCK "syncthingtestinstance")
        set_tests_properties("${FULL_TEST_NAME_${TEST}}" PROPERTIES ENVIRONMENT "LIB_SYNCTHING_CONNECTOR_LOG_ALL=1")
    endif ()
endforeach ()
