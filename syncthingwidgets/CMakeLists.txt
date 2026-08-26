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
if (QUICK_GUI AND WIDGETS_GUI)
    set(META_QT_VERSION 6.10.0)
elseif (QUICK_GUI)
    set(META_QT_VERSION 6.9.0)
endif ()

# use testfiles directory from syncthingconnector
set(META_SRCDIR_REFS "${CMAKE_CURRENT_SOURCE_DIR}\n${CMAKE_CURRENT_SOURCE_DIR}/../syncthingconnector")

# configure modes of the Qt Quick GUI
set(META_QUICK_GUI_MODES "desktop" "app")
if (ANDROID)
    set(DEFAULT_QUICK_GUI_MODES "app")
endif ()

# add project files
set(HEADER_FILES
    misc/diffhighlighter.h
    misc/internalerror.h
    misc/statusinfo.h
    misc/syncthinglauncher.h
    misc/utils.h
    misc/syncthingdata.h
    misc/syncthingmodels.h)
set(SRC_FILES
    misc/diffhighlighter.cpp
    misc/internalerror.cpp
    misc/statusinfo.cpp
    misc/syncthinglauncher.cpp
    misc/utils.cpp
    misc/syncthingdata.cpp
    misc/syncthingmodels.cpp)
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
set(QML_HEADER_FILES quick/helpers.h quick/quickui.h quick/scenegraph/managedtexturenode.h)
set(QML_SRC_FILES quick/quickui.cpp quick/scenegraph/managedtexturenode.cpp)
set(QML_HEADER_FILES_APP quick/appbase.h quick/appservice.h quick/app.h)
set(QML_SRC_FILES_APP quick/appbase.cpp quick/appservice.cpp quick/app.cpp)
set(QML_MAIN_FILES
    quick/qml/AboutDialog.qml
    quick/qml/AdvancedConfigPage.qml
    quick/qml/AdvancedDevConfigPage.qml
    quick/qml/AdvancedDirConfigPage.qml
    quick/qml/AdvancedPage.qml
    quick/qml/AdvancedMainPage.qml
    quick/qml/ArrayElementButtons.qml
    quick/qml/AppWindow.qml
    quick/qml/ChangesListView.qml
    quick/qml/ChangesPage.qml
    quick/qml/CopyPasteButtons.qml
    quick/qml/CloseDialog.qml
    quick/qml/CustomDelegate.qml
    quick/qml/CustomDialog.qml
    quick/qml/CustomFlickable.qml
    quick/qml/CustomListView.qml
    quick/qml/CustomMenu.qml
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
    quick/qml/DynamicSectionHeader.qml
    quick/qml/EditTextFieldDialog.qml
    quick/qml/EditNumericFieldDialog.qml
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
    quick/qml/LocalSyncProgress.qml
    quick/qml/LogPage.qml
    quick/qml/MainTabBar.qml
    quick/qml/MainTabButton.qml
    quick/qml/MainToolBar.qml
    quick/qml/MenuItemInstantiator.qml
    quick/qml/MenuSeparatorInstantiator.qml
    quick/qml/Meta.qml
    quick/qml/NeededPage.qml
    quick/qml/Notifications.qml
    quick/qml/ObjectConfigDelegate.qml
    quick/qml/ObjectConfigPage.qml
    quick/qml/OpenLinkDialog.qml
    quick/qml/OutOfSyncDirs.qml
    quick/qml/PageStack.qml
    quick/qml/PendingDevices.qml
    quick/qml/PendingDirs.qml
    quick/qml/RemoteSyncProgress.qml
    quick/qml/SelectiveImportDelegate.qml
    quick/qml/SettingsPage.qml
    quick/qml/StartPage.qml
    quick/qml/Statistics.qml
    quick/qml/StatisticsRow.qml
    quick/qml/StatisticsPage.qml
    quick/qml/SectionHeader.qml
    quick/qml/SectionLabel.qml
    quick/qml/Theming.qml
    quick/qml/Utils.qml)
set(QML_MAIN_FILES_DESKTOP quick/qml/DesktopWindow.qml quick/qml/FilterField.qml quick/qml/PageWindow.qml
                           quick/qml/PageWindowBackground.qml quick/qml/TrayView.qml)
set(QML_MAIN_FILES_APP_CONTROL quick/qml/AppControl.qml)
set(QML_MAIN_SOURCE_FILES quick/quickicon.h quick/quickicon.cpp)
set(QML_WIDGETS_DLGS_FILES quick/widgetsdialogs.h quick/widgetsdialogs.cpp)
set(QML_SINGLETON_FILES quick/qml/Utils.qml)
list(
    APPEND
    EXCLUDED_FILES
    ${QML_MAIN_FILES}
    ${QML_MAIN_FILES_DESKTOP}
    ${QML_MAIN_FILES_APP_CONTROL}
    ${QML_MAIN_SOURCE_FILES}
    ${QML_WIDGETS_DLGS_FILES})
if (ANDROID)
    list(APPEND QML_HEADER_FILES quick/android.h)
    list(APPEND QML_SRC_FILES quick/android.cpp)
else ()
    list(APPEND EXCLUDED_FILES quick/android.h quick/android.cpp)
endif ()
option(QUICK_GUI_APP_CONTROL "enables AppControl as alternative entry point for the Qt Quick GUI" OFF)
if (QUICK_GUI_APP_CONTROL)
    list(APPEND QML_MAIN_FILES ${QML_MAIN_FILES_APP_CONTROL})
endif ()
option(QUICK_GUI_WIDGETS_DIALOGS "enables Qt Widgets based dialogs for the Qt Quick GUI" OFF)
if (QUICK_GUI_WIDGETS_DIALOGS)
    list(APPEND QML_MAIN_SOURCE_FILES ${QML_WIDGETS_DLGS_FILES})
endif ()

set(TS_FILES
    translations/${META_PROJECT_NAME}_zh_CN.ts translations/${META_PROJECT_NAME}_cs_CZ.ts
    translations/${META_PROJECT_NAME}_de_DE.ts translations/${META_PROJECT_NAME}_ru_RU.ts
    translations/${META_PROJECT_NAME}_fr_FR.ts translations/${META_PROJECT_NAME}_en_US.ts)

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

# configure test files
set(QT_TESTS misc)
set(QT_WIDGETS_TESTS wizard)
foreach (TEST "${QT_WIDGETS_TESTS}")
    list(APPEND EXCLUDED_FILES "tests/${TEST}.cpp")
endforeach ()
list(
    APPEND
    EXCLUDED_FILES
    tests/testhelper.h
    tests/app/tst_app.qml
    tests/apptests.cpp
    tests/desktop/tst_desktop_ui.qml
    tests/desktopuitests.cpp)
if (WIDGETS_GUI)
    list(APPEND QT_TESTS "${QT_WIDGETS_TESTS}")
endif ()

# find c++utilities
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.31.0 REQUIRED)
use_cpp_utilities(VISIBILITY PUBLIC)

# find qtutilities
set(REQUIRED_QTUTILITIES_VERSION 6.21.0)
if (QUICK_GUI)
    set(REQUIRED_QTUTILITIES_VERSION 6.22.0)
endif ()
find_package(${PACKAGE_NAMESPACE_PREFIX}qtutilities${CONFIGURATION_PACKAGE_SUFFIX_QTUTILITIES}
             ${REQUIRED_QTUTILITIES_VERSION} REQUIRED)
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
option(EXTENDED_CLIENT_AREA "enables extended client area of Qt Quick GUI for Edge-to-edge support" OFF)
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
if (QUICK_GUI)
    find_package(${PACKAGE_NAMESPACE_PREFIX}qtquickforkawesome${CONFIGURATION_PACKAGE_SUFFIX_QTFORKAWESOME} 0.1.0 REQUIRED)
    use_qt_quick_fork_awesome()
    use_standard_filesystem()

    list(APPEND ADDITIONAL_QT_MODULES Concurrent Svg)
    list(APPEND ADDITIONAL_QT_REPOS svg)
    list(APPEND PRIVATE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/quick")
    qt_utilities_configure_qt_quick_controls("Material" "quick/quickui.cpp")

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

    # add main QML module
    if ("desktop" IN_LIST QUICK_GUI_ENABLED_MODES)
        list(APPEND QML_MAIN_FILES ${QML_MAIN_FILES_DESKTOP})
    endif ()
    if (BUILD_SHARED_LIBS)
        set(META_LIBRARY_TYPE SHARED)
        set(PLUGIN_ARG NO_PLUGIN) # creating a plugin library can be avoided
    else ()
        set(META_LIBRARY_TYPE STATIC)
        set(PLUGIN_ARG) # Icon will not be available without having a plugin (and importing it via
                        # Q_IMPORT_PLUGIN(MainPlugin))
    endif ()
    qt_policy(SET QTP0004 NEW)
    qt_utilities_change_qt_quick_controls_style(QML_MAIN_FILES)
    set_source_files_properties(${QML_SINGLETON_FILES} PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
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
        ${QML_MAIN_SOURCE_FILES}
        RESOURCE_PREFIX
        "/qt/qml/"
        ${PLUGIN_ARG}
        NO_PLUGIN_OPTIONAL)
    if (NOT BUILD_SHARED_LIBS)
        list(APPEND PUBLIC_LIBRARIES ${META_TARGET_NAME}plugin)
        list(APPEND BUNDLED_TARGETS ${META_TARGET_NAME}plugin ${META_TARGET_NAME}plugin_init)
    endif ()

    # add unprocessed QML files to target so they show up in Qt Creator
    target_sources(${META_TARGET_NAME} PRIVATE ${UNPROCESSED_QML_FILES})

    # export additional interface link libraries added by qt_add_qml_module
    get_target_property(REQUIRED_TARGETS "${META_TARGET_NAME}" INTERFACE_LINK_LIBRARIES)
    set(REQUIRED_TARGET_NAMES)
    foreach (REQUIRED_TARGET ${REQUIRED_TARGETS})
        if ("${REQUIRED_TARGET}" MATCHES ".*(${META_TARGET_NAME}_resources_[0-9]+).*")
            list(APPEND BUNDLED_TARGETS "${CMAKE_MATCH_1}")
        endif ()
    endforeach ()

    # configure whether a dynamic style is used
    if (QUICK_GUI_CONTROLS_STYLE STREQUAL dynamic)
        set_property(
            SOURCE quick/quickui.cpp
            APPEND
            PROPERTY COMPILE_DEFINITIONS SYNCTHING_APP_DYNAMIC_STYLE)
    endif ()

    # configure icon size
    set(ICON_SIZE 16)
    set(ICON_WIDTH_DELEGATE 16)
    if (QUICK_GUI_CONTROLS_STYLE STREQUAL FluentWinUI3)
        set(ICON_WIDTH_DELEGATE 24)
    elseif (QUICK_GUI_CONTROLS_STYLE STREQUAL dynamic)
        set(ICON_WIDTH_DELEGATE "QQuickStyle::name() == QStringLiteral(\"FluentWinUI3\") ? 24 : 16")
    endif ()
    set_property(
        SOURCE quick/quickui.cpp
        APPEND
        PROPERTY COMPILE_DEFINITIONS SYNCTHING_APP_ICON_SIZE=${ICON_SIZE}
                 SYNCTHING_APP_ICON_WIDTH_DELEGATE=${ICON_WIDTH_DELEGATE})
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
    if (TREAT_WARNINGS_AS_ERRORS AND (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
        foreach (QML_FILE AdvancedDevConfigPage AdvancedDirConfigPage)
            set_source_files_properties(
                "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/${META_TARGET_NAME}_quick/qml/${QML_FILE}_qml.cpp"
                PROPERTIES COMPILE_FLAGS "-Wno-error=conversion")
        endforeach ()
    endif ()

    # allow use of newer native Android APIs, see https://developer.android.com/ndk/guides/using-newer-apis
    if (ANDROID)
        set_source_files_properties(
            "quick/quickui.cpp" PROPERTIES COMPILE_FLAGS
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
        "tests/testhelper.h"
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

if (QUICK_GUI)
    list(APPEND QT_QUICK_TEST_LIBRARIES ${META_TARGET_NAME})
    use_qt_module(LIBRARIES_VARIABLE "QT_QUICK_TEST_LIBRARIES" PREFIX "${QT_PACKAGE_PREFIX}" MODULE "QuickTest")
    configure_test_target(
        TEST_NAME
        "basic_quick_tests"
        SRC_FILES
        "tests/testhelper.h"
        "tests/basicquicktests.cpp"
        LIBRARIES
        "${QT_QUICK_TEST_LIBRARIES}"
        FULL_TEST_NAME_OUT_VAR
        FULL_TEST_NAME_BASIC_QUICK
        FULL_TEST_TARGET_OUT_VAR
        FULL_TEST_TARGET_BASIC_QUICK
        UNSTABLE)
    target_compile_definitions("${FULL_TEST_TARGET_BASIC_QUICK}"
                               PRIVATE "QUICK_TEST_SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/basic\"")
endif ()
if (QUICK_GUI AND "app" IN_LIST QUICK_GUI_ENABLED_MODES)
    configure_test_target(
        TEST_NAME
        "app_tests"
        SRC_FILES
        "tests/testhelper.h"
        "tests/apptests.cpp"
        LIBRARIES
        "${QT_QUICK_TEST_LIBRARIES}"
        FULL_TEST_NAME_OUT_VAR
        FULL_TEST_NAME_APP
        FULL_TEST_TARGET_OUT_VAR
        FULL_TEST_TARGET_APP
        UNSTABLE)
    target_compile_definitions("${FULL_TEST_TARGET_APP}"
                               PRIVATE "QUICK_TEST_SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/app\"")
    set_tests_properties("${FULL_TEST_NAME_APP}" PROPERTIES RESOURCE_LOCK "syncthingtestinstance")
    set_tests_properties("${FULL_TEST_NAME_APP}" PROPERTIES ENVIRONMENT "LIB_SYNCTHING_CONNECTOR_LOG_ALL=1")
endif ()
if (QUICK_GUI AND "desktop" IN_LIST QUICK_GUI_ENABLED_MODES)
    configure_test_target(
        TEST_NAME
        "desktop_ui_tests"
        SRC_FILES
        "tests/testhelper.h"
        "tests/desktopuitests.cpp"
        LIBRARIES
        "${QT_QUICK_TEST_LIBRARIES}"
        FULL_TEST_NAME_OUT_VAR
        FULL_TEST_NAME_DESKTOP_UI
        FULL_TEST_TARGET_OUT_VAR
        FULL_TEST_TARGET_DESKTOP_UI
        UNSTABLE)
    target_compile_definitions("${FULL_TEST_TARGET_DESKTOP_UI}"
                               PRIVATE "QUICK_TEST_SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/desktop\"")
    set_tests_properties("${FULL_TEST_NAME_DESKTOP_UI}" PROPERTIES RESOURCE_LOCK "syncthingtestinstance")
    set_tests_properties("${FULL_TEST_NAME_DESKTOP_UI}" PROPERTIES ENVIRONMENT "LIB_SYNCTHING_CONNECTOR_LOG_ALL=1")
endif ()
