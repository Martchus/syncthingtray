cmake_minimum_required(VERSION 3.17.0 FATAL_ERROR)

# metadata
set(META_PROJECT_NAME syncthingwidgets)
set(META_PROJECT_TYPE library)
set(META_APP_NAME "UI elements of Syncthing Tray")
set(META_PUBLIC_QT_MODULES Gui Widgets WebEngineCore WebEngineWidgets)
set(META_WEBVIEW_SRC_DIR webview)
set(META_WEBVIEW_WITH_CORE ON)
set(META_GUI_OPTIONAL ON)

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
find_package(${PACKAGE_NAMESPACE_PREFIX}c++utilities${CONFIGURATION_PACKAGE_SUFFIX} 5.25.0 REQUIRED)
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

# link also explicitly against the following Qt modules
list(APPEND ADDITIONAL_QT_MODULES Network Concurrent)

# include modules to apply configuration
include(BasicConfig)
include(QtGuiConfig)
include(QtWebViewProviderConfig)
include(QtConfig)
include(WindowsResources)
include(LibraryTarget)
include(Doxygen)
include(ConfigHeader)

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
